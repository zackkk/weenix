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
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH. (0xc0000000) 3GB
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should NOT fail as the man page says. Instead,
 * "return" the current break. We use this to implement sbrk(0) without writing
 * a separate syscall. Look in user/libc/syscall.c if you're curious.
 *
 * Also, despite the statement on the manpage, you MUST support combined use
 * of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
        
        /*If we request the same, just return*/
        if(addr == curproc->p_brk){
                return 0;               /*Do nothing*/
        }
        
        /*check address is null*/
        if(addr == NULL){
                *ret = curproc->p_brk;
                return 0;
        }
        
        /*Disallow going below p_start_brk*/
        if(addr < curproc->p_start_brk){
                ret = curproc->p_brk;
                return -1;              /*check*/
        }
        
        uint32_t cur_brk_vfn = 0;
        uint32_t cur_brk = (uint32_t)curproc->p_brk;

        /*Get vfn current p_brk belongs to... Need to calculate the vfn (virtual frame number)*/
        /*
         *For example, if brk = 8192, it means brk is on third vfn (since addresses start at 0)
         *vfn 0 goes from 0 to 4095
         *vfn 1 goes from 4096 to 8191
         *vfn 2 goes from 8192 to 12287
         */
        if(cur_brk % PAGE_SIZE == 0){
                cur_brk_vfn = ((uint32_t)curproc->p_brk) - (((uint32_t)curproc->p_brk) % PAGE_SIZE);           /*this gets us address of the frame boundary of the frame that containt address p_brk*/                                                                                               
                cur_brk_vfn = (cur_brk_vfn / PAGE_SIZE) + 1;                                                   /*now divide address by frame size, this gets us the vf. cur_brk_vfn in the division is multiple of PAGE_SIZE*/
        }
        else{
                cur_brk_vfn = ((uint32_t)curproc->p_brk) - (((uint32_t)curproc->p_brk) % PAGE_SIZE);           
                cur_brk_vfn = (cur_brk_vfn / PAGE_SIZE) + 1;                                                                      
        }
        
        /*get the vmarea*/
        vmarea_t *heap_vmarea =  vmmap_lookup(curproc->p_vmmap, cur_brk_vfn);                                                                   
        uint32_t req_address = (uint32_t)addr;
        
        
        
        /*We want to reduce the heap memory area...*/
        if(req_address < heap_vmarea->vma_start + heap_vmarea->vma_start){
                
                
                /*Case 1 & 2  --> WE want to reduce brk (aligned or not) to new address, which is page aligned.*/
                /*Thus, we unmap any pages above the new address, until previous brk page...*/
                if(req_address % PAGE_SIZE == 0){                               
                
                        /*calculate req_address vfn*/
                        uint32_t req_addr_vfn = (req_address / PAGE_SIZE) + 1;
                        
                        /*Unmap region the p_break - req_addr*/
                        vmmap_remove(curproc->p_vmmap, req_addr_vfn + 1, cur_brk_vfn - req_addr_vfn);
                        
                        curproc->p_brk = addr;
                        *ret = addr;
                        return 0;
                        
                }
                else{   /*new break is not page */
                        /*find next page boundary*/
                        req_address = req_address - (req_address % PAGE_SIZE);
                         
                        /*Calculate vfn*/
                        uint32_t req_addr_vfn = (req_address / PAGE_SIZE) + 1;
                        
                        if(cur_brk_vfn == req_addr_vfn){
                                /*If we end up reducing to the same page, we do no ummaping of pages*/
                                curproc->p_brk = (void *)req_address;
                                *ret = (void *)req_address;
                                return 0;
                        }
                        else{
                                /*We want to reduce to an address that on a lower page than the current brk*/
                                /*remove mapping starting from next after req_addr_vfn*/
                                vmmap_remove(curproc->p_vmmap, req_addr_vfn + 1, cur_brk_vfn - req_addr_vfn);
                                
                                /*REturn requested address. By this time, we unmmaped the vfn above*/
                                curproc->p_brk = (void *)req_address;
                                *ret = (void *)req_address;
                                return 0;
                        }   
                }
                
        }
        else{
                /*We want to increase it*/
                if(req_address > USER_MEM_HIGH){
                        return -1;                      /*Can't extend beyond userland*/
                }
                else{
                        
                }
        }
        
        
        return 0;
}
