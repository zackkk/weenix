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
        int res = 0;
        res = o->mmo_ops->lookuppage(o, pagenum, forwrite, pf); /*check...lookup on itself??*/
        return res;
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_fillpage");*/
        KASSERT(o != NULL);
        int res = 0;
        res = o->mmo_ops->fillpage(o, pf); /*check...lookup on itself??*/
        return res;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_dirtypage");*/
        KASSERT(o != NULL);
        int res = 0;
        res = o->mmo_ops->dirtypage(o, pf); /*check...lookup on itself??*/
        return res;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        /*NOT_YET_IMPLEMENTED("VM: anon_cleanpage");*/
        KASSERT(o != NULL);
        int res = 0;
        res = o->mmo_ops->cleanpage(o, pf); /*check...lookup on itself??*/
        return res;
}
