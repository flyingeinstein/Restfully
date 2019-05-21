//
// Created by colin on 11/9/2018.
//

#pragma once

#include <string>
#include <cstring>
#include <cassert>
#include "Pool.h"

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
        inline Type() : name_index(0), type_mask(0) {}
        Type(const char* _name, unsigned short _typemask)
                : name_index(binbag_insert_distinct(literals_index, _name, strcasecmp)),
                  type_mask(_typemask) {
        }

        Type(long _name_index, unsigned short _typemask)
                : name_index(_name_index),
                  type_mask(_typemask) {
        }

        inline const char* name() const { return binbag_get(literals_index, name_index); }

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

        Argument(const Type& arg) : Type(arg), type(0), ul(0) {}

        Argument(const Type& arg, long _l) : Type(arg), type(ARG_MASK_INTEGER), l(_l) {}
        Argument(const Type& arg, unsigned long _ul) : Type(arg), type(ARG_MASK_UINTEGER), ul(_ul) {}
        Argument(const Type& arg, double _d) : Type(arg), type(ARG_MASK_NUMBER), d(_d) {}
        Argument(const Type& arg, bool _b) : Type(arg), type(ARG_MASK_BOOLEAN), b(_b) {}
        Argument(const Type& arg, const char* _s) : Type(arg), type(ARG_MASK_STRING), s(strdup(_s)) {}

        virtual ~Argument() { if(s && type == ARG_MASK_STRING) ::free(s); }

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

        bool operator==(const Argument& rhs) const {
            return (type == rhs.type) && (
                    (type==ARG_MASK_STRING && strcmp(s, rhs.s)==0) ||
                    (type==ARG_MASK_INTEGER && l==rhs.l) ||
                    (type==ARG_MASK_UINTEGER && ul==rhs.ul) ||
                    (type==ARG_MASK_NUMBER && d==rhs.d) ||
                    (type==ARG_MASK_BOOLEAN && b==rhs.b)
             );
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
        inline bool isSignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_INTEGER; }
        inline bool isUnsignedInteger() const { return (type&ARG_MASK_UINTEGER)==ARG_MASK_UINTEGER; }
        inline bool isNumber() const { return (type&ARG_MASK_NUMBER)>0; }
        inline bool isBoolean() const { return (type&ARG_MASK_BOOLEAN)==ARG_MASK_BOOLEAN; }
        inline bool isString() const { return (type&ARG_MASK_STRING)==ARG_MASK_STRING; }

        // only supported in C++11
        inline operator int() const { assert(type&ARG_MASK_INTEGER); return (int)l; }
        inline operator unsigned int() const { assert(type&ARG_MASK_INTEGER); return (int)ul; }
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
        using _size_t = short;

    public:
        Arguments()
            : args(nullptr), _capacity(0), _count(0)
        {}

        Arguments(_size_t _capacity)
            : args(nullptr), _capacity(0), _count(0)
        {
            alloc(_capacity);
        }

        Arguments(Argument* _args, _size_t n)
                : args(nullptr), _capacity(0), _count(0)
        {
            copy(_args, _args + n);
        }

        Arguments(Argument* _args, _size_t n, _size_t _capacity)
                : args(nullptr), _capacity(0), _count(0)
        {
            if(_capacity<n) _capacity = n;
            alloc(_capacity);
            copy(_args, _args + n);
        }

        Arguments(const Arguments& _copy)
            : args(nullptr), _capacity(0), _count(0)
        {
            alloc(_copy._capacity);
            copy(_copy.args, _copy.args + _copy._count);
        }

        virtual ~Arguments() { free(); }

        inline Arguments& operator=(const Arguments& _copy) {
            free();
            alloc(_copy._capacity);
            copy(_copy.args, _copy.args + _copy._count);
            return *this;
        }

        Arguments operator+(const Arguments& rhs) {
            Arguments a(_count + rhs._count);
            a.copy(args, args + _count);                         // left
            a.copy(rhs.args, rhs.args + rhs._count, a._count);    // right
            return a;
        }

        /*Arguments concat(const Argument* _begin, const Argument* _end) {
            decltype(nargs) i;
            size_t cnt = _end - _begin;
            Arguments a(nargs + cnt);
            for(i=0; i<nargs; i++)
                a.args[i] = args[i];
            while(_begin < _end)
                a.args[i++] = *_begin++;
            return a;
        }*/

        const Argument& operator[](int idx) const {
            return (idx<_count)
                   ? args[idx]
                   : Argument::null;
        }

        const Argument& operator[](const char* _name) const {
            for(decltype(_count) i=0; i<_count; i++) {
                if (strcmp(_name, args[i].name()) == 0)
                    return args[i];
            }
            return Argument::null;
        }

        Argument& add(Type& t) {
            // ensure we have room to add
            if(_count >= _capacity)
                alloc(_capacity+1);
            assert(_count < _capacity);

            // add the argument
            Argument* arg = new (&args[_count++]) Argument(t);    // placement copy constructor
            assert(arg);
            return *arg;
        }

        Argument& add(const Argument& t) {
            // ensure we have room to add
            if(_count >= _capacity)
                alloc(_capacity+1);
            assert(_count < _capacity);

            // add the argument
            Argument* arg = new (&args[_count++]) Argument(t);    // placement copy constructor
            assert(arg);
            return *arg;
        }

        inline void reserve(_size_t _count) { alloc(_count); }

        inline _size_t count() const { return _count; }

        inline _size_t capacity() const { return _capacity; }

    protected:
        Argument* args;
        _size_t _capacity;
        _size_t _count;

        void alloc(_size_t _count) {
            if(_count ==0) {
                // just free
                free();
            } else if(_count != _capacity) {
                _capacity = _count;
                args = (args != nullptr)
                    ? (Argument*)realloc(args, _capacity * sizeof(Argument))
                    : (Argument*)calloc(_capacity, sizeof(Argument));
            }
        }

        void free() {
            if(args) {
                //for(Argument *a = args, *_a=args+nargs; a < _a; a++)
                //    a->~Argument();
                ::free(args);
                args = nullptr;
                _capacity = 0;
                _count = 0;
            }
        }

        void copy(Argument* begin, Argument* end, _size_t dest_index=0) {
            _size_t n = end - begin;
            if(_capacity < n)
                alloc( n );

            Argument* dest = args + dest_index;
            while(begin < end) {
                new(dest++) Argument(*begin++);    // placement copy constructor
                _count++;
            }
        }
    };


}; // ns: Rest
