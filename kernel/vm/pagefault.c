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

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{
		pde_t page_dir_flag   = PD_PRESENT | PD_USER;
		pte_t page_table_flag = PT_PRESENT | PT_USER;
		int forwrite = 0;
		int res = 0;
		pframe_t *pf = NULL;
		int vmarea_pagenum = 0;
		int mmobj_pagenum = 0;

	    /* Find the vmarea that contains the address that was faulted on using vpn */
		vmarea_pagenum = ADDR_TO_PN(vaddr);
		vmarea_t *vma = vmmap_lookup(curproc->p_vmmap, vmarea_pagenum);
		if(NULL == vma){
			do_exit(EFAULT);
		}

	    /* Make sure to check the permissions on the area to see if the process has permission to do [cause]. */
		/* pagefault.h  mm/mman.h */
		if(cause & FAULT_WRITE){
			if(vma->vma_prot & PROT_WRITE){
				forwrite = 1;
				page_dir_flag |= PD_WRITE;
				page_table_flag |= PT_WRITE;
			}
			else{
				do_exit(EFAULT);
			}
		}

		if(cause & FAULT_EXEC){
			if(!(vma->vma_prot & PROT_EXEC)){
				do_exit(EFAULT);
			}
		}

		/* Find the correct page, Make sure that if the user writes to the page it will be handled correctly. */
		KASSERT(vma);
		KASSERT(vma->vma_obj);
		KASSERT(vmarea_pagenum);
		mmobj_pagenum = vmarea_pagenum + vma->vma_off - vma->vma_start;
		res = pframe_lookup(vma->vma_obj, mmobj_pagenum, forwrite, &pf);
		KASSERT(res == 0);
		if(forwrite){
			pframe_dirty(pf);
		}

	    /* 5. Call pt_map(in pagetable.c) to have the new mapping placed into the appropriate page table */
		KASSERT(NULL != pf);
		/* PAGE_ALIGN_DOWN: set least significant 12 bits to zero */
		res = pt_map(curproc->p_pagedir, (uintptr_t)PAGE_ALIGN_DOWN(vaddr), pt_virt_to_phys((uintptr_t)pf->pf_addr), page_dir_flag, page_table_flag);
		KASSERT(res == 0);
}
