/// \file
/// \brief Efficiently stores a growing array of character or binary strings (AKA string table)
#ifndef ANALYTICSAPI_BINBAG_H
#define ANALYTICSAPI_BINBAG_H

#include "ds-config.h"
#include <stdint.h>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

// split string flags
#define SF_NONE             0
#define SF_IGNORE_EMPTY     1



typedef struct _binbag {
    // allocated memory buffer range
    // We only allocate a single buffer and then grow as needed. Strings are inserted into the buffer consecutively.
    // An array of pointers is kept where each element points into this buffer for each string inserted. This array
    // is stored at the end of the allocated memory and thus the insertion for this array like a heap grows down (decrements).
    // When the buffer grows, the pointer array is copied to the new end of the buffer.
    // Memory layout:
    // [begin                tail              ][element 3 | 2 | 1 | 0 ](end)
    char *begin;
    char *end;

    // insertion point for new strings
    char *tail;

    // the start of the array of pointers to each string
    const char **elements;

    // parameters of the binbag
    double growth_rate;

    // the number of growths we did
    size_t growths;
} binbag;

/// \brief Allocate a new empty binbag
DS_EXPORT binbag *binbag_create(size_t capacity_bytes, double growth_rate);

DS_EXPORT binbag* binbag_split_string(int seperator, unsigned long flags, const char* str);

//binbag* binbag_create_from_array(const char** arr, size_t count, double fill_space, double growth_rate);

DS_EXPORT void binbag_free(binbag *bb);

DS_EXPORT size_t binbag_resize(binbag *bb, size_t capacity);

/// \brief Returns the bytes used for all the strings combined
DS_EXPORT size_t binbag_byte_length(binbag *bb);

DS_EXPORT size_t binbag_count(binbag* bb);

/// \brief Returns the character capacity of the binbag not including space used for elements
DS_EXPORT size_t binbag_capacity(binbag *bb);

DS_EXPORT size_t binbag_free_space(binbag *bb);

DS_EXPORT long binbag_insert(binbag *bb, const char *str);
DS_EXPORT long binbag_insertn(binbag *bb, const char *str, int length);

/// \brief Insert a string if not exists
/// This function is the same as binbag_insert() but first it checks to see if it already exists in the binbag. If so,
/// then the existing element is returned.
DS_EXPORT long  binbag_insert_distinct(binbag* bb, const char* str);

DS_EXPORT void binbag_inplace_reverse(binbag *bb);

//const unsigned char* binbag_binary_insert(const unsigned char* str, size_t len);

DS_EXPORT const char *binbag_get(binbag *bb, size_t idx);

//const unsigned char* binbag_binary_get(int idx, size_t* len_out);

/// \brief Find the ordinal index of the given string using the supplied comparison function (works with strcmp, strcasecmp, etc)
DS_EXPORT long binbag_find(binbag *bb, const char* match, int (*compar)(const char*,const char*));

/// \brief Find the ordinal index of the given string
DS_EXPORT long binbag_find_case(binbag *bb, const char* match);

/// \brief Find the ordinal index of the given string using a case insensitive comparison
DS_EXPORT long binbag_find_nocase(binbag *bb, const char* match);


/// \brief Compare function used to sort in ascending order
DS_EXPORT int binbag_element_sort_asc (const void * _lhs, const void * _rhs);

/// \brief Compare function used to sort in descending order
DS_EXPORT int binbag_element_sort_desc (const void * _lhs, const void * _rhs);

/// \brief Sort the binbag.
/// You can use one of the existing sort routines for standard character sorting, binbag_element_sort_asc or binbag_element_sort_desc.
DS_EXPORT binbag* binbag_sort(binbag *bb, int (*compar)(const void*,const void*));

/// \brief Sort the values and reduce memory usage by aliasing duplicates to the same string memory.
/// This method does not eliminate duplicates, it keeps duplicates in the string table but after reduction these duplicates
/// will point to the same C string in memory. This is useful for having semi-static lookup tables where duplicates are
/// desired but you also want to save memory.
//binbag* binbag_flyweight_reduce(binbag *bb, int (*compar)(const void*,const void*)); - not yet implemented

/// \brief Reduce the binbag to only distinct string values.
/// Values will also be sorted using binbag_element_sort_asc.
DS_EXPORT binbag* binbag_distinct(binbag *bb);

/// \brief Reduce the binbag to only distinct string values.
/// Values will also be sorted. You can use one of the existing sort routines for standard character sorting, binbag_element_sort_asc or binbag_element_sort_desc.
DS_EXPORT binbag* binbag_sort_distinct(binbag *bb, int (*compar)(const void*,const void*));

DS_EXPORT void binbag_debug_print(binbag* bb);

DS_EXPORT const char **binbag_begin_iterator(binbag *bb);

DS_EXPORT const char **binbag_end_iterator(binbag *bb);

#ifdef __cplusplus
}
#endif

#endif //ANALYTICSAPI_BINBAG_H
