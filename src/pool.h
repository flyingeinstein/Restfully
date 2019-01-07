/// @file
/// @brief Manages a collection of objects in an efficient manner.
/// An object pool is allocated with a starting capacity. This allocates memory for multiple objects (capacity) but with
/// a 'length' of zero elements (objects). As new elements are requested the 'length' (number of elements) is incremented
/// but the elements are returned from the preallocated memory until length equals capacity. When capacity is reached
/// the pool is reallocated with a larger buffer. This reallocation uses realloc() and thus may or may not move memory
/// locations. Thus, you should consider any existing pointers to objects in the pool as invalid after new elements are
/// created.
///
/// These pool functions should not be used directly by an API user but instead be wrapped with functions that cast
/// to the type of element the pool contains. See refstack.h for an example.
///
#ifndef ANALYTICSAPI_POOL_H
#define ANALYTICSAPI_POOL_H

#include "ds-config.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char* pool_element_ptr;

/// \brief Allocate a new pool, or grow an existing pool of elements.
/// This is used in leu of any pool create function. On entry if the head, tail and end pointers are NULL then
/// new memory is allocated of the given capacity. If the head/tail/end pointers are valid, then memory is reallocated.
/// Reallocation can be used to grow or shrink the array. if capacity is less than the current pool length then the
/// elements beyond capacity will be truncated and the pool length will shrink.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \param capacity - the starting capacity of the pool
/// \return the number of elements allocated (the capacity)
DS_EXPORT size_t pool_reserve(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                  size_t capacity);

/// \brief Return the number of elements in the pool (the number in use)
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \return the number of used elements
DS_EXPORT size_t pool_length(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size);

/// \brief return the current holding capacity of the pool before more memory will be requested.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \return
DS_EXPORT size_t pool_capacity(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size);

/// \brief free all memory related to the pool.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
DS_EXPORT void pool_free(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size);

/// \brief get element at the given 0th-based index.
/// This is the official element getter, but you might as well just save yourself a function call and use &head[n], it's the same thing.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \param index - the 0th based index of the element to fetch
DS_EXPORT pool_element_ptr pool_get_element(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size, size_t index);

/// \brief Get a new element from the pool for use.
/// The next element in the pool is returned for use. No new memory is allocated unless we were at capacity then the
/// pool will grow to fit the new element. This increases the length by one.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \param element_out - on return, this pointer is set to point to the new element.
/// \return non-zero on success, 0 or <0 on failure
DS_EXPORT int pool_new_element(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                  pool_element_ptr* element_out);

/// \brief Get a contiguous number of elements from the pool for use.
/// The next *count* elements in the pool are reserved and the first element is returned in *first_element_out*
/// argument. No new memory is allocated unless we are over capacity then the pool will grow to fit the new elements.
/// This increases the length by count.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \param count - the number of elements to return
/// \param first_element_out - on return, this pointer is set to the first element.
/// \return the number of elements returned.
DS_EXPORT int pool_new_elements(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                   size_t count,
                   pool_element_ptr* first_element_out);

/// \brief Push a new element to the stack (end of the pool).
/// The contents of the given element is coped to the next available element in the pool. The length of the pool
/// increases by one.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \param element - the element to be copied to the pool
/// \return 1 if the element was pushed onto the stack, or 0 on error.
DS_EXPORT int pool_push(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
                     pool_element_ptr element);

/// \brief Return the element on top of the stack (end of the pool).
/// Returns the last element without affecting the pool. If there are no elements in the pool yet then NULL is returned.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \param element_out - on return, points to the element on top of the stack (the last element in the pool)
/// \return non-zero if an element was returned, or 0 if there were no elements in the stack.
DS_EXPORT int pool_top(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size,
             pool_element_ptr* element_out);

/// \brief Remove the last element in the stack.
/// This reduces the length of the pool by one.
/// \param head - pointer to the pool's head. Should be NULL for new pools or a valid pointer for existing pool.
/// \param tail - pointer to the pool's insertion pointer or the element after the last valid element in the pool (length).
/// \param end - pointer to the end of the pool's memory (capacity).
/// \param element_size - the size of each element (use the C sizeof operator)
/// \return 1 if an element was pooped, 0 on error.
DS_EXPORT int pool_pop(pool_element_ptr* head, pool_element_ptr* tail, pool_element_ptr* end, size_t element_size);

#ifdef __cplusplus
}
#endif

#endif //ANALYTICSAPI_POOL_H
