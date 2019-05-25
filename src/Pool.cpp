//
// Created by Colin MacKenzie on 2019-04-06.
//
#include "Pool.h"

#include <cstring>

namespace Rest {

    PagedPool::PagedPool(size_t page_size, bool use_index)
        : _page_size(page_size), _head(nullptr ), _use_index(use_index)
    {
    }

    PagedPool::PagedPool(const PagedPool& copy)
            : _page_size(copy._page_size), _head(nullptr), _use_index(copy._use_index)
    {
        // copy all pages
        Page *srcp = copy._head;
        Page **dstp = &_head;
        while(srcp) {
            *dstp = new Page(*srcp);
            dstp = & (*dstp)->_next;   // set destination to 'ptr to next ptr'
        }
    }

    PagedPool::PagedPool(PagedPool&& move) noexcept
        : _page_size(move._page_size), _head(move._head), _use_index(move._use_index)
    {
        move._head = nullptr;
        move._use_index = false;
    }

    PagedPool& PagedPool::operator=(PagedPool&& move) noexcept {
        _page_size = move._page_size;
        _head = move._head;
        _use_index = move._use_index;
        move._head = nullptr;
        move._use_index = false;
        return *this;
    }

    PagedPool::Page::Page(size_t _size)
        : _data( (unsigned char*)calloc(1, _size) ), _capacity(_size), _insertp(0), _indexSize(0), _next(nullptr)
    {
    }

    PagedPool::Page::Page(const Page& copy)
        : _data(nullptr), _capacity(copy._capacity), _insertp(copy._insertp), _indexSize(copy._indexSize), _next(nullptr)
        {
        _data = (unsigned char*)calloc(1, _capacity);
        memcpy(_data, copy._data, _capacity);
    }


}