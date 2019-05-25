//
// Created by colin on 11/9/2018.
//

#pragma once

#include <cstdint>

namespace Rest {

    class Literal {
    public:
        using size_type = long;

        inline Literal() : id(0), isNumeric(false) {}
        inline Literal(size_type _id, bool _isNumeric = false) : id(_id), isNumeric(_isNumeric) {}

        inline Literal(const Literal& copy) : id(copy.id), isNumeric(copy.isNumeric) {}

        // if this argument is matched, the value is added to the request object under this field name
        // this id usually indicates an index into an array of text terms (binbag)
        size_type id;

        // true if the id should be take as a numeric value and not a string index ID
        bool isNumeric;

        inline bool isValid() { return isNumeric || (id >= 0); }
    };

} // ns: Rest