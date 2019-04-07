//
// Created by Colin MacKenzie on 2019-04-06.
//
#include "Pool.h"

namespace Rest {

    PagedPool::PagedPool(size_t initial_capacity, size_t page_size)
        : _page_size(page_size), _head( new Page(initial_capacity) )
    {
    }

    PagedPool::PagedPool(PagedPool&& move) noexcept
        : _head(move._head), _page_size(move._page_size)
    {
        move._head = new Page(_head->_capacity);  // todo: even better if we didnt allocate a page until we need it
    }

    PagedPool& PagedPool::operator=(PagedPool&& move) noexcept {
        _page_size = move._page_size;
        _head = move._head;
        move._head = new Page(_head->_capacity);
        return *this;
    }

    PagedPool::Page::Page(size_t _size)
        : _data( (unsigned char*)calloc(1, _size) ), _capacity(_size), _insertp(0), _next(nullptr)
    {
    }


}