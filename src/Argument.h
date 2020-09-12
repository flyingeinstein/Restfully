//
// Created by guru on 9/11/20.
//

#ifndef RESTFULLY_ARGUMENT_H
#define RESTFULLY_ARGUMENT_H

#include "Variant.h"

namespace Rest {

    class Argument : public Variant
    {
    public:
        Name name;

    public:
        Argument() {};

        Argument(const Name& _name, const Variant& var)
                : name(_name), Variant(var)
        {}

        Argument(Name&& _name, Variant&& var)
            : name(std::move(_name)), Variant(std::move(var))
        {}

        template<class T> static Argument from(Name _name, T& obj) {
            return Argument(_name, Variant::of(obj));
        }

        template<class T> static Argument from(Name _name, const T& obj) {
            return Argument(_name, Variant::of(obj));
        }

    #if 0
        Variant(const Type& arg, long _l) : Type(arg), _type(ARG_MASK_INTEGER), l(_l) {}
            Variant(const Type& arg, unsigned long _ul) : Type(arg), _type(ARG_MASK_UINTEGER), ul(_ul) {}
            Variant(const Type& arg, double _d) : Type(arg), _type(ARG_MASK_NUMBER), d(_d) {}
            Variant(const Type& arg, bool _b) : Type(arg), _type(ARG_MASK_BOOLEAN), b(_b) {}
            Variant(const Type& arg, const char* _s) : Type(arg), _type(ARG_MASK_STRING), s(strdup(_s)) {}
    #endif
    };

} // ns:Rest

#endif //RESTFULLY_ARGUMENT_H
