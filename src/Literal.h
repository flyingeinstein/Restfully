//
// Created by colin on 11/9/2018.
//

#pragma once

namespace Rest {

    class Literal {
    public:
        // if this argument is matched, the value is added to the request object under this field name
        // this id usually indicates an index into an array of text terms (binbag)
        ssize_t id;

        // true if the id should be take as a numeric value and not a string index ID
        bool isNumeric;

        inline bool isValid() { return isNumeric || (id >= 0); }
    };

} // ns: Rest