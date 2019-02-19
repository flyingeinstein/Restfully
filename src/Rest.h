#pragma once

#include <stdint.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <cassert>
#include <functional>


#include "Token.h"
#include "Node.h"
#include "Argument.h"
#include "Literal.h"
#include "Link.h"
#include "Pool.h"
#include "Parser.h"
#include "handler.h"

// format:    /api/test/:param_name(integer|real|number|string|boolean)/method

namespace Rest {

/// \brief Implements a compiled list of Rest Uri Endpoint expressions
/// Each UriExpression contains one or more Uri Endpoint expressions in a compiled form.
/// You can store expressions for all Rest methods in the application if desired. This
/// compiled byte-code machine can optimally compare and match a request Uri to an
/// Endpoint specification.
template<class THandler = std::function<short(void)> >
class Endpoints {
public:
    typedef THandler Handler;

    using Literal = Rest::Literal;
    using ArgumentType = Rest::Type;
    using Argument = Rest::Argument;
    using NodeData = Rest::NodeData<Literal, ArgumentType, THandler>;

    using Token = Rest::Token;
    using Parser = Rest::Parser<NodeData, Token, Pool<NodeData> >;


public:
    class Node : public Rest::Node<NodeData, THandler> {
    public:
        using Super = Rest::Node<NodeData, THandler>;

        using Super::attach;
        using Super::exception;

        // test to ensure the base class has an attach(HttpMethod, THandler) method
        static_assert(
                has_method<Super, attach_caller, void(HttpMethod method, THandler handler) >::value,
                "Node class should have attach method with signature void(HttpMethod,THandler)");

        template<typename ... TArgs>
        Node(Endpoints* _endpoints, TArgs ... args) : endpoints(_endpoints), Super(args...) {}

        Node(const Node& copy) : endpoints(copy.endpoints), Super(copy) {}

        Node& operator=(const Node& copy) {
            endpoints = copy.endpoints;
            Super::operator=(copy);
            return *this;
        }

        inline Node on(const char *endpoint_expression ) {
            if(Super::node!=nullptr && endpoints!=nullptr && endpoints->exception==0) {
                return endpoints->on(Super::node, endpoint_expression);
            } else {
                // todo: set exception here
                return Node(endpoints, URL_FAIL_NO_ENDPOINT);   // invalid NodeRef
            }
        }

        template<typename H> inline Node& GET(H handler) { attach(HttpGet, handler); return *this; }
        template<typename H> inline Node& PUT(H handler) { attach(HttpPut, handler); return *this; }
        template<typename H> inline Node& PATCH(H handler) { attach(HttpPatch, handler); return *this; }
        template<typename H> inline Node& POST(H handler) { attach(HttpPost, handler); return *this; }
        template<typename H> inline Node& DELETE(H handler) { attach(HttpDelete, handler); return *this; }
        template<typename H> inline Node& OPTIONS(H handler) { attach(HttpOptions, handler); return *this; }
        template<typename H> inline Node& ANY(H handler) { attach(HttpMethodAny, handler); return *this; }

        template<typename H> inline Node& GET(const char* expr, H handler) { attach(expr, HttpGet, handler); return *this; }
        template<typename H> inline Node& PUT(const char* expr, H handler) { attach(expr, HttpPut, handler); return *this; }
        template<typename H> inline Node& PATCH(const char* expr, H handler) { attach(expr, HttpPatch, handler); return *this; }
        template<typename H> inline Node& POST(const char* expr, H handler) { attach(expr, HttpPost, handler); return *this; }
        template<typename H> inline Node& DELETE(const char* expr, H handler) { attach(expr, HttpDelete, handler); return *this; }
        template<typename H> inline Node& OPTIONS(const char* expr, H handler) { attach(expr, HttpOptions, handler); return *this; }
        template<typename H> inline Node& ANY(const char* expr, H handler) { attach(expr, HttpMethodAny, handler); return *this; }

        template<class HandlerT>
        inline void attach(const char* expr, HttpMethod method, HandlerT handler ) {
            if (Super::node != nullptr) {
                Node r = on(expr);
                r.attach(method, handler);
                if(r.exception!=0)
                    exception = r.exception;
            }
        }

    protected:
        Endpoints* endpoints;
    };

    using Exception = typename Node::Exception;


    class Endpoint : public Arguments {
    public:
        int status;
        std::string name;
        HttpMethod method;
        Handler handler;

        inline Endpoint() :  Arguments(0), status(0), method(HttpMethodAny) {}
        inline Endpoint(HttpMethod _method, const Handler& _handler, int _status) :  Arguments(0), status(_status), method(_method), handler(_handler) {}
        inline Endpoint(const Endpoint& copy) : Arguments(copy), status(copy.status), name(copy.name), method(copy.method), handler(copy.handler) {}

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

    Endpoints& onDefault(const Handler& _handler) { *defaultHandler = _handler; return *this; }

    inline Node on(const char *endpoint_expression) { return on(nullptr, endpoint_expression); }

    /// \brief Match a Uri against the compiled list of Uri expressions.
    /// If a match is found with an associated http method handler, the resolved UriEndpoint object is filled in.
    Endpoints::Endpoint resolve(HttpMethod method, const char *uri) {
        short rs;
        Argument args[maxUriArgs+1];
        typename Parser::EvalState ev(&parser, &uri);
        if(ev.state<0)
            return Endpoint(method, *defaultHandler, URL_FAIL_INTERNAL);
        ev.mode = Parser::resolve;
        ev.szargs = maxUriArgs+1;
        ev.args = args;

        // parse the input
        if((rs=parser.parse( &ev )) >=URL_MATCHED) {
            // successfully resolved the endpoint
            Endpoint endpoint;
            Handler& handler = ev.ep->handle(method);
            if(handler !=nullptr) {
                endpoint.status = URL_MATCHED;
                endpoint.handler = handler;
                endpoint.name = ev.methodName;
                endpoint.method = method;
                endpoint.nargs = ev.nargs;
                endpoint.args = new Argument[ ev.nargs ];
                for(int i=0; i<ev.nargs; i++)
                    endpoint.args[i] = ev.args[i];
            } else {
                endpoint.handler = *defaultHandler;
                endpoint.status = URL_FAIL_NO_HANDLER;
            }
            return endpoint;
        } else {
            // cannot resolve
            return Endpoint(method, *defaultHandler, rs);
        }
    }

    protected:
        /// \brief Parse and add single Uri Endpoint expressions to our list of Endpoints
        Node on(NodeData* node, const char *endpoint_expression) {
            short rs;

            if(node==nullptr || (*endpoint_expression == '/')) {
                node = &parser.getRoot();
                if(*endpoint_expression == '/')
                    endpoint_expression++;
            }

            typename Parser::EvalState ev(&parser, &endpoint_expression);
            ev.szargs = 20;
            ev.mode = Parser::expand;         // tell the parser we are adding this endpoint
            ev.ep = node;

            if((rs = parser.parse(&ev)) <URL_MATCHED) {
                return Node(this, URL_FAIL_SYNTAX);
            } else {
                // if we encountered more args than we did before, then save the new value
                if(ev.nargs > maxUriArgs)
                    maxUriArgs = ev.nargs;

                // attach the handler to this endpoint
                return Node(this, ev.ep);
            }
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
