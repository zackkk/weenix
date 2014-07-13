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

#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1) {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                        if (p->p_pid == pid) {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid) {
                                        return -1;
                                } else {
                                        goto failed;
                                }
                        }
                } list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
	dbg(DBG_PRINT, "proc_code_path_check\n");
        proc_t *new_process = NULL;
        
        /*Create a slab, using the proc_allocator*/
        new_process = (proc_t *)slab_obj_alloc(proc_allocator);
        
        /*Set process fields accordingly*/
        int pid = _proc_getid();
         
        new_process->p_pid = pid;
        
        /*copy process name*/
        memset(new_process->p_comm, 0, PROC_NAME_LEN);
        if(name != NULL){
                strncpy(new_process->p_comm, name, PROC_NAME_LEN);
                new_process->p_comm[PROC_NAME_LEN-1] = 0;               /*make sure string is always null terminated*/  
        }
        
        /*caller of this function is the running process, so it becomes the parent..*/
        new_process->p_pproc = curproc;
        
        
        KASSERT(PID_IDLE != pid || list_empty(&_proc_list)); /* pid can only be PID_IDLE if this is the first process */
        dbg(DBG_PRINT,"(GRADING1A 2.a) New process pid is %d and is %s process\n", new_process->p_pid, new_process->p_comm);
        
        KASSERT(PID_INIT != pid || PID_IDLE == curproc->p_pid); /* pid can only be PID_INIT when creating from idle process*/
        dbg(DBG_PRINT,"(GRADING1A 2.a) New process pid is %d and is %s process\n", new_process->p_pid, new_process->p_comm);
        
        /*If this is the init process (pid = 1), the proc_initproc to point to it*/
        if(new_process->p_pid == 1){
                proc_initproc = new_process;
        }
        
        /*init the children and thread list, empty*/
        list_init(&new_process->p_children);
        list_init(&new_process->p_threads);             
                                                         
        /*create thread, add it to the list of threads?*/

        /*Set exit status to zero initially (may change)*/
        new_process->p_status = 0;

        /*Set process status to running*/
        new_process->p_state = PROC_RUNNING;
                                                                                          
        /*init wait queue*/
        sched_queue_init(&new_process->p_wait);
        
        /*Create process page directory*/
        new_process->p_pagedir = pt_create_pagedir();
        
        /* init p_list_link and p_child_link */
        /* when created we have no children*/
        list_link_init(&(new_process->p_list_link));      /* link on the list of all processes */
        list_link_init(&(new_process->p_child_link));      /* link on proc list of children */
                                                             
        if(pid != 0){
                dbg(DBG_PRINT, "Inserted child with pid %d into parent (pid %d) children list\n", new_process->p_pid, curproc->p_pid);
                /*new process is child of current process, excep for idle process*/
                list_insert_tail(&(curproc->p_children), &(new_process->p_child_link));
        }

         /*no open files yet..*/
        int i = 0;
        for(i = 0; i < NFILES; i++){
                new_process->p_files[i] = NULL;        
        }
        
        /*TODO: current working directory*/
        new_process->p_cwd = vfs_root_vn;
        
        /* VM */
        new_process->p_brk = NULL;           /* process break; see brk(2) */
        new_process->p_start_brk = NULL;     /* initial value of process break */
        new_process->p_vmmap = NULL;         /* list of areas mapped into
                                          * process' user address
                                          * space */
        
        /*Add process to process list.*/
        new_process->p_list_link.l_next = NULL;
        new_process->p_list_link.l_prev = NULL;
        list_insert_tail(&_proc_list, &new_process->p_list_link);
        
        return new_process;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS) OK
 *    - Cleaning up VM mappings (VM) OK
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{
		dbg(DBG_PRINT, "proc_code_path_check\n");
        /*current process calls this function*/
        
        KASSERT(NULL != proc_initproc); /* should have an "init" process */
        dbg(DBG_PRINT,"(GRADING1A 2.b) We have an init process\n");
        
        KASSERT(1 <= curproc->p_pid); /* this process should not be idle process */
        dbg(DBG_PRINT,"(GRADING1A 2.b) This is not the idle process\n");
        
        KASSERT(NULL != curproc->p_pproc);
        dbg(DBG_PRINT,"(GRADING1A 2.b) The current process (pid %d) has a parent\n", curproc->p_pid);
        
        
        /*DEAD process*/
        curproc->p_state = PROC_DEAD;
        /*set exit status*/
        curproc->p_status = status;
        /*reparent child process to init process*/
        list_link_t *link = NULL;
        proc_t *my_child_proc = NULL;
	
	/*Close all open files...*/
	int i = 0;
        for(i = 0; i < NFILES; i++){
                if(curproc->p_files[i] != NULL){
			do_close(i);
		}
        }
        

        /*Do this for any process except init process*/
        /*we don't check for idle, since we dont exit from idle this way*/
        if(curproc->p_pid > 1){
        
                for(link = curproc->p_children.l_next; link != &(curproc->p_children); ){
                        my_child_proc = list_item(link, proc_t, p_child_link);
                        
                        /*assign init as new parent of orphaned child*/
                        my_child_proc->p_pproc = proc_initproc;
                        
                        /*before we lose the next address, point to next element in dead process child list*/
                        link =  my_child_proc->p_child_link.l_next;
                        
                        /*remove this child from dead process children list*/
                        list_remove(&my_child_proc->p_child_link);
                        
                        /*add child to list of init children...*/
                        list_insert_tail(&(proc_initproc->p_children), &(my_child_proc->p_child_link));
                }
                
                /*DEAD process will be removed when PARENT call waitpid on it, since we need the return status*/
                sched_wakeup_on(&curproc->p_pproc->p_wait);
                sched_switch();
        }
        /*
         * the current process is init
         * p_children must point to itself, since we CAN'T do cleanup while it has children.
         */
        else{
                int w = 0;
                
                while(do_waitpid(-1, 0, &w) != -ECHILD);
                
                KASSERT(curproc == proc_initproc && curproc->p_children.l_next == &(curproc->p_children));

                /*DEAD process*/
                curproc->p_state = PROC_DEAD;
        
                /*set exit status*/
                curproc->p_status = status;

                /*
                 * switch back to idle process
                 */
                dbg(DBG_PRINT, "Switching context: Old: process %d \n", curproc->p_pid);

                kthread_t *oldThread = curthr;
                curproc = curproc->p_pproc;
                
                /* get kthread of the parent process, and set it as the current thread */
                list_link_t *link;
                link = curproc->p_threads.l_prev;
                curthr = list_item(link, kthread_t, kt_plink);
                list_remove(link);

                dbg(DBG_PRINT, "Switching context: New: process %d \n", curproc->p_pid);
                context_switch(&oldThread->kt_ctx, &curthr->kt_ctx);
        }
        
        KASSERT(NULL != curproc->p_pproc); /* this process should have parent process */
        dbg(DBG_PRINT,"(GRADING1A 2.b) The current dead process has a parent\n");

        return;
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
	dbg(DBG_PRINT, "proc_code_path_check\n");
        KASSERT(p != proc_initproc);           
        struct kthread *parent_thread = NULL;
        
        list_link_t *link2;
	link2 = p->p_threads.l_next;
	kthread_t *thr = list_item(link2, kthread_t, kt_plink);
	dbg(DBG_PRINT, "Current process being killed %d\n", p->p_pid);
	
        sched_cancel(thr);
        sched_make_runnable(curthr);
        sched_switch();

        p->p_state = PROC_DEAD;
        p->p_status = status;

        /*reparent child process to init process*/
        list_link_t *link = NULL;
        proc_t *my_child_proc = NULL;
        
        for(link = p->p_children.l_next; link != &(p->p_children); ){
                my_child_proc = list_item(link, proc_t, p_child_link);
                
                /*assihn init as new parent of orphaned child*/
                my_child_proc->p_pproc = proc_initproc;
                
                /*before we lose the next address, point to next element in dead process child list*/
                link =  my_child_proc->p_child_link.l_next;
                
                /*remove this child from dead process children list*/
                list_remove(&my_child_proc->p_child_link);
                
                /*add ALIVE child to HEAD of list of init children...*/
                list_insert_head(&(proc_initproc->p_children), &(my_child_proc->p_child_link));
        }
        dbg(DBG_PRINT,"Process %s (pid %d) has been killed\n", p->p_comm, p->p_pid);
        return;
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
	dbg(DBG_PRINT, "proc_code_path_check\n");
        /*Dont kill init nor idle*/
        /*kill using proc_kill*/
        proc_t *current_proc = NULL;
        list_link_t *list_item = NULL;
        
        dbg(DBG_PRINT, "Process with pid %d called kill_all()\n", curproc->p_pid);
        
        /*If no dead children... exit*/
        if(proc_initproc->p_children.l_next == &proc_initproc->p_children){
                dbg(DBG_PRINT, "Init process has no children\n");
                return;
        }
        /*
         * This loop will exit when all children of init are dead.
         * Each time we kill a process, we reparent its children to init
         * Thus, when we exit the loop, all process except idle and init will be alive
         */
        int alive_children = 0;
        list_link_t *link = NULL;
        while(1){
                
                /*Get head of queue of init process children list*/
                /*Get first children.*/
                link = proc_initproc->p_children.l_next;
                
                /*Assume no alive children until one found*/
                alive_children = 0;
                

                while(link != &proc_initproc->p_children){
                        current_proc =  list_item(link, proc_t, p_child_link);
                        

                        if(current_proc->p_state == PROC_RUNNING){
                                alive_children = 1;
                                break;
                        }
                        else{
                                link = link->l_next;
                        }
                        
                }
                
                /*If we reached the head of the p_queue*/
                if(!alive_children){
                        /*List pointing to itself is empty, it means we have no more children.*/
                        break;
                }
                else{
                        if(current_proc->p_pproc->p_pid != 0){
                            /*Kill that process, reparenting children to init*/
                                proc_kill(current_proc, 0);
                                dbg(DBG_PRINT, "returned from proc_kill()\n");
                        }
                        else{
                                dbg(DBG_PRINT, "current_process has parent with pid %d\n", current_proc->p_pproc->p_pid);
                        }
                        

                }
        }
        dbg(DBG_PRINT, "switching processes\n");
        sched_switch();         
        return;
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
		dbg(DBG_PRINT, "proc_code_path_check\n");
    	/* check NULL for page fault */
    	int status = retval == NULL ? 0 : *((int*)retval);
    	proc_cleanup(status);
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 * 
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{
		dbg(DBG_PRINT, "proc_code_path_check\n");
        KASSERT(pid == -1 || pid > 0);
        KASSERT(options == 0);
        
        proc_t *p = NULL;
        kthread_t *thr = NULL;
        list_link_t *link = NULL;
        int return_pid = -1;

        /* case 3: If current process has no children, return -ECHILD */
        if(pid >= -1  && curproc->p_children.l_next == &curproc->p_children){
                return -ECHILD;
        }
        
        /* case 1: If pid == -1 */
        if(pid == -1){

                link = curproc->p_children.l_next;  
                
                while(1){
                        
                        /*If we wrap around the list, we didn't found child*/
                        if(link == &(curproc->p_children)){
                                dbg(DBG_PRINT, "No dead child found for process %s yet. Waiting on p_wait\n", curproc->p_comm);
                                
                                sched_sleep_on(&curproc->p_wait);
                                
                                /*if sleep_on blocks wait_pid()*/
                                /*Start again from head of queue, looking for a dead child*/
                                link = curproc->p_children.l_next;
                        }
                        
                        /*get child*/
                        p = list_item(link, proc_t, p_child_link);
                        
                        KASSERT(NULL != p);
                        dbg(DBG_PRINT,"(GRADING1A 2.c) Process p is not NULL\n");
                        
                        KASSERT(NULL != p->p_pagedir);
                        dbg(DBG_PRINT,"(GRADING1A 2.c) Process p has a page directory\n");
                        
                        
                        /*We found a dead child*/
                        if(p->p_state == PROC_DEAD){
                                
                                *status = p->p_status;                      /*return status*/
                                list_remove(&p->p_child_link);               /*remove dead process from list of curproc children list*/
                                list_remove(&p->p_list_link);                /*remove from global process list*/
                                return_pid = p->p_pid;                      /*copy return pid*/
                                
                                KASSERT(-1 == pid || p->p_pid == pid);
                                dbg(DBG_PRINT,"(GRADING1A 2.c) Found a dead process with pid %d\n", p->p_pid);
                                
                                thr = list_item(p->p_threads.l_next, kthread_t, kt_plink);
                                

                                KASSERT(KT_EXITED == thr->kt_state);    /* thr points to a thread to be destroied */
                                dbg(DBG_PRINT,"(GRADING1A 2.c) thr points to a thread to be destroied \n");

                                slab_obj_free(proc_allocator, p);           /*free memory used by process*/
                                return return_pid;
                        }
                        link = link->l_next;
                }
        }
        /* case 2: If pid > 0 and given pid is child of curproc*/
        else if(pid > 0){
                /*Look for the given pid...*/
                for(link = curproc->p_children.l_next; link != &(curproc->p_children); link = link->l_next){
                        
                        p = list_item(link, proc_t, p_child_link);
                        
                        KASSERT(NULL != p);
                        dbg(DBG_PRINT,"(GRADING1A 2.c) Process p is not NULL\n");
                        
                        KASSERT(NULL != p->p_pagedir);
                        dbg(DBG_PRINT,"(GRADING1A 2.c) Process p has a page directory\n");
                        
                        dbg(DBG_PRINT,"Current child pid %d, requested pid is %d\n", p->p_pid, pid);
                        
                        /*we found the process with the given pid...*/
                        if(p->p_pid == pid){
                                if(p->p_state == PROC_DEAD){
                                        *status = p->p_status;                      
                                        list_remove(&p->p_child_link);               
                                        list_remove(&p->p_list_link);           
                                        return_pid = p->p_pid;                      
                                        
                                        KASSERT(-1 == pid || p->p_pid == pid);
                                        dbg(DBG_PRINT,"(GRADING1A 2.c) Found a dead process with pid %d (status %d)\n", p->p_pid, p->p_status);
                                        
                                        slab_obj_free(proc_allocator, p);         
                                        
                                        return return_pid;
                                        
                                }
                                else{
                                        dbg(DBG_PRINT,"(GRADING1A 2.c) Found child process with pid %d, but alive... waiting.\n", p->p_pid);
                                        /*waiting for the child to die to switching context in sched_sleep_on*/
                                        sched_sleep_on(&curproc->p_wait);
                                        
                                        /*if sched_sleep_on() is block, */
                                        *status = p->p_status;                      
                                        list_remove(&p->p_child_link);               
                                        list_remove(&p->p_list_link);                
                                        return_pid = p->p_pid;                      
                                        
                                        KASSERT(-1 == pid || p->p_pid == pid);
                                        dbg(DBG_PRINT,"(GRADING1A 2.c) Found a dead process with pid %d (status %d)\n", p->p_pid, p->p_status);
                                        
                                        slab_obj_free(proc_allocator, p);           
                                        
                                        return return_pid;
                                }
                        }
                }
                return -ECHILD;                
        }
        return -ECHILD;
}

/*
 * Cancel all threads, join with them, and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
		dbg(DBG_PRINT, "proc_code_path_check\n");
        kthread_cancel(curthr, &status);
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        return size;
}


