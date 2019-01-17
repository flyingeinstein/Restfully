#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <cassert>
#include <functional>


#include "Token.h"
#include "Argument.h"
#include "Literal.h"
#include "Link.h"
#include "Pool.h"
#include "Parser.h"
#include "handler.h"

// format:    /api/test/:param_name(integer|real|number|string|boolean)/method

namespace Rest {


typedef enum {
  HttpHandler,
  RestHandler
} HandlerPrototype;


/// \defgroup MethodHandlers Associates an http method verb to a handler function
/// \@{
/// \brief Class used to associate a http method verb with a handler function
/// This is used when adding a handler to an endpoint for specific http verbs such as GET, PUT, POST, PATCH, DELETE, etc.
/// Do not use this class, but instead use the template functions  GET(handler), PUT(handler), POST(handler), etc.
#if 0
template<class H>
class MethodHandler {
public:
    HttpMethod method;
    const H& handler;

    MethodHandler(HttpMethod _method, const H& _handler) : method(_method), handler(_handler) {}
};
#endif

#if 1
    //template<class H> Rest::Handler<H::first_argument_type &> _GET(H& handler) { return Rest::Handler<typename function_traits<H>::template argument<0>::type &>(Rest::HttpGet, std::function<int(H&)>(handler)); }
    //template<class H> Rest::Handler<H&> GET(std::function<int(H&)> handler) { return Rest::Handler<H&>(Rest::HttpGet, std::function<int(H&)>(handler)); }

//    namespace Generics {
    template<class H> typename function_traits<H>::HandlerType PUT(H handler) { return typename function_traits<H>::HandlerType(Rest::HttpPut, typename function_traits<H>::FunctionType(handler)); }
    template<class H> typename function_traits<H>::HandlerType POST(H handler) { return typename function_traits<H>::HandlerType(Rest::HttpPost, typename function_traits<H>::FunctionType(handler)); }
    template<class H> typename function_traits<H>::HandlerType PATCH(H handler) { return typename function_traits<H>::HandlerType(Rest::HttpPatch, typename function_traits<H>::FunctionType(handler)); }
    template<class H> typename function_traits<H>::HandlerType DELETE(H handler) { return typename function_traits<H>::HandlerType(Rest::HttpDelete, typename function_traits<H>::FunctionType(handler)); }
    template<class H> typename function_traits<H>::HandlerType OPTIONS(H handler) { return typename function_traits<H>::HandlerType(Rest::HttpOptions, typename function_traits<H>::FunctionType(handler)); }
    template<class H> typename function_traits<H>::HandlerType ANY(H handler) { return typename function_traits<H>::HandlerType(Rest::HttpMethodAny, typename function_traits<H>::FunctionType(handler)); }

    template<class H> typename function_traits<H>::HandlerType GET(H& handler) { return typename function_traits<H>::HandlerType(Rest::HttpGet, typename function_traits<H>::FunctionType(handler)); }
    template<typename H> typename function_traits<H>::HandlerType GET(H&& handler) { return typename function_traits<H>::HandlerType(Rest::HttpGet, handler); }
#if !defined( _LIBCPP_VERSION )
    template<class R, typename... Args, typename... FArgs> Handler<FArgs...> GET(std::_Bind<R(*(FArgs...))(Args...)> handler) { return Handler<FArgs...>(Rest::HttpGet, handler); }
    template<class R, class K, typename... Args, typename... FArgs> Handler<K, FArgs...> GET(std::_Bind<R(K::*(FArgs...))(Args...)> handler) { return Handler<K, FArgs...>(Rest::HttpGet, handler); }
#endif

    // we probably dont need these
    //template<class R, class... Args> Handler<Args...> GET(std::function<R(Args...)> handler) { return Handler<Args...>(Rest::HttpGet, handler); }

#define DEFINE_HTTP_METHOD_HANDLERS(x)
//    }

#else
namespace Generics {
    template<class H> Handler<H&> GET(std::function<int(H&)> handler) { return Handler<H&>(HttpGet, std::move(handler)); }
    template<class H> Handler<H&> PUT(std::function<int(H&)> handler) { return Handler<H&>(HttpPut, handler); }
    template<class H> Handler<H&> POST(std::function<int(H&)> handler) { return Handler<H&>(HttpPost, handler); }
    template<class H> Handler<H&> PATCH(std::function<int(H&)> handler) { return Handler<H&>(HttpPatch, handler); }
    template<class H> Handler<H&> DELETE(std::function<int(H&)> handler) { return Handler<H&>(HttpDelete, handler); }
    template<class H> Handler<H&> OPTIONS(std::function<int(H&)> handler) { return Handler<H&>(HttpOptions, handler); }
    template<class H> Handler<H&> ANY(std::function<int(H&)> handler) { return Handler<H&>(HttpMethodAny, handler); }
}

#define DEFINE_HTTP_METHOD_HANDLER(RequestType, M, e)  \
    inline Rest::Handler<RequestType&> M(std::function<int(RequestType&)> handler) { return Rest::Handler<RequestType&>(Rest::e, std::move(handler)); }
#define DEFINE_HTTP_METHOD_HANDLERS(RequestType)  \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, GET, HttpGet)   \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, PUT, HttpPut)   \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, POST, HttpPost)   \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, PATCH, HttpPatch)   \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, DELETE, HttpDelete)   \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, OPTIONS, HttpOptions)   \
    DEFINE_HTTP_METHOD_HANDLER(RequestType, ANY, HttpMethodAny)
#endif
/// \@}


/// \brief Implements a compiled list of Rest Uri Endpoint expressions
/// Each UriExpression contains one or more Uri Endpoint expressions in a compiled form.
/// You can store expressions for all Rest methods in the application if desired. This
/// compiled byte-code machine can optimally compare and match a request Uri to an
/// Endpoint specification.
template<class THandler>
class Endpoints {
public:
    typedef THandler Handler;
    typedef ::Rest::Argument Argument;

protected:
    class Node;
    typedef Link<Rest::Literal, Node> Literal;
    typedef Link<Rest::Type, Node> ArgumentType;
    typedef ::Rest::Token Token;
    typedef ::Rest::Parser<Node, Rest::Literal, Rest::Argument, Token> Parser;

public:

    class Endpoint : public Arguments {
    public:
        int status;
        std::string name;
        HttpMethod method;
        Handler handler;

        inline Endpoint() :  Arguments(0), status(0), method(HttpMethodAny) {}
        inline Endpoint(HttpMethod _method, const Handler& _handler, int _status) :  Arguments(0), status(_status), method(_method), handler(_handler) {}
        inline Endpoint(const Endpoint& copy) : Arguments(copy), status(copy.status), name(copy.name), method(copy.method), handler(copy.handler)
        {
        }

        inline Endpoint& operator=(const Endpoint& copy) {
            Arguments::operator=(copy);
            status = copy.status;
            name = copy.name;
            method = copy.method;
            handler = copy.handler;
            return *this;
        }

        inline explicit operator bool() const { return status==URL_MATCHED; }

        friend Endpoints;
    };

protected:
    class Node {
    public:
        // if there is more input in parse stream
        Literal *literals;    // first try to match one of these literals

        // if no literal matches, then try to match based on token type
        ArgumentType *string, *numeric, *boolean;

        // if no match is made, we can optionally call a wildcard handler
        Node *wild;

        // if we are at the end of the URI then we can pass to one of the http verb handlers
        Handler *GET, *POST, *PUT, *PATCH, *DELETE, *OPTIONS;

        inline Node() : literals(nullptr), string(nullptr), numeric(nullptr), boolean(nullptr), wild(nullptr),
                        GET(nullptr), POST(nullptr), PUT(nullptr), PATCH(nullptr), DELETE(nullptr), OPTIONS(nullptr)
        {}
    };

public:
    class NodeRef {
        friend class Endpoints;

    public:
        inline NodeRef() : endpoints(nullptr), node(nullptr) {}
        inline NodeRef(const NodeRef& copy) : endpoints(copy.endpoints), node(copy.node) {}

        NodeRef& operator=(const NodeRef& copy) {
            endpoints=copy.endpoints;
            node=copy.node;
            return *this;
        }

        inline operator bool() const { return node!= nullptr && endpoints!= nullptr; }

        inline NodeRef on(const char *endpoint_expression, THandler methodHandler ) {
            if(node!= nullptr && endpoints!= nullptr && endpoints->exception== nullptr) {
                endpoints->on(*node, endpoint_expression, methodHandler);
                return (endpoints->exception == nullptr)
                       ? *this
                       : NodeRef();
            } else
                return *this;   // invalid NodeRef
        }

    protected:
        Endpoints* endpoints;
        Node* node;
        // todo: do we want endpoint->name? we can add it

        inline NodeRef(Endpoints* _endpoints, Node* _node) : endpoints(_endpoints), node(_node) {}
    };

public:
    /// \brief Initialize an empty UriExpression with a maximum number of code size.
    Endpoints()
            : defaultHandler(new Handler()), maxUriArgs(0), exception(nullptr)
    {
    }

    template<class... Targs>
    Endpoints(Targs... args)
            : defaultHandler(new Handler(args...)), maxUriArgs(0), exception(nullptr)
    {
    }

    /// \brief Destroys the RestUriExpression and releases memory
    virtual ~Endpoints() {
        delete exception;
        if(defaultHandler)
            delete defaultHandler;
    }

    Endpoints& onDefault(const Handler& _handler) {
        *defaultHandler = _handler;
        return *this;
    }

    NodeRef from(const char *endpoint_expression) {
        short rs;

        typename Parser::EvalState ev(&parser, &endpoint_expression);
        if(ev.state<0) {
            return NodeRef();
        }

        ev.szargs = 20;
        ev.mode = Parser::expand;         // tell the parser we are adding this endpoint

        if((rs = parser.parse(&ev)) <URL_MATCHED) {
            // parse evaluation error
            return NodeRef();
        } else {
            // if we encountered more args than we did before, then save the new value
            if (ev.nargs > maxUriArgs)
                maxUriArgs = ev.nargs;

            // attach the handler to this endpoint
            Node *epc = ev.ep;
            return NodeRef(this, epc);
        }
    }

    Endpoints& on(const char *endpoint_expression, THandler methodHandler ) {
        return on(parser.getRoot(), endpoint_expression, methodHandler);
    }

#if __cplusplus < 201103L || (defined(_MSC_VER) && _MSC_VER < 1900)
    // for pre-c++11 support we have to specify a number of add handler methods
    inline Endpoints& on(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2 ) {
        on(endpoint_expression, h1);
        return on(endpoint_expression, h2);
    }
    inline Endpoints& on(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2, MethodHandler<Handler> h3 ) {
        on(endpoint_expression, h1, h2);
        return on(endpoint_expression, h3);
    }
    inline Endpoints& on(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2, MethodHandler<Handler> h3, MethodHandler<Handler> h4 ) {
        on(endpoint_expression, h1, h2, h3);
        return on(endpoint_expression, h4);
    }
    inline Endpoints& on(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2, MethodHandler<Handler> h3, MethodHandler<Handler> h4, MethodHandler<Handler> h5 ) {
        on(endpoint_expression, h1, h2, h3, h4);
        return on(endpoint_expression, h5);
    }
    inline Endpoints& on(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2, MethodHandler<Handler> h3, MethodHandler<Handler> h4, MethodHandler<Handler> h5, MethodHandler<Handler> h6 ) {
        on(endpoint_expression, h1, h2, h3, h4, h5);
        return on(endpoint_expression, h6);
    }
    inline Endpoints& on(const char *endpoint_expression, MethodHandler<Handler> h1, MethodHandler<Handler> h2, MethodHandler<Handler> h3, MethodHandler<Handler> h4, MethodHandler<Handler> h5, MethodHandler<Handler> h6, MethodHandler<Handler> h7 ) {
        on(endpoint_expression, h1, h2, h3, h4, h5, h6);
        return on(endpoint_expression, h7);
    }
#else
    // c++11 using parameter pack expressions to recursively call add()
    template<class T, class... Targs>
    Endpoints& on(const char *endpoint_expression, T h1, Targs... rest ) {
        on(endpoint_expression, h1);   // add first argument
        return on(endpoint_expression, rest...);   // add the rest (recursively)
    }
#endif

    inline Endpoints& katch(const std::function<void(Endpoint)>& endpoint_exception_handler) {
        if(exception != nullptr) {
            endpoint_exception_handler(*exception);
            delete exception;
            exception = nullptr;
        }
        return *this;
    }

    /// \brief Match a Uri against the compiled list of Uri expressions.
    /// If a match is found with an associated http method handler, the resolved UriEndpoint object is filled in.
    Endpoints::Endpoint resolve(HttpMethod method, const char *uri) {
        short rs;
        typename Parser::EvalState ev(&parser, &uri);
        if(ev.state<0)
            return Endpoint(method, *defaultHandler, URL_FAIL_INTERNAL);
        ev.mode = Parser::resolve;
        ev.args = new Argument[ev.szargs = maxUriArgs+1];

        // parse the input
        if((rs=parser.parse( &ev )) >=URL_MATCHED) {
            // successfully resolved the endpoint
            Endpoint endpoint;
            Handler* handler;

            switch(method) {
                case HttpGet: handler = ev.ep->GET; break;
                case HttpPost: handler = ev.ep->POST; break;
                case HttpPut: handler = ev.ep->PUT; break;
                case HttpPatch: handler = ev.ep->PATCH; break;
                case HttpDelete: handler = ev.ep->DELETE; break;
                case HttpOptions: handler = ev.ep->OPTIONS; break;
                default: handler = defaultHandler;
            }

            if(handler !=nullptr) {
                endpoint.status = URL_MATCHED;
                endpoint.handler = *handler;
                endpoint.name = ev.methodName;
                endpoint.method = method;
                endpoint.args = ev.args;
                endpoint.nargs = ev.nargs;
            } else {
                endpoint.handler = *defaultHandler;
                endpoint.status = URL_FAIL_NO_HANDLER;
            }
            return endpoint;
        } else {
            // cannot resolve
            return Endpoint(method, *defaultHandler, rs);
        }
        // todo: we need to release memory from EV struct
    }

    protected:
        /// \brief Parse and add single Uri Endpoint expressions to our list of Endpoints
        Endpoints& on(Node& node, const char *endpoint_expression, THandler methodHandler ) {
            short rs;

            // if exception was set, abort
            if(exception != nullptr)
                return *this;

            typename Parser::EvalState ev(&parser, &endpoint_expression);
            if(ev.state<0) {
                exception = new Endpoint(methodHandler.method, *defaultHandler, URL_FAIL_INTERNAL);
                return *this;
            }

            ev.ep = &node;
            ev.szargs = 20;
            ev.mode = Parser::expand;         // tell the parser we are adding this endpoint

            if((rs = parser.parse(&ev)) <URL_MATCHED) {
                //printf("parse-eval-error %d   %s\n", rs, ev.uri);
                exception = new Endpoint(methodHandler.method, *defaultHandler, rs);
                exception->name = endpoint_expression;
                return *this;
            } else {
                // if we encountered more args than we did before, then save the new value
                if(ev.nargs > maxUriArgs)
                    maxUriArgs = ev.nargs;

                // attach the handler to this endpoint
                Node* epc = ev.ep;
                Endpoint endpoint;
                endpoint.name = ev.methodName;

                if(methodHandler.method == HttpMethodAny) {
                    // attach to all remaining method handlers
                    Handler* h = new Handler(methodHandler.handler);
                    short matched = 0;
                    if(!epc->GET) { epc->GET = h; matched++; }
                    if(!epc->POST) { epc->POST = h; matched++; }
                    if(!epc->PUT) { epc->PUT = h; matched++; }
                    if(!epc->PATCH) { epc->PATCH = h; matched++; }
                    if(!epc->DELETE) { epc->DELETE = h; matched++; }
                    if(!epc->GET) { epc->GET = h; matched++; }
                    if(matched ==0)
                        delete h;   // no unset methods, but not considered an error
                    return *this; // successfully added
                } else {
                    Handler** target = nullptr;

                    // get a pointer to the Handler member variable from the node
                    switch(methodHandler.method) {
                        case HttpGet: target = &epc->GET; break;
                        case HttpPost: target = &epc->POST; break;
                        case HttpPut: target = &epc->PUT; break;
                        case HttpPatch: target = &epc->PATCH; break;
                        case HttpDelete: target = &epc->DELETE; break;
                        case HttpOptions: target = &epc->OPTIONS; break;
                        default:
                            exception = new Endpoint(methodHandler.method, *defaultHandler, URL_FAIL_INTERNAL); // unknown method type
                            exception->name = endpoint_expression;
                            return *this;
                    }

                    if(target !=nullptr) {
                        // set the target method handler but error if it was already set by a previous endpoint declaration
                        if(*target !=nullptr ) {
                            //fprintf(stderr, "fatal: endpoint %s %s was previously set to a different handler\n",
                            //        uri_method_to_string(methodHandler.method), endpoint_expression);
                            exception = new Endpoint(methodHandler.method, *defaultHandler, URL_FAIL_DUPLICATE);
                            exception->name = endpoint_expression;
                            return *this;
                        } else {
                            *target = new Handler(methodHandler.handler);
                            return *this; // successfully added
                        }
                    }
                }
            }
            return *this;
        }

    public:
    Handler* defaultHandler; // like a 404 handler

protected:
    // some statistics on the endpoints
    size_t maxUriArgs;       // maximum number of embedded arguments on any one endpoint expression

        /// \brief The parser splits the URL into a tree expression where handlers can be atttached to any branch
        /// or endpoint in the tree.
    Parser parser;

    /// \brief if an error occurs during an add() this member will be set
    /// all further add() calls will instantly return without adding Endpoints. Use katch() member to handle this
    /// exception at some point after one or more add() calls.
    Endpoint* exception;
};

} // ns:Rest

using Rest::GET;
using Rest::POST;
using Rest::PUT;
using Rest::PATCH;
using Rest::DELETE;
using Rest::OPTIONS;
using Rest::ANY;

