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

#pragma once

#include "proc/sched.h"

typedef struct kmutex {
        ktqueue_t       km_waitq;       /* wait queue */
        struct kthread *km_holder;      /* current holder */
} kmutex_t;

/**
 * Initializes the fields of the specified kmutex_t.
 *
 * @param mtx the mutex to initialize
 */
void kmutex_init(kmutex_t *mtx);

/**
 * Locks the specified mutex.
 *
 * Note: This function may block.
 *
 * Note: These locks are not re-entrant
 *
 * @param mtx the mutex to lock
 */
void kmutex_lock(kmutex_t *mtx);

/**
 * Locks the specified mutex, but puts the current thread into a
 * cancellable sleep if the function blocks.
 *
 * Note: This function may block.
 *
 * Note: These locks are not re-entrant.
 *
 * @param mtx the mutex to lock
 * @return 0 if the current thread now holds the mutex and -EINTR if
 * the sleep was cancelled and this thread does not hold the mutex
 */
int  kmutex_lock_cancellable(kmutex_t *mtx);

/**
 * Unlocks the specified mutex.
 *
 * @mtx the mutex to unlock
 */
void kmutex_unlock(kmutex_t *mtx);
