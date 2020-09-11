//
// Created by Colin MacKenzie on 2018-11-09.
//

#pragma once

#include "Token.h"
#include "handler.h"
#include "UriRequest.h"
#include "Exception.h"

#include <algorithm>

namespace Rest {

    // list of possible states
    enum {
        expectPathPartOrSep,        // first token only
        expectPathSep,
        expectPathPart,
        expectPathPartOrParam,
        expectParam,
        expectParameterValue,
        endParam,
        errorExpectedIdentifier,
        errorExpectedIdentifierOrString,
        expectHtmlSuffix,
        expectEof
    };


    class Parser
    {
    public:
        // parser input string (gets eaten as parsing occurs)
        UriRequest request;

        // current token 't' and look-ahead token 'peek' pulled from the input string 'uri' (above)
        Token t, peek;
        int token;  // current token being pointed to

        // parser state machine current state
        int state;

        // reference to the previous state in the current parse chain
        // todo: this must be std::sharedp_ptr'ized since we are linking
        Parser *_parent;

        // parse result
        int status;


    public:
        Parser(UriRequest& request)
            : request(request), _parent(nullptr), token(0), status(0)
        {
        }

        inline operator bool() const { return status >= 0; }

        virtual void abort(int code) { status = code; }

        inline bool isSuccessful() const { return status == 0 || (status >= 200 && status < 300); }

        Parser operator/(const char* input) {
            if(status != 0) return *this;
            // compare the current token
            Token t = request.words[token];
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                if(strcasecmp(t.s, input) ==0) {
                    return Parser(request, *this, token + 1, 0);
                }
            }
            return Parser(request, *this, token, NoHandler);
        }

        Parser operator/(std::string input) {
            if(status != 0) return *this;
            // compare the current token
            Token t = request.words[token];
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                input = t.s;
                if(strcasecmp(t.s, input.c_str()) ==0)
                    return Parser(request, *this, token + 1, 0);
            }
            return Parser(request, *this, token, NoHandler);
        }

        Parser operator/(std::string* input) {
            if(status != 0) return *this;
            // compare the current token
            Token t = request.words[token];
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                *input = t.s;
                return Parser(request, *this, token + 1, 0);
            }
            return Parser(request, *this, token, NoHandler);
        }

        Parser operator/(int input) {
            if(status != 0) return *this;
            // compare the current token
            Token t = request.words[token];
            if(t.is(TID_INTEGER)) {
                if(t.i == input) {
                    return Parser(request, *this, token + 1, 0);
                }
            }
            return Parser(request, *this, token, NoHandler);
        }

        Parser operator/(int* input) {
            if(status != 0) return *this;
            // compare the current token
            Token t = request.words[token];
            if(t.is(TID_INTEGER)) {
                *input = t.i;        // set variable
                return Parser(request, *this, token + 1, 0);
            }
            return Parser(request, *this, token, NoHandler);
        }

        Parser operator/(UriRequestHandler handler) {
            if (status != 0) return *this;
            if (handler.matches(request))
                this->status = handler.call(*this);
            return *this;
        }

    protected:
        Parser(UriRequest& request, Parser& parent, int tokenOrdinal, int _result)
                : request(request), _parent(&parent), token(tokenOrdinal), status(_result)
        {
        }
    };
}
