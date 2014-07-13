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
 * TODO
 * 
 * 
 * check conext (only allow thread context)
 * 
 * 
 */
#include "globals.h"
#include "errno.h"
#include "util/debug.h"
#include "proc/kthread.h"
#include "proc/kmutex.h"
#include "util/list.h"

/*
 * IMPORTANT: Mutexes can _NEVER_ be locked or unlocked from an
 * interrupt context. Mutexes are _ONLY_ lock or unlocked from a
 * thread context.
 */

void
kmutex_init(kmutex_t *mtx)
{
	/*initialize km_waitq*/
	sched_queue_init(&(mtx->km_waitq));
	
	/*initialize km_holder*/
	mtx->km_holder = NULL;
	
}

/*
 * This should block the current thread (by sleeping on the mutex's
 * wait queue) if the mutex is already taken.
 *
 * No thread should ever try to lock a mutex it already has locked.
 */
void
kmutex_lock(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr != mtx->km_holder));
	dbg(DBG_PRINT, "(GRADING1A 5.a) Current thread is not empty and Current thread does not have the mutex\n");

	/*if mutex already locked, add curthr to mutex wait queue*/
	if(mtx->km_holder != NULL) {
		dbg(DBG_PRINT, "(GRADING1E) Mutex already locked\n");
		sched_sleep_on( &(mtx->km_waitq));
		/*put current thread in the mutex wait queue*/
		/* sched_sleep_on will do context_switch */
	}
	/*mutex not held by anyone. take it, and continue execution*/
	else {
		dbg(DBG_PRINT, "(GRADING1E) Mutex not locked\n");
		mtx->km_holder = curthr;
	}
}

/*
 * This should do the same as kmutex_lock, but use a cancellable sleep
 * instead.
 */
int
kmutex_lock_cancellable(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr != mtx->km_holder));
	dbg(DBG_PRINT, "(GRADING1A 5.b) Current thread is not empty and Current thread does not have the mutex\n");

	/*if mutex already locked, add curthr to mutex wait queue*/
	if(mtx->km_holder != NULL) {
		dbg(DBG_PRINT, "(GRADING1E) Mutex already locked\n");
		sched_cancellable_sleep_on(&(mtx->km_waitq));
		/*put current thread in the mutex wait queue*/
		/* sched_sleep_on will do context_switch */
	}
	/*mutex not held by anyone. take it, and continue execution*/
	else {
		dbg(DBG_PRINT, "(GRADING1E) Mutex not locked\n");
		mtx->km_holder = curthr; /*mutex is now locked by current thread, keep executing*/

	}
	
	/*if thread was cancelled, it does not hold the mutex, so return -EINTR*/
	if(curthr != mtx->km_holder) {
		dbg(DBG_PRINT, "(GRADING1E) Cancelled thread does not hold the mutex\n");
		return -EINTR;
	}
	
	/*thread holds mutex*/
	return 0;
}

/*
 * If there are any threads waiting to take a lock on the mutex, one
 * should be woken up and given the lock.
 *
 * Note: This should _NOT_ be a blocking operation!
 *
 * Note: Don't forget to add the new owner of the mutex back to the
 * run queue.
 *
 * Note: Make sure that the thread on the head of the mutex's wait
 * queue becomes the new owner of the mutex.
 *
 * @param mtx the mutex to unlock
 */
void
kmutex_unlock(kmutex_t *mtx)
{
        KASSERT(curthr && (curthr == mtx->km_holder)); /*we know that the caller has mutex locked*/
        dbg(DBG_PRINT, "(GRADING1A 5.c) Current thread is not empty and Current thread does not have the mutex\n");
	
        /*if mutex queue is empty, unlock mutex*/
        if(sched_queue_empty(&(mtx->km_waitq))) {
        	dbg(DBG_PRINT, "(GRADING1E) Mutex queue is empty\n");
        	mtx->km_holder = NULL;
        }
        /*mutex queue not empty: give mutex to head of list, put head of list in run queue*/
        else {
        	dbg(DBG_PRINT, "(GRADING1E) Mutex queue is not empty\n");
        	mtx->km_holder = sched_wakeup_on(&(mtx->km_waitq)); /*de-queue head of mutex wait list, set thread as mutex owner*/
        }
	
        KASSERT(curthr != mtx->km_holder);
        dbg(DBG_PRINT, "(GRADING1A 5.c) Mutex unlocked - old thread no longer hold mutex\n");
}
