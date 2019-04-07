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
        explicit PagedPool(size_t initial_capacity=128, size_t page_size=64);
        PagedPool(PagedPool&& move) noexcept;

        PagedPool& operator=(PagedPool&& move) noexcept;


        template<class T, typename ...Args>
        T* make(Args ... args) {
            size_t sz = sizeof(T);
            Page *p = _head, *lp = nullptr;
            while(p) {
                T* test = p->make<T>(args...);
                if(test)
                    return test;
                lp = p;
                p = p->_next;
            }

            // no existing pages, must add a new one
            assert(lp); // wierd if it wasnt valid
            lp->_next = p = new Page( std::max(sz, _page_size) );
            T* test = p->make<T>(args...);
            return test;
        }

        template<class T, typename ...Args>
        T* makeArray(size_t n, Args ... args) {
            size_t sz = sizeof(T)*n;
            Page *p = _head, *lp = nullptr;
            while(p) {
                T* test = p->makeArray<T>(n, args...);
                if(test)
                    return test;
                lp = p;
                p = p->_next;
            }

            // no existing pages, must add a new one
            assert(lp); // wierd if it wasnt valid
            lp->_next = p = new Page( std::max(sz, _page_size) );
            T* test = p->makeArray<T>(n, args...);
            return test;
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

            template<class T, typename ...Args>
            T* make(Args ... args) {
                size_t sz = sizeof(T);
                if(sz==0 || (_insertp+sz > _capacity))
                    return nullptr;
                T* p = new ( _data + _insertp ) T(args...);
                _insertp += sz;
                return p;
            }

            template<class T, typename ...Args>
            T* makeArray(size_t n, Args ... args) {
                size_t sz = sizeof(T)*n;
                if(sz==0 || (_insertp+sz > _capacity))
                    return nullptr;
                T* p = (T*) (_data + _insertp);
                for(size_t i=0; i<n; i++) {
                    new(_data + _insertp) T(args...);
                    _insertp += sizeof(T);
                }
                return p;
            }

            unsigned char* _data;
            size_t _capacity;    // size of buffer
            size_t _insertp;     // current insert position
            Page* _next;        // next page (unless we are the end)
        };

    protected:
        size_t _page_size;
        Page *_head;        // first page in linked list of pages
    };

}

