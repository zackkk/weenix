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

#include "config.h"
#include "globals.h"

#include "errno.h"

#include "util/init.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"

#include "mm/slab.h"
#include "mm/page.h"

kthread_t *curthr; /* global */
static slab_allocator_t *kthread_allocator = NULL;

static ktqueue_t pointless_queue;

#ifdef __MTP__
/* Stuff for the reaper daemon, which cleans up dead detached threads */
static proc_t *reapd = NULL;
static kthread_t *reapd_thr = NULL;
static ktqueue_t reapd_waitq;
static list_t kthread_reapd_deadlist; /* Threads to be cleaned */
                                        

static void *kthread_reapd_run(int arg1, void *arg2);
#endif

void
kthread_init()
{
        kthread_allocator = slab_allocator_create("kthread", sizeof(kthread_t));
        KASSERT(NULL != kthread_allocator);
}

/**
 * Allocates a new kernel stack.
 *
 * @return a newly allocated stack, or NULL if there is not enough
 * memory available
 */
static char *
alloc_stack(void)
{
        /* extra page for "magic" data */
        char *kstack;
        int npages = 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT);
        kstack = (char *)page_alloc_n(npages);

        return kstack;
}

/**
 * Frees a stack allocated with alloc_stack.
 *
 * @param stack the stack to free
 */
static void
free_stack(char *stack)
{
        page_free_n(stack, 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT));
}

void
kthread_destroy(kthread_t *t)
{
        KASSERT(t && t->kt_kstack);
        free_stack(t->kt_kstack);
        if (list_link_is_linked(&t->kt_plink))
                list_remove(&t->kt_plink);

        slab_obj_free(kthread_allocator, t);
}

/*
 * Allocate a new stack with the alloc_stack function. The size of the
 * stack is DEFAULT_STACK_SIZE.
 *
 * Don't forget to initialize the thread context with the
 * context_setup function. The context should have the same pagetable
 * pointer as the process.
 */
kthread_t *
kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2)
{
        KASSERT(NULL != p);
        dbg(DBG_PRINT, "(GRADING1A 3.a) The process:%s of the kthread is not empty\n", p->p_comm);

        kthread_t *thr = (kthread_t *)slab_obj_alloc(kthread_allocator);  /* set up size in kthread_init(); */
									    
	
        thr->kt_kstack = alloc_stack();
	
	KASSERT(thr->kt_kstack != NULL);

        context_setup(&thr->kt_ctx, func, arg1, arg2, thr->kt_kstack, DEFAULT_STACK_SIZE, p->p_pagedir);

        thr->kt_retval = NULL;
        thr->kt_errno = NULL;
        thr->kt_cancelled = 0;
        thr->kt_wchan = NULL;
        thr->kt_proc = p;
        thr->kt_state = KT_RUN;
        list_insert_head(&(p->p_threads), &(thr->kt_plink));
        return thr;
}

/*
 * If the thread to be cancelled is the current thread, this is
 * equivalent to calling kthread_exit. Otherwise, the thread is
 * sleeping and we need to set the cancelled and retval fields of the
 * thread.
 *
 * If the thread's sleep is cancellable, cancelling the thread should
 * wake it up from sleep.
 *
 * If the thread's sleep is not cancellable, we do nothing else here.
 */
void
kthread_cancel(kthread_t *kthr, void *retval)
{
		KASSERT(NULL != kthr);
		dbg(DBG_PRINT, "(GRADING1A 3.b) The kthread is not empty (thread process pid %d)\n", kthr->kt_proc->p_pid);

		if(kthr == curthr){
			dbg(DBG_PRINT, "(GRADING1E) The thread to be cancelled is the current thread");
			kthread_exit(retval);
		}
		else{
			dbg(DBG_PRINT, "(GRADING1E) The thread to be cancelled is not the current thread");
			kthr->kt_cancelled = 1;
			kthr->kt_retval = retval;
			if(kthr->kt_state == KT_SLEEP_CANCELLABLE){
				sched_make_runnable(kthr);
			}
		}
}

/*
 * You need to set the thread's retval field, set its state to
 * KT_EXITED, and alert the current process that a thread is exiting
 * via proc_thread_exited.
 *
 * It may seem unneccessary to push the work of cleaning up the thread
 * over to the process. However, if you implement MTP, a thread
 * exiting does not necessarily mean that the process needs to be
 * cleaned up.
 */
void
kthread_exit(void *retval)
{
		dbg(DBG_PRINT, "kthread_code_path_check\n");
		curthr->kt_wchan = NULL;
		curthr->kt_qlink.l_next = NULL;
		curthr->kt_qlink.l_prev = NULL;

		KASSERT(!curthr->kt_wchan);
		dbg(DBG_PRINT, "(GRADING1A 3.c) Kthread's blocked on queue is empty\n");
		KASSERT(!curthr->kt_qlink.l_next && !curthr->kt_qlink.l_prev);
		dbg(DBG_PRINT, "(GRADING1A 3.c) Kthread's link on ktqueue is empty\n");
		KASSERT(curthr->kt_proc == curproc);
		dbg(DBG_PRINT, "(GRADING1A 3.c) Curthr and curproc match\n");

		curthr->kt_retval = retval;
		curthr->kt_state = KT_EXITED;
                
		proc_thread_exited(retval);
		return;
}

/*
 * The new thread will need its own context and stack. Think carefully
 * about which fields should be copied and which fields should be
 * freshly initialized.
 *
 * You do not need to worry about this until VM.
 */
kthread_t *
kthread_clone(kthread_t *thr)
{
        /*NOT_YET_IMPLEMENTED("VM: kthread_clone");*/
        kthread_t *cloned_thr = NULL;

		/*Create new thread*/
		cloned_thr = slab_obj_alloc(kthread_allocator);

		KASSERT(thr != NULL);

		/*new context and stack...*/
		thr->kt_kstack = alloc_stack();
		KASSERT(thr->kt_kstack != NULL);

		/*New context... EBP points to correct place in stack after returning from this function*/
			context_setup(&cloned_thr->kt_ctx, NULL, NULL, NULL, cloned_thr->kt_kstack, DEFAULT_STACK_SIZE, curproc->p_pagedir); /*current process page directory...*/

		/*
		 *Now copy address of func, arg1 and arg2 from old thread's context
		 *to new thread context. We know where they are with respect to
		 *EBP.
		 *Function is at EBP - sizeof(context_func_t)
		 *arg1 at EBP - sizeof(context_func_t) - sizeof(long)
		 *arg2 at EBP - sizeof(context_func_t) - sizeof(long) - sizeof(void *)
		 *
		 */
		/*CHECK!!!!*/
		/*arg2 address*/
		*(void **)(cloned_thr->kt_ctx.c_ebp - sizeof(context_func_t) - sizeof(long) - sizeof(void *)) = *(void **)(thr->kt_ctx.c_ebp - sizeof(context_func_t) - sizeof(long) - sizeof(void *));

		/*arg1 address*/
		*(int *)(cloned_thr->kt_ctx.c_ebp - sizeof(context_func_t) - sizeof(long)) = *(int *)(thr->kt_ctx.c_ebp - sizeof(context_func_t) - sizeof(long));

		/*function*/
		*(context_func_t *)(cloned_thr->kt_ctx.c_ebp - sizeof(context_func_t)) = *(context_func_t *)(thr->kt_ctx.c_ebp - sizeof(context_func_t));

		/*init other values..*/
		cloned_thr->kt_retval = NULL;
			cloned_thr->kt_errno = NULL;
			cloned_thr->kt_cancelled = thr->kt_cancelled;
			cloned_thr->kt_wchan = thr->kt_wchan;		/*Same wait queue?*/
			cloned_thr->kt_proc = curproc;			/*Single thread per process. Assign to current process?? Newly created proess..?*/
			cloned_thr->kt_state = thr->kt_state;   	/*Copy state...?*/

		/*Insert thread in current process?? Curproc should be cloned process?? Or does fork do this?...*/
		/*list_insert_head(&(curproc->p_threads), &(cloned_thr->kt_plink));*/
        return cloned_thr;
}

/*
 * The following functions will be useful if you choose to implement
 * multiple kernel threads per process. This is strongly discouraged
 * unless your weenix is perfect.
 */
#ifdef __MTP__
int
kthread_detach(kthread_t *kthr)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_detach");
        return 0;
}

int
kthread_join(kthread_t *kthr, void **retval)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_join");
        return 0;
}

/* ------------------------------------------------------------------ */
/* -------------------------- REAPER DAEMON ------------------------- */
/* ------------------------------------------------------------------ */
static __attribute__((unused)) void
kthread_reapd_init()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_init");
}
init_func(kthread_reapd_init);
init_depends(sched_init);

void
kthread_reapd_shutdown()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_shutdown");
}

static void *
kthread_reapd_run(int arg1, void *arg2)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_run");
        return (void *) 0;
}
#endif
