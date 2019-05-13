//
// Created by Colin MacKenzie on 2018-11-09.
//

#pragma once

#include "Token.h"
#include "handler.h"
#include "Argument.h"

#include <algorithm>

namespace Rest {

    typedef enum {
        UriMatched                          = 1,
        UriMatchedWildcard                  = 2,
        NoEndpoint                          = -1,
        NoHandler                           = -2,
        InvalidHandler                      = -3,
        Duplicate                           = -4,
        InvalidParameterType                = -5,
        MissingParameter                    = -6,
        URL_FAIL_AMBIGUOUS_PARAMETER        = -7,
        URL_FAIL_EXPECTED_PATH_SEPARATOR    = -8,
        URL_FAIL_EXPECTED_EOF               = -9,
        URL_FAIL_INVALID_TYPE               = -10,
        URL_FAIL_SYNTAX                     = -11,
        URL_FAIL_INTERNAL                   = -15,
        URL_FAIL_INTERNAL_BAD_STRING        = -16,
        URL_FAIL_NULL_ROOT                  = -17,
        URL_FAIL_EXPECTED_IDENTIFIER        = -18,
        URL_FAIL_EXPECTED_STRING            = -19
    } ParseResult;

    /// \brief Convert a return value to a string.
    /// Typically use this to get a human readable string for an error result.
    const char* uri_result_to_string(short result);

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

    class UriRequest {
    public:
        HttpMethod method;
        const char* uri;
        Arguments args;
        int status;

        inline UriRequest() :  method(HttpMethodAny), uri(nullptr), status(0) {}
        inline UriRequest(HttpMethod _method, const char* _uri, int _status=0) : method(_method), uri(_uri), status(_status) {}
        inline UriRequest(const UriRequest& copy) : method(copy.method), uri(copy.uri), args(copy.args), status(copy.status) {}

        inline const Argument& operator[](size_t idx) const { return args.operator[](idx); }
        inline const Argument& operator[](const char* name) const { return args.operator[](name); }

        inline UriRequest& operator=(const UriRequest& copy) = default;

        virtual void abort(int code) { status = code; }
    };

    /// \brief Contains state for resolving or expanding a Url expression tree
    class ParserState {
    public:
        typedef enum {
            expand = 1,           // indicates adding a new endpoint/handler
            resolve = 2        // indicates we are resolving a URL to a defined handler
        } mode_e;

        ParserState(const UriRequest& _request)
                : mode(resolve), request(_request), state(expectPathPartOrSep),
                  nargs(0), result(0)
        {
            if(request.uri != nullptr) {
                // scan first token
                if (!t.scan(&request.uri, 1))
                    goto bad_eval;
                peek.scan(&request.uri, 1);
            }
            return;
        bad_eval:
            state = -1;
        }

        ParserState(const ParserState& copy)
            : mode(copy.mode), request(copy.request), t(copy.t), peek(copy.peek), state(copy.state),
              nargs(copy.nargs), result(copy.result)
        {
        }

        ParserState& operator=(const ParserState& copy) {
            mode = copy.mode;
            request = copy.request;
            t = copy.t;
            peek = copy.peek;
            state = copy.state;
            nargs = copy.nargs;
            result = copy.result;
            return *this;
        }

    public:
        // indicates if we are parsing to resolve or to add an endpoint
        mode_e mode;

        // parser input string (gets eaten as parsing occurs)
        UriRequest request;

        // current token 't' and look-ahead token 'peek' pulled from the input string 'uri' (above)
        Token t, peek;

        // parser state machine current state
        int state;

        // will contain the arguments embedded in the URL.
        // when adding, will contain argument type info
        // when resolving, will contain argument values
        size_t nargs;

        // parse result
        int result;
    };


#define GOTO_STATE(st) { ev->state = st; goto rescan; }
#define NEXT_STATE(st) { ev->state = st; }
#define SCAN { ev->t.clear(); ev->t.swap( ev->peek ); if(ev->t.id!=TID_EOF) ev->peek.scan(&ev->request.uri, ev->mode == ParserState::expand); if(ev->peek.id==TID_ERROR) return URL_FAIL_SYNTAX; }

    template<
            class TNode,
            class TPool,
            class TToken=Token
    >
    class Parser
    {
    public:
        using LiteralType = typename TNode::LiteralType;
        using ArgumentType = typename TNode::ArgumentType;

        using Node = TNode;
        using Token = TToken;

        /// The pool from which to create new endpoint nodes, literals or arguments
        /// Usually this is a Endpoints<> class, but any class that supports newNode, newLiteralString, newLiteralNumber, newArgumentType will work.
        TPool* pool;

        /// the node context we are currently parsing
        /// This updates as path nodes are parsed
        Node* context;

    public:
        Parser(Node* _context, TPool* _pool) : pool(_pool), context(_context)
        {
        }

        /// \brief Parses a url and either adds or resolves within the expression tree
        /// The Url and parse mode are set in ParseData and determine if parse() returns when expression tree hits a dead-end
        /// or if it starts expanding the expression tree.
        ParseResult parse(ParserState* ev)
        {
            ParseResult rv;
            long wid;
            Node* epc = context;
            LiteralType* lit;
            ArgumentType* arg;

            // read datatype or decl type
            while(ev->t.id!=TID_EOF) {
                rescan:
                epc = context;

                switch(ev->state) {
                    case expectPathPartOrSep:
                        // check if we have a sep, and jump immediately to other state
                    GOTO_STATE( (ev->t.id=='/')
                                ? expectPathSep
                                : expectPathPart
                    );
                        break;
                    case expectPathSep: {
                        // expect a slash or end of URL
                        if(ev->t.id=='/') {
                            NEXT_STATE( (ev->mode==ParserState::resolve) ? expectPathPart : expectPathPartOrParam);

                        } else if(ev->t.id == TID_EOF) {
                            // safe place to end
                            goto done;
                        }
                    } break;
                    case expectHtmlSuffix: {
                        if(ev->t.is(TID_STRING , TID_IDENTIFIER)) {
                            if(strcasecmp(ev->t.s, "html") !=0) {
                                return NoEndpoint;    // only supports no suffix, or html suffix
                            } else
                            NEXT_STATE(expectEof);  // always expect eof after suffix
                        }
                    } break;
                    case expectPathPart: {
                        if(ev->mode == ParserState::expand && ev->t.is(TID_WILDCARD)) {
                            // encountered wildcard, must be last token
                            if(epc->wild==nullptr) {
                                context = epc->wild = pool->newNode();
                            } else {
                                context = epc->wild;
                            }

                            return ev->peek.is(TID_EOF)
                                ? UriMatchedWildcard
                                : URL_FAIL_SYNTAX;
                        }
                        else if(ev->t.is(TID_STRING, TID_IDENTIFIER)) {
                            // we must see if we already have a literal with this name
                            lit = nullptr;
                            wid = pool->findLiteral(ev->t.s);
                            if(wid>=0 && epc->literals) {
                                // word exists in dictionary, see if it is a literal of current endpoint
                                lit = epc->literals;
                                while(lit && lit->isValid() && lit->id!=wid)
                                    lit = lit->next;
                                if(lit && lit->id!=wid)
                                    lit=nullptr;
                            }

                            // add the new literal if we didnt find an existing one
                            if(lit==nullptr) {
                                if(ev->mode == ParserState::expand) {
                                    // regular URI word, add to lexicon and generate code
                                    lit = pool->newLiteralString(context, ev->t.s);
                                    context = lit->nextNode = pool->newNode();
                                } else if(ev->mode == ParserState::resolve && epc->string!=nullptr) {
                                    GOTO_STATE(expectParameterValue);
                                } else {
                                    if(epc->wild != nullptr) {
                                        // match wildcard
                                        context = epc->wild;

                                        // add remaining URL as argument
                                        ev->request.args.add( Argument(Type("_url", ARG_MASK_STRING), ev->t.original) );

                                        return UriMatchedWildcard;
                                    } else
                                        return NoEndpoint;
                                }
                            } else
                                context = lit->nextNode;

                            NEXT_STATE( expectPathSep );

                        } else if(ev->mode == ParserState::resolve) {
                            GOTO_STATE(expectParameterValue);
                        } else
                            NEXT_STATE( errorExpectedIdentifierOrString );
                    } break;
                    case expectParameterValue: {
                        //assert(ev->args);   // must have collection of args
                        //assert(ev->nargs < ev->_capacity);

                        // try to match a parameter by type
                        if(ev->t.is(TID_STRING, TID_IDENTIFIER) && epc->string!=nullptr) {
                            // we can match by string argument type (parameter match)
                            ev->request.args.add( Argument(*epc->string, ev->t.s) );
                            context = epc->string->nextNode;
                        } else if(ev->t.id==TID_INTEGER && epc->numeric!=nullptr) {
                            // numeric argument
                            ev->request.args.add( Argument(*epc->numeric, (long)ev->t.i) );
                            context = epc->numeric->nextNode;
                        } else if(ev->t.id==TID_FLOAT && epc->numeric!=nullptr) {
                            // numeric argument
                            ev->request.args.add( Argument(*epc->numeric, ev->t.d) );
                            context = epc->numeric->nextNode;
                        } else if(ev->t.id==TID_BOOL && epc->boolean!=nullptr) {
                            // numeric argument
                            ev->request.args.add( Argument(*epc->boolean, ev->t.i>0) );
                            context = epc->boolean->nextNode;
                        } else
                        NEXT_STATE( errorExpectedIdentifierOrString ); // no match by type

                        // successful match, jump to next endpoint node
                        NEXT_STATE( expectPathSep );
                    } break;
                    case expectPathPartOrParam: {
                        if(ev->t.id==':')
                        NEXT_STATE(expectParam)
                        else
                        GOTO_STATE(expectPathPart);
                    } break;
                    case expectParam: {
                        if(ev->t.id==TID_IDENTIFIER) {
                            uint16_t typemask = 0;

                            // copy the token since it has the name
                            Token name;
                            name.swap(ev->t);

                            // read parameter spec
                            if(ev->peek.id=='(') {
                                SCAN;

                                // read parameter types
                                do {
                                    SCAN;
                                    if(ev->t.id!=TID_IDENTIFIER)
                                        return URL_FAIL_SYNTAX;

                                    // convert the string identifier into a typemask
                                    switch(ev->t.toEnum({"integer","real","number","string","boolean","any"}, true)) {
                                        case 0: typemask |= ARG_MASK_INTEGER; break;
                                        case 1: typemask |= ARG_MASK_REAL; break;
                                        case 2: typemask |= ARG_MASK_NUMBER; break;
                                        case 3: typemask |= ARG_MASK_STRING; break;
                                        case 4: typemask |= ARG_MASK_BOOLEAN; break;
                                        case 5: typemask |= ARG_MASK_ANY; break;
                                        default: return URL_FAIL_INVALID_TYPE;
                                    }

                                    SCAN;
                                } while(ev->t.id=='|');

                                // expect closing tag
                                if(ev->t.id !=')')
                                    return URL_FAIL_SYNTAX;
                            } else typemask = ARG_MASK_ANY;

                            // determine the typemask of any already set handlers at this endpoint.
                            // we cannot have two different handlers that handle the same type, but if the typemask
                            // exactly matches we can just consider a match and jump to that endpoint.
                            uint16_t tm_values[3] = { ARG_MASK_NUMBER, ARG_MASK_STRING, ARG_MASK_BOOLEAN };
                            ArgumentType* tm_handlers[3] = { epc->numeric, epc->string, epc->boolean };

                            // we loop through our list of handlers, we save the first non-nullptr handler encountered and
                            // then find more instances of that handler and build a typemask. We set the handlers in our
                            // list of handlers to nullptr for each matching one so eventually we will have all nullptr handlers
                            // and each distinct handler will have been checked.
                            arg = nullptr;
                            while(arg==nullptr && (tm_handlers[0]!=nullptr || tm_handlers[1]!=nullptr || tm_handlers[2]!=nullptr)) {
                                int i;
                                uint16_t tm=0;
                                ArgumentType *x=nullptr;
                                for(i=0; i<sizeof(tm_handlers)/sizeof(tm_handlers[0]); i++) {
                                    if(tm_handlers[i]!=nullptr) {
                                        if(x==nullptr) {
                                            // captured the first non-NULL element
                                            x = tm_handlers[i];
                                            tm |= tm_values[i];
                                            tm_handlers[i]=0;
                                        } else if(x==tm_handlers[i]){
                                            // already encountered the first element, found a second match
                                            tm |= tm_values[i];
                                            tm_handlers[i]=0;
                                        }
                                    }
                                }

                                if(x != nullptr) {
                                    uint16_t _typemask = (uint16_t)(((typemask & ARG_MASK_NUMBER)>0) ? typemask | ARG_MASK_NUMBER : typemask);
                                    assert(tm>0); // must have gotten at least some typemask then
                                    if(tm == _typemask) {
                                        // exact match, we can jump to the endpoint
                                        context = x->nextNode;
                                        arg = x;
                                    } else if((tm & _typemask) >0) {
                                        // uh-oh, user specified a rest endpoint that handles the same type but has differing
                                        // endpoint targets. The actual target will be ambiquous.
                                        return URL_FAIL_AMBIGUOUS_PARAMETER;
                                    }
                                    // otherwise no cross-over type match so this type handler can be ignored
                                }
                            }


                            if(arg == nullptr) {
                                // add the argument to the Endpoint
                                assert( name.indexed );
                                arg = pool->newArgumentType(name.i, typemask);
                                context = arg->nextNode = pool->newNode();

                                if ((typemask & ARG_MASK_NUMBER) > 0) {
                                    // int or real
                                    if (epc->numeric == nullptr)
                                        epc->numeric = arg;
                                }
                                if ((typemask & ARG_MASK_BOOLEAN) > 0) {
                                    // boolean
                                    if (epc->boolean == nullptr)
                                        epc->boolean = arg;
                                }
                                if ((typemask & ARG_MASK_STRING) > 0) {
                                    // string
                                    if (epc->string == nullptr)
                                        epc->string = arg;
                                }
                            }

                            // mark that we encountered a new argument
                            ev->nargs++;
#if 0 // going away
                            if(arg != nullptr) {
                                if(ev->argtypes == nullptr)
                                    ev->argtypes = (Rest::Type*)calloc(ev->_capacity, sizeof(Rest::Type));
                                assert(ev->nargs < ev->_capacity);
                                ev->argtypes[ev->nargs++] = *arg;    // add to list of args we encountered
                            }
#endif

                            NEXT_STATE( expectPathSep );

                            // recursively call parse so we can add more code at the end of this match
                            if((rv=parse(ev))!=0)
                                return rv;
                            return UriMatched;   // inner recursive call would have completed call, so we are done too

                        }
                    } break;

                    case expectEof:
                        return (ev->t.id !=TID_EOF)
                               ? NoEndpoint
                               : UriMatched;
                    case errorExpectedIdentifier:
                        return URL_FAIL_EXPECTED_IDENTIFIER;
                    case errorExpectedIdentifierOrString:
                        return URL_FAIL_EXPECTED_STRING;
                }

                // next token
                SCAN;
            }

            done:
            return UriMatched;
        }
    };
}
