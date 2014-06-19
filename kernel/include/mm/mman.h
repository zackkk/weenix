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

/* Page protection flags.
*/
#define PROT_NONE       0x0     /* No access. */
#define PROT_READ       0x1     /* Pages can be read. */
#define PROT_WRITE      0x2     /* Pages can be written. */
#define PROT_EXEC       0x4     /* Pages can be executed. */

/* Return value for mmap() on failure.
*/
#define MAP_FAILED      ((void*)-1)

/* Mapping type - shared or private.
*/
#define MAP_SHARED      1
#define MAP_PRIVATE     2
#define MAP_TYPE        3     /* mask for above types */

/* Mapping flags.
*/
#define MAP_FIXED       4
#define MAP_ANON        8
