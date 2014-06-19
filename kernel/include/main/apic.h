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

/* Initializes the APIC using data from the ACPI tables.
 * ACPI handlers must be initialized before calling this
 * function. */
void apic_init();

/* Maps the given IRQ to the given interrupt number. */
void apic_setredir(uint32_t irq, uint8_t intr);

/* Starts the APIC timer */
void apic_enable_periodic_timer(uint32_t freq);

/* Stops the APIC timer */
void apic_disable_periodic_timer();

/* Sets the interrupt to raise when a spurious
 * interrupt occurs. */
void apic_setspur(uint8_t intr);

/* Sets the interrupt priority level. This function should
 * be accessed via wrappers in the interrupt subsystem. */
void apic_setipl(uint8_t ipl);

/* Gets the interrupt priority level. This function should
 * be accessed via wrappers in the interrupt subsystem. */
uint8_t apic_getipl();

/* Writes to the APIC's memory mapped end-of-interrupt
 * register to indicate that the handling of an interrupt
 * originating from the APIC has been finished. This function
 * should only be called from the interrupt subsystem. */
void apic_eoi();
