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

#define init_func(func)                         \
        __asm__ (                               \
                ".pushsection .init\n\t"        \
                ".long " #func "\n\t"           \
                ".string \"" #func "\"\n\t"     \
                ".popsection\n\t"               \
        );
#define init_depends(name)                      \
        __asm__ (                               \
                ".pushsection .init\n\t"        \
                ".long 0\n\t"                   \
                ".string \"" #name "\"\n\t"     \
                ".popsection\n\t"               \
        );

typedef void (*init_func_t)();

void init_call_all(void);
