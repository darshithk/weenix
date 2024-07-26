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
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/stat.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        //NOT_YET_IMPLEMENTED("VM: do_mmap");
        uint32_t lowest_page = 0;
        file_t *file_obj;
        uint32_t user_mem_range = USER_MEM_HIGH - USER_MEM_LOW;
        vmarea_t *vm_area = NULL;
        int return_code;

        /*
        From man page:
        EINVAL We don't like addr, length, or offset (e.g., they are too
              large, or not aligned on a page boundary).

        EINVAL (since Linux 2.6.12) length was 0.

        EINVAL flags contained none of MAP_PRIVATE, MAP_SHARED, or
                MAP_SHARED_VALIDATE.
        */

        if(len <= 0 || len > user_mem_range || !PAGE_ALIGNED(off) || (addr && !PAGE_ALIGNED(addr))){
                dbg(DBG_PRINT,"(GRADING3D\n");
                return -EINVAL;
        }

        // If flags have neither MAP_SHARED or MAP_PRIVATE set
        // OR if it has both set
        if(!((flags & MAP_SHARED) || (flags & MAP_PRIVATE)) || ((flags & MAP_SHARED) && (flags & MAP_PRIVATE))){
                dbg(DBG_PRINT,"(GRADING3D\n");
                return -EINVAL;
        }

        /*
        MAP_FIXED
              Don't interpret addr as a hint: place the mapping at
              exactly that address.  addr must be suitably aligned: for
              most architectures a multiple of the page size is
              sufficient; however, some architectures may impose
              additional restrictions.  If the memory region specified
              by addr and length overlaps pages of any existing
              mapping(s), then the overlapped part of the existing
              mapping(s) will be discarded.  If the specified address
              cannot be used, mmap() will fail.
        */

        if(flags & MAP_FIXED){
                dbg(DBG_PRINT,"(GRADING3B\n");
                //The mapping has to be placed exactly at the given address.
                //Check if the address is within the range
                if(addr == NULL){
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        return -EINVAL;
                }
                lowest_page = ADDR_TO_PN(addr);
        }


        if (flags & MAP_ANON){
                dbg(DBG_PRINT,"(GRADING3B\n");
                return_code = vmmap_map(curproc->p_vmmap, 0, lowest_page, (len - 1) / PAGE_SIZE + 1, prot, flags, off, VMMAP_DIR_HILO, &vm_area);        
                if (return_code < 0){
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        return return_code;
                }
        }
        else{
                /*
                EBADF  fd is not a valid file descriptor (and MAP_ANONYMOUS was
                not set).
                */
                dbg(DBG_PRINT,"(GRADING3B\n");
                if((fd < 0 || fd >= NFILES)){
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        return -EBADF;
                }

                file_obj = fget(fd);

                if(!file_obj){
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        return -EBADF;
                }

                /*
                        EACCES A file descriptor refers to a non-regular file.  Or a file
                        mapping was requested, but fd is not open for reading.  Or
                        MAP_SHARED was requested and PROT_WRITE is set, but fd is
                        not open in read/write (O_RDWR) mode.  Or PROT_WRITE is
                        set, but the file is append-only.
                */


                if(flags & MAP_SHARED && prot & PROT_WRITE && !(file_obj->f_mode & FMODE_READ && file_obj->f_mode & FMODE_WRITE) ){
                        fput(file_obj);
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        return -EACCES;
                }


                vmmap_map(curproc->p_vmmap, file_obj->f_vnode, lowest_page,  (len - 1) / PAGE_SIZE + 1, prot, flags, off, VMMAP_DIR_HILO, &vm_area);
                
                fput(file_obj);
                
        }

        KASSERT(NULL != curproc->p_pagedir);
        dbg(DBG_PRINT,"(GRADING3A 2.a)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");

        *ret = PN_TO_ADDR(vm_area->vma_start);
        tlb_flush_all();
        return 0;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
        //NOT_YET_IMPLEMENTED("VM: do_munmap");
        uint32_t user_mem_range = USER_MEM_HIGH - USER_MEM_LOW;
        if (len <= 0 || (len > (user_mem_range)) || (USER_MEM_LOW > (uint32_t) addr) || (((uint32_t)addr + len) > USER_MEM_HIGH)) {
                dbg(DBG_PRINT,"(GRADING3D\n");
                return -EINVAL;
        }

        int return_code = vmmap_remove(curproc->p_vmmap, ADDR_TO_PN(addr), ((len - 1) / PAGE_SIZE ) + 1);

        tlb_flush_all();
        dbg(DBG_PRINT,"(GRADING3B\n");
        return return_code;
}
