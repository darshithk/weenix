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
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"
#include "mm/tlb.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_create");
        vmmap_t* vmmap = (vmmap_t*)slab_obj_alloc(vmmap_allocator);
        list_init(&vmmap->vmm_list);
        vmmap->vmm_proc = NULL;
        dbg(DBG_PRINT,"(GRADING3B\n");
        return vmmap;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
        vmarea_t* vmarea = NULL;

        KASSERT(NULL != map);
        dbg(DBG_PRINT,"(GRADING3A 3.a)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");
  
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                list_remove(&vmarea->vma_plink);   
                list_remove(&vmarea->vma_olink); 
                vmarea->vma_obj->mmo_ops->put(vmarea->vma_obj);
                vmarea->vma_obj = NULL;
                vmarea_free(vmarea);
        } list_iterate_end();
  
        slab_obj_free(vmmap_allocator, map);
        dbg(DBG_PRINT,"(GRADING3B\n");
        
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_insert");
        KASSERT(NULL != map && NULL != newvma);
        KASSERT(NULL == newvma->vma_vmmap);
        KASSERT(newvma->vma_start < newvma->vma_end);
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
        dbg(DBG_PRINT,"(GRADING3A 3.b)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");

        vmarea_t *vmarea, *temp = NULL;
        newvma->vma_vmmap = map;
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                if (vmarea->vma_start >= newvma->vma_start ) {
                        temp = vmarea;
                        list_insert_before(&temp->vma_plink, &newvma->vma_plink);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                        return;
                }
        } list_iterate_end();
        list_insert_tail(&map->vmm_list, &newvma->vma_plink);
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        unsigned int maxPageNo = ADDR_TO_PN(USER_MEM_HIGH);
        unsigned int minPageNo = ADDR_TO_PN(USER_MEM_LOW);
        unsigned int pages = 0, start_vfn = -1;
        vmarea_t *vmarea, *prev_area = NULL;

        if(!list_empty(&map->vmm_list)){
                if (dir == VMMAP_DIR_HILO)
                {
                        list_link_t *list;
                        /*
                                Since we are allocating in HILO address space, iterate in the reverse direction.
                                If we find any free area with atleast npages, there is a range of free frames.
                        */
                        for (list = (&map->vmm_list)->l_prev; list != &map->vmm_list; list = list->l_prev) {
                                vmarea_t *vmarea = list_item(list, vmarea_t, vma_plink);

                                // Space between last vm_area and list start 
                                if(list->l_next == &map->vmm_list) { 
                                        pages = maxPageNo - vmarea->vma_end;
                                        start_vfn = maxPageNo - npages;
                                }
                                // Space between current vm_area and previous vm_area.
                                else {
                                        vmarea_t *next_vmarea = list_item(list->l_next, vmarea_t, vma_plink);
                                        pages = next_vmarea->vma_start - vmarea->vma_end;
                                        start_vfn = next_vmarea->vma_start - npages;   
                                }
                                
                                // If space found is > npages, we can have start of the space as start_vfn.
                                if(pages >= npages)
                                {       
                                        dbg(DBG_PRINT,"(GRADING3B\n");
                                        return start_vfn;
                                }

                                prev_area = vmarea;
                        }

                        // If we didn't find space between any of the existing vm_area's
                        // We check if we have space between first vm_area and USER_MEM_LOW
                        if(list == &map->vmm_list){
                                pages = prev_area->vma_start - minPageNo;
                                start_vfn = prev_area->vma_start - npages;
                                if(pages >= npages)
                                {
                                        dbg(DBG_PRINT,"(GRADING3D\n");
                                        return start_vfn;
                                }
                                dbg(DBG_PRINT,"(GRADING3B\n");
                        }
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
        }
        
        // We didn't find any free range in the current vmmap :(
        dbg(DBG_PRINT,"(GRADING3D\n");

        return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
        KASSERT(NULL != map);
        dbg(DBG_PRINT,"(GRADING3A 3.c)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");

        vmarea_t* vmarea = NULL;
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                // For every vm_area, check if the vfn to be found is enclosed by the vm_area
                if (vfn >= vmarea->vma_start && vfn < vmarea->vma_end)
                {
                        dbg(DBG_PRINT,"(GRADING3B\n");
                        return vmarea;
                }
  
        } list_iterate_end(); 
        dbg(DBG_PRINT,"(GRADING3B\n");
        //We didn't find the vfn in the vmmap :(
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        vmmap_t* new_map = vmmap_create();
        vmarea_t* vmarea = NULL;
  
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                vmarea_t* vm = vmarea_alloc();
                vm->vma_start = vmarea->vma_start;
                vm->vma_end = vmarea->vma_end;
                vm->vma_off = vmarea->vma_off;
                vm->vma_prot = vmarea->vma_prot;
                vm->vma_flags = vmarea->vma_flags;
                vm->vma_vmmap = new_map;
                list_link_init(&vm->vma_plink);
                list_link_init(&vm->vma_olink);    
                list_insert_tail(&new_map->vmm_list, &vm->vma_plink);
                // list_insert_tail(mmobj_bottom_vmas(vmarea->vma_obj), &vm->vma_olink);   
        } list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3B\n");
        return new_map;
}


/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_map");
        KASSERT(NULL != map);
        KASSERT(0 < npages);
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
        KASSERT(PAGE_ALIGNED(off));
        dbg(DBG_PRINT,"(GRADING3A 3.d)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");
        
        int error = 0;
        vmarea_t *new_vmarea;
        
        if (lopage == 0)
        {
                /*
                        If lopage is zero, we will find a range of virtual addresses in the
                        process that is big enough, by using vmmap_find_range with the same
                        dir argument.
                */
                int start_vfn = vmmap_find_range(map, npages, dir);

                // There is no free range available :(
                if(start_vfn == -1) {
                        dbg(DBG_PRINT,"(GRADING3D\n");
                        return -1;
                }
                
                new_vmarea = vmarea_alloc();
                
                if(new_vmarea == NULL) {
                        return -1;
                }

                new_vmarea->vma_start = start_vfn;
                new_vmarea->vma_end = start_vfn + npages;
                dbg(DBG_PRINT,"(GRADING3B\n");
        }
        else
        {
                /*
                        If lopage is non-zero and the specified region
                        contains another mapping that mapping should be unmapped.
                */

                // Check if the range is already mapped
                if(!vmmap_is_range_empty(map,lopage,npages)){
                        
                        // Remove the existing mapping
                        error = vmmap_remove(map, lopage, npages);
                        
                        if(error != 0) {
                                return error;
                        }
                }

                new_vmarea = vmarea_alloc();
                
                if(new_vmarea == NULL) {
                        return -1;
                }
                
                new_vmarea->vma_start = lopage;
                new_vmarea->vma_end = lopage + npages;
                dbg(DBG_PRINT,"(GRADING3B\n");
        }

        new_vmarea->vma_prot=prot;
        new_vmarea->vma_flags=flags;
        new_vmarea->vma_off=ADDR_TO_PN(off);
        
        list_link_init(&new_vmarea->vma_plink);
        list_link_init(&new_vmarea->vma_olink);

        if(flags & MAP_PRIVATE) {
            new_vmarea->vma_obj=shadow_create();

                if(file){
                        error = file->vn_ops->mmap(file, new_vmarea, &(new_vmarea->vma_obj->mmo_shadowed));
                        if(error < 0){
                                return error;
                        }
                        new_vmarea->vma_obj->mmo_un.mmo_bottom_obj = new_vmarea->vma_obj->mmo_shadowed;
                }
                else{
                        new_vmarea->vma_obj->mmo_un.mmo_bottom_obj = anon_create();
                        new_vmarea->vma_obj->mmo_shadowed = new_vmarea->vma_obj->mmo_un.mmo_bottom_obj;
                }

            list_insert_tail(&new_vmarea->vma_obj->mmo_shadowed->mmo_un.mmo_vmas, &new_vmarea->vma_olink);
            dbg(DBG_PRINT,"(GRADING3B\n");
        }else {
                //new_vmarea->vma_obj = anon_mmobj;
                //list_insert_head(&anon_mmobj->mmo_un.mmo_vmas, &new_vmarea->vma_olink);

                if(file != NULL){
                       error=(file->vn_ops->mmap)(file,new_vmarea,&(new_vmarea->vma_obj));
                        if(error<0) {
                                return error;
                        } 
                }
                else{
                        new_vmarea->vma_obj=anon_create();
                }
                list_insert_tail(&(new_vmarea->vma_obj->mmo_un.mmo_vmas),
				  &(new_vmarea->vma_olink));
                dbg(DBG_PRINT,"(GRADING3B\n");
        }

        //Insert the new_vmarea in the vmmap
        vmmap_insert(map, new_vmarea);
        
        if(new){
                *new=new_vmarea;
        }

        dbg(DBG_PRINT,"(GRADING3B\n");
        return 0;
}
/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        if(!list_empty(&map->vmm_list)){
                dbg(DBG_PRINT,"(GRADING3B\n");
                vmarea_t *vmarea;
                int temp_pages;
                list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {  
                        if(npages != 0){
                                if (lopage > vmarea->vma_start && lopage + npages < vmarea->vma_end ){
                                        vmarea_t* new_vmarea = vmarea_alloc(); 
                                        new_vmarea->vma_start = lopage + npages;
                                        new_vmarea->vma_end = vmarea->vma_end;
                                        new_vmarea->vma_off = vmarea->vma_off + lopage + npages - vmarea->vma_start;
                                        new_vmarea->vma_prot = vmarea->vma_prot;
                                        new_vmarea->vma_flags = vmarea->vma_flags;
                                        new_vmarea->vma_vmmap = map;
                                        new_vmarea->vma_obj = vmarea->vma_obj;
                                        vmarea->vma_end = lopage;

                                        vmarea_t *after_vmarea = list_item((vmarea->vma_plink).l_next, vmarea_t,vma_plink);
                                        list_insert_before(&after_vmarea->vma_plink,&new_vmarea->vma_plink);
                                        
                                        mmobj_t *tmp_obj = vmarea->vma_obj;
                                        mmobj_t *shadow1 = shadow_create();
                                        vmarea->vma_obj = shadow1;
                                        mmobj_t *shadow2 = shadow_create();
                                        new_vmarea->vma_obj = shadow2;

                                        new_vmarea->vma_obj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(vmarea->vma_obj);
                                        vmarea->vma_obj->mmo_un.mmo_bottom_obj = new_vmarea->vma_obj->mmo_un.mmo_bottom_obj;
                                        vmarea->vma_obj->mmo_shadowed = tmp_obj;
                                        new_vmarea->vma_obj->mmo_shadowed = tmp_obj;
                                        tmp_obj->mmo_ops->ref(tmp_obj);
                                        list_insert_tail(mmobj_bottom_vmas(tmp_obj), &new_vmarea->vma_olink);

                                        tlb_flush_all();
                                        pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(lopage + npages));

                                } else if (lopage > vmarea->vma_start && lopage < vmarea->vma_end && lopage + npages >= vmarea->vma_end) {
                                        tlb_flush_all();
                                        pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(lopage), (uintptr_t)PN_TO_ADDR(vmarea->vma_end));
                                        vmarea->vma_end = lopage;

                                } else if(lopage <= vmarea->vma_start && lopage + npages < vmarea->vma_end && lopage + npages > vmarea->vma_start) {
                                        tlb_flush_all();
                                        pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vmarea->vma_start), (uintptr_t)PN_TO_ADDR(lopage+npages));
                                        vmarea->vma_off = vmarea->vma_off + lopage + npages - vmarea->vma_start;
                                        vmarea->vma_start = lopage + npages;       
                                } else if (lopage <= vmarea->vma_start && lopage + npages >= vmarea->vma_end) {
                                        tlb_flush_all();
                                        pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vmarea->vma_start), (uintptr_t)PN_TO_ADDR(vmarea->vma_end));

                                        list_remove(&vmarea->vma_olink);
                                        list_remove(&vmarea->vma_plink);
                                        vmarea->vma_obj->mmo_ops->put(vmarea->vma_obj);
                                        vmarea->vma_obj = NULL;
                                        vmarea_free(vmarea);

                                }
                        }
                }list_iterate_end();
        }
        
        dbg(DBG_PRINT,"(GRADING3B\n");
        
        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        
        vmarea_t* vmarea = NULL;
        uint32_t ending_vfn = startvfn + npages;
        KASSERT((startvfn < ending_vfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= ending_vfn));
        dbg(DBG_PRINT,"(GRADING3A 3.e)\n");
        dbg(DBG_PRINT,"(GRADING3B\n");
        
        /*
                Iterate through all the vm_area's in process vmmap.
                Find if there is a vm_area which encloses startvfn and ending_vfn.
        */
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
  
                if (ending_vfn > vmarea->vma_start && startvfn < vmarea->vma_end)
                {
                        dbg(DBG_PRINT,"(GRADING3B\n");
                        return 0;
                }
  
        } list_iterate_end(); 
        dbg(DBG_PRINT,"(GRADING3B\n");
        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_read");
        uint32_t start = ADDR_TO_PN(vaddr), start_offset = PAGE_OFFSET(vaddr);

        uint32_t end = ADDR_TO_PN((uint32_t)vaddr + count), end_offset = PAGE_OFFSET((uint32_t)vaddr + count);

        uint32_t curr_page = start;
  
        if(start == end) {
                vmarea_t *vmarea = vmmap_lookup(map, curr_page);
                pframe_t* frame = NULL;
                int retval = pframe_lookup(vmarea->vma_obj, start-vmarea->vma_start + vmarea->vma_off ,1, &frame);
                memcpy(buf, ((char *)frame->pf_addr)+start_offset, count);
                dbg(DBG_PRINT,"(GRADING3B\n");
                return 0;
        }
        while (curr_page <= end) {
                vmarea_t *vmarea = vmmap_lookup(map, curr_page);
                int vma_off = vmarea->vma_off;
                pframe_t* frame = NULL;
                int retval = pframe_lookup(vmarea->vma_obj, curr_page-vmarea->vma_start + vmarea->vma_off ,1, &frame);
  
                if (curr_page == start)
                {
                        memcpy(buf, ((char *)frame->pf_addr)+start_offset, PAGE_SIZE - start_offset);
                        buf = ((char *) buf + PAGE_SIZE - start_offset);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
                else if (curr_page == end) {
                        memcpy(buf, frame->pf_addr, end_offset);
                        buf = ((char *) buf + end_offset);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
                else {
                        memcpy(buf, frame->pf_addr, PAGE_SIZE);
                        buf = ((char *) buf + PAGE_SIZE);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
                ++curr_page;
        }
        dbg(DBG_PRINT,"(GRADING3D\n");
        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_write");
        uint32_t start = ADDR_TO_PN(vaddr), start_offset = PAGE_OFFSET(vaddr);

        uint32_t end = ADDR_TO_PN((uint32_t)vaddr + count - 1), end_offset = PAGE_OFFSET((uint32_t)vaddr + count - 1);
        dbg(DBG_PRINT,"(GRADING3B\n");
        uint32_t curr_page = start;
        if(start == end) {
                vmarea_t *vmarea = vmmap_lookup(map, curr_page);
                pframe_t* frame = NULL;
                int retval = pframe_lookup(vmarea->vma_obj, start-vmarea->vma_start + vmarea->vma_off ,1, &frame);
                memcpy(((char *)frame->pf_addr)+start_offset, buf, count);
                pframe_dirty(frame);
                dbg(DBG_PRINT,"(GRADING3B\n");
                return 0;
        }
        while (curr_page <= end) {
                vmarea_t *vmarea = vmmap_lookup(map, curr_page);
                int vma_off = vmarea->vma_off;
                pframe_t* frame = NULL;  
                int retval = pframe_lookup(vmarea->vma_obj, curr_page-vmarea->vma_start + vmarea->vma_off ,1, &frame);
  
                if (curr_page == start)
                {
                        memcpy(((char *)frame->pf_addr)+start_offset,buf, PAGE_SIZE - start_offset);
                        buf = ((char *) buf + PAGE_SIZE - start_offset);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
                else if (curr_page == end){
                        memcpy(frame->pf_addr, buf, end_offset);
                        buf = ((char *) buf + end_offset);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                } 
                else {
                        memcpy( frame->pf_addr, buf, PAGE_SIZE);
                        buf = ((char *) buf + PAGE_SIZE);
                        dbg(DBG_PRINT,"(GRADING3B\n");
                }
                pframe_dirty(frame);
                ++curr_page;
        }
        return 0;
}
