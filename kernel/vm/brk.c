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
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
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
 * If addr is NULL, you should "return" the current break. We use this to
 * implement sbrk(0) without writing a separate syscall. Look in
 * user/libc/syscall.c if you're curious.
 *
 * You should support combined use of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
        //NOT_YET_IMPLEMENTED("VM: do_brk");

        if(!addr){
                dbg(DBG_PRINT,"(GRADING3B\n");
                *ret = curproc->p_brk;
                return 0;
        }

        /*
                If addr < p_start_brk, addr is unaccessible i.e not in heap.
                OR
                If addr is higher than USER_MEM_HIGH, which is the limit and can't go further

                ==> return -ENOMEM
        */

        if((unsigned int)addr < (unsigned int)curproc->p_start_brk || (unsigned int)addr > USER_MEM_HIGH) {
                dbg(DBG_PRINT,"(GRADING3D\n");
            	return -ENOMEM;
        }

        //Page next to the page having addr
        int ending_vfn = ADDR_TO_PN((uint32_t)addr - 1) + 1; 

        //Page next to the page having p_brk
        int starting_vfn = ADDR_TO_PN((uint32_t)curproc->p_brk - 1) + 1; 
        

        /*
                We are setting addr as the brk for the process.
                There can be three cases:
                1. ending_vfn == starting_vfn: In this case, we set p_brk to addr and return.
                2. ending_vfn < starting_vfn:
                        We are reducing the heap size.
                        Remove everything the vmmap from ending_vfn to starting_vfn(excluded).
                3. ending_vfn > starting_vfn:
                        We are increasing heap size.
                        Look up for the vmarea having starting_vfn-1 i.e. vmarea having p_brk.
                        Check if we have free contiguous memory from starting_vfn till ending_vfn(excluded).
                        If NO: return -ENOMEM
                        if YES: extend the vmarea of starting_vfn by changing vma_end to ending_vfn.
        */
        if(starting_vfn == ending_vfn)
                goto done;
        else if(ending_vfn < starting_vfn){
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        vmmap_remove(curproc->p_vmmap, ending_vfn, (starting_vfn - ending_vfn));
                        goto done;
                }
        else{   	
                        vmarea_t *vm_area = vmmap_lookup(curproc->p_vmmap, starting_vfn-1);                
                        if(vmmap_is_range_empty(curproc->p_vmmap, starting_vfn, (ending_vfn - starting_vfn)) == 0) { 
                                dbg(DBG_PRINT,"(GRADING3D\n");
                                return -ENOMEM;
                        }
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        vm_area->vma_end = ending_vfn;
                        goto done;
                }

        done:
        curproc->p_brk = addr;
        *ret = addr;
        dbg(DBG_PRINT,"(GRADING3B\n");
        return 0;
}
