//
// Created by guru on 9/11/20.
//

#ifndef RESTFULLY_PARAMETER_H
#define RESTFULLY_PARAMETER_H

#include "Variant.h"

namespace Rest {

    class Parameter
    {
    public:
        Name name;
        Type type;

        Parameter() {}
        Parameter(const char* _name, Type _type) : name(_name), type(_type) {}

        inline static Parameter String(const char* name) { return Parameter(name, ARG_MASK_STRING); }
        inline static Parameter Integer(const char* name) { return Parameter(name, ARG_MASK_INTEGER); }
        inline static Parameter Unsigned(const char* name) { return Parameter(name, ARG_MASK_UNSIGNED); }
        inline static Parameter Real(const char* name) { return Parameter(name, ARG_MASK_REAL); }
        inline static Parameter Boolean(const char* name) { return Parameter(name, ARG_MASK_BOOLEAN); }
    };


} //ns:Rest

#endif //RESTFULLY_PARAMETER_H
