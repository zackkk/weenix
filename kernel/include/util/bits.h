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
#include "kernel.h"

#define BIT(n) (1<<(n))

static inline void
bit_flip(void *addr, uintptr_t bit)
{
        uint32_t *map = (uint32_t *)addr;
        map += (bit >> 5);
        *map ^= (uint32_t)(1 << (bit & 0x1f));
}

static inline int
bit_check(const void *addr, uintptr_t bit)
{
        const uint32_t *map = (const uint32_t *)addr;
        map += (bit >> 5);
        return (*map & (1 << (bit & 0x1f)));
}

