/******************************************************************************/
/* Important Spring 2022 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

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
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);
static void       hard_shutdown(void);

static context_t bootstrap_context;
extern int gdb_wait;

extern void *faber_thread_test(int, void*);
extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern int vfstest_main(int, char**);
extern int faber_fs_thread_test(kshell_t *kshell,int, char**);
extern int faber_directory_test(kshell_t *kshell,int, char**);



/* Below queue and mutex are meant to run the custom mutex */
ktqueue_t test_queue;
kmutex_t test_mutex;

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
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5fe480c61ff40c2bc39f86049086faa4cfbcf6ab2ea6173d1e845e0be42eaa9b3f22f3007986a2ae5b\n");
        /* necessary to finalize page table information */
        pt_template_init();

        //NOT_YET_IMPLEMENTED("PROCS: bootstrap");

        proc_t *idle_process = proc_create("Idle Process");
        curproc = idle_process;

        KASSERT(NULL != curproc);
        KASSERT(PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT,"(GRADING1A 1.a)\n");

        kthread_t *idle_thread = kthread_create(idle_process, idleproc_run, arg1, arg2);
        
        curthr = idle_thread;
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT,"(GRADING1A 1.a)\n");

        context_switch(&bootstrap_context, &curthr->kt_ctx);

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

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);
        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");
        do_mkdir("/dev");
        
        do_mknod("/dev/null",S_IFCHR,MKDEVID(1,0));
        do_mknod("/dev/zero",S_IFCHR,MKDEVID(1,1));
        do_mknod("/dev/tty0",S_IFCHR,MKDEVID(2,0));
        do_mknod("/dev/tty1",S_IFCHR,MKDEVID(2,1));
        do_mknod("/dev/sda", S_IFBLK, MKDEVID(1,0));

        //struct stat Status;
        //do_stat("/dev/tty0", &Status);

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
initproc_create()
{
        //NOT_YET_IMPLEMENTED("PROCS: initproc_create");

        proc_t *init_process = proc_create("Init Process");
        KASSERT(NULL != init_process);
        KASSERT(PID_INIT == init_process->p_pid);
        dbg(DBG_PRINT,"(GRADING1A 1.b)\n");

        kthread_t *init_thread = kthread_create(init_process, initproc_run, 0, 0);
        
        KASSERT(NULL != init_thread);
        dbg(DBG_PRINT,"(GRADING1A 1.b)\n");

        return init_thread;
}

#ifdef __DRIVERS__

int run_faber_thread_test(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        //dbg(DBG_TEMP, "(GRADING#X Y.Z): do_foo() is invoked, argc = %d, argv = 0x%08x\n",
        //        argc, (unsigned int)argv);

        proc_t *process = proc_create("Faber Thread Test");
        kthread_t *thread = kthread_create(process, faber_thread_test, 0, NULL);
        sched_make_runnable(thread);

        do_waitpid(process->p_pid, 0, NULL);

        /*
        * Must not call a test function directly.
        * Must create a child process, create a thread in that process and
        *     set the test function as the first procedure of that thread,
        *     then wait for the child process to die.
        */
        return 0;
}

int run_sunghan_test(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        //dbg(DBG_TEMP, "(GRADING#X Y.Z): do_foo() is invoked, argc = %d, argv = 0x%08x\n",
        //        argc, (unsigned int)argv);

        proc_t *process = proc_create("Sunghan Test");
        kthread_t *thread = kthread_create(process, sunghan_test, 0, NULL);
        sched_make_runnable(thread);

        do_waitpid(process->p_pid, 0, NULL);

        /*
        * Must not call a test function directly.
        * Must create a child process, create a thread in that process and
        *     set the test function as the first procedure of that thread,
        *     then wait for the child process to die.
        */
        return 0;
}

int run_sunghan_deadlock_test(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        //dbg(DBG_TEMP, "(GRADING#X Y.Z): do_foo() is invoked, argc = %d, argv = 0x%08x\n",
        //        argc, (unsigned int)argv);

        proc_t *process = proc_create("Sunghan Deadlock Test");
        kthread_t *thread = kthread_create(process, sunghan_deadlock_test, 0, NULL);
        sched_make_runnable(thread);

        do_waitpid(process->p_pid, 0, NULL);

        /*
        * Must not call a test function directly.
        * Must create a child process, create a thread in that process and
        *     set the test function as the first procedure of that thread,
        *     then wait for the child process to die.
        */
        return 0;
}

void* pre_cancel_test(int arg1, void* arg2) {
        if(sched_cancellable_sleep_on(&test_queue) == -EINTR) {
                do_exit(0);
        }
        do_exit(-1);
        return NULL;
}

void* pre_mutex_cancel_test(int arg1, void* arg2) {

        if(kmutex_lock_cancellable(&test_mutex) == -EINTR)
        {
                //Cancelled thread executes this
                do_exit(0);
        }
        do_exit(-1);
        return NULL;
}

void* mutex_sleep_cancel_test(int arg1, void* arg2) {
        kthread_t *thread_to_be_cancelled;
        if (arg2) {
                thread_to_be_cancelled = (kthread_t*)arg2;
        } else {
                sched_wakeup_on(&test_queue);
        }

        if(kmutex_lock_cancellable(&test_mutex) == -EINTR)
        {
                //Cancelled thread executes this
                do_exit(0);
        }
        
        // First thread executes this
        if(sched_cancellable_sleep_on(&test_queue) == -EINTR) {
                do_exit(-1);
        }

        if(thread_to_be_cancelled != curthr){
                kthread_cancel(thread_to_be_cancelled,0);
                kmutex_unlock(&test_mutex);
        }
        do_exit(0);
        return NULL;
}

void* vfs_test_helper(int arg1, void* arg2){
        vfstest_main(arg1, arg2);
        return NULL;
}

int run_custom_tests(kshell_t *kshell, int argc, char** argv) {
        KASSERT(kshell != NULL);
        kmutex_init(&test_mutex);
        sched_queue_init(&test_queue);
        
        /* Below test is to check if sched_cancellable sleep on 
         * returns when the thread is already cancelled
         */
        proc_t *proc1 = proc_create("Proc to be cancelled");
        kthread_t *thread = kthread_create(proc1, pre_cancel_test, 0 , NULL);
        kthread_cancel(thread,0);
        sched_make_runnable(thread);
        while(do_waitpid(proc1->p_pid,0,NULL) != proc1->p_pid)
                ;
        
        /* Below test is to check if sched_make_runnable 
         * returns immediately when NULL is passed
         */
        thread = NULL;
        sched_make_runnable(thread);

        /* Below test is to check if a cancelled thread can acquire a lock,
         * We create a thread which acquires a cancellable lock and is 
         * cancelled before it is made runnable
         */
        proc_t *proc3 = proc_create("Proc to be cancelled");
        kthread_t *thread3 = kthread_create(proc3, pre_mutex_cancel_test, 0 , NULL);
        kthread_cancel(thread3,0);
        sched_make_runnable(thread3);
        while(do_waitpid(proc3->p_pid,0,NULL) != proc3->p_pid)
                ;

        /* Below test is to check if a cancelled thread can acquire a lock 
         * after getting out of sleep queue,
         * We create a thread which acquires a cancellable lock and is 
         * cancelled after it is made runnable but befor it can acquire a lock
         */
        proc_t *proc4 = proc_create("Proc to acquire mutex");
        proc_t *proc5 = proc_create("Proc to be cancelled");
        kthread_t *thread5 = kthread_create(proc5, mutex_sleep_cancel_test, 0 , NULL);
        kthread_t *thread4 = kthread_create(proc4, mutex_sleep_cancel_test, 0 , (void*)thread5);
        sched_make_runnable(thread4);
        sched_make_runnable(thread5);
        while(do_waitpid(-1,0,NULL) != -ECHILD)
                ;
        return 0;
}

int custom_test_kernel2(kshell_t *kshell, int argc, char** argv) {
        int fd;
        dbg(DBG_PRINT,"(TEST INSIDE)\n");
        do_mkdir("test_dir");
        do_chdir("test_dir");
        fd = do_open("test_file", O_CREAT | O_RDONLY);
        do_close(fd);

        fd = do_open("another_file", O_CREAT | O_RDONLY);
        do_close(fd);
        
        do_rename("test_file", "file");
        
        do_rename("test_file", "file");
        
        do_rename("another_file", "file");
        
        do_rename("anotherfile", "new_file");
        
        do_rename("another_file", "file/my_file");

        do_unlink("file");
        do_unlink("another_file");

        do_chdir("..");
        do_rmdir("test_dir");

        return NULL;
}


int run_vfs_test(kshell_t *kshell, int argc, char **argv)
{
        KASSERT(kshell != NULL);
        //dbg(DBG_TEMP, "(GRADING#X Y.Z): do_foo() is invoked, argc = %d, argv = 0x%08x\n",
        //        argc, (unsigned int)argv);

        proc_t *process = proc_create("Sunghan Deadlock Test");
        kthread_t *thread = kthread_create(process, vfs_test_helper, 1, NULL);
        sched_make_runnable(thread);

        do_waitpid(process->p_pid, 0, NULL);

        /*
        * Must not call a test function directly.
        * Must create a child process, create a thread in that process and
        *     set the test function as the first procedure of that thread,
        *     then wait for the child process to die.
        */
        return 0;
}

void* kshell_execute(int arg1, void *arg2){
        kshell_add_command("faber", run_faber_thread_test, "Runs Faber Test");
        kshell_add_command("sunghan", run_sunghan_test, "Runs Sunghan Test");
        kshell_add_command("custom_test1", run_custom_tests, "Runs Custom Tests for Grading 1E");
        kshell_add_command("sunghan_deadlock", run_sunghan_deadlock_test, "Runs Sunghan Test");
        kshell_add_command("faber_fs", faber_fs_thread_test, "Runs Faber FS");
        kshell_add_command("faber_dir", faber_directory_test, "Runs Faber Directory Test");
        kshell_add_command("vfs_test", run_vfs_test, "Runs Faber FS");
        kshell_add_command("custom_test2", custom_test_kernel2, "Runs Custom Tests for Grading 2D");


        
        /* Create a kshell on a tty */
        int err = 0;
        kshell_t *ksh = kshell_create(0);
        KASSERT(ksh && "did not create a kernel shell as expected");
        dbg(DBG_PRINT,"(GRADING1B)\n");
        /* Run kshell commands until user exits */
        while ((err = kshell_execute_next(ksh)) > 0);
        KASSERT(err == 0 && "kernel shell exited with an error\n");
        kshell_destroy(ksh);
        return (void*) NULL;
}


#endif /* __DRIVERS__ */

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2)
{
        //NOT_YET_IMPLEMENTED("PROCS: initproc_run");

        //kshell_execute(0, 0);
        char *const argvec[] = { "/sbin/init", NULL};
        char *const envvec[] = { NULL };
        kernel_execve("/sbin/init", argvec, envvec);


        int status;
        while(do_waitpid(-1, 0, &status) != -ECHILD)
                ;

        //faber_thread_test(0,NULL);
        
        
        return NULL;
}





