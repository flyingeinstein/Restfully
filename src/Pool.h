//
// Created by colin on 11/9/2018.
//

#pragma once

#include "binbag.h"

#include <assert.h>
#include <algorithm>


namespace Rest {
    // shared index of text strings
    // assigned a unique integer ID to each word stored
    extern binbag *literals_index;

    /// \brief dynamic memory allocator using memory pages
    /// This class tries to alleviate issues of memory fragmentation on small devices. By allocating pages of memory for
    /// small objects it can hopefully lower fragmentation by not leaving holes of free memory after Endpoints configration
    /// is done. The catch is these objects must be long lived because objects from the pages are never freed or deleted.
    /// Future:
    ///    We could possibly have a lifetime mode on object create, only objects of the same lifetime setting could
    ///    reside together.
    class PagedPool
    {
    public:
        class Info {
        public:
            size_t count;
            size_t bytes;
            size_t available;
            size_t capacity;

            Info() : count(0), bytes(0), available(0), capacity(0) {}
        };

    public:
        explicit PagedPool(size_t page_size=64);
        PagedPool(PagedPool&& move) noexcept;

        PagedPool& operator=(PagedPool&& move) noexcept;


        template<class T, typename ...Args>
        T* make(Args ... args) {
            size_t sz = sizeof(T);
            unsigned char* bytes = alloc(sz);
            return bytes
                ? new (bytes) T(args...)
                : nullptr;
        }

        template<class T, typename ...Args>
        T* makeArray(size_t n, Args ... args) {
            size_t sz = sizeof(T)*n;
            T* first = (T*)alloc(sz);
            if(first) {
                T *p = first;
                for (size_t i = 0; i < n; i++)
                    new(p++) T(args...);
            }
            return first;
        }

        Info info() const {
            Info info;
            Page *p = _head;
            while(p) {
                info.count++;
                info.capacity += p->_capacity;
                info.bytes += p->_insertp;
                info.available += p->_capacity - p->_insertp;
                p = p->_next;
            }
            return info;
        }

    protected:
        class Page {
        public:
            explicit Page(size_t _size);
            Page(const Page& copy) = delete;
            Page& operator=(const Page& copy) = delete;

            unsigned char* get(size_t sz) {
                if(sz==0 || (_insertp+sz > _capacity))
                    return nullptr;
                unsigned char* out =  _data + _insertp;
                _insertp += sz;
                return out;
            }

            unsigned char* _data;
            size_t _capacity;    // size of buffer
            size_t _insertp;     // current insert position
            Page* _next;        // next page (unless we are the end)
        };

        unsigned char* alloc(size_t sz) {
            Page *p = _head, *lp = nullptr;
            unsigned char* out;
            if(sz==0) return nullptr;
            while(p) {
                out = p->get(sz);
                if(out != nullptr)
                    return out;

                lp = p;
                p = p->_next;
            }

            // no existing pages, must add a new one
            p = new Page( std::max(sz, _page_size) );
            if(lp)
                lp->_next = p;
            else
                _head = p;  // first page
            return p->get(sz);
        }

    protected:
        size_t _page_size;
        Page *_head;        // first page in linked list of pages
    };

}

