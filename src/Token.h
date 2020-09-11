//
// Created by colin on 11/9/2018.
//

#pragma once

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "StringPool.h"

#if defined(HAS_MALLOC_H)
#include <malloc.h>
#endif

// simple token IDs
#define TID_EOF               300
#define TID_INTEGER           303
#define TID_FLOAT             305
#define TID_BOOL              307
#define TID_WILDCARD          '*'

// String token IDs
// the following token types allocate memory for string tokens and so must be freed
#define TID_ERROR                500
#define TID_STRING               501
#define TID_IDENTIFIER           502


#ifdef ESP8266
// require typeof operator
#define typeof(x) __typeof__(x)
#endif


namespace Rest {

// shared index of text strings
// assigned a unique integer ID to each word stored
extern StringPool literals_index;


class Token {
  public:
    typedef enum {
        allocString,
        indexAlways,
        indexIfExists
    } index_option;

  public:
    short id;
    const char *s;
    int64_t i;
    double d;

    bool indexed;   // true if string was stored in binbag index, otherwise string was allocated

    // reference back to the original string
    const char* original;

    inline Token() : id(0), s(nullptr), i(0), d(0), indexed(false), original(nullptr) {}

    Token(const Token& copy)
        : id(copy.id), s(nullptr), i(copy.i), d(copy.d), indexed(false), original(copy.original)
    {
      if(copy.s && copy.id >= 500) {
        s = indexed
            ? copy.s          // no need to allocate, copy was indexed in binbag
            : strdup(copy.s);      }
    }

    Token& operator=(const Token& copy) {
      id = copy.id;
      i = copy.i;
      d = copy.d;
      indexed = copy.indexed;
      original = copy.original;
      if(copy.s && copy.id >= 500) {
        s = indexed
              ? copy.s          // no need to allocate, copy was indexed in binbag
              : strdup(copy.s);
      }
      return *this;
    }

    ~Token() {
      clear();
    }

    inline bool operator==(const char* value) const {
        return (id == TID_STRING || id == TID_IDENTIFIER) && strcmp(s, value) == 0;
    }

    inline bool operator==(int value) const { return (id == TID_INTEGER) && i == value; }
    //inline bool operator==(unsigned int value) const { return (id == TID_INTEGER) && i == value; }
    inline bool operator==(long value) const { return (id == TID_INTEGER) && i == value; }
    //inline bool operator==(unsigned long value) const { return (id == TID_INTEGER) && i == value; }

    inline bool operator==(double value) const {
        return (id == TID_FLOAT) && d == value;
    }

    /// \brief clears the token and frees memory if token was a string
    void clear()
    {
      if (s && id >= 500 && !indexed)
        ::free((void*)s);
      id = 0;
      s = nullptr;
      i = 0;
      d = 0.0;
      indexed = false;
      original = nullptr;
    }

    /// \brief swap the values of two tokens
    /// If using a current and peek token during parsing, this can be more efficient than copying the token
    /// across by saving a possible string copy and memory allocation.
    void swap(Token& rhs) {
      short _id = rhs.id;
      const char *_s = rhs.s;
      int64_t _i = rhs.i;
      double _d = rhs.d;
      bool _indexed = rhs.indexed;
      const char* _original = rhs.original;
      rhs.id = id;
      rhs.s = s;
      rhs.i = i;
      rhs.d = d;
      rhs.indexed = indexed;
      rhs.original = original;
      id = _id;
      s = _s;
      i = _i;
      d = _d;
      indexed = _indexed;
      original = _original;
    }

    void set(short _id, const char* _begin, const char* _end, index_option _index = allocString)
    {
      assert(_id >= 500);  // only IDs above 500 can store a string
      id = _id;
      indexed = indexAlways;

      if(_index == indexIfExists) {
        // look in index and if word exists then use it
        long idx = (_end == nullptr)
                ? literals_index.find(_begin, strcasecmp)
                : literals_index.find(_begin, _end - _begin, strncasecmp);
        if(idx >=0) {
          indexed = true;
          s = literals_index[i = idx];
          return;
        }
      }

      if(_index == indexAlways) {
        // insert into the index
        i = (_end == nullptr)
                ? literals_index.insert_distinct(_begin, strcasecmp)
                : literals_index.insert_distinct(_begin, _end - _begin, strncasecmp);
        s = literals_index[i];
      } else {
        // allocate memory and copy the string
        if (_end == nullptr) {
          s = strdup(_begin);
        } else {
          s = (char *) calloc(1, _end - _begin + 1);
          memcpy((void*)s, _begin, _end - _begin);
        }
      }
    }

    inline bool is(short _id) const {
      return id == _id;
    }

    template<typename... Targs>
    inline bool is(short _id, Targs... args) const {
      return id == _id || is(args...);
    }

    int isOneOf(std::initializer_list<const char*> enum_values, bool case_insensitive = true) {
      if (id <= 500)
        return -2;
      typeof(strcmp) *cmpfunc = case_insensitive
                                ? &strcasecmp
                                : &strcmp;
      int j = 0;
      for (std::initializer_list<const char*>::const_iterator x = enum_values.begin(), _x = enum_values.end(); x != _x; x++, j++) {
        if (cmpfunc(s, *x) == 0)
          return j;
      }
      return -1;
    }

    int toEnum(std::initializer_list<const char*> enum_values, bool case_insensitive = true) {
      if (id <= 500)
        return -2;
      typeof(strcmp) *cmpfunc = case_insensitive
                                ? &strcasecmp
                                : &strcmp;
      int j = 0;
      for (std::initializer_list<const char*>::const_iterator x = enum_values.begin(), _x = enum_values.end(); x != _x; x++, j++) {
        if (cmpfunc(s, *x) == 0) {
          clear();
          id = TID_INTEGER;
          i = j;
          return j;
        }
      }
      return -1;
    }

    /// \brief scans the next token from the URL line
    int scan(const char** pinput, short allow_parameters)
    {
      const char* input = *pinput;
      char error[512];

      clear();

      if (*input == 0) {
        id = TID_EOF;
        *pinput = input;
        return 0;
      }

      original = input;

      // check for single character token
      // note: if we find a single char token we break and then return, otherwise (default) we jump over
      // to check for longer token types like keywords and attributes
      if (strchr("/", *input) != nullptr) {
        s = nullptr;
        id = *input++;
        goto done;
      } else if (allow_parameters && strchr("=:?(|)*", *input) != nullptr) {
        // these symbols are allowed when we are scanning a Rest URL match expression
        // but are not valid in normal URLs, or at least considered part of normal URL matching below
        s = nullptr;
        id = *input++;
        goto done;
      }

      // todo: probably we should be reading all chars up to next / token then interpreting the type as number, boolean, identifier or string.

      // check for literal float
      if (input[0] == '.') {
        if (isdigit(input[1])) {
          // decimal number
          char *p;
          id = TID_FLOAT;
          d = strtod(input, &p);
          i = (int64_t) d;
          input = p;
          goto done;
        } else {
          // plain dot symbol
          id = '.';
          s = nullptr;
          input++;
          goto done;
        }
      } else if (input[0] == '0' && input[1] == 'x') {
        // hex constant
        char* p;
        id = TID_INTEGER;
        i = (int64_t)strtoll(input, &p, 16);
        input = p;
        goto done;
      } else if (isdigit(*input)) {
        //scan_number:
        // integer or float constant
        char *p;
        id = TID_INTEGER;
        i = (int64_t)strtoll(input, &p, 0);
        if (*p == '.') {
          id = TID_FLOAT;
          d = strtod(input, &p);
          input = p;
        } else
          input = p;
        goto done;
      }
      // check for boolean value
      else if (strncasecmp(input, "false", 5) == 0 && !isalnum(input[5])) {
        input += 5;
        id = TID_BOOL;
        i = 0;
        goto done;
      }
      else if (strncasecmp(input, "true", 4) == 0 && !isalnum(input[4])) {
        input += 4;
        id = TID_BOOL;
        i = 1;
        goto done;
      }
#if 0
      // check for identifier
      else if (isalpha(*input) || *input == '_') {
        // pull out an identifier match
        short ident = TID_IDENTIFIER;
        const char* p = input;
        while (*input && (*input == '_' || *input == '-' || isalnum(*input))) {
          input++;
        }
        set(ident, p, input);
        goto done;
      }
#else
      // assume an identifier but if we find a non-identifier char then change to string
      else {
        short ident = TID_IDENTIFIER;
        const char* p = input;
        while( *input && *input!='/' ) {
          if(!isalnum(*input) && *input != '_' && *input != '-' && *input != '.') {
            // encountered non-alpha character
            if(!allow_parameters) {
              // we can interpret as string since we arent limited to expression syntax
              ident = TID_STRING;
            } else {
              // only identifiers allowed, so we stop here
              break;
            }
          }
          input++;
        }

        // success if we consumed 1 or more characters
        if(input > p) {
          set(ident, p, input,
                  (ident != TID_IDENTIFIER)
                      ? allocString                   // never use index on strings
                      : allow_parameters
                         ? indexAlways                // in expression mode so add identifiers to index
                         : indexIfExists        // resolving URIs, so only use index if pre-existing identifier
          );
          goto done;
        }
      }
#endif

      sprintf(error, "syntax error, unexpected '%c' in input", *input);
      input++;
      set(TID_ERROR, error, nullptr);

done:
      *pinput = input;
      return 1;
    }
};


} // ns:Rest
