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

        /*Get vfn current p_brk belongs to... Need to calculate the vfn (virtual frame number)*/
        /*
         *For example, if brk = 8192, it means brk is on third vfn (since addresses start at 0)
         *vfn 0 goes from 0 to 4095
         *vfn 1 goes from 4096 to 8191
         *vfn 2 goes from 8192 to 12287
         */
        uint32_t cur_brk_vfn = 0;
        uint32_t cur_brk_add = (uint32_t)curproc->p_brk;

        if(cur_brk_add % PAGE_SIZE == 0){
                /*plus one since addresses start at 0...*/
                cur_brk_vfn = (cur_brk_add / PAGE_SIZE) + 1;
        }
        else{
                cur_brk_vfn = cur_brk_add - (cur_brk_add % PAGE_SIZE);
                cur_brk_vfn = (cur_brk_vfn / PAGE_SIZE) + 1;
        }

        /*Calculate requested address vfn*/
        uint32_t req_addr_vfn = 0;
        uint32_t req_address = (uint32_t)addr;

        if(req_address % PAGE_SIZE == 0){
                /*calculate req_address vfn*/
                uint32_t req_addr_vfn = (req_address / PAGE_SIZE) + 1;
        }
        else{
                /*find next page boundary*/
                req_address = req_address - (req_address % PAGE_SIZE);

                /*Calculate vfn of the requesting address*/
                uint32_t req_addr_vfn = (req_address / PAGE_SIZE) + 1;
        }

        /*get the vmareas for each...*/
        vmarea_t *cur_brk_vmarea =  vmmap_lookup(curproc->p_vmmap, cur_brk_vfn);
        vmarea_t *req_brk_vmarea =  vmmap_lookup(curproc->p_vmmap, req_addr_vfn);

        /*We want to reduce the heap memory area...*/
        if(req_address < cur_brk_add){

                if(cur_brk_vmarea == req_brk_vmarea){
                        /*our destination  address lies within the same area*/
                        /*Just reduce the brk...*/
                        curproc->p_brk = addr;
                        *ret = addr;
                        return 0;
                }
                else{
                        /*we reduce from many areas*/
                        /*Check if each area between p_brk and addr is NOT in use*/
                        list_link_t *area_link = cur_brk_vmarea->vma_plink.l_prev; /*start from area (backward) from the new brk's area*/
                        vmarea_t *cur_area = NULL;
                        int exit_flag = 0;

                        /*Loop through areas between */

                        while(1){
                                /*if we reached the the req vmarea area, we must exit after doing some checks..*/
                                if(cur_area == req_brk_vmarea){
                                       break;
                                }

                                /*get area...*/
                                cur_area = list_item(area_link, vmarea_t, vma_plink);

                                /*if range is empty of mappings (area), remove mapping*/
                                if(vmmap_is_range_empty(curproc->p_vmmap, cur_area->vma_start, (cur_area->vma_end - cur_area->vma_start))){
                                        vmmap_remove(curproc->p_vmmap, cur_area->vma_start, (cur_area->vma_end - cur_area->vma_start));
                                }

                                /*got to next area*/
                                area_link = area_link->l_prev;
                        }

                        /*set the new break...*/
                        curproc->p_brk = addr;
                        *ret = addr;
                        return 0;
                }
        }
        else{
                /*We want to increase it*/
                if(req_address > USER_MEM_HIGH){
                        return -EPERM;                      /*Can't extend beyond userland*/
                }
                else{
                        /*we want to increase the brk...*/
                        /*Two cases...*/
                        if(cur_brk_vmarea == cur_brk_vmarea){
                                /*extend within the same area, area is already mapped*/
                                 curproc->p_brk = addr;
                                *ret = addr;
                                return 0;

                        }
                        else{
                                /*extend beyond the current area...*/

                                /*
                                 *we need to check that the space between the end of the current brk vmarea (in vfn #) and the requested vfn
                                 *for the new brk is empty.
                                 */
                                int res = vmmap_is_range_empty(curproc->p_vmmap, cur_brk_vmarea->vma_end + 1, req_addr_vfn - cur_brk_vmarea->vma_end);

                                if(res){
                                        vmarea_t *result = NULL;

                                        /*range is empty, we can add new vmarea*/
                                        /*Offset must be zero (since whatever extra space we need, will start at the next area's first page boundary*/
                                        vmmap_map(curproc->p_vmmap, NULL, cur_brk_vmarea->vma_end + 1, req_addr_vfn - cur_brk_vmarea->vma_end,
                                                  PROT_EXEC | PROT_READ, MAP_PRIVATE, 0, 0, &result);

                                        if(result != NULL){
                                                curproc->p_brk = addr;
                                                *ret = addr;
                                                return 0;
                                        }
                                        else{
                                                *ret = NULL;
                                                return -ENOMEM;
                                        }
                                }
                                else{
                                        /*there's no space to increment the brk to the desired address location*/
                                        return -ENOMEM ;
                                }
                        }
                }
        }
        return 0;
}
