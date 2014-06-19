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

/* Starts the Programmable Interval Timer (PIT)
 * delivering periodic interrupts at 1000 Hz
 * (i.e., one every millisecond) to the given interrupt. */
void pit_init(uint8_t intr);
void pit_starttimer(uint8_t intr);
