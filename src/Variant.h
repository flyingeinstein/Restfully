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
#include "Exception.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

// Bitmask values for different types
// todo:  convert this to an enum, combine with Token type, and add typeid info for Object types
#define ARG_MASK_INTEGER       ((unsigned short)1)
#define ARG_MASK_REAL          ((unsigned short)2)
#define ARG_MASK_BOOLEAN       ((unsigned short)4)
#define ARG_MASK_STRING        ((unsigned short)8)
#define ARG_MASK_UNSIGNED      ((unsigned short)16)
#define ARG_MASK_OBJECT        ((unsigned short)64)
#define ARG_MASK_CONST_OBJECT  ((unsigned short)128)
#define ARG_MASK_UINTEGER      (ARG_MASK_UNSIGNED|ARG_MASK_INTEGER)
#define ARG_MASK_NUMBER        (ARG_MASK_REAL|ARG_MASK_INTEGER)
#define ARG_MASK_ANY           (ARG_MASK_NUMBER|ARG_MASK_BOOLEAN|ARG_MASK_STRING)


namespace Rest {

    // shared index of text strings
    // assigned a unique integer ID to each word stored
    extern StringPool literals_index;

    class Type
    {
    protected:
        // collection of ARG_MASK_xxxx bits represent what types are supported for this argument
        unsigned short type_mask;

        inline void setType(unsigned short m) { type_mask = m; }

    public:
        inline Type() : type_mask(0) {}
        Type(unsigned short _typemask) : type_mask(_typemask) {}

        inline unsigned short typemask() const { return type_mask; }
        inline bool supports(unsigned short mask) const { return (mask & type_mask)==mask; }

        inline bool operator==(unsigned short _type) const { return _type == type_mask; }
        inline bool operator==(const Type& rhs) const { return type_mask == rhs.type_mask; }

        inline bool isVoid() const { return type_mask == 0; }
        inline bool isInteger() const { return (type_mask & ARG_MASK_INTEGER); }
        inline bool isSigned() const { return (type_mask & ARG_MASK_INTEGER) && (type_mask & ARG_MASK_UNSIGNED)==0; }
        inline bool isUnsigned() const { return (type_mask & ARG_MASK_INTEGER) && (type_mask & ARG_MASK_UNSIGNED); }
        inline bool isReal() const { return (type_mask & ARG_MASK_REAL); }
        inline bool isBoolean() const { return (type_mask & ARG_MASK_BOOLEAN); }
        inline bool isString() const { return (type_mask & ARG_MASK_STRING); }
        inline bool isObject() const { return (type_mask & (ARG_MASK_OBJECT | ARG_MASK_CONST_OBJECT)); }
        inline bool isConstObject() const { return (type_mask & ARG_MASK_CONST_OBJECT); }
    };

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

        bool operator==(int n) const {
            return isInteger() && (l == n);
        }

        bool operator==(unsigned int n) const {
            return isInteger() && (l == n);
        }

        bool operator==(long n) const {
            return isInteger() && (l == n);
        }

        bool operator==(unsigned long n) const {
            return isInteger() && (ul == n);
        }

        bool operator==(double n) const {
            return ((isReal() && (d == n)) || (isInteger() && ((double)l == n)));
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
        inline explicit operator int() const { if(!isInteger()) throw std::bad_cast(); return (int)l; }
        inline explicit operator unsigned int() const { if(!isInteger()) throw std::bad_cast(); return (int)ul; }
        inline explicit operator long() const { if(!isInteger()) throw std::bad_cast(); return l; }
        inline explicit operator unsigned long() const { if(!isInteger()) throw std::bad_cast(); return ul; }
        inline explicit operator double() const { if(!isInteger() && !isReal()) throw std::bad_cast(); return isReal() ? d : (double)l; }
        inline explicit operator bool() const { return isBoolean() ? b : (ul > 0); }
        inline explicit operator const char*() const { if(!isString()) throw std::bad_cast(); return s; }

        template<class T>
        inline const T& get() const {
            if(!isObject())
                throw Rest::Exception(InvalidParameterType);
            return *(T*)obj;
        }

        template<class T>
        inline T& get() {
            if(!isObject())
                throw std::bad_cast();
            else if(isConstObject())
                throw std::bad_cast();
            else
                return *(T*)obj;
        }


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
