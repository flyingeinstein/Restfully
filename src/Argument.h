//
// Created by colin on 11/9/2018.
//

#pragma once

#include <string>
#include <cstring>
#include <cassert>

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

    class Type
    {
    public:
        inline Type() : name(nullptr), typemask(0) {}
        Type(const char* _name, unsigned short _typemask) : name(_name), typemask(_typemask) {}

    public:
        // if this argument is matched, the value is added to the request object under this field name
        const char* name;

        // collection of ARG_MASK_xxxx bits represent what types are supported for this argument
        unsigned short typemask;
    };

    class Argument : protected Type {
    public:
        typedef Type Type;

        Argument() : type(0), ul(0) {}
        Argument(const Argument& copy) : Type(copy), type(copy.type), ul(copy.ul) {
            if(type == ARG_MASK_STRING)
                s = strdup(copy.s);
            else if((type & ARG_MASK_REAL) >0)
                d = copy.d;
            else
                ul = copy.ul;
        }

        Argument(const Type& arg) : Type(arg), type(0), ul(0) {}

        Argument(const Type& arg, long _l) : Type(arg), type(ARG_MASK_INTEGER), l(_l) {}
        Argument(const Type& arg, unsigned long _ul) : Type(arg), type(ARG_MASK_UINTEGER), ul(_ul) {}
        Argument(const Type& arg, double _d) : Type(arg), type(ARG_MASK_NUMBER), d(_d) {}
        Argument(const Type& arg, bool _b) : Type(arg), type(ARG_MASK_BOOLEAN), b(_b) {}
        Argument(const Type& arg, const char* _s) : Type(arg), type(ARG_MASK_STRING), s(strdup(_s)) {}

        ~Argument() { if(s && type == ARG_MASK_STRING) ::free(s); }

        Argument& operator=(const Argument& copy) {
            Type::operator=(copy);
            type = copy.type;
            if(type == ARG_MASK_STRING)
                s = strdup(copy.s);
            else if((type & ARG_MASK_REAL) >0)
                d = copy.d;
            else
                ul = copy.ul;
            return *this;
        }

        inline const char* name() const { return Type::name; }

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
        inline bool isSignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_INTEGER; }
        inline bool isUnsignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_UINTEGER; }
        inline bool isNumber() const { return (type&ARG_MASK_NUMBER)==ARG_MASK_NUMBER; }
        inline bool isBoolean() const { return (type&ARG_MASK_BOOLEAN)==ARG_MASK_BOOLEAN; }
        inline bool isString() const { return (type&ARG_MASK_STRING)==ARG_MASK_STRING; }

        // only supported in C++11
        inline operator long() const { assert(type&ARG_MASK_INTEGER); return l; }
        inline operator unsigned long() const { assert(type&ARG_MASK_INTEGER); return ul; }
        inline operator double() const { assert(type&ARG_MASK_NUMBER); return d; }
        inline operator bool() const { return (type == ARG_MASK_BOOLEAN) ? b : (ul>0); }
        inline operator const char*() const { assert(type&ARG_MASK_STRING); return s; }

#if defined(ARDUINO)
        inline operator String() const { assert(type&ARG_MASK_STRING); return String(s); }

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

    class Arguments {
    public:
        Arguments(size_t n)
            : args(nullptr), nargs(n)
        {
            if(n>0)
                args = new Argument[nargs];
        }

        Arguments(const Arguments& copy)
            : args(nullptr), nargs(copy.nargs)
        {
            if(nargs>0) {
                args = new Argument[nargs];
                for (size_t i = 0; i < nargs; i++)
                    args[i] = copy.args[i];
            }
        }

        virtual ~Arguments() { delete [] args; }

        inline Arguments& operator=(const Arguments& copy) {
            delete [] args; // no need to check null
            nargs = copy.nargs;
            if(nargs>0) {
                args = new Argument[nargs];
                for (size_t i = 0; i < nargs; i++)
                    args[i] = copy.args[i];
            } else
                args = nullptr;
            return *this;
        }

        Arguments operator+(const Arguments& rhs) {
            int i, j;
            Arguments a(nargs + rhs.nargs);
            for(i=0; i<nargs; i++)
                a.args[i] = args[i];
            for(j=0; j<rhs.nargs; j++)
                a.args[i+j] = args[j];
            return a;
        }

        const Argument& operator[](size_t idx) const {
            return (idx<nargs)
                   ? args[idx]
                   : Argument::null;
        }

        const Argument& operator[](const char* _name) const {
            for(size_t i=0; i<nargs; i++) {
                if (strcmp(_name, args[i].name()) == 0)
                    return args[i];
            }
            return Argument::null;
        }

    protected:
        Argument* args;
        size_t nargs;
    };


}; // ns: Rest
