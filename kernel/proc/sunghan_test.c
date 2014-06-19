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
 * Author: Sung-Han Lin <sunghan@usc.edu>
 */

#include "kernel.h"
#include "globals.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kmutex.h"

typedef struct my_node {
	int length;
	kmutex_t my_mutex;
	ktqueue_t my_queue;
} my_node_t;

static my_node_t mynode;

static int rand_x = 200, rand_y = 50, rand_z = 3000;

static int 
random_function() {
	rand_x = ( rand_x * 171 ) % 30269;
	rand_y = ( rand_y * 172 ) % 30307;
	rand_z = ( rand_z * 170 ) % 30323;
	float n = ((((float)rand_x)/30269.0) + (((float)rand_y)/30307.0) + (((float)rand_z)/30323.0)) * 100;

	return n;
}

void 
check_sleep(char *str) {
	if (random_function() % 10 < 5) {
		dbg(DBG_SCHED, "Thread %s goes to sleep\n", str);
		sched_broadcast_on(&mynode.my_queue);
		sched_sleep_on(&mynode.my_queue);
		dbg(DBG_SCHED, "Thread %s awake\n", str);
	}
}

static void *
add_my_node(int arg1, void *arg2) {
	int counter = 10;
	dbg(DBG_INIT, "Invoke add_mynode\n");

	int rand_number, i=0;	

	while (counter > 0) {
		check_sleep("add");
		kmutex_lock(&mynode.my_mutex);

		check_sleep("add");

		if (mynode.length < 5) {
			mynode.length++;
			counter--;
		}

		dbg(DBG_INIT, "Add node: %d\n", mynode.length);
		kmutex_unlock(&mynode.my_mutex);
	}
	
	return NULL;
}

static void *
remove_my_node(int arg1, void *arg2) {
	int counter = 10;
	dbg(DBG_INIT, "Invoke remove_mynode\n");

	int rand_number, i=0;	

	while (counter > 0) {
		check_sleep("remove");

		kmutex_lock(&mynode.my_mutex);

		check_sleep("remove");
		
		if (mynode.length > 0) {
			mynode.length--;
			counter--;
		}

		dbg(DBG_INIT, "Remove node: %d\n", mynode.length);		
		kmutex_unlock(&mynode.my_mutex);
	}
	
	return NULL;
}

static void *
watch_dog(int arg1, void *arg2)
{
	while(!sched_queue_empty(&mynode.my_queue)) {
		dbg(DBG_SCHED, "Watch_dog wake up all sleeping thread\n");
		sched_broadcast_on(&mynode.my_queue);
		sched_sleep_on(&mynode.my_queue);
	}

	return NULL;
}

void *
sunghan_test(int arg1, void *arg2)
{
	int status;
	int proc_count = 0;
	pid_t proc_pid[3];

	int i;

	dbg(DBG_INIT, "Start running sunghan_test()...\n");

	mynode.length = 0;
	kmutex_init(&mynode.my_mutex);
	sched_queue_init(&mynode.my_queue);

	proc_t *p1 = proc_create("add_node");
        KASSERT(NULL != p1);
	kthread_t *thr1 = kthread_create(p1, add_my_node, 0, NULL);
        KASSERT(NULL != thr1);
	sched_make_runnable(thr1);
	proc_pid[proc_count++] = p1->p_pid;

	proc_t *p2 = proc_create("remove_node");
        KASSERT(NULL != p2);
	kthread_t *thr2 = kthread_create(p2, remove_my_node, 0, NULL);
        KASSERT(NULL != thr2);
	sched_make_runnable(thr2);
	proc_pid[proc_count++] = p2->p_pid;
	
	proc_t *p3 = proc_create("watch_dog");
       	KASSERT(NULL != p3);
	kthread_t *thr3 = kthread_create(p3, watch_dog, 0, NULL);
       	KASSERT(NULL != thr3);
	sched_make_runnable(thr3);
	proc_pid[proc_count++] = p3->p_pid;
	
	for (i=0; i<2; ++i) {
		do_waitpid(proc_pid[i], 0, &status);
	}
	sched_broadcast_on(&mynode.my_queue);
	do_waitpid(proc_pid[2], 0, &status);

	while (!do_waitpid(p2->p_pid, 0, &status));

	dbg(DBG_INIT, "sunghan_test() terminated\n");

        return NULL;
}

void *
sunghan_deadlock_test(int arg1, void *arg2)
{
	int status;
	int proc_count = 0;
	pid_t proc_pid[3];

	int i;

	dbg(DBG_INIT, "Start running sunghan_deadlock_test()...\n");

	mynode.length = 0;
	kmutex_init(&mynode.my_mutex);
	sched_queue_init(&mynode.my_queue);

	proc_t *p1 = proc_create("add_node");
        KASSERT(NULL != p1);
	kthread_t *thr1 = kthread_create(p1, add_my_node, 0, NULL);
        KASSERT(NULL != thr1);
	sched_make_runnable(thr1);
	proc_pid[proc_count++] = p1->p_pid;

	proc_t *p2 = proc_create("remove_node");
        KASSERT(NULL != p2);
	kthread_t *thr2 = kthread_create(p2, remove_my_node, 0, NULL);
        KASSERT(NULL != thr2);
	sched_make_runnable(thr2);
	proc_pid[proc_count++] = p2->p_pid;
	
	for (i=0; i<2; ++i) {
		do_waitpid(proc_pid[i], 0, &status);
	}
	sched_broadcast_on(&mynode.my_queue);

	dbg(DBG_INIT, "Shouldn't have gotten here in sunghan_deadlock_test().  Did NOT deadlock.\n");

        return NULL;
}
