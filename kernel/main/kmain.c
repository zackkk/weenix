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

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "test/kshell/kshell.h"
#include "errno.h"

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void       hard_shutdown(void);
static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);

static context_t bootstrap_context;
static int gdb_wait = GDBWAIT;

extern void *testproc(int arg1, void *arg2);
extern void *sunghan_test(int arg1, void *arg2);
extern void *sunghan_deadlock_test(int arg1, void *arg2);

extern void *vfstest_main(int, void*);

/* not used */
extern int faber_sparse_test(kshell_t *ksh, int argc, char **argv);
extern int faber_space_test(kshell_t *ksh, int argc, char **argv);
extern int faber_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);
extern int faber_cleardir(kshell_t *ksh, int argc, char **argv);


/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
	    pci_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
        /* This little loop gives gdb a place to synch up with weenix.  In the
         * past the weenix command started qemu was started with -S which
         * allowed gdb to connect and start before the boot loader ran, but
         * since then a bug has appeared where breakpoints fail if gdb connects
         * before the boot loader runs.  See
         *
         * https://bugs.launchpad.net/qemu/+bug/526653
         *
         * This loop (along with an additional command in init.gdb setting
         * gdb_wait to 0) sticks weenix at a known place so gdb can join a
         * running weenix, set gdb_wait to zero  and catch the breakpoint in
         * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
         *
         * DANGER: if GDBWAIT != 0, and gdb is not running, this loop will never
         * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
         * you expect.
         */
        while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2)
{
        /* necessary to finalize page table information */
        pt_template_init();

        /* NOT_YET_IMPLEMENTED("PROCS: bootstrap"); */
        proc_t *proc = proc_create("idle_process");
        curproc = proc;
        KASSERT(NULL != curproc);
        dbg(DBG_PRINT, "(GRADING1A 1.a) The current process (idle) is not NULL\n");
        KASSERT(PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.a) The current process is the idle process\n");

        /* context is created in kthread_create */
        kthread_t *thr = kthread_create(curproc, idleproc_run, 0, NULL);
        curthr = thr;
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a) The current thread is for the idle process\n");

        context_make_active(&(thr->kt_ctx));

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

	
        dbg(DBG_PRINT, "vn_root_node not NULL\n");
        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        /* NOT_YET_IMPLEMENTED("VFS: idleproc_run"); */

        /*
         *  Get the processes
         */
        proc_t *idle_proc = proc_lookup(PID_IDLE);
        proc_t *init_proc = proc_lookup(PID_INIT);
        KASSERT(NULL != idle_proc);
        KASSERT(NULL != init_proc);

        /*
         *  Set the current working directory
         */
        
        idle_proc->p_cwd = vfs_root_vn;
        init_proc->p_cwd = vfs_root_vn;
		
        /*
         *  Increment the reference count of the provided vnode.
         */
        vref(vfs_root_vn);
        vref(vfs_root_vn);

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        /* NOT_YET_IMPLEMENTED("VFS: idleproc_run"); */

        do_mkdir("/dev");
        int rc_null = do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID);
        int rc_zero = do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);
        int i;
        char tty_path[32];
        /*
         * ???????????????????????????????????????????????
         * number of tty dev
         */
        for(i = 0; i < 10; i++){
        	memset(tty_path, '\0', 32);
        	sprintf(tty_path, "/dev/tty%d", i);
        	int rc_tty_i = do_mknod(tty_path, S_IFCHR, MKDEVID(2,i));
        }

#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
	
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif


#ifdef __SHADOWD__
        /* wait for shadowd to shutdown */
        shadowd_shutdown();
#endif

#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void)
{
		proc_t *proc = proc_create("init_process");
		KASSERT(NULL != proc);
		dbg(DBG_PRINT, "(GRADING1A 1.b) The pointer to the init process is not NULL\n");
		KASSERT(PID_INIT == proc->p_pid);
		dbg(DBG_PRINT, "(GRADING1A 1.b) The pid of the init process is PID_INIT\n");

		/* kthread_t *kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2);
		 * thread is contained in process in kthread_create;
		 * argument 1234 is used for test purpose */
		kthread_t *thr = kthread_create(proc, initproc_run, 1234, NULL);
		KASSERT(NULL != thr);
		dbg(DBG_PRINT, "(GRADING1A 1.b) The pointer to the thread for the init process is not NULL\n");

        return thr;

}


#ifdef __DRIVERS__

static void *
faber_test(int arg1, void *arg2){
	/* faber_test.c */
	testproc(0,0);
	return NULL;
}
static void *
sunhan_test(int arg1, void *arg2){
	/* sunhan_test.c */
	sunghan_test(0,0);
	return NULL;
}
static void *
sunhan_deadlock_test(int arg1, void *arg2){
	/* sunhan_test.c */
	sunghan_deadlock_test(0,0);
	return NULL;
}


int ftests(kshell_t *kshell, int argc, char **argv)
{
    KASSERT(kshell != NULL);
    proc_t *p = proc_create("faber_test_proc");
    kthread_t *thr = kthread_create(p, faber_test, 555, NULL);
    sched_make_runnable(thr);
    sched_sleep_on(&curproc->p_wait); /* including context switch */
    return 0;
}
int stests(kshell_t *kshell, int argc, char **argv)
{
    KASSERT(kshell != NULL);
    proc_t *p = proc_create("sunhan_test_proc");
    kthread_t *thr = kthread_create(p, sunhan_test, 666, NULL);
    sched_make_runnable(thr);
    sched_sleep_on(&curproc->p_wait); /* including context switch */
    return 0;
}
int dtests(kshell_t *kshell, int argc, char **argv)
{
    KASSERT(kshell != NULL);
    proc_t *p = proc_create("sunhan_deadlock_test_proc");
    kthread_t *thr = kthread_create(p, sunhan_deadlock_test, 777, NULL);
    sched_make_runnable(thr);
    sched_sleep_on(&curproc->p_wait); /* including context switch */
    return 0;
}


#endif /* __DRIVERS__ */


#ifdef __VFS__

static void *
vfs_test(int arg1, void *arg2){
	/* sunhan_test.c */

	vfstest_main(1,0);
	return NULL;
}

int vtests(kshell_t *kshell, int argc, char **argv)
{
    KASSERT(kshell != NULL);
    proc_t *p = proc_create("vfs_test");
    kthread_t *thr = kthread_create(p, vfs_test, 888, NULL);
    sched_make_runnable(thr);
    sched_sleep_on(&curproc->p_wait); /* including context switch */
    return 0;
}

#endif /* __VFS__ */

#ifdef __VM__

static void *
hello_test(int arg1, void *arg2){
	/* sunhan_test.c */
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	kernel_execve("/usr/bin/hello", argv, envp);
	return NULL;
}

int hellotests(kshell_t *kshell, int argc, char **argv)
{
    KASSERT(kshell != NULL);
    proc_t *p = proc_create("hello_test");
    kthread_t *thr = kthread_create(p, hello_test, 1111, NULL);
    sched_make_runnable(thr);
    sched_sleep_on(&curproc->p_wait); /* including context switch */
    return 0;
}

#endif /* __VM__ */

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/bin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2)
{
	#ifdef __DRIVERS__
	
	kshell_add_command("ftest", ftests, "Invokes testproc()...");
	kshell_add_command("stest", stests, "Invokes sunghan_test()...");
	kshell_add_command("dtest", dtests, "Invokes sunghan_deadlock_test()...");
	#endif /* __DRIVERS__ */

#ifdef __VFS__

	kshell_add_command("vtest", vtests, "Invokes vfs_test()...");

#endif /* __VFS__ */

#ifdef __VM__

	kshell_add_command("hellotest", hellotests, "Invokes hello_test()...");

#endif /* __VM__ */


	kshell_t *kshell = kshell_create(0);
	if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
	while (kshell_execute_next(kshell))
		;
	kshell_destroy(kshell);

	vput(curproc->p_cwd);

    return NULL;
}


