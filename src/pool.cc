
#include "pool.h"

#include <assert.h>
#include <memory.h>

// we always pass in these standard list of parameters, so might as well save us some time
#define STANDARD_POOL_PARAMS    head, tail, end, element_size

// most functions require valid pool inputs, let's validate them
#define POOL_INPUTS_MUST_BE_VALID     \
        assert(*head!=NULL);   \
        assert(*tail!=NULL);   \
        assert(*end!=NULL);    \
        assert(element_size>0); /* elements must be of non-zero size */ \
        assert(*head <= *tail); /* head must be less than the tail   */ \
        assert(*head < *end);   /* head must be less than the end    */ \
        assert(*tail < *end);   /* tail must be less than the end    */



size_t pool_reserve(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                  size_t capacity)
{
    if(capacity<=0)
        capacity = 16;
    if(*head!=NULL) {
        assert(*tail!=NULL && *end!=NULL);  // these must also be valid if head is valid

        // reallocate more memory, or trim the amount of memory we have
        //size_t existing_capacity = pool_capacity(STANDARD_POOL_PARAMS);
        size_t length = pool_length(STANDARD_POOL_PARAMS);
        *head = (pool_element_ptr)realloc(*head, capacity * element_size);
        *end = *head + capacity*element_size;

        // now adjust the tail depending on if our previous number of items still fit within the array
        *tail = (capacity < length)
                ? *end
                : *head + length*element_size;

        // now clear out memory from tail to end as calloc() would do
        if(end > tail) {
            size_t clear_length = (unsigned char*)*end - (unsigned char*)*tail;
            memset(*tail, 0, clear_length);
        }

        return capacity;
    } else {
        // new allocation
        *head = *tail = (pool_element_ptr)calloc(capacity, element_size);
        *end = *head + capacity * element_size;
        return capacity;
    }

}

size_t pool_length(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size)
{
    POOL_INPUTS_MUST_BE_VALID
    return (*tail - *head) / element_size;
}

size_t pool_capacity(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size)
{
    POOL_INPUTS_MUST_BE_VALID
    return (*end - *head) / element_size;
}

void pool_free(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size)
{
    POOL_INPUTS_MUST_BE_VALID
    free(*head);
    *head = *tail = *end = NULL;
}

pool_element_ptr pool_get_element(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size, size_t index)
{
    POOL_INPUTS_MUST_BE_VALID
    short index_within_bounds = index < ((*tail - *head) / element_size);
    assert(index_within_bounds   /* index out of bounds */ );
    return index_within_bounds ? (*head + index*element_size) : NULL;
}

int pool_new_element(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                     pool_element_ptr* element_out)
{
    POOL_INPUTS_MUST_BE_VALID
    return pool_new_elements(STANDARD_POOL_PARAMS, 1, element_out);
}

int pool_new_elements(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                      size_t count,
                      pool_element_ptr* first_element_out)
{
    POOL_INPUTS_MUST_BE_VALID
    pool_element_ptr _tail = (unsigned char*)*tail + element_size * count;
    if(_tail >= *end) {
        // allocate more items
        pool_reserve(STANDARD_POOL_PARAMS, (size_t)(pool_capacity(STANDARD_POOL_PARAMS) * 1.5)
        );

        // recurse
        return pool_new_elements(STANDARD_POOL_PARAMS, count, first_element_out);
    }

    // requested count fits in pool
    *first_element_out = *tail;
    *tail = _tail;
    return count;
}

int pool_push(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
              pool_element_ptr element)
{
    POOL_INPUTS_MUST_BE_VALID
    pool_element_ptr o;
    if(pool_new_element(STANDARD_POOL_PARAMS, &o)) {
        // copy the object onto the top of the pool
        memcpy(o, element, element_size);
        return 1;
    } else
        return 0;
}

int pool_top(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
             pool_element_ptr* element_out)
{
    POOL_INPUTS_MUST_BE_VALID
    if(*head==*tail) {
        // no items on stack
        *element_out = NULL;
        return 0;
    } else {
        *element_out = (unsigned char*)*tail - element_size;
        return 1;
    }
}

int pool_pop(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size)
{
    POOL_INPUTS_MUST_BE_VALID
    //assert(*head != *tail);   // you betta not be crashin into the bottom of my stack!
    if(*head==*tail)
        return 0;
    *tail -= element_size;
    return 1;
}

