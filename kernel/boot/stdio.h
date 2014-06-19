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

#ifndef __STDIO_H__
#define __STDIO_H__

puts16:
		pusha /* save registers */
		mov		$0x00, %bh
		mov		$0x07, %bl
1:
		lodsb /* load next byte from si into al and increment si */
		or		%al, %al /* is al 0? (null terminator) */
		jz		1f /* null terminated, exit now */
		mov		$0x0e, %ah /* int 0x10 function 0x0e prints character */
		int		$0x10
		jmp		1b /* repeat until null terminator found */

1:
		popa /* restore registers */
		ret

#endif /* __STDIO_H__ */
