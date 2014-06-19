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

#ifndef __A20_H__
#define __A20_H__

		/* checks if the A20 line is enabled, sets
		 * ax to 0 if disabled, or 1 otherwise */
 
check_a20:
		pushf
		push	%ds
		push	%es
		push	%di
		push	%si
 
		cli
 
		xor		%ax, %ax /* ax = 0 */
		mov		%ax, %es
 
		not		%ax /* ax = 0xFFFF */
		mov		%ax, %ds
 
		mov		$0x0500, %di
		mov		$0x0510, %si
 
		movb	%es:(%di), %al
		push	%ax
 
		movb	%ds:(%si), %al
		push	%ax
 
		movb 	$0x00, %es:(%di)
		movb	$0xFF, %ds:(%si)
 
		cmpb	$0xFF, %es:(%di)
 
		pop		%ax
		movb	%al, %ds:(%si)
 
		pop		%ax
		movb	%al, %es:(%di)
 
		mov		$0x00, %ax
		je		1f
 
		mov		$0x01, %ax
 
1:
		pop		%si
		pop		%di
		pop		%es
		pop		%ds
		popf
 
		ret

#endif /* __A20_H__ */
