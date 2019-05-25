//
// Created by colin on 11/9/2018.
//

#pragma once

#include <assert.h>
#include <algorithm>


namespace Rest {

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
        PagedPool(size_t page_size=64, bool use_index = false);
        PagedPool(const PagedPool& copy);
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
                info.capacity += p->capacity();
                info.bytes += p->_insertp;
                info.available += p->available();
                p = p->_next;
            }
            return info;
        }

        size_t capacity() const {
            size_t n=0;
            Page *p = _head;
            while(p) {
                n += p->capacity();
                p = p->_next;
            }
            return n;
        }

        size_t available() const {
            size_t n=0;
            Page *p = _head;
            while(p) {
                n += p->capacity();
                p = p->_next;
            }
            return n;
        }

        size_t bytes() const {
            size_t n=0;
            Page *p = _head;
            while(p) {
                n += p->_insertp;
                p = p->_next;
            }
            return n;
        }

    protected:
        class Page;

        class AllocatedObject {
            public:
                Page* page;
                unsigned char* data;
                size_t ordinal;

                inline AllocatedObject() : page(nullptr), data(nullptr), ordinal(0) {}
        };

        class Page {
        public:
            explicit Page(size_t _size);
            Page(const Page& copy);
            Page& operator=(const Page& copy) = delete;

            inline size_t capacity() const { return _capacity - _indexSize*sizeof(unsigned char*); }
            inline size_t available() const { return _capacity - _insertp - _indexSize*sizeof(unsigned char*); }

            unsigned char* get(size_t sz) {
                if(sz==0 || (_insertp+sz > _capacity))
                    return nullptr;
                unsigned char* out =  _data + _insertp;
                _insertp += sz;
                return out;
            }

            const unsigned char** getIndex() const {
                assert(_data != nullptr);
                return (const unsigned char**)((_data + _capacity) - _indexSize*sizeof(unsigned char*));
            }

            const unsigned char* fromIndex(size_t ord) const {
                auto _index = getIndex();
                return (ord < _indexSize)
                    ? _index[ord]
                    : nullptr;
            }

            size_t index(const unsigned char* element) {
                assert(_data != nullptr);
                _indexSize++;
                auto _index = (const unsigned char**)((_data + _capacity) - _indexSize*sizeof(unsigned char*));
                *_index = element;
                return 0;
            }

            unsigned char* _data;
            size_t _capacity;    // size of buffer
            size_t _insertp;     // current insert position
            size_t _indexSize;   // number of elements in the index (index grows from tail inward)
            Page* _next;        // next page (unless we are the end)
        };

        AllocatedObject allocObject(size_t sz) {
            AllocatedObject obj;
            Page *p = _head, *lp = nullptr;
            if(sz==0) return obj;
            while(p) {
                obj.data = p->get(sz);
                if(obj.data != nullptr) {
                    obj.page = p;
                    // add to index
                    if(_use_index)
                        obj.ordinal = p->index(obj.data);
                    return obj;
                }

                lp = p;
                p = p->_next;
            }

            // no existing pages, must add a new one
            p = new Page( std::max(sz, _page_size) );
            if(lp)
                lp->_next = p;
            else
                _head = p;  // first page
            obj.data =  p->get(sz);
            obj.page = p;
            if(_use_index)
                obj.ordinal = p->index(obj.data);
            return obj;
        }

        unsigned char* alloc(size_t sz) {
            return allocObject(sz).data;
        }

    protected:
        size_t _page_size;
        bool _use_index;
        Page *_head;        // first page in linked list of pages
    };

}

