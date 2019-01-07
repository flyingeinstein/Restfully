/// @file
/// @brief Manages a collection of objects in an efficient manner using pages of memory.
/// A paged object pool uses a collection of memory pages of constant size. Within each page it can store a certain
/// number of whole objects before requiring another page to be allocated. Since each page in a pool is the same size,
/// each page is capable of storing the same number of objects. Thus lookup of an element by it's index is simple math
/// calculation of the page number and page offset.
/// Another advantage of paged memory is the memory addresses of previous objects do not change or invalidate when the
/// collection grows. Thus, you can keep pointers to any objects in the pool and they will stay valid as long as the pool
/// exists.
/// Memory paging is also more efficient for large collections as realloc() is never used to grow large memory regions
/// as is needed for vector type arrays and lists. The need for new pages will take constant time even for large pools.
#ifndef ANALYTICSAPI_PAGED_PAGED_POOL_H
#define ANALYTICSAPI_PAGED_PAGED_POOL_H

#include "ds-config.h"
#include <stdlib.h>
#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
	typedef unsigned char* paged_pool_element_ptr;
#else
	typedef void* paged_pool_element_ptr;
#endif

// opague type that represents the pool
typedef struct _paged_pool paged_pool;

// opague type that represents a page
typedef struct _pp_page pp_page;
typedef pp_page pp_page_iterator;			// alias to make it clear what is an iterator

typedef long (*paged_pool_element_callback)(paged_pool_element_ptr element, void* pUserData, size_t index);

/// \brief Create/allocate a new object pool with the default page size (currently 2048).
/// \param pool - a pointer to a pool* which will receive the new paged pool.
/// \param element_sizeof - pass in the value of sizeof() for your storage type.
/// \return the current capacity of the pool, or negative number if creation failed.
DS_EXPORT size_t paged_pool_create(paged_pool** pool, size_t element_sizeof);

/// \brief Create/allocate a new object pool.
/// A new pool is created. Initially no pages are allocated. Since memory is allocated in pages if given a good page_size
/// value calls to the memory allocator (calloc/malloc) will be few and thus optimal.
/// \param pool - a pointer to a pool* which will receive the new paged pool.
/// \param element_sizeof - pass in the value of sizeof() for your storage type.
/// \param page_size - The desired size of each page in bytes.
/// \return the current capacity of the pool, or negative number if creation failed.
DS_EXPORT size_t paged_pool_create_ex(paged_pool** pool, size_t element_sizeof, size_t page_size);

#if 0 // this function doesnt do anything but reserve the page headers which isnt much of an advantage, if needed it should create pages too
/// \brief Grow or shrink an existing pool of elements to optimize for a specific number of elements.
/// if capacity is less than the current pool length then the
/// elements beyond capacity will be truncated and the pool length will shrink.
/// \param pool - the object pool to be resized, must be an existing pool.
/// \param capacity - the starting capacity of the pool
/// \return the number of elements allocated (the capacity)
DS_EXPORT size_t paged_pool_reserve(paged_pool* pool, size_t capacity);
#endif

/// \brief Return the number of elements in the pool (the number in use)
/// \param pool - a valid object pool
/// \param element_size - the size of each element (use the C sizeof operator)
/// \return the number of used elements
DS_EXPORT size_t paged_pool_length(paged_pool* pool);

/// \brief return the current holding capacity of the pool before more memory will be requested.
/// This is equal to the number of pages times the number of items that can be wholly contained in a single page.
/// \param pool - a valid object pool
/// \return the current capacity of the pool
DS_EXPORT size_t paged_pool_capacity(paged_pool* pool);

/// \brief Return the number of pages allocated in the pool
/// \param pool - pointer to a paged pool object
/// \return the number of used elements
DS_EXPORT size_t paged_pool_pagecount(paged_pool* pool);

/// \brief free all memory related to the pool.
/// \param pool - a valid object pool
DS_EXPORT void paged_pool_free(paged_pool* pool);

/// \brief Get a new element from the pool for use.
/// The next element in the pool is returned for use. No new memory is allocated unless we were at capacity then the
/// pool will grow to fit the new element. This increases the length by one.
/// \param pool - a valid object pool
/// \param element_out - on return, this pointer is set to point to the new element.
/// \return non-zero on success, 0 or <0 on failure
DS_EXPORT int paged_pool_new_element(paged_pool* pool, paged_pool_element_ptr* element_out);

/// \brief Push a new element to the stack (end of the pool).
/// The contents of the given element is coped to the next available element in the pool. The length of the pool
/// increases by one.
/// \param pool - a valid object pool
/// \param element - the element to be copied to the pool
/// \return 1 if the element was pushed onto the stack, or 0 on error.
DS_EXPORT int paged_pool_push(paged_pool* pool, paged_pool_element_ptr element);

/// \brief Return the element on top of the stack (end of the pool).
/// Returns the last element without affecting the pool. If there are no elements in the pool yet then NULL is returned.
/// \param pool - a valid object pool
/// \param element_out - on return, points to the element on top of the stack (the last element in the pool)
/// \return non-zero if an element was returned, or 0 if there were no elements in the stack.
DS_EXPORT int paged_pool_top(paged_pool* pool, paged_pool_element_ptr* element_out);

/// \brief Checks if the given element is contained in any of our pages
/// This does a search of all pages checking if the given element pointer is contained in the page's memory range. No
/// validation of the element contents is done, it's just a 'is memory address in a page' check.
/// \param pool - a valid object pool
/// \param element - a pointer to an element which is expected to be in one of the pool's pages
/// \return the page index that contains the pointer, or -1 if not within the paged pool memory
DS_EXPORT long paged_pool_contains_element(paged_pool* pool, paged_pool_element_ptr element);

/// \brief Get the Nth element in the pool
/// Used for direct access to an element. Lookup is constant O(c) because with an index we can compute the page it's
/// in using a division operation and then the offset within the page using modulus/remainder. Access to pages are
/// also of constant time.
/// \param pool - a valid object pool
/// \param idx - the 0-based index of the object to access
/// \return a pointer to the Nth element
DS_EXPORT paged_pool_element_ptr paged_pool_get_element(paged_pool* pool, size_t idx);

/// \brief Call the element callback function for each valid element in the pool collection
/// The cb function is called for each element. If the callback returns 0 then iteration continues to the next element.
/// If the callback returns anything but zero, then iteration stops and the for-each returns the forwarded value from the callback.
/// \param pool - a valid object pool
/// \param pUserData - optional object pointer passed to the callback as-is. This can be NULL.
/// \param cb - The function callback to call for each element.
DS_EXPORT long paged_pool_foreach(paged_pool* pool, void* pUserData, paged_pool_element_callback cb);

/// \brief Remove the last element in the stack.
/// This reduces the length of the pool by one.
/// \param pool - a valid object pool
/// \return 1 if an element was pooped, 0 on error.
DS_EXPORT int paged_pool_pop(paged_pool* pool);

/// \brief Start a new frame
/// Begins a new page even if the current one is not full and marks the page as a FRAME START. This can be useful if
/// you are using the paged-pool as a way to implement a framed stack. Framed stacks can be used to push groups of
/// elements onto a stack and pop them off as a group. A familiar example being stack frames of recursive function calls.
/// \param pool - a valid object pool
/// \param puserdata - set user data value associated to the new page
/// \returns The new frame page iterator
DS_EXPORT pp_page_iterator* paged_pool_push_frame(paged_pool* pool, void* framedata_ptr);

/// \brief Pops the current frame and any elements within it
/// Returns to the previous frame. Elements associated with the current frame are removed.
/// \param pool - a valid object pool
DS_EXPORT void paged_pool_pop_frame(paged_pool* pool);

/// \brief Returns the frame page immediately following the given iterator
/// Advances pages from the given itr until a new frame page is found. If itr is NULL, then the first (oldest) frame page
/// is returned.
/// \param pool - a valid object pool
/// \param itr - iterator pointer to frame page in which to retrieve the dataptr, or NULL to return first frame
DS_EXPORT pp_page_iterator* paged_pool_next_frame(paged_pool* pool, pp_page_iterator* itr);

/// \brief Returns the previous frame page from given iterator
/// Rewinds pages from the given itr until a new frame page is found. If itr is NULL, then the last (current) frame page
/// is returned.
/// \param pool - a valid object pool
/// \param itr - iterator pointer to frame page in which to retrieve the dataptr, or NULL to return last (current) frame
DS_EXPORT pp_page_iterator* paged_pool_previous_frame(paged_pool* pool, pp_page_iterator* itr);

/// \brief Return the frame data ptr associated with the given frame, or the current frame
/// \param pool - a valid object pool
/// \param itr - iterator pointer to frame page in which to retrieve the dataptr, or NULL for current frame
/// \returns A user-defined frame data ptr value, or NULL if not defined
DS_EXPORT void* paged_pool_frame_dataptr(paged_pool* pool, pp_page_iterator* itr);

/// \brief Call the element callback function for each valid element in the given frame
/// The cb function is called for each element. If the callback returns 0 then iteration continues to the next element.
/// If the callback returns anything but zero, then iteration stops and the for-each returns the forwarded value from the callback.
/// \param pool - a valid object pool
/// \param itr - iterator pointer to frame page in which to retrieve the dataptr, or NULL for current frame
/// \param pUserData - optional object pointer passed to the callback as-is. This can be NULL.
/// \param cb - The function callback to call for each element.
DS_EXPORT long paged_pool_frame_foreach(paged_pool* pool, pp_page_iterator* itr, void* pUserData, paged_pool_element_callback cb);

#ifdef __cplusplus
}
#endif

#endif //ANALYTICSAPI_PAGED_POOL_H
