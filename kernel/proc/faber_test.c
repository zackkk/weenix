/******************************************************************************/
/* Important CSCI 402 usage information:                                      */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove this comment block from this file.    */
/******************************************************************************/

/*
 * Author: Ted Faber <faber@isi.edu>
 * With modifications by: Bill Cheng <bill.cheng@usc.edu>
 *
 * BC: You must NOT change this file.  If you submit a modified version
 * of this file, it will be deleted from your submission.
 */
#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kmutex.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

ktqueue_t wake_me_q;
int wake_me_len = 0;
int race=0;

kmutex_t mutex;

typedef struct {
    struct proc *p;
    struct kthread *t;
} proc_thread_t;

/*
 * Create a process and a thread with the given name and calling teh given
 * function. Arg1 is passed to the function (arg2 is always NULL).  The thread
 * is immediately placed on the run queue.  A proc_thread_t is returned, giving
 * the caller a pointer to the new process and thread to coordinate tests.  NB,
 * the proc_thread_t is returned by value, so there are no stack problems.
 */
static void start_proc(proc_thread_t *ppt, char *name, kthread_func_t f, int arg1) {
    proc_thread_t pt;

    pt.p = proc_create(name);
    pt.t = kthread_create(pt.p, f, arg1, NULL);
    KASSERT(pt.p && pt.t && "Cannot create thread or process");
    sched_make_runnable(pt.t);
    if (ppt != NULL) {
        memcpy(ppt, &pt, sizeof(proc_thread_t));
    }
}

/**
 * Call do_waitpid with the process ID of the given process.  Print a debug
 * message with the exiting process's status.
 */
static void wait_for_proc(proc_t *p) {
    int rv;
    pid_t pid;
    char pname[PROC_NAME_LEN];

    strncpy(pname, p->p_comm, PROC_NAME_LEN); 
    pname[PROC_NAME_LEN-1] = '\0'; 
    pid = do_waitpid(p->p_pid, 0, &rv);
    dbg(DBG_PRINT, "%s (%d) exited: %d\n", pname, pid, rv);
}

/**
 * Call waitpid with a -1 pid and print a message about any process that exits.
 * Returns the pid found, including -ECHILD when this process has no children.
 */
static pid_t wait_for_any() {
    int rv;
    pid_t pid;

    pid = do_waitpid(-1, 0, &rv);
    if ( pid != -ECHILD) dbg(DBG_PRINT, "child (%d) exited: %d\n", pid, rv);
    return pid;
}

/*
 * Repeatedly call wait_for_any() until it returns -ECHILD
 */
static void wait_for_all() {
    while (wait_for_any() != -ECHILD)
        ;
}

/*
 * Keep context switching until *count >= tot.  Used to count the number of
 * nodes waiting for the kernel thread queue.
 */
static void stop_until_queued(int tot, int *count) {
    while ( *count < tot) {
	sched_make_runnable(curthr);
	sched_switch();
    }
}

/*
 * Keep context switching until *count == 0.  Used to count the number of
 * nodes waiting for the kernel thread queue.
 */
static void stop_until_zero(int *count) {
    while ( *count > 0) {
	sched_make_runnable(curthr);
	sched_switch();
    }
}

/*
 * A thread function that simply calls do_exit() with status arg1
 */
void *waitpid_test(int arg1, void *arg2) {
    do_exit(arg1);
    return NULL;
}

/*
 * A thread function that returns NULL, silently invoking kthread_exit()
 */
void *kthread_exit_test(int arg1, void *arg2) {
    return NULL;
}

/*
 * A thread function that waits on wake_me_q and exist when released.  If it is
 * cancelled, it prints an error message.
 */
void *wakeme_test(int arg1, void *arg2) {
    wake_me_len++;
    if (sched_cancellable_sleep_on(&wake_me_q) == -EINTR ) {
	dbg(DBG_PRINT, "Wakeme cancelled?! pid (%d)\n", curproc->p_pid);
	wake_me_len--;
	do_exit(-1);
    }
    wake_me_len--;
    do_exit(arg1);
    return NULL;
}

/*
 * A thread function that uncancellably waits on wake_me_q and exist when
 * released.  
 */
void *wakeme_uncancellable_test(int arg1, void *arg2) {
    wake_me_len++;
    sched_sleep_on(&wake_me_q);
    wake_me_len--;
    do_exit(arg1);
    return NULL;
}


/*
 * A thread function that waits on wake_me_q and exist when released.  If it is
 * not cancelled, it prints an error message.
 */
void *cancelme_test(int arg1, void *arg2) {
    wake_me_len++;
    if (sched_cancellable_sleep_on(&wake_me_q) != -EINTR ) {
	dbg(DBG_PRINT, "Wakeme returned?! pid (%d)\n", curproc->p_pid);
	wake_me_len--;
	do_exit(-1);
    }
    wake_me_len--;
    do_exit(arg1);
    return NULL;
}

/*
 * A Thread function that exhibits a race condition on the race global.  It
 * loads increments and stores race, context switching between each line of C.
 */
void *racer_test(int arg1, void *arg2) {
    int local;

    sched_make_runnable(curthr);
    sched_switch();
    local = race;
    sched_make_runnable(curthr);
    sched_switch();
    local++;
    sched_make_runnable(curthr);
    sched_switch();
    race = local;
    sched_make_runnable(curthr);
    sched_switch();
    do_exit(race);
    return NULL;
}

/*
 * A Thread function that exhibits a race condition on the race global being
 * removed by a mutex.  It loads increments and stores race, context switching
 * between each line of C after acquiring mutex.  The mutex acquire cannot
 * be cancelled.
 */
void *mutex_uncancellable_test(int arg1, void *arg2) {
    int local;

    kmutex_lock(&mutex); 
    sched_make_runnable(curthr);
    sched_switch();
    local = race;
    sched_make_runnable(curthr);
    sched_switch();
    local++;
    sched_make_runnable(curthr);
    sched_switch();
    race = local;
    sched_make_runnable(curthr);
    sched_switch();
    kmutex_unlock(&mutex);
    do_exit(race);
    return NULL;
}

/*
 * A Thread function that exhibits a race condition on the race global being
 * removed by a mutex.  It loads increments and stores race, context switching
 * between each line of C after acquiring mutex.  The mutex acquire can
 * be cancelled, but will print an error message if this happens.
 */
void *mutex_test(int arg1, void *arg2) {
    int local;

    if ( kmutex_lock_cancellable(&mutex) ) {
	dbg(DBG_PRINT, "Mutex cancelled? %d", curproc->p_pid);
	do_exit(-1);
    }
    sched_make_runnable(curthr);
    sched_switch();
    local = race;
    sched_make_runnable(curthr);
    sched_switch();
    local++;
    sched_make_runnable(curthr);
    sched_switch();
    race = local;
    sched_make_runnable(curthr);
    sched_switch();
    kmutex_unlock(&mutex);
    do_exit(race);
    return NULL;
}

/*
 * A Thread function that exhibits a race condition on the race global being
 * removed by a mutex.  It loads increments and stores race, context switching
 * between each line of C after acquiring mutex.  The mutex acquire can
 * be cancelled, but will print an error message if the mutex acquire succeeds
 * - it expects to be cancelled.
 */
void *mutex_test_cancelme(int arg1, void *arg2) {
    int local;

    if ( kmutex_lock_cancellable(&mutex) ) 
	do_exit(0);
    dbg(DBG_PRINT, "Mutex not cancelled? %d", curproc->p_pid);
    sched_make_runnable(curthr);
    sched_switch();
    local = race;
    sched_make_runnable(curthr);
    sched_switch();
    local++;
    sched_make_runnable(curthr);
    sched_switch();
    race = local;
    sched_make_runnable(curthr);
    sched_switch();
    kmutex_unlock(&mutex);
    do_exit(race);
    return NULL;
}

/*
 * A thread function to test reparenting.  Start a child wakeme_test process,
 * and if arg1 is > 1, create a child process that will do the same (with arg1
 * decrementd.  Calling this with arg1 results in 2 * arg1 processes, half of
 * them waiting on wake_me_q.  None of them wait, so as they exit, they should
 * all become children of init.
 */
void *reparent_test(int arg1, void *arg2) {
    start_proc(NULL, "reparented" , wakeme_test, arg1);
    if ( arg1 > 1 ) 
	start_proc(NULL, "reparent ", reparent_test, arg1-1);
    do_exit(0);
    return NULL;
}

/*
 * The core testproc code.
 * This function is meant to be invoked in a separate kernel process.
 */
void *testproc(int arg1, void *arg2) {
    proc_thread_t pt;
    pid_t pid = -1;
    proc_t *p;
    int rv = 0;
    int i = 0;

#if CS402TESTS > 0
    dbg(DBG_PRINT, "waitpid any test");
    start_proc(&pt, "waitpid any test", waitpid_test, 23);
    wait_for_any();

    dbg(DBG_PRINT, "waitpid test");
    start_proc(&pt, "waitpid test", waitpid_test, 32);
    pid = do_waitpid(2323, 0, &rv);
    if ( pid != -ECHILD ) dbg(DBG_PRINT, "Allowed wait on non-existent pid\n");
    wait_for_proc(pt.p);

    dbg(DBG_PRINT, "kthread exit test");
    start_proc(&pt, "kthread exit test", kthread_exit_test, 0);
    wait_for_proc(pt.p);

    dbg(DBG_PRINT, "many test");
    for (i = 0; i < 10; i++) 
	start_proc(NULL, "many test", waitpid_test, i);
    wait_for_all();
#endif

#if CS402TESTS > 1
    dbg(DBG_PRINT, "Context switch test");
    start_proc(&pt, "Context switch", racer_test, 0);
    wait_for_proc(pt.p);
#endif

#if CS402TESTS > 2
    sched_queue_init(&wake_me_q);

    dbg(DBG_PRINT, "wake me test");
    wake_me_len = 0;
    start_proc(&pt, "wake me test", wakeme_test, 0);
    /* Make sure p has blocked */
    stop_until_queued(1, &wake_me_len);
    sched_wakeup_on(&wake_me_q);
    wait_for_proc(pt.p);
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");

    dbg(DBG_PRINT, "broadcast me test");
    for (i = 0; i < 10; i++ ) 
	start_proc(NULL, "broadcast me test", wakeme_test, 0);
    stop_until_queued(10, &wake_me_len);
    /* Make sure the processes have blocked */
    sched_broadcast_on(&wake_me_q);
    wait_for_all();
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");
#endif

#if CS402TESTS > 3
    dbg(DBG_PRINT, "wake me uncancellable test");
    start_proc(&pt, "wake me uncancellable test", 
	    wakeme_uncancellable_test, 0);
    /* Make sure p has blocked */
    stop_until_queued(1, &wake_me_len);
    sched_wakeup_on(&wake_me_q);
    wait_for_proc(pt.p);
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");

    dbg(DBG_PRINT, "broadcast me uncancellable test");
    for (i = 0; i < 10; i++ ) 
	start_proc(NULL, "broadcast me uncancellable test", 
		wakeme_uncancellable_test, 0);
    /* Make sure the processes have blocked */
    stop_until_queued(10, &wake_me_len);
    sched_broadcast_on(&wake_me_q);
    wait_for_all();
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");
#endif

#if CS402TESTS > 4
    dbg(DBG_PRINT, "cancel me test");
    start_proc(&pt, "cancel me test", cancelme_test, 0);
    /* Make sure p has blocked */
    stop_until_queued(1, &wake_me_len);
    sched_cancel(pt.t);
    wait_for_proc(pt.p);
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");

    dbg(DBG_PRINT, "prior cancel me test");
    start_proc(&pt, "prior cancel me test", cancelme_test, 0);
    /*  Cancel before sleep */
    sched_cancel(pt.t);
    wait_for_proc(pt.p);
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");

    dbg(DBG_PRINT, "cancel me head test");
    start_proc(&pt, "cancel me head test", cancelme_test, 0);
    start_proc(NULL, "cancel me head test", wakeme_test, 0);
    stop_until_queued(2, &wake_me_len);
    sched_cancel(pt.t);
    sched_wakeup_on(&wake_me_q);
    wait_for_all();
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");
    
    dbg(DBG_PRINT, "cancel me tail test");
    start_proc(NULL, "cancel me tail test", wakeme_test, 0);
    start_proc(&pt, "cancel me tail test", cancelme_test, 0);
    stop_until_queued(2, &wake_me_len);
    sched_cancel(pt.t);
    sched_wakeup_on(&wake_me_q);
    wait_for_all();
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");
#endif

#if CS402TESTS > 5
    dbg(DBG_PRINT, "Reparenting test");
    start_proc(NULL, "Reparenting test", reparent_test, 1);
    stop_until_queued(1, &wake_me_len);
    sched_wakeup_on(&wake_me_q);
    wait_for_all();
    stop_until_zero(&wake_me_len);
    dbg(DBG_PRINT, "Reparenting stress test");
    start_proc(NULL, "Reparenting stress test", reparent_test, 10);
    stop_until_queued(10, &wake_me_len);
    sched_broadcast_on(&wake_me_q);
    wait_for_all();
    stop_until_zero(&wake_me_len);
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");
#endif

#if CS402TESTS > 6
    kmutex_init(&mutex);

    dbg(DBG_PRINT, "show race test");
    race = 0;
    for (i = 0; i < 10; i++ ) 
	start_proc(NULL, "show race test", racer_test, 0);
    wait_for_all();

    dbg(DBG_PRINT, "fix race test");
    race = 0;
    for (i = 0; i < 10; i++ ) 
	start_proc(NULL, "fix race test", mutex_uncancellable_test, 0);
    wait_for_all();

    dbg(DBG_PRINT, "fix race test w/cancel");
    race = 0;
    for (i = 0; i < 10; i++ ) {
	if ( i % 2 == 0) { 
	    start_proc(NULL, "fix race test w/cancel", mutex_test, 0);
	} else {
	    start_proc(&pt, "fix race test w/cancel", mutex_test_cancelme, 0);
	    sched_cancel(pt.t);
	}
    }
    wait_for_all();
#endif

#if CS402TESTS > 7
    dbg(DBG_PRINT, "kill child procs test");
    for ( i=0 ; i < 10; i++ )
	start_proc(NULL, "kill child procs test", cancelme_test, 0);
    stop_until_queued(10, &wake_me_len);
    list_iterate_begin(&curproc->p_children, p, proc_t, p_child_link) {
        proc_kill(p, -1);
    } list_iterate_end();
    wait_for_all();
    KASSERT(wake_me_len == 0 && "Error on wakeme bookkeeping");
#endif

#if CS402TESTS > 8
    dbg(DBG_PRINT, "proc kill all test");
    for ( i=0 ; i < 10; i++ )
	start_proc(NULL, "proc kill all test", cancelme_test, 0);
    stop_until_queued(10, &wake_me_len);
    /*
     * If you don't run this test in a separate process,
     *   the kernel should shutdown and you would fail this test.
     * Therefore, you must run testproc() in a separate process,
     *   since proc_kill_all() should not kill the init process,
     *   although this function will not return, you should
     *   be able to get your kshell prompt back.
     */
    proc_kill_all();

    dbg(DBG_PRINT, "proc_kill_all() must not return\n\n");
    KASSERT(0 && "Error in proc kill all test");
#endif

    return NULL;
}
