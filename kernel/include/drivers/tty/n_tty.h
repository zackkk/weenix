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

#include "drivers/tty/ldisc.h"
#include "proc/kmutex.h"

/*
 * The default line discipline.
 */

typedef struct n_tty n_tty_t;

/**
 * Allocate and initialize an n_tty line discipline, which is not yet
 * attached to a tty.
 *
 * @return a newly allocated n_tty line discipline
 */
tty_ldisc_t *n_tty_create();

/**
 * Destory an n_tty line discipline.
 *
 * @param ntty the n_tty line discipline to destroy
 */
void n_tty_destroy(tty_ldisc_t *ntty);
