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

#include "priv.h"

#include "test/kshell/kshell.h"

typedef struct kshell_command {
        char              kc_name[KSH_CMD_NAME_LEN + 1];
        kshell_cmd_func_t kc_cmd_func;
        char              kc_desc[KSH_DESC_LEN + 1];

        list_link_t       kc_commands_link;
} kshell_command_t;

kshell_command_t *kshell_command_create(const char *name,
                                        kshell_cmd_func_t cmd_func,
                                        const char *desc);

void kshell_command_destroy(kshell_command_t *cmd);
