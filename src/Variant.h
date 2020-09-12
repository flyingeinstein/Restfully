//
// Created by colin on 11/9/2018.
//

#pragma once

#include <string>
#include <cstring>
#include <typeinfo>

#include "StringPool.h"
#include "Token.h"
#include "Name.h"
#include "Type.h"
#include "Exception.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace Rest {

    // shared index of text strings
    // assigned a unique integer ID to each word stored
    extern StringPool literals_index;


    // todo: since Type is protected, it should just be contained
    class Variant : public Type {
    protected:
        //unsigned short _type;            // indicates type used in the union
        union {
            long l;
            unsigned long ul;
            double d;
            bool b;
            char* s;
            void* obj;
        };

    public:

        Variant() : ul(0) {}
        Variant(const Variant& copy) : Type(copy), ul(copy.ul) {
            if(isString())
                s = strdup(copy.s);
            else if(isReal())
                d = copy.d;
            else if(isObject())
                obj = copy.obj;
            else
                ul = copy.ul;
        }

        Variant(Variant&& move) : Type(move), ul(move.ul) {
            if(isString()) {
                s = move.s;
                move.s = nullptr;
            } else if(isReal())
                d = move.d;
            else if(isObject())
                obj = move.obj;
            else
                ul = move.ul;
            move.l = 0;
        }

        //Variant(const Type& vartype) : _type(vartype), ul(0) {}

        Variant(long _l) : Type(ARG_MASK_INTEGER), l(_l) {}
        Variant(unsigned long _ul) : Type(ARG_MASK_UINTEGER), ul(_ul) {}
        Variant(double _d) : Type(ARG_MASK_NUMBER), d(_d) {}
        Variant(bool _b) : Type(ARG_MASK_BOOLEAN), b(_b) {}
        Variant(const char* _s) : Type(ARG_MASK_STRING), s(strdup(_s)) {}

        Variant(const Token& _t) : l(0) {
            switch(_t.id) {
                case TID_STRING:
                case TID_IDENTIFIER:
                    setType(ARG_MASK_STRING);
                    s = strdup(_t.s);
                    break;
                case TID_INTEGER:
                    setType(ARG_MASK_INTEGER);
                    l = _t.i;
                    break;
                case TID_FLOAT:
                    setType(ARG_MASK_REAL);
                    d = _t.d;
                    break;
                case TID_BOOL:
                    setType(ARG_MASK_BOOLEAN);
                    b = _t.i > 0;
                    break;
            }
        }

        virtual ~Variant() {
            if(s && isString())
                ::free(s);
        }

        template<class T> static Variant of(T& obj) {
            return Variant(ARG_MASK_OBJECT, &obj);
        }

        template<class T> static Variant of(const T& obj) {
            return Variant(ARG_MASK_CONST_OBJECT, &obj);
        }

        Variant& operator=(const Variant& copy) {
            Type::operator=(copy);
            if(isString())
                s = copy.s;
            else if(isReal())
                d = copy.d;
            else if(isObject())
                obj = copy.obj;
            else
                ul = copy.ul;
            return *this;
        }

        inline bool empty() const { return type_mask==0; }

        bool operator==(const Variant& rhs) const {
            return Type::operator==(rhs) && (
                    (isString() && strcmp(s, rhs.s) == 0) ||
                    (isUnsigned() && ul == rhs.ul) ||
                    (isInteger() && l == rhs.l) ||
                    (isReal() && d == rhs.d) ||
                    (isBoolean() && b == rhs.b) ||
                    (isObject() && obj == rhs.obj)
             );
        }

        bool operator==(const char* text) const {
            return isString() && (strcmp(s, text) == 0);
        }

        inline bool operator==(int n) const { return operator==( (long)n ); }

        inline bool operator==(unsigned int n) const { return operator==( (unsigned long)n ); }

        bool operator==(long n) const {
            return (isInteger() && (l == n)) || (isUnsigned() && (n>0) && ((long)ul == n));
        }

        bool operator==(unsigned long n) const {
            return (isUnsigned() && (ul == n)) || (isInteger() && ((unsigned long)l == n));
        }

        bool operator==(double n) const {
            return ((isReal() && (d == n))) || (isInteger() && ((double)l == n));
        }

        inline bool operator!=(const Variant& rhs) const { return !operator==(rhs); }

        int isOneOf(std::initializer_list<const char*> enum_values, bool case_insensitive=true) {
            typeof(strcmp) *cmpfunc = case_insensitive
                                      ? &strcasecmp
                                      : &strcmp;
            if(isString())
                return -2;
            int j=0;
            for(std::initializer_list<const char*>::const_iterator x=enum_values.begin(), _x=enum_values.end(); x!=_x; x++,j++) {
                if (cmpfunc(s, *x) == 0)
                    return j;
            }
            return -1;
        }


/*        inline bool isInteger() const { return (_type & ARG_MASK_INTEGER) == ARG_MASK_INTEGER; }
        inline bool isIntegerBetween(int min, int max) const { return (_type & ARG_MASK_INTEGER) == ARG_MASK_INTEGER && min <= l && max >= l; }
        inline bool isSignedInteger() const { return (_type & ARG_MASK_UINTEGER) == ARG_MASK_INTEGER; }
        inline bool isUnsignedInteger() const { return (_type & ARG_MASK_UINTEGER) == ARG_MASK_UINTEGER; }
        inline bool isNumber() const { return (_type & ARG_MASK_NUMBER) > 0; }
        inline bool isBoolean() const { return (_type & ARG_MASK_BOOLEAN) == ARG_MASK_BOOLEAN; }
        inline bool isString() const { return (_type & ARG_MASK_STRING) == ARG_MASK_STRING; }
        inline bool isObject() const { return _type == ARG_MASK_OBJECT || _type == ARG_MASK_CONST_OBJECT; }
*/
        // only supported in C++11
        inline explicit operator int() const { assert_cast(isInteger()); return (int)l; }
        inline explicit operator unsigned int() const { assert_cast(isInteger()); return (int)ul; }
        inline explicit operator long() const { assert_cast(isInteger()); return l; }
        inline explicit operator unsigned long() const { assert_cast(isInteger()); return ul; }
        inline explicit operator double() const { assert_cast(isInteger() || isReal()); return isReal() ? d : (double)l; }
        inline explicit operator bool() const { return isBoolean() ? b : (ul > 0); }
        inline explicit operator const char*() const { assert_cast(isString()); return s; }

        template<class T>
        inline const T& get() const {
            assert_cast(isObject());
            return *(T*)obj;
        }

        template<class T>
        inline T& get() {
            assert_cast(isObject());
            assert_cast(!isConstObject());
            return *(T*)obj;
        }


#if defined(ARDUINO)
        inline explicit operator String() const { assert(isString()); return String(s); }

        String toString() const {
          if(isString())
            return String(s);
          else if(isInteger())
            return String(l, 10);
          else if(isUnsigned())
            return String(ul, 10);
          else if(isReal())
            return String(d,5);
          else if(isBoolean())
            return String( b ? "true" : "false" );
          else
            return String();
        }
#endif


#if 0   // VariantValues should never change, so use constructor only (and assignment op if needed)
        void set(long _l) { type = ARG_MASK_INTEGER; l = _l; }
        void set(unsigned long _ul) { type = ARG_MASK_UINTEGER; l = _ul; }
        void set(bool _b) { type = ARG_MASK_BOOLEAN; b = _b; }
        void set(const char* _s) { type = ARG_MASK_STRING; s = strdup(_s); }
#endif

    protected:
        Variant(Type type, void* p) : Type(type), obj(p) {}
        Variant(Type type, const void* p) : Type(type), obj((void*)p) {}
    };

}; // ns: Rest
