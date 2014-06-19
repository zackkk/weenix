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

struct proc;
struct argstr;
struct argvec;

int copy_from_user(void *kaddr, const void *uaddr, size_t nbytes);
int copy_to_user(void *uaddr, const void *kaddr, size_t nbytes);

char *user_strdup(struct argstr *ustr);
char **user_vecdup(struct argvec *uvec);

int range_perm(struct proc *p, const void *vaddr, size_t len, int perm);
int addr_perm(struct proc *p, const void *vaddr, int perm);
