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

#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{
        /*NOT_YET_IMPLEMENTED("VM: anon_init");*/
        anon_allocator = slab_allocator_create("anon-alloc", sizeof(mmobj_t));
        KASSERT(anon_allocator);
        dbg(DBG_PRINT, "(GRADING3A 4.a) Anonymus object allocator initialized successfully");
        return;
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{
        /*NOT_YET_IMPLEMENTED("VM: anon_create");*/
        
        mmobj_t *object = NULL;
        
        /*try to allocate some memory for the anon object*/
        object = (mmobj_t *)slab_obj_alloc(anon_allocator);
        
        if(object == NULL){
                dbg(DBG_PRINT, "(GRADING3E) Could not create anonymus object");
                return NULL;
        }
        
        /*Keep track of number of anon objects created */
        anon_count++;
        
        /*init the anon object...*/
        mmobj_init(object, &anon_mmobj_ops);
        
        /*CHECK: increase refcount???
        Since whoever called create
        must will be the one referencing it at
        the beginning*/
        object->mmo_refcount++;
        
        /*MACROS return pointer to list of mmobj_bottom_obj (for shadowed objects
         * or pointer to list mmobj_bottom_vmas for anon objects...*/

        return object;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_ref");*/
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.b) Anonymus object refcount > 0 and ops are not NULL");
        
        o->mmo_refcount++;
        
        return;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_put");*/
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.c) Anonymus object refcount > 0 and ops are not NULL");
        
        o->mmo_refcount--;
        
        /*Check if recount == respages*/
        if(o->mmo_refcount == o->mmo_nrespages){
                dbg(DBG_PRINT, "(GRADING3E) Anonymus object refcount == nrespages");
                
                pframe_t *frame = NULL;
                list_link_t *link = o->mmo_respages.l_next;    /*First element*/
                
                /*iterate through pages..*/
                /*Uncache and unpin*/
                while(link != &(o->mmo_respages)){                       
                        frame = list_item(link, pframe_t, pf_olink);
                        list_remove(link);
                        KASSERT(NULL != frame);
                        pframe_unpin(frame);    /*Unpin frame*/
                        pframe_free(frame);     /*CHECK: free frame??*/
                        link = link->l_next;
        
                }    
        }
        
        /*Free the object...*/
        slab_obj_free(anon_allocator, o);
        anon_count--;
        
        return;
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_lookuppage");*/
        KASSERT(o != NULL);
        
         /* Finds the correct page frame from a high-level perspective
         * for performing the given operation on an area backed by
         * the given pagenum of the given object. If "forwrite" is
         * specified then the pframe should be suitable for writing;
         * otherwise, it is permitted not to support writes. In
         * either case, it must correctly support reads.
         */
         
        int found_flag = 0;
        
        
        found_flag = pframe_get(o, pagenum, pf);
        
        /*We didn't find page with given pagenumber*/
        if(found_flag != 0){
                /*TODO set error*/
                *pf = NULL;
                return -ENOENT;
        }
        else{
                if(forwrite){
                       if( 1 /*how do we check page is suitable for writing?*/){       /*if suitable for writing CHECK!!!*/
                                while(pframe_is_busy(*pf)){
                                        sched_sleep_on(&((*pf)->pf_waitq));      /*wait till page not busy since we may be writing it*/
                                }
                                
                                return 0;
                       }
                       else{                                    /*no suitable for writing*/
                                *pf = NULL;
                                return -1;                                                 
                       }
                }
                else{
                        /*just return what we found*/
                        while(pframe_is_busy(*pf)){
                                sched_sleep_on(&((*pf)->pf_waitq));   /*wait till pframe is NOT busy, since we may be writing it*/
                        }
                       return 0; 
                }
        }
        
        return -1;
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_fillpage");*/
        KASSERT(o != NULL);
        KASSERT(pf != NULL);
        
        int res = 0;
        
        /* Fill the page frame starting at address pf->pf_addr with the
         * contents of the page identified by pf->pf_obj and pf->pf_pagenum.
         * This may block.
         * Return 0 on success and -errno otherwise.
         */
        
        pframe_t *source_pf = NULL;
        res = o->mmo_ops->lookuppage(o, pf->pf_pagenum, 0, &source_pf);
        
        KASSERT(source_pf);
        
        if(res == 0){
                /*Now copy from source_pf to pf->pf_addr*/
                memcpy(pf->pf_addr, source_pf->pf_addr, PAGE_SIZE);
                return 0;
        }
        
        return res;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_dirtypage");*/
        KASSERT(o != NULL);
        int res = 0;
        
        return res;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_fillpage");*/
        KASSERT(o != NULL);
        KASSERT(pf != NULL);
        
        int res = 0;
        
        /* Fill the page frame starting at address pf->pf_addr with the
         * contents of the page identified by pf->pf_obj and pf->pf_pagenum.
         * This may block.
         * Return 0 on success and -errno otherwise.
         */
        
        pframe_t *dest_pf = NULL;
        res = o->mmo_ops->lookuppage(o, pf->pf_pagenum, 1, &dest_pf);   /*destination pf must be available for WRITING*/
        
        KASSERT(dest_pf);
        
        if(res == 0){
                /*Now copy from pf->pf_add to dest->pf_addr*/
                memcpy(dest_pf->pf_addr, pf->pf_addr, PAGE_SIZE);
                return 0;
        }
        
        return res;
}
