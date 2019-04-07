//
// Created by Colin MacKenzie on 2019-04-06.
//
#include <Pool.h>

namespace Rest {

    PagedPool::PagedPool(size_t initial_capacity, size_t page_size)
        : _page_size(page_size), _head( new Page(initial_capacity) )
    {

    }

    PagePool::Page::Page(size_t _size)
        : data( calloc(1, _size) ), insertp(0), next(nullptr)
    {

    }


}