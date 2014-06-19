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

#include "types.h"

#define FAULT_PRESENT  0x01
#define FAULT_WRITE    0x02
#define FAULT_USER     0x04
#define FAULT_RESERVED 0x08
#define FAULT_EXEC     0x10

void handle_pagefault(uintptr_t vaddr, uint32_t cause);
