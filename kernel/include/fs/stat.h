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
 *  FILE: stat.h
 *  AUTH: mcc
 *  DESC:
 *  DATE: Fri Mar 13 23:10:46 1998
 */

#pragma once

/* Kernel and user header (via symlink) */

struct stat {
        int st_mode;
        int st_ino;
        int st_dev;
        int st_rdev;
        int st_nlink;
        int st_uid;
        int st_gid;
        int st_size;
        int st_atime;
        int st_mtime;
        int st_ctime;
        int st_blksize;
        int st_blocks;
};

/* vnode vn_mode masks */

#define S_IFCHR         0x0100  /* character special */
#define S_IFDIR         0x0200 /* directory */
#define S_IFBLK         0x0400 /* block special */
#define S_IFREG         0x0800 /* regular */
#define S_IFLNK         0x1000 /* symlink */

#define _S_TYPE(m)      ((m) & 0xFF00)
#define S_ISCHR(m)      (_S_TYPE(m) == S_IFCHR)
#define S_ISDIR(m)      (_S_TYPE(m) == S_IFDIR)
#define S_ISBLK(m)      (_S_TYPE(m) == S_IFBLK)
#define S_ISREG(m)      (_S_TYPE(m) == S_IFREG)
#define S_ISLNK(m)      (_S_TYPE(m) == S_IFLNK)
