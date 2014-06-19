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

#include "fs/vnode.h"

typedef int(*binfmt_load_func_t)(const char *filename, int fd,
                                 char *const *argv, char *const *envp, uint32_t *eip, uint32_t *esp);

int  binfmt_add(const char *id, binfmt_load_func_t loadfunc);

int binfmt_load(const char *filename, char *const *argv, char *const *envp, uint32_t *eip, uint32_t *esp);
