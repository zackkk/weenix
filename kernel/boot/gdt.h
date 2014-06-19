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

		/* install GDT */
install_gdt:
		cli				/* disable interrupts */
		pusha			/* save registers */
		lgdt .gdtdesc	/* load the gdt */
		sti				/* enable interrupts */
		popa			/* restore registers */
		ret

		/* our GDT */
.gdtdata:
		.word	0, 0
		.byte	0, 0, 0, 0

		/* kernel code segment */
		.word	0xFFFF, 0
		.byte	0, 0x9A, 0xCF, 0

		/* kernel data segment */
		.word	0xFFFF, 0
		.byte	0, 0x92, 0xCF, 0

.gdtdesc: 
		.word	0x27
		.long	.gdtdata





