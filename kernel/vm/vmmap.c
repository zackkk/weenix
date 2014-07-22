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
        vmmap_t * map = (vmmap_t*)slab_obj_alloc(vmmap_allocator);
        map->vmm_proc = NULL;
        list_init(&(map->vmm_list));
        return map;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        if(list_empty(&(map->vmm_list)))
        {
              map->vmm_proc = NULL;
              slab_obj_free(vmmap_allocator, map);
              return;
        }
        vmarea_t * vmarea;

        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
               mmobj_t *tempmmobj = vmarea->vma_obj;
               tempmmobj->mmo_ops->put(tempmmobj);
               /*
                * 1. destroy mmobj when ref == 0?
                * 2. destroy shadow objects ?
                */
               list_remove(&(vmarea->vma_plink));
               vmarea_free(vmarea);
        }list_iterate_end();

        map->vmm_proc = NULL;
        slab_obj_free(vmmap_allocator, map);
        return;
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
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
        int start = -1;
        vmarea_t * vmarea;
        if(dir == VMMAP_DIR_HILO)
        {
               /*if(list_empty(&(map->vmm_list))
               {
                      start=ADDR_TO_PN(USER_MEM_HIGH)-npages;
                      return (int)start;
               }*/
               if(vmmap_is_range_empty(map, ADDR_TO_PN(USER_MEM_HIGH)-npages, npages))
               {
                      start = ADDR_TO_PN(USER_MEM_HIGH)-npages;
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
        else if(dir == VMMAP_DIR_LOHI)
        {
               /*if(list_empty(&(map->vmm_list))
               {
                      start=ADDR_TO_PN(USER_MEM_LOW);
                      return (int)start;
               }*/
               if(vmmap_is_range_empty(map, ADDR_TO_PN(USER_MEM_LOW), npages))
               {
                      start = ADDR_TO_PN(USER_MEM_LOW);
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
        vmarea_t * vmarea;
        if(list_empty(&(map->vmm_list)))
        {
               return NULL;
        }
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
               if(vmarea->vma_start <= vfn && vfn < vmarea->vma_end)
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
        if(clonemap == NULL)
        {
               return NULL;
        }
        vmarea_t * area, * clonearea;
        list_iterate_begin(&(map->vmm_list), area, vmarea_t, vma_plink)
        {
               clonearea = vmarea_alloc();
               if(clonearea == NULL)
               {
                     return NULL;
               }
               clonearea->vma_start = area->vma_start;
               clonearea->vma_end = area->vma_end;
               clonearea->vma_off = area->vma_off;
               clonearea->vma_prot = area->vma_prot;
               clonearea->vma_flags = area->vma_flags;
               vmmap_insert(clonemap, clonearea);

         }list_iterate_end();

         return clonemap;
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
		KASSERT(map);
		KASSERT(file);
		KASSERT(PAGE_ALIGNED((uintptr_t)off));

		/****** vmmap ******/
        int startvfn = -1;
        if(lopage == 0)
        {
			startvfn = vmmap_find_range(map,npages,dir);
        }
        else
        {
			if(!vmmap_is_range_empty(map,lopage,npages))
			{
				vmmap_remove(map,lopage,npages);
			}
			startvfn = lopage;
        }
        KASSERT(-1 != startvfn);
        mmobj_t *vmmobj;
        vmarea_t * vmarea = vmarea_alloc();
        if(vmarea == NULL)
        {
                return -1;
        }
        vmarea->vma_start = startvfn;
        vmarea->vma_end = startvfn + npages;
        vmarea->vma_off = off;
        vmarea->vma_prot = prot;
        vmarea->vma_flags = flags;
        vmmap_insert(map, vmarea);

        /****** file ******/
        if(file)
        {
        	KASSERT(file->vn_ops);
			int ret = file->vn_ops->mmap(file, vmarea, &vmmobj);
			if(ret < 0)
			{
				   return ret;
			}
        }
        else
        {
			vmmobj = anon_create();
        }

        /* set up a shadow object for the mmobj is MAP_PRIVATE is specified. */
        if(flags == MAP_PRIVATE)
        {
			mmobj_t *shadowMmobj = shadow_create();
			if(shadowMmobj==NULL)
			{
				   return -1;
			}
			vmarea->vma_obj = shadowMmobj;
			shadowMmobj->mmo_shadowed = vmmobj;
			shadowMmobj->mmo_un.mmo_bottom_obj = vmmobj;
			/* increment ref count */
			vmmobj->mmo_ops->ref(vmmobj);
        }
        else
        {
			vmarea->vma_obj = vmmobj;
			/* increment ref count */
			vmmobj->mmo_ops->ref(vmmobj);
        }

        if(new)
        {
			new=&vmarea;
        }

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
        if(list_empty(&(map->vmm_list)))
        {
               return 0;
        }

        vmarea_t * vmarea = NULL;
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
        	KASSERT(NULL != vmarea);
        	/* case 1: The region to be unmapped lies completely inside the vmarea. */
        	if(vmarea->vma_start <= lopage && lopage + npages <= vmarea->vma_end)
			{
        		/* check if need to split at the end */
        		if(lopage + npages < vmarea->vma_end){
					vmarea_t * nvmarea = vmarea_alloc();
					nvmarea->vma_start = lopage + npages;
					nvmarea->vma_end = vmarea->vma_end;
					nvmarea->vma_off = vmarea->vma_off + nvmarea->vma_start - vmarea->vma_start;
					nvmarea->vma_prot = vmarea->vma_prot;
					nvmarea->vma_obj = vmarea->vma_obj;
					nvmarea->vma_flags = vmarea->vma_flags;
					vmmap_insert(map, nvmarea);
					if(nvmarea->vma_obj != NULL)
					{
						/*increment ref count*/
						nvmarea->vma_obj->mmo_ops->ref(nvmarea->vma_obj);
					}
        		}

				vmarea->vma_end = lopage;
				/* check if need to free at the beginning */
				if(vmarea->vma_start == vmarea->vma_end){
					list_remove(&(vmarea->vma_olink));
					list_remove(&(vmarea->vma_plink));
					vmarea_free(vmarea);
					if(vmarea->vma_obj != NULL)
					{
						/*decrement ref count*/
						vmarea->vma_obj->mmo_ops->put(vmarea->vma_obj);
					}
				}
			}
			/* case 2: The region overlaps the end of the vmarea. */
			else if(vmarea->vma_start < lopage && lopage < vmarea->vma_end && lopage+npages >= vmarea->vma_end)
			{
				vmarea->vma_end = lopage;
			}
			/* case 3: The region overlaps the beginning of the vmarea. */
			else if(lopage <= vmarea->vma_start && lopage+npages > vmarea->vma_start && lopage+npages < vmarea->vma_end)
			{
				vmarea->vma_start = lopage+npages;
				vmarea->vma_off += lopage+npages-vmarea->vma_start;
			}
			/* case 4: The region completely contains the vmarea.*/
			else if(lopage <= vmarea->vma_start && vmarea->vma_end <= lopage+npages)
			{
				list_remove(&(vmarea->vma_olink));
				list_remove(&(vmarea->vma_plink));
				vmarea_free(vmarea);
				if(vmarea->vma_obj != NULL)
				{
					/*decrement ref count*/
					vmarea->vma_obj->mmo_ops->put(vmarea->vma_obj);
				}
			}
			else
			{
				continue;
			}
        }list_iterate_end();
        return 0;
}


/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        if(list_empty(&(map->vmm_list)))
        {
               return 1;
        }
        vmarea_t * vmarea;
        list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
        {
               if(!(vmarea->vma_start >= startvfn + npages || vmarea->vma_end <= startvfn))
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
        if(list_empty(&(map->vmm_list)))
        {
               return -1;/*what should be return?*/
        }
        /*find the vmareas to read from*/
        vmarea_t * vmarea;
        uint32_t areavfn = ADDR_TO_PN(vaddr);

        while(count>0)
        {
               vmarea = vmmap_lookup(map,areavfn);
               if(vmarea==NULL)
               {
                      return -1;
               }

               /*find the pframes within those vmareas corresponding to the virtual addresses you want to read*/
               pframe_t * result;
               int ret =pframe_get(vmarea->vma_obj, ADDR_TO_PN(vaddr), &result);
               if(ret<0)
               {
                      return ret;
               }

               /*read from the physical memory that pframe points to*/
               if(count>PAGE_SIZE)
               {
                      memcpy((char *)buf, (char *)pt_virt_to_phys((uintptr_t)result->pf_addr),PAGE_SIZE);
               }
               else
               {
                      memcpy((char *)buf, (char *)pt_virt_to_phys((uintptr_t)result->pf_addr),count);
               }
               count -= PAGE_SIZE;
               areavfn++;
        }
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
		if(list_empty(&(map->vmm_list)))
		{
			   return -1;/*what should be return?*/
		}
		/*find the vmareas to read from*/
		vmarea_t * vmarea;
		uint32_t areavfn = ADDR_TO_PN(vaddr);

		while(count>0)
		{
			vmarea = vmmap_lookup(map,areavfn);
			if(vmarea==NULL)
			{
				  return -1;
			}

			/*find the pframes within those vmareas corresponding to the virtual addresses you want to read*/
			pframe_t *result;
			int ret = pframe_get(vmarea->vma_obj, ADDR_TO_PN(vaddr), &result);
			if(ret<0)
			{
				  return ret;
			}

			/*read from the physical memory that pframe points to*/
			if(count>PAGE_SIZE)
			{
				  memcpy((char *)pt_virt_to_phys((uintptr_t)result->pf_addr), (char *)buf,PAGE_SIZE);
			}
			else
			{
				  memcpy((char *)pt_virt_to_phys((uintptr_t)result->pf_addr), (char *)buf, count);
			}
			/* dirty page */
			pframe_dirty(result);
			count -= PAGE_SIZE;
			areavfn++;
		}
		return 0;
}
