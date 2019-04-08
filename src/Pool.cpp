//
// Created by Colin MacKenzie on 2019-04-06.
//
#include "Pool.h"

namespace Rest {

    PagedPool::PagedPool(size_t page_size)
        : _page_size(page_size), _head(nullptr )
    {
    }

    PagedPool::PagedPool(PagedPool&& move) noexcept
        : _head(move._head), _page_size(move._page_size)
    {
        move._head = nullptr;
    }

    PagedPool& PagedPool::operator=(PagedPool&& move) noexcept {
        _page_size = move._page_size;
        _head = move._head;
        move._head = nullptr;
        return *this;
    }

    PagedPool::Page::Page(size_t _size)
        : _data( (unsigned char*)calloc(1, _size) ), _capacity(_size), _insertp(0), _next(nullptr)
    {
    }


}