
#include "pool.h"
#include "paged-pool.h"
#include "paged-pool-internal.h"

#include <assert.h>
#include <memory.h>

// we always pass in these standard list of parameters, so might as well save us some time
#define STANDARD_PAGED_POOL_PARAMS(pool)    (pool_element_ptr*)&(pool)->pages_head, (pool_element_ptr*)&(pool)->pages_tail, (pool_element_ptr*)&(pool)->pages_end, sizeof(pp_page)

// most functions require valid pool inputs, let's validate them
#define PAGED_POOL_INPUTS_MUST_BE_VALID     \
        assert(pool!=NULL); \
        assert(pool->element_size>0); /* elements must be of non-zero size */ \
        assert(pool->page_size >= pool->element_size); \
        assert(pool->pages_head!=NULL);   \
        assert(pool->pages_tail!=NULL);   \
        assert(pool->pages_end!=NULL);    \
        assert(pool->pages_head <= pool->pages_tail); /* head must be less than the tail   */ \
        assert(pool->pages_head < pool->pages_end);   /* head must be less than the end    */ \
        assert(pool->pages_tail < pool->pages_end);   /* tail must be less than the end    */


inline size_t elements_per_page(paged_pool* pool)
{
    return pool->page_size / pool->element_size;
}

inline size_t required_pages_from_element_count(paged_pool* pool, size_t N)
{
    return  (N / elements_per_page(pool))+1;
}

inline void pp_page_alloc(paged_pool* pool, pp_page* page)
{
    assert(page);
    assert(page->ptr == NULL);
    page->ptr = (unsigned char*)calloc(1, pool->page_size);
}

inline void pp_page_free(paged_pool* pool, pp_page* page)
{
    assert(page);
    assert(page->ptr != NULL);
    free(page->ptr);
    page->ptr = NULL;
    page->count = 0;
    page->flags = 0;
}

inline pp_page* pp_page_current(paged_pool* pool)
{
    return (pool->pages_tail > pool->pages_head)
        ? pool->pages_tail-1
        : NULL;
}

inline unsigned char* pp_page_get_idx(paged_pool* pool, pp_page* page, int idx)
{
    return page->ptr + pool->element_size*idx;
}


size_t paged_pool_create(paged_pool** pool, size_t element_sizeof)
{
    return paged_pool_create_ex(pool, element_sizeof, 2048);
}

size_t paged_pool_create_ex(paged_pool** pool, size_t element_sizeof, size_t page_size)
{
    *pool = (paged_pool*)calloc(1, sizeof(paged_pool));
    pool_reserve(STANDARD_PAGED_POOL_PARAMS(*pool), 16);    // default to 16 page capacity
    (*pool)->element_size = element_sizeof;
    (*pool)->page_size = page_size;
    return paged_pool_capacity(*pool);
}

#if 0 // this function doesnt do anything but reserve the page headers which isnt much of an advantage, if needed it should create pages too
size_t paged_pool_reserve(paged_pool* pool, size_t element_capacity)
{
    size_t required_pages = required_pages_from_element_count(pool, element_capacity);
    assert(required_pages>0);
    pool_reserve(STANDARD_PAGED_POOL_PARAMS(pool), required_pages);
    return paged_pool_capacity(pool);
}
#endif

size_t paged_pool_length(paged_pool* pool)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    size_t N=0;
    pp_page* p = pool->pages_head;
    while(p < pool->pages_tail) {
        assert(p);
        assert(p->ptr);
        N += p->count;
        p++;
    }
    return N;
}

size_t paged_pool_capacity(paged_pool* pool)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    return pool_length(STANDARD_PAGED_POOL_PARAMS(pool)) * elements_per_page(pool);
}

size_t paged_pool_pagecount(paged_pool* pool)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    return pool->pages_tail - pool->pages_head;
}

void paged_pool_free(paged_pool* pool)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    // free all the pages
    pp_page* p = pool->pages_head;
    while(p < pool->pages_tail) {
        assert(p);
        assert(p->ptr);
        pp_page_free(pool, p);
        p++;
    }
    pool_free(STANDARD_PAGED_POOL_PARAMS(pool));
    free(pool);
}

long paged_pool_contains_element(paged_pool* pool, paged_pool_element_ptr element)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    // search address ranges of the pages
    pp_page* p = pool->pages_head;
    while(p < pool->pages_tail) {
        assert(p);
        assert(p->ptr);
        if(element >= p->ptr && element <= p->ptr + pool->page_size)
            return p - pool->pages_head;
    }
    return -1;
}

long paged_pool_foreach(paged_pool* pool, void* pUserData, paged_pool_element_callback cb)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    size_t idx=0;
    long rv;
    pp_page* p = pool->pages_head;
    while(p < pool->pages_tail) {
        assert(p);
        assert(p->ptr);
        for(uint32_t pidx=0; pidx < p->count; pidx++, idx++) {
            if((rv=cb(pp_page_get_idx(pool, p, pidx) , pUserData, idx)) !=0) {
                return rv;
            }
        }
        p++;
    }
    return 0;
}

paged_pool_element_ptr paged_pool_get_element(paged_pool* pool, size_t idx)
{
    // optimized code, simpler if we are just adding 1 element
    PAGED_POOL_INPUTS_MUST_BE_VALID
    size_t epp = elements_per_page(pool);
    size_t page_num = idx / epp;
    pp_page* cpage = pool->pages_head + page_num;
    assert(cpage < pool->pages_tail);
    return pp_page_get_idx(pool, cpage, idx % epp);
}

ANALYTICS_EXPORT pp_page* paged_pool_next_page(paged_pool* pool, void* puserdata)
{
    pp_page* cpage = NULL;
    pool_new_element(STANDARD_PAGED_POOL_PARAMS(pool), (pool_element_ptr*)&cpage);
    pp_page_alloc(pool, cpage);
    if(puserdata!=NULL)
        cpage->userptr = puserdata;
    return cpage;
}

int paged_pool_new_element(paged_pool* pool, paged_pool_element_ptr* element_out)
{
    // optimized code, simpler if we are just adding 1 element
    PAGED_POOL_INPUTS_MUST_BE_VALID
    pp_page* cpage = pp_page_current(pool);
    if(cpage == NULL || cpage->count == elements_per_page(pool)) {
        // first page, or new page required
        cpage = paged_pool_next_page(pool, NULL);
    }

    // add new element to page
    *element_out = (paged_pool_element_ptr)pp_page_get_idx(pool, cpage, cpage->count++);
    return 1;
}

int paged_pool_push(paged_pool* pool, paged_pool_element_ptr element)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    paged_pool_element_ptr o;
    if(paged_pool_new_element(pool, &o)) {
        // copy the object onto the top of the pool
        memcpy(o, element, pool->element_size);
        return 1;
    } else
        return 0;
}

int paged_pool_top(paged_pool* pool, paged_pool_element_ptr* element_out) {
    PAGED_POOL_INPUTS_MUST_BE_VALID
    pp_page *cpage = pp_page_current(pool);
    if (cpage == NULL) {
        // no items on stack
        *element_out = NULL;
        return 0;
    } else {
        assert(cpage->count>0); // a page should always have at least 1 item
        *element_out = pp_page_get_idx(pool, cpage, cpage->count-1);
        return 1;
    }
}

int paged_pool_pop(paged_pool* pool)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    pp_page *cpage = pp_page_current(pool);
    if (cpage == NULL) {
        // no items on stack
        return 0;
    } else {
        if(--cpage->count ==0) {
            // free this page
            pp_page_free(pool, cpage);
            pool->pages_tail--;
        }
        return 1;
    }
}


pp_page_iterator* paged_pool_push_frame(paged_pool* pool, void* framedata_ptr)
{
    pp_page* cp = paged_pool_next_page(pool, framedata_ptr);
    cp->flags = PPF_FRAME;
    return cp;
}

void paged_pool_pop_frame(paged_pool* pool)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    bool framepage = false;
    pp_page* p = pool->pages_tail - 1;
    while(!framepage && p >= pool->pages_head) {
        assert(p);
        assert(p->ptr);
        framepage = (p->flags & PPF_FRAME)>0;   // set if page is a frame start page
        pp_page_free(pool, p);      // free/clear the page
        pool->pages_tail--;         // remove the page
        p--;
    }
}

pp_page_iterator* paged_pool_next_frame(paged_pool* pool, pp_page_iterator* itr)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    // free all the pages
    if(itr ==NULL)
        itr = pool->pages_head;
    else
        itr++;  // start with next page
    while(itr < pool->pages_tail) {
        assert(itr);
        assert(itr->ptr);
        if(itr->flags & PPF_FRAME) {
            // found the last frame page
            return itr;
        }
        itr++;
    }
    return NULL;
}

pp_page_iterator* paged_pool_previous_frame(paged_pool* pool, pp_page_iterator* itr)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    // free all the pages
    if(itr ==NULL)
        itr = pool->pages_tail-1;
    else
        itr--;  // start with previous page
    while(itr >= pool->pages_head) {
        assert(itr);
        assert(itr->ptr);
        if(itr->flags & PPF_FRAME) {
            // found the last frame page
            return itr;
        }
        itr--;
    }
    return NULL;
}

void* paged_pool_frame_dataptr(paged_pool* pool, pp_page_iterator* itr)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    // free all the pages
    pp_page* p = pool->pages_tail - 1;
    while(p >= pool->pages_head) {
        assert(p);
        assert(p->ptr);
        if(p->flags & PPF_FRAME) {
            // found the last frame page
            return p->userptr;
        }
        p--;
    }
    return NULL;
}

long paged_pool_frame_foreach(paged_pool* pool, pp_page_iterator* itr, void* pUserData, paged_pool_element_callback cb)
{
    PAGED_POOL_INPUTS_MUST_BE_VALID
    assert(itr->flags & PPF_FRAME); // must start with a frame page
    size_t idx=0;
    long rv;
    while(itr < pool->pages_tail) {
        assert(itr);
        assert(itr->ptr);
        for(uint32_t pidx=0; pidx < itr->count; pidx++, idx++) {
            if((rv=cb(pp_page_get_idx(pool, itr, pidx) , pUserData, idx)) !=0) {
                return rv;
            }
        }

        // next page if it is a continuing page of this frame
        itr++;
        if(itr->flags & PPF_FRAME)
            break;  // found the next frame, so stop
    }
    return 0;
}