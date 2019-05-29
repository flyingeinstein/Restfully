/// \file
/// \brief Efficiently stores a growing array of character or binary strings (AKA string table)
#ifndef ANALYTICSAPI_BINBAG_H
#define ANALYTICSAPI_BINBAG_H

#include "Pool.h"

#include <stdint.h>
#include <cstring>

// split string flags
#define SF_NONE             0
#define SF_IGNORE_EMPTY     1


class StringPool : private Rest::PagedPool {
protected:
    Rest::PagedPool index;

#if !defined(NDEBUG)
    void sanity_check() const;
#endif

public:
    using size_type = size_t;
    using index_type = size_t;

    explicit StringPool(size_t page_size=512);
    StringPool(const StringPool& copy) noexcept;
    StringPool(StringPool&& move) noexcept;

    ~StringPool();

    static StringPool split(int seperator, unsigned long flags, const char* str);

////binbag* create_from_array(const char** arr, size_t count, double fill_space, double growth_rate);

    //size_t resize(size_t capacity);

/// \brief Returns the bytes used for all the strings combined
    size_t byte_length();

    /// \brief return the number of items in the binbag
    size_t count();

/// \brief Returns the character capacity of the binbag including both used and unused space
    using PagedPool::capacity;
    using PagedPool::bytes;

    /// \brief Returns the character capacity remaining
    using PagedPool::available;

    /// \brief insert a new string
    /// \returns the ordinal position of the new string
    index_type insert(const char *str);

    /// \brief insert a new string up to the given length
    /// \returns the ordinal position of the new string
    index_type insert(const char *str, size_type length);

    /// \brief Insert a string if not exists
    /// This function is the same as insert() but first it checks to see if it already exists in the binbag. If so,
    /// then the existing element is returned.
    index_type insert_distinct(const char* str, int (*compar)(const char*,const char*));

    /// \brief Insert a string up to a given length if it doesnt already exist
    /// This function is the same as insert() but first it checks to see if it already exists in the binbag. If so,
    /// then the existing element is returned.
    index_type insert_distinct(const char* str, size_type n, int (*compar)(const char*,const char*, size_t n));

    //const unsigned char* binary_insert(const unsigned char* str, size_t len);

    /// \brief returns the string at the given ordinal position
    const char* operator[](index_type idx) const;

    /// \brief get the length of the string at the given index
    size_type strlen(StringPool::index_type idx);

    /// \brief Find the ordinal index of the given string using the supplied comparison function (works with strcmp, strcasecmp, etc)
    /// \returns the ordinal position of the matched string, or -1 if not found
    index_type find(const char* match, int (*compar)(const char*,const char*)) const;

    /// \brief Find the ordinal index of the given string using the supplied comparison function with length (works with strcmp, strcasecmp, etc)
    /// \returns the ordinal position of the matched string, or -1 if not found
    index_type find(const char* match, size_type n, int (*compar)(const char*,const char*, size_t n)) const;

    /// \brief Find the ordinal index of the given string
    inline index_type find(const char* match) const { return find(match, strcmp); }

    /// \brief Find the ordinal index of the given string using a case insensitive comparison
    inline index_type find_nocase(const char* match) const { return find(match, strcasecmp); }

    /// \brief Find the ordinal index of the given string with length
    inline index_type find(const char* match, size_type n) const { return find(match, n, strncmp); }

    /// \brief Find the ordinal index of the given string using a case insensitive comparison with length
    inline index_type find_nocase(const char* match, size_type n) const { return find(match, n, strncasecmp); }

    /// \brief Sort the values and reduce memory usage by aliasing duplicates to the same string memory.
    /// This method does not eliminate duplicates, it keeps duplicates in the string table but after reduction these duplicates
    /// will point to the same C string in memory. This is useful for having semi-static lookup tables where duplicates are
    /// desired but you also want to save memory.
    // binbag* flyweight_reduce(int (*compar)(const void*,const void*)); - not yet implemented

    void debug_print();

    class const_iterator {
    private:
        PagedPool::Page* _page;
        int _idx;

    protected:
        inline explicit const_iterator(PagedPool::Page* page) : _page(page), _idx(0) {}

    public:
        inline const_iterator() : _page(nullptr), _idx(0) {}
        inline const_iterator(const const_iterator& copy) = default;
        inline ~const_iterator() {}

        const_iterator& operator=(const const_iterator& copy) = default;

        inline bool operator==(const const_iterator& rhs) const { return _page == rhs._page && _idx==rhs._idx; }
        inline bool operator!=(const const_iterator& rhs) const { return _page != rhs._page || _idx!=rhs._idx; }

        const_iterator& operator++() {
            _idx++;
            if(_idx >= indexElementCount(_page)) {
                // advance page (if we reach the end then _page becomes nullptr)
                _page = _page->_next;
                _idx = 0;
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator itr(*this);
            operator++();
            return itr;
        }

        const char* operator*() const {
            return (_page!= nullptr && _idx < indexElementCount(_page))
                ? indexElement(_page, _idx)
                : nullptr;
        }

        friend StringPool;
    };

    inline const_iterator begin() const { return const_iterator(_head); }
    inline const_iterator end() const { return const_iterator(); }


    /// todo: Sort can be done by creating an N-array and store all string pointers, then call stdlib::sort, then write all strings to a new binbag. Same with inplace_reverse and flyweight_reduce....just make it an operator or lambda function
#if 0
    // void inplace_reverse();

    /// \brief Reduce the binbag to only distinct string values.
    /// Values will also be sorted using element_sort_asc.
    StringPool distinct();

    /// \brief Reduce the binbag to only distinct string values using the given comparison function.
    /// Values will also be sorted. You can use one of the existing sort routines for standard character sorting, element_sort_asc or element_sort_desc.
    StringPool distinct(int (*compar)(const void*,const void*));

    /// \brief Sort the binbag.
    /// You can use one of the existing sort routines for standard character sorting, element_sort_asc or element_sort_desc.
    binbag* sort(int (*compar)(const void*,const void*));
#endif

    /// \brief Compare function used to sort in ascending order
    static int element_sort_asc (const void * _lhs, const void * _rhs);

    /// \brief Compare function used to sort in descending order
    static int element_sort_desc (const void * _lhs, const void * _rhs);

protected:
    inline static size_t indexElementCount(const Page* pg) { return pg->_insertp / sizeof(char*); }
    inline static const char* indexElement(const Page* pg, size_t n) { return ((const char**)pg->_data)[n]; }
};

/// \brief This is the function that prints a string to the default console.
/// This function prints using fputs(s, stdout) but it is given weak linking so you can override this function if you
/// want to redirect debug print messages to another destination.
int default_binbag_print(const char* str) __attribute__((weak));

#endif //ANALYTICSAPI_BINBAG_H
