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

#include "test/kshell/kshell.h"

#define KSHELL_CMD(name) \
        int kshell_ ## name(kshell_t *ksh, int argc, char **argv)

KSHELL_CMD(help);
KSHELL_CMD(exit);
KSHELL_CMD(echo);
#ifdef __VFS__
KSHELL_CMD(cat);
KSHELL_CMD(ls);
KSHELL_CMD(cd);
KSHELL_CMD(rm);
KSHELL_CMD(link);
KSHELL_CMD(rmdir);
KSHELL_CMD(mkdir);
KSHELL_CMD(stat);
#endif
