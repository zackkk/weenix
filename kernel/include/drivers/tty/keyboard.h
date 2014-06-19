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

typedef void (*keyboard_char_handler_t)(char);

/**
 * Initializes the keyboard subsystem.
 */
void keyboard_init(void);

/**
 * Registers a handler to receive key press events from the keyboard.
 *
 * @param handler the handler to register
 */
void keyboard_register_handler(keyboard_char_handler_t handler);
