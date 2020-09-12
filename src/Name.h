//
// Created by guru on 9/11/20.
//

#ifndef RESTFULLY_NAME_H
#define RESTFULLY_NAME_H

#include "Variant.h"

namespace Rest {

    class Name
    {
    protected:
        // if this argument is matched, the value is added to the request object under this field name
        long _index;
        mutable const char* _value;

    public:
        inline Name() : _index(-1), _value(nullptr) {}
        Name(const char* value, bool addToIndex = false)
                : _index(-1), _value(value) {
            if(addToIndex)
                _index = literals_index.insert_distinct(value, strcasecmp);
        }
        Name(long index) : _index(index), _value(nullptr) {}

        inline operator bool() const { return _index>=0 || _value; }

        inline const char* value() const {
            return _value
                   ? _value
                   : (_index >= 0)
                     ? _value = literals_index[_index]
                     : nullptr;
        }

        long index() const {
            return _index;
        }
    };


} //ns:Rest

#endif //RESTFULLY_NAME_H
