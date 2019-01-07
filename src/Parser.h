//
// Created by Colin MacKenzie on 2018-11-09.
//

#pragma once

#include "Token.h"

#define URL_MATCHED                         0
#define URL_MATCHED_WILDCARD                1
#define URL_FAIL_NO_ENDPOINT                (-1)
#define URL_FAIL_NO_HANDLER                 (-2)
#define URL_FAIL_DUPLICATE                  (-3)
#define URL_FAIL_PARAMETER_TYPE             (-4)
#define URL_FAIL_MISSING_PARAMETER          (-5)
#define URL_FAIL_AMBIGUOUS_PARAMETER        (-6)
#define URL_FAIL_EXPECTED_PATH_SEPARATOR    (-7)
#define URL_FAIL_EXPECTED_EOF               (-8)
#define URL_FAIL_INVALID_TYPE               (-9)
#define URL_FAIL_SYNTAX                     (-10)
#define URL_FAIL_INTERNAL                   (-15)
#define URL_FAIL_INTERNAL_BAD_STRING        (-16)

namespace Rest {

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

#define GOTO_STATE(st) { ev->state = st; goto rescan; }
#define NEXT_STATE(st) { ev->state = st; }
#define SCAN { ev->t.clear(); ev->t.swap( ev->peek ); ev->peek.scan(&ev->uri, 1); }

    template<
            class TNode,
            class TLiteral,
            class TArgument,
            class TToken=Token,
            class TPool=Pool<TNode, Link<TLiteral, TNode>, Link<typename TArgument::Type, TNode> >
    >
    class Parser
    {
    public:
        typedef TNode Node;
        typedef TToken Token;
        typedef TArgument Argument;
        typedef Link<TLiteral, Node> Literal;
        typedef Link<typename TArgument::Type, Node> ArgumentType;

        typedef enum {
            expand = 1,           // indicates adding a new endpoint/handler
            resolve = 2        // indicates we are resolving a URL to a defined handler
        } mode_e;

        /// \brief Contains state for resolving or expanding a Url expression tree
        class EvalState {
        public:
            mode_e mode;   // indicates if we are parsing to resolve or to add an endpoint

            // parser data
            const char *uri;    // parser input string (gets eaten as parsing occurs)
            Token t, peek;      // current token 't' and look-ahead token 'peek'

            int state;          // parser state machine current state
            int level;          // level of evaluation, typically 1 level per path separation

            Node* ep;      // current endpoint evaluated

            // holds the current method name
            // the name is generated as we are parsing the URL
            char methodName[2048];
            char *pmethodName;

            // will contain the arguments embedded in the URL.
            // when adding, will contain argument type info
            // when resolving, will contain argument values
            ArgumentType** argtypes;
            Argument* args;
            size_t nargs;
            size_t szargs;

            EvalState(Parser* _expr, const char** _uri)
                    : mode(resolve), uri(nullptr), state(0), level(0), ep( _expr->root ),
                      pmethodName(methodName), argtypes(nullptr), args(nullptr), nargs(0), szargs(0)
            {
                methodName[0]=0;
                if(_uri != nullptr) {
                    // scan first token
                    if (!t.scan(_uri, 1))
                        goto bad_eval;
                    if (!peek.scan(_uri, 1))
                        goto bad_eval;
                }
                uri = *_uri;
                return;
                bad_eval:
                state = -1;
            }
        };

    protected:
        TPool pool;
        Node* root;

    public:
        Parser() : root(pool.newNode())
        {
        }

        /// \brief Parses a url and either adds or resolves within the expression tree
        /// The Url and parse mode are set in ParseData and determine if parse() returns when expression tree hits a dead-end
        /// or if it starts expanding the expression tree.
        short parse(EvalState* ev)
        {
            short rv;
            long wid;
            Node* epc = ev->ep;
            Literal* lit;
            ArgumentType* arg;

            // read datatype or decl type
            while(ev->t.id!=TID_EOF) {
                rescan:
                epc = ev->ep;

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
                            NEXT_STATE( (ev->mode==resolve) ? expectPathPart : expectPathPartOrParam);

                            // add seperator to method name
                            if(ev->pmethodName > ev->methodName && *(ev->pmethodName-1)!='/')
                                *ev->pmethodName++ = '/';
                        } else if(ev->t.id == TID_EOF) {
                            // safe place to end
                            goto done;
                        }
                    } break;
                    case expectHtmlSuffix: {
                        if(ev->t.is(TID_STRING , TID_IDENTIFIER)) {
                            if(strcasecmp(ev->t.s, "html") !=0) {
                                return URL_FAIL_NO_ENDPOINT;    // only supports no suffix, or html suffix
                            } else
                            NEXT_STATE(expectEof);  // always expect eof after suffix
                        }
                    } break;
                    case expectPathPart: {
                        if(ev->mode == expand && ev->t.is(TID_WILDCARD)) {
                            // encountered wildcard, must be last token
                            if(epc->wild==nullptr) {
                                ev->ep = epc->wild = pool.newNode();
                            } else {
                                ev->ep = epc->wild;
                            }
                            *ev->pmethodName++ = '*';
                            *ev->pmethodName = 0;
                            return ev->peek.is(TID_EOF)
                                ? URL_MATCHED_WILDCARD
                                : URL_FAIL_SYNTAX;
                        }
                        else if(ev->t.is(TID_STRING, TID_IDENTIFIER)) {
                            // we must see if we already have a literal with this name
                            lit = nullptr;
                            wid = binbag_find_nocase(pool.text, ev->t.s);
                            if(wid>=0 && epc->literals) {
                                // word exists in dictionary, see if it is a literal of current endpoint
                                lit = epc->literals;
                                while(lit->isValid() && lit->id!=wid)
                                    lit++;
                                if(lit->id!=wid)
                                    lit=nullptr;
                            }

                            // add the new literal if we didnt find an existing one
                            if(lit==nullptr) {
                                if(ev->mode == expand) {
                                    // regular URI word, add to lexicon and generate code
                                    lit = pool.addLiteralString(ev->ep, ev->t.s);
                                    ev->ep = lit->next = pool.newNode();
                                } else if(ev->mode == resolve && epc->string!=nullptr) {
                                    GOTO_STATE(expectParameterValue);
                                } else {
                                    if(epc->wild != nullptr) {
                                        // match wildcard
                                        ev->ep = epc->wild;

                                        // complete method name
                                        *ev->pmethodName++ = '*';
                                        *ev->pmethodName = 0;

                                        // add remaining URL as argument
                                        ev->args[ev->nargs++] = Argument(Type("_url", ARG_MASK_STRING), ev->t.original);

                                        return URL_MATCHED_WILDCARD;
                                    } else
                                        return URL_FAIL_NO_ENDPOINT;
                                }
                            } else
                                ev->ep = lit->next;

                            NEXT_STATE( expectPathSep );

                            // add component to method name
                            strcpy(ev->pmethodName, ev->t.s);
                            ev->pmethodName += strlen(ev->t.s);
                        } else if(ev->mode == resolve) {
                            GOTO_STATE(expectParameterValue);
                        } else
                            NEXT_STATE( errorExpectedIdentifierOrString );
                    } break;
                    case expectParameterValue: {
                        const char* _typename=nullptr;
                        assert(ev->args);   // must have collection of args
                        assert(ev->nargs < ev->szargs);

                        // try to match a parameter by type
                        if(ev->t.is(TID_STRING, TID_IDENTIFIER) && epc->string!=nullptr) {
                            // we can match by string argument type (parameter match)
                            assert(ev->args);
                            ev->args[ev->nargs++] = Argument(*epc->string, ev->t.s);
                            ev->ep = epc->string->next;
                            _typename = "string";
                        } else if(ev->t.id==TID_INTEGER && epc->numeric!=nullptr) {
                            // numeric argument
                            ev->args[ev->nargs++] = Argument(*epc->numeric, (long)ev->t.i);
                            ev->ep = epc->numeric->next;
                            _typename = "int";
                        } else if(ev->t.id==TID_FLOAT && epc->numeric!=nullptr) {
                            // numeric argument
                            ev->args[ev->nargs++] = Argument(*epc->numeric, ev->t.d);
                            ev->ep = epc->numeric->next;
                            _typename = "float";
                        } else if(ev->t.id==TID_BOOL && epc->boolean!=nullptr) {
                            // numeric argument
                            ev->args[ev->nargs++] = Argument(*epc->boolean, ev->t.i>0);
                            ev->ep = epc->boolean->next;
                            _typename = "boolean";
                        } else
                        NEXT_STATE( errorExpectedIdentifierOrString ); // no match by type

                        // add component to method name
                        *ev->pmethodName++ = '<';
                        strcpy(ev->pmethodName, _typename);
                        ev->pmethodName += strlen(_typename);
                        *ev->pmethodName++ = '>';
                        *ev->pmethodName = 0;

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
                            char name[32];
                            memset(&name, 0, sizeof(name));
                            strcpy(name, ev->t.s);

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
                                        ev->ep = x->next;
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
                                arg = pool.newArgumentType(name, typemask);
                                ev->ep = arg->next = pool.newNode();

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

                            // save arg to EV data
                            if(arg != nullptr) {
                                if(ev->argtypes == nullptr)
                                    ev->argtypes = (ArgumentType**)calloc(ev->szargs, sizeof(ArgumentType));
                                assert(ev->nargs < ev->szargs);
                                ev->argtypes[ev->nargs++] = arg;    // add to list of args we encountered

                            }

                            NEXT_STATE( expectPathSep );

                            // recursively call parse so we can add more code at the end of this match
                            if((rv=parse(ev))!=0)
                                return rv;
                            return 0;   // inner recursive call would have completed call, so we are done too

                        }
                    } break;

                    case expectEof:
                        return (ev->t.id !=TID_EOF)
                               ? (short)URL_FAIL_NO_ENDPOINT
                               : (short)0;
                    case errorExpectedIdentifier:
                        return -2;
                    case errorExpectedIdentifierOrString:
                        return -3;
                }

                // next token
                SCAN;
            }

            // set the method name
            if (*(ev->pmethodName - 1) == '/')
                ev->pmethodName--;
            *ev->pmethodName = 0;

            done:
            return URL_MATCHED;
        }
    };
}
