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
        vmarea_t *vma, *fork_vma;
        pframe_t *pf;
        mmobj_t *to_delete, *new_shadowed;

        // NOT_YET_IMPLEMENTED("VM: do_fork");

        //Precondition KASSERTs
        KASSERT(regs != NULL);
        KASSERT(curproc != NULL);
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");
        //CREATE PROCESS
        proc_t* fork_process = proc_create("forked_child");

        //CREATE THREAD
        kthread_t *fork_thread = kthread_clone(curthr);
        fork_thread->kt_proc = fork_process;

        list_insert_tail(&fork_process->p_threads, &fork_thread->kt_plink);

        // Set EAX to 0 so that when fork_setup_stack is called below,
        // forked process will have EAX set to 0 
        // and thus, fork returns 0 in child process
        regs->r_eax = 0;

        fork_thread->kt_ctx.c_pdptr = fork_process->p_pagedir;
        fork_thread->kt_ctx.c_eip = (uint32_t) userland_entry;
        fork_thread->kt_ctx.c_esp = fork_setup_stack(regs, fork_thread->kt_kstack);
        fork_thread->kt_ctx.c_kstack = (uintptr_t) fork_thread->kt_kstack;
        fork_thread->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;

        //Destroy vmamp created in proc_create
        //vmmap_destroy(fork_process->p_vmmap);

        //CLONE VMMAP
        
        fork_process->p_vmmap = vmmap_clone(curproc->p_vmmap);
        fork_process->p_vmmap->vmm_proc = fork_process;

        //Middle KASSERTs
        KASSERT(fork_process->p_state == PROC_RUNNING);
        KASSERT(fork_process->p_pagedir != NULL);
        KASSERT(fork_thread->kt_kstack != NULL);
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");
        // For each vm_area in vm_map, add shadow object if its private 
        // Increase ref count for each vmarea
        list_iterate_begin(&curproc->p_vmmap->vmm_list, vma, vmarea_t, vma_plink){
                // Since we have cloned vmmap, all vm_area's in curproc and forked proc
                // will have same virtual addresses.
                // We have the vma which is vm area of curproc
                // Below, we do a lookup for the same vm area in fork_process
                dbg(DBG_PRINT,"(GRADING3B\n");
                fork_vma = vmmap_lookup(fork_process->p_vmmap, vma->vma_start);

                // At this point, fork_vma->vma_obj will be NULL as mmobj is not set in clone
                // We set it to fork_shadow object/vma->vma_obj below acc to MAP

                if(vma->vma_flags & MAP_PRIVATE){
                        // Since it is PRIVATE, we need to create two shadow objects
                        // One for curproc and one for forked process
                        // Link shadow object's shadowed to vma_obj
                        // Link shadown object's bottom_obj to current bottom_obj
                        // Change vma->obj to shadow object as it is top most mmobj in inverted tree

                        mmobj_t *curproc_shadow, *fork_shadow;
                        curproc_shadow = shadow_create();
                        fork_shadow = shadow_create();

                        // This is to increment the ref count for the vma_obj
                        // as both curproc and forked shadows point to same vma_obj
                        vma->vma_obj->mmo_ops->ref(vma->vma_obj);

                        curproc_shadow->mmo_shadowed = vma->vma_obj;
                        curproc_shadow->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;

                        fork_shadow->mmo_shadowed = vma->vma_obj;
                        fork_shadow->mmo_un.mmo_bottom_obj = vma->vma_obj->mmo_un.mmo_bottom_obj;

                        fork_vma->vma_obj = fork_shadow;
                        vma->vma_obj = curproc_shadow;
                        dbg(DBG_PRINT,"(GRADING3B\n");
                        
                }
                else{
                        // Since it is SHARED, there is no need for shadow objects
                        // Just point mmobj of fork vm_area to mmobj of curproc vm_area
                        fork_vma->vma_obj = vma->vma_obj;

                        // This is to increment the ref count for the vma_obj
                        // as both curproc and forked vma point to same vma_obj
                        fork_vma->vma_obj->mmo_ops->ref(fork_vma->vma_obj);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
                
                list_insert_tail(mmobj_bottom_vmas(vma->vma_obj), &fork_vma->vma_olink); 
                

        }list_iterate_end();

        // Copy the file descriptor table of the curproc into the forked
        int fd;
        for(fd=0; fd<NFILES; fd++){
                if(curproc->p_files[fd]){
                        fork_process->p_files[fd] = curproc->p_files[fd];
                        fref(fork_process->p_files[fd]);
                }
        }

        // Unmap all page entries
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        //pt_unmap_range(fork_process->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all();

        fork_process->p_brk = curproc->p_brk;
        fork_process->p_start_brk = curproc->p_start_brk;

        // Set this back to fork process PID so that 
        // fork returns pid in curproc i.e. parent
        regs->r_eax = fork_process->p_pid;

        sched_make_runnable(fork_thread);
        dbg(DBG_PRINT,"(GRADING3B\n");
        return fork_process->p_pid;
}
