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
	/*aliases*/
	ktqueue_t *waitqPtr = &(mtx->km_waitq); /*mutex wait queue*/
		
	/*initialize km_waitq*/
	sched_queue_init(waitqPtr);
	
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
	dbg(DBG_PRINT, "(GRADING1A 5.a) KASSERT successful");
	
	/*aliases*/
	ktqueue_t *waitqPtr = &(mtx->km_waitq); /*mutex wait queue*/
	kthread_t *mutexOwner = mtx->km_holder; /*pointer to owner of the mutex*/

	/*if mutex already locked, add curthr to mutex wait queue*/
	if(mutexOwner != NULL) {
		dbg(DBG_PRINT, "(GRADING1A 5.a) mutex already locked - put (non-cancellable) thread to sleep");
		sched_sleep_on(waitqPtr); /*put current thread in the mutex wait queue*/
		/* sched_sleep_on will do context_switch */
		dbg(DBG_PRINT, "(GRADING1A 5.a) (non-cancellable) thread woken up - mutex obtained");
	}
	/*mutex not held by anyone. take it, and continue execution*/
	else {
		mutexOwner = curthr; /*mutex is now locked by current thread, keep executing*/
		dbg(DBG_PRINT, "(GRADING1A 5.a) mutex not locked, (non-cancellable) thread obtained it");
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
	dbg(DBG_PRINT, "(GRADING1A 5.b)");
	
	/*aliases*/
	ktqueue_t *waitqPtr = &(mtx->km_waitq); /*mutex wait queue*/
	kthread_t *mutexOwner = mtx->km_holder; /*pointer to owner of the mutex*/

	/*if mutex already locked, add curthr to mutex wait queue*/
	if(mutexOwner != NULL) {
		dbg(DBG_PRINT, "(GRADING1A 5.b) mutex already locked - put (cancellable) thread to sleep");
		sched_cancellable_sleep_on(waitqPtr); /*put current thread in the mutex wait queue*/
		/* sched_sleep_on will do context_switch */
		dbg(DBG_PRINT, "(GRADING1A 5.b) (cancellable) thread woken up - mutex obtained");
	}
	/*mutex not held by anyone. take it, and continue execution*/
	else {
		mutexOwner = curthr; /*mutex is now locked by current thread, keep executing*/
		dbg(DBG_PRINT, "(GRADING1A 5.b) mutex not locked, (cancellable) thread obtained it");
	}
	
	/*if thread was cancelled, it does not hold the mutex, so return -EINTR*/
	if(curthr != mtx->km_holder) {
		dbg(DBG_PRINT, "(GRADING1A 5.b) (cancellable) canceled, mutex not held");
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
	dbg(DBG_PRINT, "(GRADING1A 5.c)");
	
	/*aliases*/
	ktqueue_t *waitqPtr = &(mtx->km_waitq); /*wait queue*/	
	kthread_t *mutexOwner = mtx->km_holder; /*pointer to owner of the mutex*/
	
	/*if mutex queue is empty, unlock mutex*/
	if(sched_queue_empty(waitqPtr)) { 
		mutexOwner = NULL;
		dbg(DBG_PRINT, "(GRADING1A 5.c) mutex unlocked - mutex wait queue empty");
	}
	/*mutex queue not empty: give mutex to head of list, put head of list in run queue*/
	else {
		mutexOwner = sched_wakeup_on(waitqPtr); /*de-queue head of mutex wait list, set thread as mutex owner*/
		sched_make_runnable(mutexOwner); /*add mutex owner to run queue*/
		dbg(DBG_PRINT, "(GRADING1A 5.c) mutex unlocked - head of mutex wait queue woken up, made runnable");
	}
	
	KASSERT(curthr != mtx->km_holder);
	dbg(DBG_PRINT, "(GRADING1A 5.c) mutex unlocked - old thread no longer hold mutex");
}
