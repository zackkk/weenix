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
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        
        KASSERT(regs != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a) regs are not null\n");
        KASSERT(curproc != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)current process is not NULL\n");
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a) Current process state is PROC_RUNNING\n");
        
        
        proc_t *new_process = NULL;
        
        /*Create new process*/
        new_process = proc_create(curproc->p_comm);            /*use name*/
        
        /*deallocate vmmap, since we will clone it...*/
        vmmap_destroy(new_process->p_vmmap);
        
        /*clonde the vmmap...*/
        if(new_process->p_vmmap != NULL){
                new_process->p_vmmap = vmmap_clone(curproc->p_vmmap);
        }
        
        KASSERT(new_process->p_vmmap != NULL);                   /*correctly cloned*/
                                                                 
        /*Now we need to increment refcount of all object in vmmareas...*/
        list_link_t *area_link= NULL;
        list_link_t *newproc_area_link = NULL;
        
        area_link = curproc->p_vmmap->vmm_list.l_next;   /*get first area*/
        newproc_area_link = new_process->p_vmmap->vmm_list.l_next;
        
        vmarea_t *cur_area = NULL;
        vmarea_t *newproc_cur_area = NULL;
        
        while(area_link != &(curproc->p_vmmap->vmm_list)){       /*last area will point to the  anchor...*/
                
                cur_area = list_item(area_link, vmarea_t, vma_plink);
                newproc_cur_area = list_item(newproc_area_link, vmarea_t, vma_plink);
                
                KASSERT(cur_area != NULL && newproc_cur_area != NULL);
                
                /*Now check that if sharing is PRIVATE (COW) or SHARED...*/
                if(cur_area->vma_flags == MAP_SHARED){
                        
                        /*Make new process vmarea point to shared object...*/
                        newproc_cur_area->vma_obj = cur_area->vma_obj;
                        
                        /*increase refcount, same for anon/shadow...*/
                        cur_area->vma_obj->mmo_ops->ref(cur_area->vma_obj);
                        
                        /*Add new area vmarea to vmbojt list...*/
                        list_insert_tail(&cur_area->vma_obj->mmo_un.mmo_vmas, &newproc_cur_area->vma_olink);
                        
                }
                else{
                        /*private mapping*/
                        /*create two new shadow objects, one for curproc, and one for new process*/
                        mmobj_t *curproc_shadow = shadow_create();
                        mmobj_t *newproc_shadow = shadow_create();
                        
                        KASSERT(curproc_shadow && newproc_shadow);
                        
                        /*make both objects point to the old vma_object...*/
                        curproc_shadow->mmo_shadowed = cur_area->vma_obj;
                        newproc_shadow->mmo_shadowed = cur_area->vma_obj;
                        
                        /*make new shadow objects point to the botton object too?? CHECK*/
                        mmobj_t *bottom_obj = NULL;
                        
                        bottom_obj = mmobj_bottom_obj(cur_area->vma_obj);
                        
                        /*increase bottom obj refcount, since new shadow objects will point to them? CHECK*/
                        bottom_obj->mmo_ops->ref(bottom_obj);
                        bottom_obj->mmo_ops->ref(bottom_obj);
                        
                        curproc_shadow->mmo_un.mmo_bottom_obj = bottom_obj;
                        newproc_shadow->mmo_un.mmo_bottom_obj = bottom_obj;
                        
                        /*increase vma_obj refcount by one (one new shadow object references this vma_obj*/
                        cur_area->vma_obj->mmo_ops->ref(cur_area->vma_obj);                 /*CHECK!! Increase ref of shadowed object or of the shadow object???*/
                                                                          
                        /*make vmarea's vma_obj point to their new shadow objects.*/
                        cur_area->vma_obj = curproc_shadow;
                        newproc_cur_area->vma_obj = newproc_shadow;
                }                
        }
        
        /*Copy file table*/
        int i = 0;
        for(i = 0; i < NFILES; i++){
                new_process->p_files[i] = curproc->p_files[i];
                
                if(new_process->p_files[i] != NULL){
                        fref(new_process->p_files[i]);
                }
        }

        /*workin directory...*/
        new_process->p_cwd = curproc->p_cwd;
        vref(new_process->p_cwd);               /*increment reference count...*/
                                                 
        
            
        /*copy some fields from parent process.*/
        new_process->p_brk = curproc->p_brk;
        new_process->p_start_brk = curproc->p_start_brk;
        new_process->p_state = curproc->p_state;
        new_process->p_status = curproc->p_status;
        
        KASSERT(new_process->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a) New process state is PROC_RUNNING\n");
        KASSERT(new_process->p_pagedir != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a) New process page directory is not NULL\n");
                                        
        /*remove all translations to cause Page faults, so PF handler calls COW function*/                
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all();
        
        /*clone thread...*/
        /*When we return from kthread_clone, we have a new context and stack...*/
        kthread_t *newthr = NULL;
        KASSERT(newthr->kt_kstack != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a) Cloned thread stack is not NULL\n");
        
        /*Setup new thread's context...*/
        /*make cloned text context point to new process pagedir, stack and stack size...*/
        newthr->kt_ctx.c_pdptr = new_process->p_pagedir;
        newthr->kt_ctx.c_kstack = (uintptr_t)newthr->kt_kstack;
        newthr->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
        
        /*eax for child should be zero*/
        regs->r_eax = 0;
        
        /*eip points to userland_entry*/
        newthr->kt_ctx.c_esp = fork_setup_stack(regs, newthr->kt_kstack);
        newthr->kt_ctx.c_eip = (uintptr_t)userland_entry;
                                             
        /*add new process to curproc children...*/
        list_insert_tail(&curproc->p_children, &new_process->p_child_link);
        
        /*add thread to new process and make it runnable*/
        list_insert_head(&new_process->p_threads, &newthr->kt_plink);
        sched_make_runnable(newthr);

        /*set EAX with return for parent process: should be the pid of the parent process...*/
        /*return from this function? for parent*/
        return new_process->p_pid;
}
