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

/* Returns the highest physical address of the range of usable
 * that start at kernel_start. The intention is that this will
 * be the largest available continuous range of physical
 * addresses. This function should only be used during booting
 * while the first megabyte of memory is identity mapped,
 * otherwise its behavior is undefined. */
uintptr_t phys_detect_highmem();
