//
// Created by Colin MacKenzie on 5/18/17.
//

#include "binbag.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <cstring>


#if defined(HAS_STRINGS_H)
#include <strings.h>
#endif

#if defined(HAS_STRING_H)
#include <string.h>
#endif

// std includes cannot have inline defined as a macro
#ifdef inline
#undef inline
#endif
#include <algorithm>

#if !defined(HAS_STPNCPY)
char* stpncpy(char* dest, const char* src, size_t n) {
    while(*src && --n)
        *dest++ = *src++;
//    if(n==0) {
//        dest--; // back up one byte
//        *dest = 0;
//    } else
        *dest = 0;
    return dest;
}
#endif

#if !defined(HAS_STPCPY)
char* stpcpy(char* dest, const char* src) {
    while(*src)
        *dest++ = *src++;
    *dest = 0;
    return dest;
}
#endif

#define binbag_elements_end

// the number of protective fenceposts between the string data and the string array
#define FENCEPOSTS  4

const uint32_t fencepost = 0xdefec8ed;

/* Fencepost routines
 * Enforce memory bounds between string data and string pointer array against accidental writes by writing a fencepost
 * value between the two buffers and checking the values dont change before and after string table inserts.
 */
#if defined(FENCEPOSTS) && FENCEPOSTS>0
void check_fencepost(binbag* bb)
{
    uint32_t* fpmem = (uint32_t*)binbag_begin_iterator(bb) - FENCEPOSTS;
    for(int i=0; i<FENCEPOSTS; i++)
        if(fpmem[i] != fencepost) {
            printf("binbag: fencepost was corrupted at position %d, expected %08x, got %08x\n", i, fencepost, fpmem[i]);
            binbag_debug_print(bb);
            assert(fpmem[i] == fencepost);
        }
}

void write_fencepost(binbag* bb)
{
    uint32_t* fpmem = (uint32_t*)binbag_begin_iterator(bb) - FENCEPOSTS;
    for(int i=0; i<FENCEPOSTS; i++)
        fpmem[i] = fencepost;
}

void binbag_debug_print(binbag* bb)
{
    uint32_t* fpmem = (uint32_t*)binbag_begin_iterator(bb) - FENCEPOSTS;
    size_t N;
    char s[1000];
    size_t slen = 0;
    long long offset;
    printf("memory contents of binbag: %u elements, %u of %ub capacity used   (freespace %u\nelements:\n",
           (unsigned int)(N=binbag_count(bb)), (unsigned int)binbag_byte_length(bb),
           (unsigned int)binbag_capacity(bb), (unsigned int)binbag_free_space(bb));
    for(size_t j=0; j<N; j++) {
        const char* el = binbag_get(bb, j);

        if(el==NULL) {
            strcpy(s, "NULL!");
            offset = 0;
            slen = 0;
        } else if(el < bb->begin || el > bb->tail) {
            sprintf(s, "out-of-bounds!   addr=0x%08llx", (unsigned long long) el);
            offset = 0;
            slen = 0;
        } else {
            slen = strlen(el);
            offset = el - bb->begin;
            if(slen<80)
                strcpy(s, el);
            else {
                strncpy(s, el, 40);
                s[40]=s[41]=s[42]='.';
                strcpy(&s[43], el + slen - 37);
            }
        }
        printf("   %4u: @%08llu %4ub %s\n", (unsigned int)j, offset, (unsigned int)slen, s);
    }

    // print fenceposts
#if defined(FENCEPOSTS) && FENCEPOSTS >0
    bool ok = true;
    printf("fenceposts:");
    for(int j=0; j<FENCEPOSTS; j++) {
        if(fpmem[j] != fencepost)
            ok = false;
        printf(" 0x%08x", fpmem[j]);
    }
    printf("   %s\n", ok ? "OK":"CORRUPT");
#endif
}
#else
#define FENCEPOSTS  0
#define check_fencepost(bb)
#define write_fencepost(bb)
void binbag_debug_print(binbag* bb) { printf("binbag: fencepost checking disabled.\n"); }
#endif

binbag* binbag_create(size_t capacity_bytes, double growth_rate)
{
    if(capacity_bytes<32)
        capacity_bytes = 32;
    binbag* bb = (binbag*)calloc(1, sizeof(binbag));
    if(bb== nullptr) return nullptr;
    capacity_bytes += FENCEPOSTS*sizeof(fencepost); // fencepost DMZ
    bb->begin = bb->tail = (char*)malloc(capacity_bytes);
    if(bb->begin==nullptr) {
        ::free(bb);
        return nullptr;
    }
    bb->end = bb->begin + capacity_bytes;
    bb->elements = (const char**)bb->end;
    bb->growth_rate = growth_rate;
    bb->growths = 0;
    write_fencepost(bb);
    check_fencepost(bb);
    return bb;
}

binbag* binbag_split_string(int seperator, unsigned long flags, const char* str)
{
    int slen=0, scnt = *str ? 1 : 0;
    const char *p = str, *s = str;
    while(*p) {
        slen++;
        if (*p==seperator) {
            scnt++;
            s = p;
        }
        p++;
    }
    binbag* bb = binbag_create(slen + scnt*(sizeof(char*)+1), 1.5);
    if(scnt>0) {
        p = s = str;
        while (*p) {
            if (*p == seperator) {
                if(p>s || (flags & SF_IGNORE_EMPTY)==0)
                    binbag_insertn(bb, s, p-s);
                s = p+1;
            }
            p++;
        }
        if(p>s || (flags & SF_IGNORE_EMPTY)==0)
            binbag_insertn(bb, s, p-s);
    }
    return bb;
}

//binbag* binbag_create_from_array(const char** arr, size_t count, double fill_space, double growth_rate);

void binbag_free(binbag* bb)
{
    // just a sanity check
    assert(bb->end > bb->begin);
    free(bb->begin);
    free(bb);
}


size_t binbag_byte_length(binbag* bb)
{
    return bb->tail - bb->begin;
}

size_t binbag_count(binbag* bb)
{
    return binbag_end_iterator(bb) - binbag_begin_iterator(bb);
}

size_t binbag_capacity(binbag* bb)
{
    return (char*)bb->elements - bb->begin - FENCEPOSTS*sizeof(fencepost);
}

#if !defined(NDEBUG)
void __binbag_sanity_check(binbag* bb)
{
    size_t _text_memsize = bb->tail - bb->begin;
    size_t _existing_memsize = bb->end - bb->begin;
    size_t _elements_offset = (char*)bb->elements - bb->begin;
    size_t _count = binbag_count(bb);

    // some sanity checks
    assert(_count >=0);
    assert(_existing_memsize >= _elements_offset);
    assert(_elements_offset >= _text_memsize);
    check_fencepost(bb);
}
#else
#define __binbag_sanity_check(bb)
#endif


size_t binbag_resize(binbag* bb, size_t capacity)
{
    size_t _text_memsize = bb->tail - bb->begin;    // amount of memory to store all existing strings (plus null term)
    char* old_bb_begin_ptr = bb->begin;       // keep mem location before realloc() which we need to recalculate index element pointers
    size_t _count = binbag_count(bb);   // number of string elements
    size_t _elements_offset = (char*)bb->elements - bb->begin;  // offset into memory where string index currently is at

    // compute the minumum capacity and expand requested capacity if its less
    size_t min_capacity = _count*sizeof(const char*) + _text_memsize + FENCEPOSTS* sizeof(fencepost);
    if(capacity < min_capacity)
        capacity = min_capacity;

    size_t _memsize = bb->end - bb->begin;
    //printf("  binbag: resizing from %ld to %lu with %ld elements\n", _memsize, capacity, _count);

    __binbag_sanity_check(bb);

    // if shrinking, move the index now because realloc() would free the upper memory where the index is
    if(capacity < _memsize) {
        ssize_t ofs = _memsize - capacity;
        memmove((char*)bb->elements - ofs, (char*)bb->elements, _count*sizeof(char*) );
        bb->elements = (const char**)((char*)bb->elements - ofs);
        _elements_offset -= ofs;   // reset elements_offset
    }

    // realloc the memory if we are growing. validate that realloc() didnt fail
    bb->begin = (char*)realloc(bb->begin, capacity);
    if(bb->begin ==NULL) {
        // _begin still contains the old valid memory, so abort our resize
        bb->begin = old_bb_begin_ptr;
        return 0;
    }

    // realloc success, adjust other memory pointers
    bb->end = bb->begin + capacity;
    bb->tail = bb->begin + _text_memsize;
    bb->elements = binbag_end_iterator(bb) - _count;

    // move the elements of the string index if required
    // if shrinking we already moved the index but may need to adjust string pointers in the index
    if(_count>0) {
        // memory moved, so we need to relocate the string pointers in our array to
        // point to the new memory range...right now they may point to old unallocated memory
        const char** psrc_begin = (const char**)(bb->begin + _elements_offset);
        const char** psrc_end = psrc_begin + _count;  // start at end of elements in source
        const char** pto = bb->elements;   // end of elements in destination

        if(old_bb_begin_ptr == bb->begin) {
            // no change in memory location during relocation so we can simply move our array of elements
            // memmove() takes care of overlapping memory ranges
            memmove((char *) pto, psrc_begin, _count * sizeof(char *));
            //printf("   memory was moved via memmove\n");
        } else {
            if(pto <= psrc_begin) {
                // index is moving left (or not moving at all), must copy forward
                while(psrc_begin < psrc_end) {
                    *pto++ = bb->begin + (*psrc_begin++ - old_bb_begin_ptr);
                }
                //printf("   memory was moved left\n");
            } else {
                pto += _count; // move destination to end of elements

                // index is moving right, so must copy from back to front
                while(psrc_end > psrc_begin) {
                    *--pto = bb->begin + (*--psrc_end - old_bb_begin_ptr);
                }
                //printf("   memory was moved right\n");
            }
        }

    }

    // post-op sanity checks
    assert(bb->end >= (char*)bb->elements);
    assert((char*)bb->elements >= bb->tail);
    assert(bb->tail >= bb->begin);
    write_fencepost(bb);    // update new fencepost DMZ
    bb->growths++;
    return capacity;
}

size_t binbag_free_space(binbag* bb)
{
    ssize_t free_space = (const char*)bb->elements - FENCEPOSTS* sizeof(fencepost) - bb->tail;
    return (free_space>0) ? (size_t)free_space : 0;
}

long binbag_insert_distinct(binbag* bb, const char* str, int (*compar)(const char*,const char*))
{
    for(size_t i=0, c=binbag_count(bb); i<c; i++)
        if(compar(binbag_get(bb, i), str) ==0)
            return i;
    return binbag_insert(bb, str);
}

long  binbag_insert_distinct_n(binbag* bb, const char* str, size_t n, int (*compar)(const char*,const char*, size_t n))
{
    for(size_t i=0, c=binbag_count(bb); i<c; i++)
        if(compar(binbag_get(bb, i), str, n) ==0)
            return i;
    return binbag_insertn(bb, str, n);
}

long binbag_insert(binbag* bb, const char* str)
{
    return binbag_insertn(bb, str, -1);
}

long binbag_insertn(binbag *bb, const char *str, int length)
{
    // trying to do this without needing a strlen() and a strcpy() operation
    bool all = false;
    int fs = (int)binbag_free_space(bb) - 5 - FENCEPOSTS* sizeof(fencepost); // save 4 bytes for added array element and the null character
    check_fencepost(bb);
    char *_p = NULL;

    if(fs>0 && length<fs) {
        size_t _ll = (size_t)((length<0) ? fs : std::min(fs, length+1));
        _p = stpncpy(bb->tail, str, _ll);  // must use signed inner type
        ssize_t copied = _p - bb->tail;
        if(length>=0 && copied>=length) {
            *_p = 0;
            all = true;
        } else
            all = (str[copied] == 0); // we should see the null marker terminated the string
        check_fencepost(bb);
    }

    if(!all) {
        // since we didnt get everything, we should do a strlen() to determine exact size and make sure we grow enough
        int slen = std::min(length, (int)strlen(str));

        // grow the buffer
        size_t existing_capacity = bb->end - bb->begin;
        size_t new_capacity = (size_t)(existing_capacity * std::min(10.0, std::max(1.1, bb->growth_rate))) + slen+1+sizeof(char*);
        if(0== binbag_resize(bb, new_capacity))
            return -1;
        return binbag_insertn(bb, str, slen);
    }

    // accept the new string, add string pointer to elements, advance the tail insertion pointer
    size_t idx = binbag_count(bb);      // our new string index
    *--bb->elements = bb->tail;
    bb->tail = _p+1;
    write_fencepost(bb);
    return idx;
}

//const unsigned char* binbag_binary_insert(const unsigned char* str, size_t len);

const char* binbag_get(binbag* bb, long idx)
{
    const char** e = (const char**)(bb->end - sizeof(char*)) - idx;
    return (e < binbag_end_iterator(bb))
        ? *e
        : NULL; // out of bounds
}

long binbag_find(binbag *bb, const char* match, int (*compar)(const char*,const char*))
{
    for(long j=0, N=binbag_count(bb); j<N; j++) {
        const char *el = binbag_get(bb, j);
        if(compar(match, el) ==0) {
            return j;
        }
    }
    return -1;
}

long binbag_find_case(binbag *bb, const char* match)
{
    return binbag_find(bb, match, strcmp);
}

long binbag_find_nocase(binbag *bb, const char* match)
{
    return binbag_find(bb, match, strcasecmp);
}

long binbag_find_n(binbag *bb, const char* match, size_t n, int (*compar)(const char*,const char*, size_t n))
{
    for(long j=0, N=binbag_count(bb); j<N; j++) {
        const char *el = binbag_get(bb, j);
        if(compar(match, el, n) ==0) {
            return j;
        }
    }
    return -1;
}

long binbag_find_case_n(binbag *bb, const char* match, size_t n)
{
    return binbag_find_n(bb, match, n, strncmp);
}

long binbag_find_nocase_n(binbag *bb, const char* match, size_t n)
{
    return binbag_find_n(bb, match, n, strncasecmp);
}

void binbag_inplace_reverse(binbag *bb)
{
    // reverse string table in place
    char **begin = (char**)binbag_begin_iterator(bb),
         **end = (char**)binbag_end_iterator(bb)-1; // actually 'last'
    while(begin < end) {
        // swap entries
        char* w = *begin;
        *begin++ = *end;
        *end-- = w;
    }
}

int binbag_element_sort_desc (const void * _lhs, const void * _rhs)
{
    // we get pointers to the elements, so since our elements are pointers then we must
    // dereference twice to get the resolver structure
    const char* lhs = *(const char**)_lhs;
    const char* rhs = *(const char**)_rhs;
    return strcmp(lhs, rhs);
}

int binbag_element_sort_asc (const void * _lhs, const void * _rhs)
{
    // we get pointers to the elements, so since our elements are pointers then we must
    // dereference twice to get the resolver structure
    const char* lhs = *(const char**)_lhs;
    const char* rhs = *(const char**)_rhs;
    return strcmp(rhs, lhs);    // swapped order
}

binbag* binbag_sort(binbag *bb, int (*compar)(const void*,const void*))
{
    check_fencepost(bb);
    size_t N = binbag_count(bb);

    // create a new binbag with just enough capacity to hold a copy of bb (but sorted)
    size_t capacity = binbag_byte_length(bb) + N*sizeof(char*);
    binbag* sorted = binbag_create(capacity, bb->growth_rate);

    // copy the string table index (this index will still reference strings from old table)
    sorted->elements = binbag_end_iterator(sorted) - N;
    assert((void*)sorted->elements > sorted->begin && (void*)sorted->elements < sorted->end );
    write_fencepost(sorted);    // fencepost in new sorted binbag should still be intact
    memcpy((void*)binbag_begin_iterator(sorted), (const void*)binbag_begin_iterator(bb), sizeof(char*) * N);

    // sort the index we just copied. this doesnt affect the source binbag even though we are still
    // referencing strings from it.
    qsort((char*)binbag_begin_iterator(sorted), N, sizeof(char*), compar);
    check_fencepost(bb);

    // now we copy the strings over. since our index is sorted so will the strings copied over. We update our sorted
    // binbag index with new string locations as we copy
    const char **begin = binbag_begin_iterator(sorted), **end = binbag_end_iterator(sorted);
    int n=0;
    while(begin < end) {
        const char* src = *begin;
        assert(src >= bb->begin && src<bb->end);
        *begin = sorted->tail;
        char* _p = stpcpy(sorted->tail, src); // tail is now at the next character after this string
        sorted->tail = _p + 1;
        n++;
        begin++;
    }

    return sorted;
}

binbag* binbag_distinct(binbag *bb)
{
    return binbag_sort_distinct(bb, binbag_element_sort_asc);
}

binbag* binbag_sort_distinct(binbag *bb, int (*compar)(const void*,const void*))
{
#if 0
    if(binbag_count(bb)<=1)
        return; // duh, arrays of 1 are sorted and distinct

    // sort list so non-unique strings are always next to each other
    binbag_sort(bb, compar);

    // now shrink the elements by eliminating those that are duplicates
    // we are going to build the unique set in-place. So we have two interators. They both start at the 0th element.
    // The 'u' iterator advances only as we encounter a unique element, The 'nu' iterator advanced over each element
    // unique or not. When we encounter a new unique string at 'nu' we copy it to the position at 'u', then advance 'u'.
    char **u = (char**)binbag_begin_iterator(bb);     // i points to the insertion point of the unique set
    char **e = (char**)binbag_end_iterator(bb);       // last element
    char **nu = u;                                    // j points to the next element and iterates over the older non-unique set
    char *insertion_point = *u;           // point where new strings are inserted
    while(nu < e) { // nu will always be ahead of u
        if(strcmp(*u, *nu)!=0) {
            // new unique string
            // advance u, then copy from nu => u
            u++;
            if(u!=nu) {
                char* _p = stpcpy(insertion_point, *nu);
                *u = insertion_point;
                insertion_point = _p+1;     // now immediately after the string we just inserted
            } else
                insertion_point = *(u+1);   // would be the starting point of the next string

            nu++;
        }
    }

    // now if we eliminated any duplicates we must move the string index array to the end of memory
    if(u != nu) {
        size_t new_count = (const char**)u - binbag_begin_iterator(bb);
        const char** new_begin = binbag_end_iterator(bb) - new_count;
        memmove(new_begin, binbag_begin_iterator(bb), new_count);
        bb->elements = new_begin;

    }
#endif
    return NULL;
}

const char **binbag_begin_iterator(binbag *bb) {
	return bb->elements;
}

const char **binbag_end_iterator(binbag *bb) {
	return (const char **)bb->end;
}

//const unsigned char* binbag_binary_get(binbag* bb, size_t idx, size_t* len_out);

