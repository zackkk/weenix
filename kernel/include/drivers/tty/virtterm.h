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

struct tty_driver;

/* The width of the virtual terminal display */
#define DISPLAY_WIDTH 80

/* The height of the virtual terminal display */
#define DISPLAY_HEIGHT 25

/* The size of the virtual terminal display */
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)

/**
 * Initializes the virtual terminal subsystem
 */
void vt_init(void);

/**
 * Returns the number of virtual terminals available to the system.
 *
 * @return the number of virtual terminals
 */
int vt_num_terminals();

/**
 * Returns a pointer to the tty_driver_T for the virtual terminal with
 * a given id. The terminals are numbered 0 through vt_num_terminals()
 * - 1.
 *
 * @param id the id of the virtual terminal to get the driver for
 * @return a pointer to the driver for the specified virtual terminal
 */
struct tty_driver *vt_get_tty_driver(int id);

/**
 * Scrolls the current terminal either up or down by a given number of
 * lines.
 *
 * @param lines the number of lines to scroll
 * @param scroll_up if true, scrolls up, otherwise, scrolls down
 */
void vt_scroll(int lines, int scroll_up);

/**
 * Switches to the virtual terminal with the given id.
 *
 * @param id the id of the virtual terminal to switch to
 * @return 0 on success and <0 on error
 */
int vt_switch(int id);

/**
 * Prints a shutdown message to the screen.
 */
void vt_print_shutdown(void);
