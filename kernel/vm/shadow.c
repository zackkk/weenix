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

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
* number of shadow objects with a single parent, that is another shadow
* object in the shadow objects tree(singletons)
*/
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
        .ref = shadow_ref,
        .put = shadow_put,
        .lookuppage = shadow_lookuppage,
        .fillpage = shadow_fillpage,
        .dirtypage = shadow_dirtypage,
        .cleanpage = shadow_cleanpage
};

/*
* This function is called at boot time to initialize the
* shadow page sub system. Currently it only initializes the
* shadow_allocator object.
*/
void
shadow_init()
{
        shadow_allocator = slab_allocator_create("shadow_mmobj", sizeof(mmobj_t));
        KASSERT(NULL != shadow_allocator);
        dbg(DBG_PRINT,"(GRADING3A 6.a) Initialized shadow_allocator object.\n");
}

/*
* You'll want to use the shadow_allocator to allocate the mmobj to
* return, then then initialize it. Take a look in mm/mmobj.h for
* macros which can be of use here. Make sure your initial
* reference count is correct.
*/
mmobj_t *
shadow_create()
{
mmobj_t *new_shadow = (mmobj_t *) slab_obj_alloc(shadow_allocator);
KASSERT(NULL != new_shadow);
mmobj_init(new_shadow, &shadow_mmobj_ops);
shadow_count++; /* global variable for debugging purpose */
new_shadow->mmo_refcount = 1;
dbg(DBG_PRINT,"(GRADING3E) Initialized shadow object.\n");
        return new_shadow;
}

/* Implementation of mmobj entry points: */

/*
* Increment the reference count on the object.
*/
static void
shadow_ref(mmobj_t *o)
{
KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
dbg(DBG_PRINT,"(GRADING3A 6.b) Referencing shadow object at %p!\n", o);
o->mmo_refcount++;
}

/*
* Decrement the reference count on the object. If, however, the
* reference count on the object reaches the number of resident
* pages of the object, we can conclude that the object is no
* longer in use and, since it is a shadow object, it will never
* be used again. You should unpin and uncache all of the object's
* pages and then free the object itself.
*/
static void
shadow_put(mmobj_t *o)
{
        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT,"(GRADING3A 6.c) Putting shadow object at %p!\n", o);
        
        o->mmo_refcount--;
        if (0 == o->mmo_refcount) {
                pframe_t *p;
                list_iterate_begin(&o->mmo_respages, p, pframe_t, pf_olink) {
                        while (pframe_is_busy(p))
                        sched_sleep_on(&(p->pf_waitq));
                        pframe_unpin(p);
                        pframe_free(p);
                } list_iterate_end();
        
                KASSERT(0 == o->mmo_refcount);
                /* decrease the ref count of its child in the shadow chain */
                o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
                shadow_count--;
                slab_obj_free(shadow_allocator, o);
        }
        return;
}

/* This function looks up the given page in this shadow object. The
* forwrite argument is true if the page is being looked up for
* writing, false if it is being looked up for reading. This function
* must handle all do-not-copy-on-not-write magic (i.e. when forwrite
* is false find the first shadow object in the chain which has the
* given page resident). copy-on-write magic (necessary when forwrite
* is true) is handled in shadow_fillpage, not here. It is important to
* use iteration rather than recursion here as a recursive implementation
* can overflow the kernel stack when looking down a long shadow chain */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
KASSERT(o);

/* being looked up for writing */
if(forwrite){
return pframe_get(o, pagenum, pf);
}
/* being looked up for reading */
else{
while(o != mmobj_bottom_obj(o)){
*pf = pframe_get_resident(o, pagenum);
if(*pf){
return 0;
}
o = o->mmo_shadowed;
}
/* not in the chain * */
*pf = pframe_get_resident(mmobj_bottom_obj(o), pagenum);
return 0;
}

panic("shadow_lookuppage doesn't return, BAD!!!\n");
}

/* As per the specification in mmobj.h, fill the page frame starting
* at address pf->pf_addr with the contents of the page identified by
* pf->pf_obj and pf->pf_pagenum. This function handles all
* copy-on-write magic (i.e. if there is a shadow object which has
* data for the pf->pf_pagenum-th page then we should take that data,
* if no such shadow object exists we need to follow the chain of
* shadow objects all the way to the bottom object and take the data
* for the pf->pf_pagenum-th page from the last object in the chain).
* It is important to use iteration rather than recursion here as a
* recursive implementation can overflow the kernel stack when
* looking down a long shadow chain */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(pframe_is_busy(pf));
        dbg(DBG_PRINT,"(GRADING3A 6.d) pframe is busy.\n");
        KASSERT(!pframe_is_pinned(pf));
        dbg(DBG_PRINT,"(GRADING3A 6.d) pframe is not pinned yet\n");

        pframe_pin(pf);
        o=o->mmo_shadowed;
        while(o != mmobj_bottom_obj(o)){
         /*
* Take that data if there is a shadow object which has data for the pf->pf_pagenum-th page,
*/
pframe_t *src = pframe_get_resident(o, pf->pf_pagenum);
if(NULL != src){
memcpy(pf->pf_addr, src->pf_addr, PAGE_SIZE);
return 0;
}
o = o->mmo_shadowed;
}

        /* If there doesn't exist a shadow object which has data for the pf->pf_pagenum-th page. */
        pframe_t *src = pframe_get_resident(mmobj_bottom_obj(o), pf->pf_pagenum);
        KASSERT(NULL != src);
        memcpy(pf->pf_addr, src->pf_addr, PAGE_SIZE);
        return 0;
}

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
/*
* last slide of Lecture 17: Weenix doesn't support swap space
*/
KASSERT(NULL != o);
KASSERT(NULL != pf);
pframe_dirty(pf);
        return 0;
}

/*
* Write the contents of the page frame starting at address
* pf->pf_addr to the page identified by pf->pf_obj and
* pf->pf_pagenum.
* This may block.
* Return 0 on success and -errno otherwise.
*/
static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
KASSERT(NULL != o);
KASSERT(NULL != pf);
/*
* last slide of Lecture 17: Weenix doesn't support swap space
*/
return 0;
/*
pframe_t *dest = pframe_get_resident(o, pf->pf_pagenum);
if(dest == NULL){
return -ENOENT;
}
else{
memcpy(dest->pf_addr, pf->pf_addr, PAGE_SIZE);
return 0;
}

panic("shadow_cleanpage doesn't return, BAD!!!\n");
*/
}
