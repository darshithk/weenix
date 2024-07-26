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
        //NOT_YET_IMPLEMENTED("PROCS: proc_create");
        proc_t *process = (proc_t*) slab_obj_alloc(proc_allocator);

        pid_t pid = _proc_getid();
        KASSERT(PID_IDLE != pid || list_empty(&_proc_list));
        KASSERT(PID_INIT != pid || PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT,"(GRADING1A 2.a)\n");

        process->p_pid = pid;
        strcpy(process->p_comm, name);
        process->p_pproc = curproc;
        process->p_state = PROC_RUNNING;

        list_init(&process->p_threads);
        list_init(&process->p_children);
        sched_queue_init(&process->p_wait);
        /* TODO: Replace below line with pt_create_dir and add corresponding pt_destroy in destroy_pcb for Kernel 3 */
        process->p_pagedir = pt_create_pagedir();
      
        list_link_init(&process->p_list_link);
        list_insert_tail(&_proc_list, &process->p_list_link);

        list_link_init(&process->p_child_link);
        if(process->p_pid != PID_IDLE){
                list_insert_tail(&curproc->p_children, &process->p_child_link);
                dbg(DBG_PRINT,"(GRADING1A)\n");
        }

        /* TODO: Handle these in Kernel 2
         *
         * *p_files[NFILES];
         * *p_cwd;
         * *p_brk;
         * *p_start_brk;
         * *p_vmmap;
         */

#ifdef __VFS__
        if(vfs_root_vn){
                process->p_cwd = curproc->p_cwd? curproc->p_cwd: vfs_root_vn;
                vref(process->p_cwd);
        }
        
        
        int fd;
        for(fd = 0;fd<NFILES;fd++){
                process->p_files[fd] = NULL;
        }
        
        /* set current process to INIT */
        if(process->p_pid == PID_INIT){
                proc_initproc = process;
                dbg(DBG_PRINT,"(GRADING1A)\n");
        }
#endif

#ifdef __VM__
        process->p_vmmap = vmmap_create();
        process->p_brk = NULL;
        process->p_start_brk = NULL;
        process->p_vmmap->vmm_proc = process;
#endif

        return process;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
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
        int fd;
        //NOT_YET_IMPLEMENTED("PROCS: proc_cleanup");

        KASSERT(NULL != proc_initproc);
        KASSERT(1 <= curproc->p_pid);
        KASSERT(NULL != curproc->p_pproc);
        dbg(DBG_PRINT,"(GRADING1A 2.b)\n");

        proc_t *process = curproc;
        process->p_status = status;
        process->p_state = PROC_DEAD;

        /* TODO: cleanup follow variables in Kernel 2
         *
         *      *p_files[NFILES];
         *      *p_cwd;
         *      *p_brk;
         *      *p_start_brk;
         *      *p_vmmap;
         */

        /* Below we are waking up the parent from its wait queue `p_wait` 
           parent is awakened by removing it from its p_wait queue and is added to kt_runq */

        if(!sched_queue_empty(&process->p_pproc->p_wait)){
                sched_wakeup_on(&process->p_pproc->p_wait);
                dbg(DBG_PRINT,"(GRADING1A)\n");
        }

        //REPARENT CHILDREN TO INIT
        //Iterate through p_children and change p_pproc to point to INIT
        //Remove and link process in children list of INIT

        /* Reparenting of Non-critical and not DEAD child processes, from the DEAD parent to INIT */
        proc_t *p;
        if(!list_empty(&process->p_children)){
                list_iterate_begin(&process->p_children, p, proc_t, p_child_link) {
                        if(curproc != proc_initproc){
                                p->p_pproc = proc_initproc;
                                if(p->p_state != PROC_DEAD)
                                        list_insert_tail(&proc_initproc->p_children, &p->p_child_link);
                        }
                } list_iterate_end();
                dbg(DBG_PRINT,"(GRADING1C)\n");
        }

#ifdef __VFS__
        for(fd = 0;fd<NFILES;fd++){
                if(process->p_files[fd] && process->p_files[fd]->f_refcount > 0){
                        do_close(fd);
                }
        }

        if(process->p_pid != 2 && process->p_cwd){
                vput(process->p_cwd);
                curproc->p_cwd = NULL;
        }
#endif

#ifdef __VM__
        if (curproc->p_vmmap)
        {
                vmmap_destroy(curproc->p_vmmap);
                curproc->p_vmmap = NULL;
        }
#endif   

        KASSERT(NULL != curproc->p_pproc);
        KASSERT(KT_EXITED == curthr->kt_state);
        dbg(DBG_PRINT,"(GRADING1A 2.b)\n");

}

/*
 * This is a custom function which contains cleanup logic,
 * used to free up memory allocated to dead processes and their corresponding threads
 * Following actions occur:
 *  - destory the first thread in the p_threads list as we are not following MTP
 *  - remove the p_list_link entry
 *  - remove the p_child_link entry
 *  - free up memory using slab_obj_free
 */
void destroy_pcb(proc_t* process) {
        kthread_t *thread = list_head(&process->p_threads, kthread_t, kt_plink);
        list_remove_head(&process->p_threads);
        kthread_destroy(thread);
        pt_destroy_pagedir(process->p_pagedir);

#ifdef __VM__
        if(process->p_vmmap){
                vmmap_destroy(process->p_vmmap);
                process->p_vmmap = NULL;
        }
#endif

        list_remove(&process->p_list_link);
        list_remove(&process->p_child_link);
        slab_obj_free(proc_allocator, (void *)process);
        dbg(DBG_PRINT,"(GRADING1A)\n");
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
        // NOT_YET_IMPLEMENTED("PROCS: proc_kill");
        kthread_t* thread;
        dbg(DBG_PRINT,"(GRADING1C)\n");
        list_iterate_begin(&p->p_threads, thread, kthread_t, kt_plink) {
                /* cancel all threads in process */
                kthread_cancel(thread, (void*)status);
        } list_iterate_end();
        p->p_status = status;
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
        // NOT_YET_IMPLEMENTED("PROCS: proc_kill_all");
        proc_t* p;
        dbg(DBG_PRINT,"(GRADING1C)\n");
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                /* checking if the process is not IDLE and not a child of IDLE */
                if(p->p_pid != PID_IDLE && p->p_pproc->p_pid != PID_IDLE)
                {
                        dbg(DBG_PRINT,"(GRADING1C)\n");
                        if(curproc != p)
                                proc_kill(p,0);
                }
        } list_iterate_end();
        /* kill the current process if it is not IDLE/INIT */
        if(curproc->p_pid != PID_IDLE && p->p_pproc->p_pid != PID_IDLE){
                dbg(DBG_PRINT,"(GRADING1C)\n");
                proc_kill(curproc, 0);
        }
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
        //NOT_YET_IMPLEMENTED("PROCS: proc_thread_exited");
        proc_cleanup((int)retval);
        dbg(DBG_PRINT,"(GRADING1A)\n");
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
        //NOT_YET_IMPLEMENTED("PROCS: do_waitpid");
        dbg(DBG_PRINT,"(GRADING1C)\n");
        int child_present = 0;
        proc_t *process = curproc, *p;

        /* Check if the process who wants to wait has children,
         * if specific PID is mentioned, then we check if `pid` is a child of current process
         * sets is_child to 1 and status to p->p_status which is 
         */
        list_iterate_begin(&process->p_children, p, proc_t, p_child_link) {
                if(p->p_pid == pid || pid == -1)
                {
                        child_present = 1;
                        dbg(DBG_PRINT,"(GRADING1C)\n");
                }
        } list_iterate_end();

        if(child_present == 0){
                dbg(DBG_PRINT,"(GRADING1C)\n");
                return -ECHILD;
        }

        //If child process is not DEAD yet, add on p_wait queue and give up CPU
        do{
                if(pid == -1){
                        list_iterate_begin(&process->p_children, p, proc_t, p_child_link) {
                                if(p->p_state == PROC_DEAD)
                                {
                                        pid_t ret_pid = p->p_pid;
                                        dbg(DBG_PRINT,"(GRADING1C)\n");
                                        if(status){
                                                *status = p->p_status;
                                                dbg(DBG_PRINT,"(GRADING1C)\n");
                                        }

                                        KASSERT(NULL != p);
                                        KASSERT(NULL != p->p_pagedir);
                                        dbg(DBG_PRINT,"(GRADING1A 2.c)\n");
                                        destroy_pcb(p);

                                        return ret_pid;      
                                }
                        } list_iterate_end();
                }
                else{
                        list_iterate_begin(&process->p_children, p, proc_t, p_child_link) {
                                if(p->p_pid == pid && p->p_state == PROC_DEAD)
                                {
                                        pid_t ret_pid = p->p_pid;
                                        dbg(DBG_PRINT,"(GRADING1C)\n");
                                        if(status){
                                                *status = p->p_status;
                                                dbg(DBG_PRINT,"(GRADING1C)\n");
                                        }
                                        
                                        KASSERT(NULL != p);
                                        KASSERT(-1 == pid || p->p_pid == pid);
                                        KASSERT(NULL != p->p_pagedir);
                                        dbg(DBG_PRINT,"(GRADING1A 2.c)\n");
                                        destroy_pcb(p);

                                        return ret_pid; 
                                }
                        } list_iterate_end();
                }
                sched_cancellable_sleep_on(&process->p_wait);
        }while(1);

        return 0;
}

/*
 * Cancel all threads and join with them (if supporting MTP), and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
        // NOT_YET_IMPLEMENTED("PROCS: do_exit");
        //Since we are not implementing MTP, we exit from curthr
        dbg(DBG_PRINT,"(GRADING1C)\n");
        kthread_exit((void*)status);

}

