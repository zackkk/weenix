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

/*  dirent.h - filesystem-independent directory entry
 *  mcc, kma, jal
 */
#pragma once

/* Kernel and user header (via symlink) */

#ifdef __KERNEL__
#include "config.h"
#else
#include "weenix/config.h"
#endif

typedef struct dirent {
        ino_t   d_ino;                  /* entry inode number */
        off_t   d_off;                  /* seek pointer of next entry */
        char    d_name[NAME_LEN + 1];   /* filename */
} dirent_t;

#define d_fileno d_ino
