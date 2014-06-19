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

/* Kernel and user header (via symlink) */

#define NULL 0

typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef signed short       int16_t;
typedef unsigned short     uint16_t;
typedef signed int         int32_t;
typedef unsigned int       uint32_t;
#if defined(__i386__)
typedef signed long long   int64_t;
typedef unsigned long long uint64_t;
typedef signed int         intptr_t;
typedef unsigned int       uintptr_t;
#elif defined(__x86_64__) || defined(__ia64__)
typedef signed long        int64_t;
typedef unsigned long      uint64_t;
typedef signed long        intptr_t;
typedef unsigned long      uintptr_t;
#endif

typedef uint32_t           size_t;
typedef int32_t            ssize_t;
typedef int32_t            off_t;
typedef int64_t            off64_t;
typedef int32_t            pid_t;
typedef uint16_t           mode_t;
typedef uint32_t           blocknum_t;
typedef uint32_t           ino_t;
typedef uint32_t           devid_t;
