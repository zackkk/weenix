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

#include "globals.h"

#include "main/interrupt.h"
#include "main/apic.h"
#include "main/pit.h"

#include "util/debug.h"
#include "util/init.h"

#include "proc/sched.h"
#include "proc/kthread.h"

#define APIC_TIMER_IRQ 32 /* Map interrupt 32 */

#ifdef __UPREEMPT__
static unsigned int ms = 0;

static void pit_handler(regs_t *regs)
{
  dbg(DBG_CORE, "PIT HANDLER FIRED\n");
}

/* Uncomment this to enable the apic timer to 
 * call the pit_handler for userland preemption
 */

static __attribute__((unused)) void time_init(void)
{
  intr_map(APIC_TIMER_IRQ, APIC_TIMER_IRQ);
  intr_register(APIC_TIMER_IRQ, pit_handler);
  /* TODO: figure out how this argument converts to hertz */
  apic_enable_periodic_timer(8);
}
init_func(time_init);

#endif
