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
static int anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int anon_fillpage(mmobj_t *o, pframe_t *pf);
static int anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage = anon_fillpage,
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
        anon_allocator = slab_allocator_create("anon-alloc", sizeof(mmobj_t));
        KASSERT(anon_allocator);
        dbg(DBG_PRINT, "(GRADING3A 4.a) Anonymous object allocator initialized successfully\n");
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
        mmobj_t *object = NULL;
        object = (mmobj_t *)slab_obj_alloc(anon_allocator);
        KASSERT(object);
        anon_count++;
        mmobj_init(object, &anon_mmobj_ops);
        object->mmo_refcount = 1;
        dbg(DBG_PRINT,"(GRADING3E) address of respages (anon create) is %p", &object->mmo_respages);
        return object;
}

/* Implementation of mmobj entry points: */

/*
* Increment the reference count on the object.
*/
static void
anon_ref(mmobj_t *o)
{
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.b) Anonymous object refcount > 0 and ops are not NULL\n");
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
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.c) Anonymous object refcount > 0 and ops are not NULL\n");

        o->mmo_refcount--;
        if (o->mmo_nrespages == o->mmo_refcount) {
                pframe_t *p;
                list_iterate_begin(&o->mmo_respages, p, pframe_t, pf_olink) {
                        /* while (pframe_is_busy(p))
                        sched_sleep_on(&(p->pf_waitq)); */
                        pframe_unpin(p);
                        pframe_free(p);
                } list_iterate_end();
                anon_count--;
        }
        return;
}

/* Get the corresponding page from the mmobj. No special handling is
* required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        KASSERT(o != NULL);
        int found_flag = 0;

        /* will create new if it doesn't exist */
        found_flag = pframe_get(o, pagenum, pf);

        /*We didn't find page with given page number*/
        if(found_flag != 0){
                *pf = NULL;
                return -1;
        }
        else{
                return found_flag;
        }

        KASSERT(0);/* should never reach to this point */
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(pframe_is_busy(pf));
        dbg(DBG_PRINT, "(GRADING3A 4.d) pframe is busy\n");
        KASSERT(!pframe_is_pinned(pf));
        dbg(DBG_PRINT, "(GRADING3A 4.d) pframe is not pinned yet\n");

        int res = 0;
        memset(pf->pf_addr, 0, PAGE_SIZE);
        pframe_pin(pf);
        return res;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(o != NULL);
        KASSERT(pf != NULL);
        int res = 0;
        return res;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(o != NULL);
        KASSERT(pf != NULL);
        int res = 0;
        return res;
        /* Fill the page frame starting at address pf->pf_addr with the
* contents of the page identified by pf->pf_obj and pf->pf_pagenum.
* This may block.
* Return 0 on success and -errno otherwise.
*/
        /*
pframe_t *dest_pf = NULL;
res = o->mmo_ops->lookuppage(o, pf->pf_pagenum, 1, &dest_pf);
KASSERT(dest_pf);
if(res == 0){
memcpy(dest_pf->pf_addr, pf->pf_addr, PAGE_SIZE);
return 0;
}
*/
}
