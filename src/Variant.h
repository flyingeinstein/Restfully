//
// Created by colin on 11/9/2018.
//

#pragma once

#include <string>
#include <cstring>
#include <cassert>

#include "StringPool.h"
#include "Token.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

// Bitmask values for different types
#define ARG_MASK_INTEGER       ((unsigned short)1)
#define ARG_MASK_REAL          ((unsigned short)2)
#define ARG_MASK_BOOLEAN       ((unsigned short)4)
#define ARG_MASK_STRING        ((unsigned short)8)
#define ARG_MASK_UNSIGNED      ((unsigned short)16)
#define ARG_MASK_UINTEGER      (ARG_MASK_UNSIGNED|ARG_MASK_INTEGER)
#define ARG_MASK_NUMBER        (ARG_MASK_REAL|ARG_MASK_INTEGER)
#define ARG_MASK_ANY           (ARG_MASK_NUMBER|ARG_MASK_BOOLEAN|ARG_MASK_STRING)


namespace Rest {

    // shared index of text strings
    // assigned a unique integer ID to each word stored
    extern StringPool literals_index;

    class Type
    {
    public:
        inline Type() : name_index(-1), type_mask(0) {}
        Type(const char* _name, unsigned short _typemask)
                : name_index(literals_index.insert_distinct(_name, strcasecmp)),
                  type_mask(_typemask) {
        }

        Type(long _name_index, unsigned short _typemask)
                : name_index(_name_index),
                  type_mask(_typemask) {
        }

        inline const char* name() const { return (name_index >= 0) ?  literals_index[name_index] : nullptr; }

        inline unsigned short typemask() const { return type_mask; }
        inline bool supports(unsigned short mask) const { return (mask & type_mask)==mask; }


    protected:
        // if this argument is matched, the value is added to the request object under this field name
        long name_index;

        // collection of ARG_MASK_xxxx bits represent what types are supported for this argument
        unsigned short type_mask;
    };

    class Argument : protected Type {
    public:
        typedef Type Type;

        using Type::name;

        Argument() : type(0), ul(0) {}
        Argument(const Argument& copy) : Type(copy), type(copy.type), ul(copy.ul) {
            if(type == ARG_MASK_STRING)
                s = strdup(copy.s);
            else if((type & ARG_MASK_REAL) >0)
                d = copy.d;
            else
                ul = copy.ul;
        }

        Argument(Argument&& move) : Type(move), type(move.type), ul(move.ul) {
            if(type == ARG_MASK_STRING) {
                s = move.s;
                move.s = nullptr;
            } else if((type & ARG_MASK_REAL) >0)
                d = move.d;
            else
                ul = move.ul;
            move.type = 0;
            move.l = 0;
        }

        Argument(const Type& arg) : Type(arg), type(0), ul(0) {}

        Argument(const Type& arg, long _l) : Type(arg), type(ARG_MASK_INTEGER), l(_l) {}
        Argument(const Type& arg, unsigned long _ul) : Type(arg), type(ARG_MASK_UINTEGER), ul(_ul) {}
        Argument(const Type& arg, double _d) : Type(arg), type(ARG_MASK_NUMBER), d(_d) {}
        Argument(const Type& arg, bool _b) : Type(arg), type(ARG_MASK_BOOLEAN), b(_b) {}
        Argument(const Type& arg, const char* _s) : Type(arg), type(ARG_MASK_STRING), s(strdup(_s)) {}

        Argument(const Type& arg, const Token& _t) : Type(arg), l(0) {
            switch(_t.id) {
                case TID_STRING:
                case TID_IDENTIFIER:
                    type = ARG_MASK_STRING;
                    s = strdup(_t.s);
                    break;
                case TID_INTEGER:
                    type = ARG_MASK_INTEGER;
                    l = _t.i;
                    break;
                case TID_FLOAT:
                    type = ARG_MASK_REAL;
                    d = _t.d;
                    break;
                case TID_BOOL:
                    type = ARG_MASK_BOOLEAN;
                    b = _t.i > 0;
                    break;
            }
        }

        virtual ~Argument() {
            if(s && type == ARG_MASK_STRING)
                ::free(s);
        }

        Argument& operator=(const Argument& copy) {
            Type::operator=(copy);
            type = copy.type;
            if(type == ARG_MASK_STRING)
                s = copy.s;
            else if((type & ARG_MASK_REAL) >0)
                d = copy.d;
            else
                ul = copy.ul;
            return *this;
        }

        bool operator==(const Argument& rhs) const {
            return (type == rhs.type) && (
                    (type==ARG_MASK_STRING && strcmp(s, rhs.s)==0) ||
                    (type==ARG_MASK_INTEGER && l==rhs.l) ||
                    (type==ARG_MASK_UINTEGER && ul==rhs.ul) ||
                    (type==ARG_MASK_NUMBER && d==rhs.d) ||
                    (type==ARG_MASK_BOOLEAN && b==rhs.b)
             );
        }

        bool operator==(const char* text) const {
            return (type == ARG_MASK_STRING) && (strcmp(s, text)==0);
        }

        bool operator==(int n) const {
            return (type & ARG_MASK_INTEGER) && (l==n);
        }

        bool operator==(unsigned int n) const {
            return (type & ARG_MASK_INTEGER) && (l==n);
        }

        bool operator==(long n) const {
            return (type & ARG_MASK_INTEGER) && (l==n);
        }

        bool operator==(unsigned long n) const {
            return (type & ARG_MASK_UINTEGER) && (ul==n);
        }

        bool operator==(double n) const {
            return (((type&ARG_MASK_REAL) && (d==n)) || (isNumber() && ((double)l==n)));
        }

        inline bool operator!=(const Argument& rhs) const { return !operator==(rhs); }

        int isOneOf(std::initializer_list<const char*> enum_values, bool case_insensitive=true) {
            typeof(strcmp) *cmpfunc = case_insensitive
                                      ? &strcasecmp
                                      : &strcmp;
            if((type&ARG_MASK_STRING)!=ARG_MASK_STRING)
                return -2;
            int j=0;
            for(std::initializer_list<const char*>::const_iterator x=enum_values.begin(), _x=enum_values.end(); x!=_x; x++,j++) {
                if (cmpfunc(s, *x) == 0)
                    return j;
            }
            return -1;
        }


        inline bool isInteger() const { return (type&ARG_MASK_INTEGER)==ARG_MASK_INTEGER; }
        inline bool isIntegerBetween(int min, int max) const { return (type&ARG_MASK_INTEGER)==ARG_MASK_INTEGER && min<=l && max>=l; }
        inline bool isSignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_INTEGER; }
        inline bool isUnsignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_UINTEGER; }
        inline bool isNumber() const { return (type&ARG_MASK_NUMBER)>0; }
        inline bool isBoolean() const { return (type&ARG_MASK_BOOLEAN)==ARG_MASK_BOOLEAN; }
        inline bool isString() const { return (type&ARG_MASK_STRING)==ARG_MASK_STRING; }

        // only supported in C++11
        inline explicit operator int() const { assert(type&ARG_MASK_INTEGER); return (int)l; }
        inline explicit operator unsigned int() const { assert(type&ARG_MASK_INTEGER); return (int)ul; }
        inline explicit operator long() const { assert(type&ARG_MASK_INTEGER); return l; }
        inline explicit operator unsigned long() const { assert(type&ARG_MASK_INTEGER); return ul; }
        inline explicit operator double() const { assert(type&ARG_MASK_NUMBER); return (type&ARG_MASK_REAL) ? d : (double)l; }
        inline explicit operator bool() const { return (type == ARG_MASK_BOOLEAN) ? b : (ul>0); }
        inline explicit operator const char*() const { assert(type&ARG_MASK_STRING); return s; }

#if defined(ARDUINO)
        inline explicit operator String() const { assert(type&ARG_MASK_STRING); return String(s); }

        String toString() const {
          if(type & ARG_MASK_STRING)
            return String(s);
          else if((type&ARG_MASK_INTEGER)==ARG_MASK_INTEGER)
            return String(l, 10);
          else if((type&ARG_MASK_UINTEGER)==ARG_MASK_UINTEGER)
            return String(ul, 10);
          else if((type&ARG_MASK_NUMBER)==ARG_MASK_NUMBER)
            return String(d,5);
          else if((type&ARG_MASK_BOOLEAN)==ARG_MASK_BOOLEAN)
            return String( b ? "true":"false" );
          else
            return String();
        }
#endif


#if 0   // ArgumentValues should never change, so use constructor only (and assignment op if needed)
        void set(long _l) { type = ARG_MASK_INTEGER; l = _l; }
        void set(unsigned long _ul) { type = ARG_MASK_UINTEGER; l = _ul; }
        void set(bool _b) { type = ARG_MASK_BOOLEAN; b = _b; }
        void set(const char* _s) { type = ARG_MASK_STRING; s = strdup(_s); }
#endif
        unsigned short type;

        union {
            long l;
            unsigned long ul;
            double d;
            bool b;
            char* s;
        };

    public:
        static const Argument null;
    };

}; // ns: Rest
