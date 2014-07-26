/******************************************************************************/
/* Important CSCI 402 usage information: */
/* */
/* This fils is part of CSCI 402 kernel programming assignments at USC. */
/* Please understand that you are NOT permitted to distribute or publically */
/* display a copy of this file (or ANY PART of it) for any reason. */
/* If anyone (including your prospective employer) asks you to post the code, */
/* you must inform them that you do NOT have permissions to do so. */
/* You are also NOT permitted to remove this comment block from this file. */
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
                               "%#.8x-%#.8x %c%c%c %7s 0x%p %#.5x %#.5x-%#.5x\n",
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
        vmmap_t * map = (vmmap_t*)slab_obj_alloc(vmmap_allocator);
        if(map!=NULL)
        {
               map->vmm_proc = curproc;
               list_init(&(map->vmm_list));
        }
        return map;
}

/* Removes all vmareas from the address space and frees the
* vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.a) map is not NULL. vmmap_destroy\n");

        if(list_empty(&(map->vmm_list)))
        {
              map->vmm_proc = NULL;
              slab_obj_free(vmmap_allocator, map);
              dbg(DBG_PRINT, "des return\n");
              return;
        }
        vmarea_t * vmarea;
        dbg(DBG_PRINT, "1\n");
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
                dbg(DBG_PRINT, "2\n");
               list_remove(&(vmarea->vma_plink));
               dbg(DBG_PRINT, "3\n");
               if (list_link_is_linked(&vmarea->vma_olink)) 
               {
                      dbg(DBG_PRINT, "7\n");
                      
                      list_remove(&vmarea->vma_olink);
                      dbg(DBG_PRINT, "8\n");
               }
               dbg(DBG_PRINT, "4\n");
               mmobj_t * tempmmobj = vmarea->vma_obj;
               if(tempmmobj!=NULL && tempmmobj->mmo_ops!=NULL)
               {
                      tempmmobj->mmo_ops->put(tempmmobj);
               }
               dbg(DBG_PRINT, "5\n");
               vmarea_free(vmarea);
               dbg(DBG_PRINT, "6\n");
        }list_iterate_end();
        
        
        map->vmm_proc = NULL;
        slab_obj_free(vmmap_allocator, map);

        dbg(DBG_PRINT, "vmmap_destroy succeed\n");
        return;
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
* the vmarea is valid. This involves finding where to put it in the list
* of VM areas, and adding it. Don't forget to set the vma_vmmap for the
* area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        KASSERT(NULL != map && NULL != newvma);
        dbg(DBG_PRINT, "(GRADING3A 3.b) map and newvma are not NULL. vmmap_insert\n");
        KASSERT(NULL == newvma->vma_vmmap);
        dbg(DBG_PRINT, "(GRADING3A 3.b) newvma->vma_vmmap is not NULL. vmmap_insert\n");
        KASSERT(newvma->vma_start < newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b) vma_start<vma_end. vmmap_insert\n");
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b) vfn bounds. vmmap_insert\n");

        newvma->vma_vmmap = map;
        if(list_empty(&(map->vmm_list)))
        {
               list_insert_head(&(map->vmm_list),&(newvma->vma_plink));
                return;
        }

        vmarea_t * vmarea;
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
                if(vmarea -> vma_start > newvma -> vma_start)
                {
                       list_insert_before(&(vmarea->vma_plink),&(newvma->vma_plink));
                       return;
                }
        }list_iterate_end();
        /* insert the newvma to the end.*/
        list_insert_tail(&(map->vmm_list),&(newvma->vma_plink));
        return;

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
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.c) map is not NULL. vmmap_find_range\n");
        KASSERT(0 < npages);
        dbg(DBG_PRINT, "(GRADING3A 3.c) npages>0. vmmap_find_range\n");

        uint32_t start =-1;
        vmarea_t * vmarea;
        if(dir==VMMAP_DIR_HILO)
        { /*range in the top*/
               if(vmmap_is_range_empty(map, ADDR_TO_PN(USER_MEM_HIGH)-npages, npages))
               {
                      start=ADDR_TO_PN(USER_MEM_HIGH)-npages;
                      return (int)start;
               }
               list_iterate_reverse(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
               {
                      if(vmmap_is_range_empty(map, vmarea -> vma_start-npages, npages))
                      {
                             start = vmarea -> vma_start-npages;
                             return (int)start;
                      }
               }list_iterate_end();
        }
        else
        {
               /*range in the bottom*/
               if(vmmap_is_range_empty(map, ADDR_TO_PN(USER_MEM_LOW), npages))
               {
                      start=ADDR_TO_PN(USER_MEM_LOW);
                      return (int)start;
               }
               list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
               {

                      if(vmmap_is_range_empty(map, vmarea -> vma_end, npages))
                      {
                             start = vmarea -> vma_end;
                             return (int)start;
                      }
               }list_iterate_end();
        }

        return (int)start;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
* looking for a vma whose range covers vfn. If the page is unmapped,
* return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.d) map is not NULL. vmmap_lookup\n");

        vmarea_t *vmarea = NULL;
        if(list_empty(&(map->vmm_list)))
        {
               return NULL;
        }
        
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
                
               if(vmarea->vma_start <= vfn && vmarea->vma_end > vfn)
               {
                    return vmarea;
               }
        }list_iterate_end();
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
* given map. The areas should have no mmobjs set yet. Returns pointer
* to the new vmmap on success, NULL on failure. This function is
* called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        vmmap_t * clonemap = vmmap_create();
        if(clonemap==NULL)
        {
               return NULL;
        }
        vmarea_t * area, * clonearea;
        list_iterate_begin(&(map->vmm_list), area, vmarea_t, vma_plink)
        {
               clonearea = vmarea_alloc();
               if(clonearea==NULL)
               {
                     vmmap_destroy(clonemap);
                     return NULL;
               }
               clonearea->vma_start = area->vma_start;
               clonearea->vma_end = area->vma_end;
               clonearea->vma_off = area->vma_off;
               clonearea->vma_prot = area->vma_prot;
               clonearea->vma_flags = area->vma_flags;
               vmmap_insert(clonemap, clonearea);
               
               /*Add the cloned area to the obj vmas list...*/
               list_insert_head(&(area->vma_obj->mmo_un.mmo_vmas), &(clonearea->vma_olink));    /*ADDED, CHECK*/
                                                                                                 
         }list_iterate_end();

         return clonemap;
}

/* Insert a mapping into the map starting at lopage for npages pages.
* If lopage is zero, we will find a range of virtual addresses in the
* process that is big enough, by using vmmap_find_range with the same
* dir argument. If lopage is non-zero and the specified region
* contains another mapping that mapping should be unmapped.
*
* If file is NULL an anon mmobj will be used to create a mapping
* of 0's. If file is non-null that vnode's file will be mapped in
* for the given range. Use the vnode's mmap operation to get the
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
        KASSERT(NULL != map);
        dbg(DBG_PRINT, "(GRADING3A 3.e) map is not NULL. vmmap_map\n");
        KASSERT(0 < npages);
        dbg(DBG_PRINT, "(GRADING3A 3.e) npages>0. vmmap_map\n");
        KASSERT(!(~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot));
        dbg(DBG_PRINT, "(GRADING3A 3.e) prot checking. vmmap_map\n");
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
        dbg(DBG_PRINT, "(GRADING3A 3.e) flags checking. vmmap_map\n");
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
        dbg(DBG_PRINT, "(GRADING3A 3.e) lopage checking. vmmap_map\n");
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
        dbg(DBG_PRINT, "(GRADING3A 3.e) lopage and npages checking. vmmap_map\n");
        KASSERT(PAGE_ALIGNED(off));
        dbg(DBG_PRINT, "(GRADING3A 3.e) PAGE_ALIGNED(off) checking. vmmap_map\n");

        int startvfn;
        if(lopage==0)
        {
                startvfn=vmmap_find_range(map,npages,dir);
                if(startvfn<0)
                {
                       return startvfn;
                }
        }
        else
        {
                if(!vmmap_is_range_empty(map,lopage,npages))
                {
                       vmmap_remove(map,lopage,npages);
                }
                startvfn = lopage;
        }
        mmobj_t * vmmobj;

        vmarea_t * vmarea = vmarea_alloc();
        if(vmarea==NULL)
        {
                return -1;
        }
        vmarea->vma_start = startvfn;
        vmarea->vma_end = startvfn+npages;
        vmarea->vma_off = off/PAGE_SIZE;
        vmarea->vma_prot = prot;
        vmarea->vma_flags = flags;
        vmmap_insert(map, vmarea);
        if(file)
        {
                KASSERT(file->vn_ops);
                int ret=file->vn_ops->mmap(file,vmarea,&vmmobj);
                if(ret<0)
                {
                       return ret;
                }
        }
        else
        {
                vmmobj=anon_create();
        }

        if(flags==MAP_PRIVATE)
        {
                mmobj_t * shadowMmobj=shadow_create();
                if(shadowMmobj==NULL)
                {
                       return -1;
                }
                shadowMmobj->mmo_shadowed = vmmobj;
                if(file)
                {
                      vmmobj->mmo_ops->ref(vmmobj);
                }
                vmarea->vma_obj = shadowMmobj;
                shadowMmobj->mmo_un.mmo_bottom_obj = vmmobj;
                list_insert_head(&(vmmobj->mmo_un.mmo_vmas), &(vmarea->vma_olink));
        }
        else
        {
                vmarea->vma_obj = vmmobj;
                /* increment ref count */
                if(file)
                {
                      vmmobj->mmo_ops->ref(vmmobj);
                }
                list_insert_head(&(vmmobj->mmo_un.mmo_vmas), &(vmarea->vma_olink));
        }
        if(new)
        {
               new=&vmarea;
        }
        dbg(DBG_PRINT,"vm_map succeed\n");
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
* [ ] Existing VM Area
* ******* Region to be unmapped
*
* Case 1: [ ****** ]
* The region to be unmapped lies completely inside the vmarea. We need to
* split the old vmarea into two vmareas. be sure to increment the
* reference count to the file associated with the vmarea.
*
* Case 2: [ *******]**
* The region overlaps the end of the vmarea. Just shorten the length of
* the mapping.
*
* Case 3: *[***** ]
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
        KASSERT(map);
        if(list_empty(&(map->vmm_list)))
        {
               return 0;
        }
        vmarea_t * vmarea;
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
               /* case 4: The region completely contains the vmarea.*/
               if(vmarea->vma_start >= lopage && vmarea->vma_end <= lopage+npages)
               {
                        list_remove(&(vmarea->vma_olink));
                        list_remove(&(vmarea->vma_plink));
                        if(vmarea->vma_obj != NULL)
                        {
                        /*decrement ref count*/
                                 vmarea->vma_obj->mmo_ops->put (vmarea->vma_obj);
                        }
                        vmarea_free(vmarea);
               }
               /* case 2: The region overlaps the end of the vmarea. */

               else if(vmarea->vma_start < lopage && vmarea->vma_end>lopage && vmarea->vma_end <= lopage+npages )
               {
                     vmarea->vma_end = lopage;
               }
               /* case 3: The region overlaps the beginning of the vmarea. */
               else if(vmarea->vma_start >= lopage && vmarea->vma_start < lopage+npages && vmarea->vma_end > lopage+npages)
               {
                     vmarea->vma_start = lopage+npages;
                     vmarea->vma_off += lopage+npages-vmarea->vma_start;
               }
               /* case 1: The region to be unmapped lies completely inside the vmarea.*/
               else if(vmarea->vma_start < lopage && vmarea->vma_end > lopage+npages)
               {
                     vmarea_t * nvmarea = vmarea_alloc();
                     if(nvmarea==NULL)
                     {
                            return -1;
                     }
                     nvmarea->vma_start = lopage+npages;
                     nvmarea->vma_end = vmarea->vma_end;
                     nvmarea->vma_off = vmarea->vma_off + nvmarea->vma_start - vmarea->vma_start;
                     nvmarea->vma_prot = vmarea->vma_prot;
                     nvmarea->vma_obj = vmarea->vma_obj;
                     nvmarea->vma_flags = vmarea->vma_flags;

                     vmarea->vma_end = lopage;
                     vmmap_insert(map, nvmarea);
                     
                     /* insert nvmarea to the bottom shadow obj*/
                     list_insert_head(&(vmarea->vma_obj->mmo_un.mmo_bottom_obj->mmo_un.mmo_vmas), &(nvmarea->vma_olink));
                     if(vmarea->vma_flags == MAP_PRIVATE)
                     {                           
                            /*create two shadow, one for vmarea and one for nvmarea*/
                            mmobj_t * shadowForVmarea = shadow_create();
                            mmobj_t * shadowForNvmarea = shadow_create();
                            if(shadowForVmarea == NULL || shadowForNvmarea==NULL)
                            {
                                   return -1;
                            }
                            /*set shadow bottom*/
                            shadowForVmarea->mmo_un.mmo_bottom_obj =vmarea->vma_obj->mmo_un.mmo_bottom_obj;
                            shadowForNvmarea->mmo_un.mmo_bottom_obj =vmarea->vma_obj->mmo_un.mmo_bottom_obj;
                            if(vmarea->vma_obj!=NULL)
                            {
                                   vmarea->vma_obj->mmo_ops->ref(vmarea->vma_obj);
                            }
                            /*shadowed vmarea and nvmarea*/
                            shadowForVmarea->mmo_shadowed = vmarea->vma_obj;
                            shadowForNvmarea->mmo_shadowed = vmarea->vma_obj;

                            /*set vmarea and nvmarea obj*/
                            vmarea->vma_obj = shadowForVmarea;
                            nvmarea->vma_obj = shadowForNvmarea;
                     }
                     else
                     {                           
                            if(nvmarea->vma_obj!=NULL)
                            {
                                  nvmarea->vma_obj->mmo_ops->ref(nvmarea->vma_obj);
                            }
                     }
               }
        }list_iterate_end();
        /*tlb_flush_all();
        pt_unmap_range(curproc->p_pagedir,(uintptr_t)PN_TO_ADDR(lopage),(uintptr_t)PN_TO_ADDR(lopage+npages));

        */
        return 0;
}


/*
* Returns 1 if the given address space has no mappings for the
* given range, 0 otherwise.
*/
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        uint32_t endvfn = startvfn+npages;
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
        dbg(DBG_PRINT, "(GRADING3A 3.e) vfn bounds. vmmap_is_range_empty\n");

        KASSERT(map);
        if(list_empty(&(map->vmm_list)))
        {
               return 1;
        }
        vmarea_t * vmarea;
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
               if(!(vmarea->vma_start >= endvfn || vmarea->vma_end <= startvfn))
               {
                   return 0;
               }
        }list_iterate_end();

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
        dbg(DBG_PRINT, "vmmap_read: starts\n");
        KASSERT(map);
        KASSERT(!list_empty(&(map->vmm_list)));
        vmarea_t *vmarea = NULL;

        while (count > 0)
        {
              dbg(DBG_PRINT, "vmmap_read: count:%d\n",count);
              uint32_t pageoff = PAGE_OFFSET(vaddr); /* vaddr is updating */

              /* find vmarea */
              vmarea = vmmap_lookup(map, ADDR_TO_PN(vaddr));
              KASSERT(vmarea);
              KASSERT(vmarea->vma_obj);

              /* find pframe */
              pframe_t *vmpframe = NULL;
              int ret = pframe_lookup(vmarea->vma_obj, ADDR_TO_PN(vaddr) + vmarea->vma_off - vmarea->vma_start,1, &vmpframe);
              if(ret < 0)
              {
                     return ret;
              }

              /* read from the physical memory that pframe points to*/
              /* pf->pf_addr is page-aligned according to vcleanpage in vnode.c */
              if(count > PAGE_SIZE - pageoff)
              {
                     memcpy((char *)buf, (char *)vmpframe->pf_addr + pageoff, PAGE_SIZE - pageoff);
                     count -= (PAGE_SIZE - pageoff);
                     vaddr = (char *)vaddr + (PAGE_SIZE - pageoff);
                     buf = (char *)buf + (PAGE_SIZE - pageoff);
                     continue;
              }
              else
              {
                     memcpy((char *)buf, (char *)vmpframe->pf_addr + pageoff, count);
                     vaddr = (char *)vaddr + count;
                     buf = (char *)buf + count;
                     break;
              }
       }
       dbg(DBG_PRINT, "vmmap_read: succeed\n");
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
       dbg(DBG_PRINT, "vmmap_write: starts\n");
       KASSERT(map);
       KASSERT(map->vmm_proc);
       KASSERT(!list_empty(&(map->vmm_list)));
       vmarea_t *vmarea = NULL;

       while (count > 0){
       dbg(DBG_PRINT, "vmmap_write: count:%d\n",count);
       uint32_t pageoff = PAGE_OFFSET(vaddr); /* vaddr is updating */

       /* find vmarea */
       vmarea = vmmap_lookup(map, ADDR_TO_PN(vaddr));
       KASSERT(vmarea);
       KASSERT(vmarea->vma_obj);

       /* find pframe */
       pframe_t *vmpframe = NULL;
       int ret = pframe_lookup(vmarea->vma_obj, ADDR_TO_PN(vaddr) + vmarea->vma_off - vmarea->vma_start,1, &vmpframe);
       if(ret < 0)
       {
              return ret;
       }
       pframe_dirty(vmpframe);

       /* read from the physical memory that pframe points to*/
       /* pf->pf_addr is page-aligned according to vcleanpage in vnode.c */
       if(count > PAGE_SIZE - pageoff)
       {
              memcpy((char *)vmpframe->pf_addr + pageoff, (char *)buf, PAGE_SIZE - pageoff);
              count -= (PAGE_SIZE - pageoff);
              vaddr = (char *)vaddr + (PAGE_SIZE - pageoff);
              buf = (char *)buf + (PAGE_SIZE - pageoff);
              continue;
       }
       else{
              memcpy((char *)vmpframe->pf_addr + pageoff, (char *)buf, count);
              vaddr = (char *)vaddr + count;
              buf = (char *)buf + count;
              break;
       }
       }
       dbg(DBG_PRINT, "vmmap_write: succeed\n");
       return 0;
}
