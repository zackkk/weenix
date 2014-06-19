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

#define _UTSNAME_LENGTH 128

struct utsname {
        char sysname[_UTSNAME_LENGTH];
        char nodename[_UTSNAME_LENGTH];
        char release[_UTSNAME_LENGTH];
        char version[_UTSNAME_LENGTH];
        char machine[_UTSNAME_LENGTH];
};

int uname(struct utsname *buf);
