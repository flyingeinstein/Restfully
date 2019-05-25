//
// Created by Colin MacKenzie on 5/18/17.
//

#include "StringPool.h"
#include "Pool.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <cstring>

using namespace Rest;

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

#if defined(ARDUINO)
#include <Arduino.h>

int default_BinBag::print(const char* str)
{
    return Serial.print(str);
}
#else
int default_binbag_print(const char* str)
{
    return fputs(str, stdout);
}
#endif

#if !defined(HAS_STPNCPY)
char* stpncpy(char* dest, const char* src, size_t n) {
    while(*src && n--)
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

StringPool::StringPool(size_t page_size)
    : PagedPool(page_size, true)
{

}

StringPool::StringPool(const StringPool& copy) noexcept
    : PagedPool(copy)
{
}

StringPool::StringPool(StringPool&& move) noexcept
    : PagedPool(move)
{

}

StringPool::~StringPool()
{
}

StringPool StringPool::split(int seperator, unsigned long flags, const char* str)
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

    StringPool bb;
    if(scnt>0) {
        p = s = str;
        while (*p) {
            if (*p == seperator) {
                if(p>s || (flags & SF_IGNORE_EMPTY)==0)
                    bb.insert(s, p-s);
                s = p+1;
            }
            p++;
        }
        if(p>s || (flags & SF_IGNORE_EMPTY)==0)
            bb.insert(s, p-s);
    }
    return bb;
}

size_t StringPool::byte_length()
{
    return info().bytes;
}

size_t StringPool::count()
{
    size_t n=0;
    Page *p = _head;
    while(p) {
        n += p->_indexSize;
        p = p->_next;
    }
    return n;
}

#if !defined(NDEBUG)
void StringPool::sanity_check() const
{

}
#else
#define __BinBag::sanity_check(bb)
#endif


StringPool::index_type StringPool::insert_distinct(const char* str, int (*compar)(const char*,const char*))
{
    long idx = find(str, compar);
    return (idx < 0)
        ? insert(str)
        : idx;
}

StringPool::index_type StringPool::insert_distinct(const char* str, size_t n, int (*compar)(const char*,const char*, size_t n))
{
    long idx = find(str, n, compar);
    return (idx < 0)
           ? insert(str)
           : idx;
}

StringPool::index_type StringPool::insert(const char* str)
{
    // we have to use strlen() here because PagedPool requires lengths
    return StringPool::insert(str, ::strlen(str));
}

StringPool::index_type StringPool::insert(const char *str, size_type length)
{
    auto sp = allocObject(length+1);
    if(sp.data != nullptr)
        memcpy(sp.data, str, length);
    sp.data[length]=0;   // null terminate
    return sp.ordinal;
}

const char* StringPool::get(index_type idx)
{
    Page *p = _head;
    if(idx<0) return nullptr;
    while(p) {
        if(idx < p->_indexSize)
            return (const char*)p->fromIndex(idx);  // index is within this page
        else
            idx -= p->_indexSize;                   // index is in a subsequent page
        p = p->_next;
    }
    return nullptr; // out of bounds
}

StringPool::size_type StringPool::strlen(StringPool::index_type idx)
{
    Page *p = _head;
    if(idx<0) return -1;
    while(p) {
        if(idx < p->_indexSize) {
            // index is within this page
            auto s = p->fromIndex(idx);
            idx++;  // now we need to count bytes to following string
            return (idx < p->_indexSize)
                ? p->fromIndex(idx) - s             // string is not the last string in the index
                : p->_insertp - (s - p->_data);     // was last string in index, so use insertp as end of string
        } else
            idx -= p->_indexSize;                   // index is in a subsequent page
        p = p->_next;
    }
    return -1; // out of bounds
}

StringPool::size_type StringPool::find(const char* match, int (*compar)(const char*,const char*)) const
{
    Page *p = _head;
    if(match==nullptr || compar==nullptr)
        return -1;
    long idx=0;
    while(p) {
        auto _elements = (const char**)p->getIndex();
        for(int i=0, _i = p->_indexSize; i<_i; i++)
            if(compar(match, _elements[i]) ==0)
                return idx + i;
        idx += p->_indexSize;
        p = p->_next;
    }
    return -1;
}

StringPool::size_type StringPool::find(const char* match, size_type n, int (*compar)(const char*,const char*, size_t n)) const
{
    Page *p = _head;
    if(match==nullptr || compar==nullptr)
        return -1;
    long idx=0;
    while(p) {
        auto _elements = (const char**)p->fromIndex(0);
        for(int i=0, _i = p->_indexSize; i<_i; i++)
            if(compar(match, _elements[i], n) ==0)
                return idx + i;
        idx += p->_indexSize;
        p = p->_next;
    }
    return -1;
}

#if 0
void StringPool::inplace_reverse(binbag *bb)
{
    // reverse string table in place
    char **begin = (char**)StringPool::begin_iterator(bb),
            **end = (char**)StringPool::end_iterator(bb)-1; // actually 'last'
    while(begin < end) {
        // swap entries
        char* w = *begin;
        *begin++ = *end;
        *end-- = w;
    }
}

binbag* StringPool::distinct(binbag *bb)
{
    return StringPool::sort_distinct(bb, StringPool::element_sort_asc);
}

binbag* StringPool::sort_distinct(binbag *bb, int (*compar)(const void*,const void*))
{
#if 0
    if(StringPool::count(bb)<=1)
        return; // duh, arrays of 1 are sorted and distinct

    // sort list so non-unique strings are always next to each other
    StringPool::sort(bb, compar);

    // now shrink the elements by eliminating those that are duplicates
    // we are going to build the unique set in-place. So we have two interators. They both start at the 0th element.
    // The 'u' iterator advances only as we encounter a unique element, The 'nu' iterator advanced over each element
    // unique or not. When we encounter a new unique string at 'nu' we copy it to the position at 'u', then advance 'u'.
    char **u = (char**)StringPool::begin_iterator(bb);     // i points to the insertion point of the unique set
    char **e = (char**)StringPool::end_iterator(bb);       // last element
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
        size_t new_count = (const char**)u - StringPool::begin_iterator(bb);
        const char** new_begin = StringPool::end_iterator(bb) - new_count;
        memmove(new_begin, StringPool::begin_iterator(bb), new_count);
        bb->elements = new_begin;

    }
#else
    assert(false);
#endif
    return NULL;
}

binbag* StringPool::sort(binbag *bb, int (*compar)(const void*,const void*))
{
    check_fencepost(bb);
    size_t N = StringPool::count(bb);

    // create a new binbag with just enough capacity to hold a copy of bb (but sorted)
    size_t capacity = StringPool::byte_length(bb) + N*sizeof(char*);
    binbag* sorted = StringPool::create(capacity, bb->growth_rate);

    // copy the string table index (this index will still reference strings from old table)
    sorted->elements = StringPool::end_iterator(sorted) - N;
    assert((void*)sorted->elements > sorted->begin && (void*)sorted->elements < sorted->end );
    write_fencepost(sorted);    // fencepost in new sorted binbag should still be intact
    memcpy((void*)StringPool::begin_iterator(sorted), (const void*)StringPool::begin_iterator(bb), sizeof(char*) * N);

    // sort the index we just copied. this doesnt affect the source binbag even though we are still
    // referencing strings from it.
    qsort((char*)StringPool::begin_iterator(sorted), N, sizeof(char*), compar);
    check_fencepost(bb);

    // now we copy the strings over. since our index is sorted so will the strings copied over. We update our sorted
    // binbag index with new string locations as we copy
    const char **begin = StringPool::begin_iterator(sorted), **end = StringPool::end_iterator(sorted);
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
#endif

int StringPool::element_sort_desc (const void * _lhs, const void * _rhs)
{
    // we get pointers to the elements, so since our elements are pointers then we must
    // dereference twice to get the resolver structure
    const char* lhs = *(const char**)_lhs;
    const char* rhs = *(const char**)_rhs;
    return strcmp(lhs, rhs);
}

int StringPool::element_sort_asc (const void * _lhs, const void * _rhs)
{
    // we get pointers to the elements, so since our elements are pointers then we must
    // dereference twice to get the resolver structure
    const char* lhs = *(const char**)_lhs;
    const char* rhs = *(const char**)_rhs;
    return strcmp(rhs, lhs);    // swapped order
}

//const unsigned char* StringPool::binary_get(binbag* bb, size_t idx, size_t* len_out);

