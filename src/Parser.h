//
// Created by Colin MacKenzie on 2018-11-09.
//

#pragma once

#include "Token.h"
#include "UriRequest.h"
#include "Exception.h"
#include "Parameter.h"
#include "Argument.h"

#include <functional>

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

    template<class TUriRequest>
    class Parser
    {
    public:
        class Delegate {
        public:
            //virtual Parser delegate(const Parser& p) const = 0;
            virtual Parser delegate(Parser& p) = 0;
        };

        class Handler: public UriRequestMatch {
        public:
            using handlerType0 = std::function<int()>;
            using handlerType1 = std::function<int(UriRequest&)>;
            using handlerType2 = std::function<int(const Parser&)>;

            Handler(HttpMethod _method, std::function<int()> _handler)
                : UriRequestMatch(_method), handler(std::move(_handler)), handlerType(0)
            {}

            Handler(HttpMethod _method, std::function<int(UriRequest&)> _handler)
                : UriRequestMatch(_method), handler_req(std::move(_handler)), handlerType(1)
            {}

            Handler(HttpMethod _method, std::function<int(const Parser&)> _handler)
                : UriRequestMatch(_method), handler_parser(std::move(_handler)), handlerType(2)
            {}

            Handler(const Handler& copy)
                : UriRequestMatch(copy), handlerType(copy.handlerType)
            {
                // using the "placement" copy constructor to properly initialize our union object
                switch(handlerType) {
                    case 0: new (&handler) handlerType0(copy.handler); break;
                    case 1: new (&handler_req) handlerType1 (copy.handler_req); break;
                    case 2: new (&handler_parser) handlerType2 (copy.handler_parser); break;
                }
            }

            virtual ~Handler() {
                // using the "placement" copy constructor to properly initialize our union object
                switch(handlerType) {
                    case 0: handler.~handlerType0(); break;
                    case 1: handler_req.~handlerType1(); break;
                    case 2: handler_parser.~handlerType2(); break;
                }
            }

            virtual int call(Parser& parser) {
                switch(handlerType) {
                    case 0: return handler();
                    case 1: return handler_req(*parser._request);
                    case 2: return handler_parser(parser);
                }
                return 0;
            }

            short handlerType;
            union {
                handlerType0 handler;
                handlerType1 handler_req;
                handlerType2 handler_parser;
            };
        };

    public:
        UriRequest* _request;

        // reference to the previous state in the current parse chain
        // todo: this must be std::sharedp_ptr'ized since we are linking
        Parser *_parent;

        int _tokenOrdinal;

        // parse result
        int status;

        Type _type;             // type information for this node (either explicitly set or derived from Token)
        Name _name;             // node name. Will be valid for arguments but typically blank for const path nodes
        Variant _value;         // preset value of this node (usually the value is derived from the token)

    public:
        Parser()
            : _request(nullptr), _parent(nullptr), _tokenOrdinal(0), status(NoHandler)
        {}

        explicit Parser(UriRequest& request)
            : _request(&request), _parent(nullptr), _tokenOrdinal(0), status(request.status)
        {
        }

        inline operator bool() const { return status >= 0; }

        virtual void abort(int code) { status = code; }

        inline bool isSuccessful() const { return status == 0 || (status >= 200 && status < 300); }

        Token token() const {
            return (_tokenOrdinal >= 0 && _tokenOrdinal < _request->words.size())
                ? _request->words[_tokenOrdinal]
                : Token();
        }

        inline int ordinal() const { return _tokenOrdinal; }

        Type type() const {
            return _type.isVoid()
            ? typeFromToken(token())
            : _type;
        }

#if defined(ARDUINO)
        String name() const { return String(_name.value()); }
#else
        std::string name() const { return std::string(_name.value()); }
#endif

        Variant value() const {
            return _value.empty()
               ? valueFromToken(token())
               : _value;
        }

        Argument operator[](const char* argname) const {
            if(_name && strcmp(_name.value(), argname) == 0) {
                // check if we have a preset value for this node
                return Argument(_name, value());
            }
            return _parent
                ? _parent->operator[](argname)
                : Argument();
        }

        Parser operator/(const char* input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                if(strcasecmp(t.s, input) ==0) {
                    return Parser(_request, *this, _tokenOrdinal + 1);
                }
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

        Parser operator/(std::string input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                input = t.s;
                if(strcasecmp(t.s, input.c_str()) ==0)
                    return Parser(_request, *this, _tokenOrdinal + 1);
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

        Parser operator/(std::string* input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                *input = t.s;
                return Parser(_request, *this, _tokenOrdinal + 1);
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

#if defined(ARDUINO)
        Parser operator/(String input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                input = t.s;
                if(strcasecmp(t.s, input.c_str()) ==0)
                    return Parser(_request, *this, _tokenOrdinal + 1, 0);
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

        Parser operator/(String* input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_STRING, TID_IDENTIFIER)) {
                *input = t.s;
                return Parser(_request, *this, _tokenOrdinal + 1, 0);
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }
#endif

        Parser operator/(int input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_INTEGER)) {
                if(t.i == input) {
                    return Parser(_request, *this, _tokenOrdinal + 1);
                }
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

        Parser operator/(int* input) {
            if(status != 0) return *this;
            // compare the current token
            auto t = token();
            if(t.is(TID_INTEGER)) {
                *input = t.i;        // set variable
                return Parser(_request, *this, _tokenOrdinal + 1);
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

        /// @brief expect a parameter of specific type and store as an argument
        Parser operator/(Parameter param) {
            if (status != 0) return *this;

            auto t = token();
            if(typeMatch(t.id, param.type)) {
                // matched as argument
                _type = param.type;
                _name = param.name;
                return Parser(_request, *this, _tokenOrdinal + 1);
            }
            return Parser(_request, *this, _tokenOrdinal, NoHandler);
        }

        /// @brief store a variable in an argument without reading from input Uri
        Parser operator/(Argument _arg) {
            if (status != 0) return *this;

                // matched as argument
                _type = _arg;
            _name = _arg.name;
            _value = _arg;
                return Parser(_request, *this, _tokenOrdinal);
        }

        Parser operator/(Handler handler) {
            if (status != 0) return *this;
            if (handler.matches(*_request)) {
                this->status = handler.call(*this);

                // if handler returned an HTTP status code then we complete
                if(this->status > 0)
                    _request->status = this->status;
            }
            return *this;
        }

        Parser operator/(Delegate& target) {
            if (status != 0) return *this;

            // send the next token to the delegator
            auto next = Parser(_request, *this, _tokenOrdinal);
            return target.delegate(next);
        }


    protected:
        Parser(UriRequest* request, Parser& parent, int tokenOrdinal)
                : _request(request), _parent(&parent), _tokenOrdinal(tokenOrdinal), status(request->status)
        {
        }

        Parser(UriRequest* request, Parser& parent, int tokenOrdinal, int _result)
                : _request(request), _parent(&parent), _tokenOrdinal(tokenOrdinal), status(_result)
        {
        }

        static bool typeMatch(short tokenType, Type argType) {
            switch(argType.typemask()) {
                case ARG_MASK_UNSIGNED:
                case ARG_MASK_UINTEGER:
                case ARG_MASK_NUMBER:
                case ARG_MASK_INTEGER:
                    return (tokenType & TID_INTEGER) > 0;
                case ARG_MASK_REAL:
                    return (tokenType & TID_INTEGER) > 0;
                case ARG_MASK_BOOLEAN:
                    return (tokenType & TID_BOOL) > 0;
                case ARG_MASK_STRING:
                    return tokenType >= TID_STRING;
                default:
                    return false;
            }
        }

        static Type typeFromToken(short tokenType) {
            switch(tokenType) {
                case TID_IDENTIFIER:
                case TID_STRING:
                case TID_ERROR:
                    return Type(ARG_MASK_STRING);
                case TID_INTEGER:
                    return Type(ARG_MASK_INTEGER);
                case TID_BOOL:
                    return Type(ARG_MASK_BOOLEAN);
                case TID_FLOAT:
                    return Type(ARG_MASK_REAL);
                default:
                    return Type();
            }
        }

        static Variant valueFromToken(Token t) {
            // the argument is the token value
            switch(t.id) {
                case TID_IDENTIFIER:
                case TID_STRING:
                case TID_ERROR:
                    return Variant(t.s);
                case TID_INTEGER:
                    return Variant(t.i);
                case TID_BOOL:
                    return Variant(t.i > 0);
                case TID_FLOAT:
                    return Variant(t.d);
                default:
                    return Variant();
            }
        }
    };
}
