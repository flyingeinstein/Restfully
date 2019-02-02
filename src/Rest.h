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

/// \brief Implements a compiled list of Rest Uri Endpoint expressions
/// Each UriExpression contains one or more Uri Endpoint expressions in a compiled form.
/// You can store expressions for all Rest methods in the application if desired. This
/// compiled byte-code machine can optimally compare and match a request Uri to an
/// Endpoint specification.
template<class THandler = std::function<short(void)> >
class Endpoints {
public:
    typedef THandler Handler;
    typedef ::Rest::Argument Argument;

protected:
    class NodeData;
    typedef Link<Rest::Literal, NodeData> Literal;
    typedef Link<Rest::Type, NodeData> ArgumentType;
    typedef ::Rest::Token Token;
    typedef ::Rest::Parser<NodeData, Rest::Literal, Rest::Argument, Token> Parser;

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
    class NodeData {
    public:
        // if there is more input in parse stream
        Literal *literals;    // first try to match one of these literals

        // if no literal matches, then try to match based on token type
        ArgumentType *string, *numeric, *boolean;

        // if no match is made, we can optionally call a wildcard handler
        NodeData *wild;

        // if we are at the end of the URI then we can pass to one of the http verb handlers
        using HandlerType = Handler;
        HandlerType GET, POST, PUT, PATCH, DELETE, OPTIONS;

        inline NodeData() : literals(nullptr), string(nullptr), numeric(nullptr), boolean(nullptr), wild(nullptr),
                        GET(nullptr), POST(nullptr), PUT(nullptr), PATCH(nullptr), DELETE(nullptr), OPTIONS(nullptr)
        {}

        inline bool isSet(const HandlerType& h) const { return h != nullptr; }

        NodeData::HandlerType& handle(HttpMethod method) {
            // get a pointer to the Handler member variable from the node
            switch(method) {
                case HttpGet: return GET;
                case HttpPost: return POST;
                case HttpPut: return PUT;
                case HttpPatch: return PATCH;
                case HttpDelete: return DELETE;
                case HttpOptions: return OPTIONS;
                default: return GET;
            }
        }

        void handle(HttpMethod method, const NodeData::HandlerType& handler) {
            // get a pointer to the Handler member variable from the node
            switch(method) {
                case HttpGet: GET = handler; break;
                case HttpPost: POST = handler; break;
                case HttpPut: PUT = handler; break;
                case HttpPatch: PATCH = handler; break;
                case HttpDelete: DELETE = handler; break;
                case HttpOptions: OPTIONS = handler; break;
                case HttpMethodAny:
                    if(!isSet(GET)) GET = handler;
                    if(!isSet(POST)) POST = handler;
                    if(!isSet(PUT)) PUT = handler;
                    if(!isSet(PATCH)) PATCH = handler;
                    if(!isSet(DELETE)) DELETE = handler;
                    if(!isSet(OPTIONS)) OPTIONS = handler;
                    break;
            }
        }

    };

public:
    class Exception;

    class Node {
        friend class Endpoints;

    public:
        inline Node() : endpoints(nullptr), node(nullptr), exception(0) {}
        inline Node(const Node& copy) : endpoints(copy.endpoints), node(copy.node), exception(copy.exception) {}

        Node& operator=(const Node& copy) {
            endpoints=copy.endpoints;
            node=copy.node;
            exception = copy.exception;
            return *this;
        }

        inline operator bool() const { return node!=nullptr && endpoints!=nullptr; }

        inline Node on(const char *endpoint_expression ) {
            if(node!=nullptr && endpoints!=nullptr && endpoints->exception==0) {
                return endpoints->on(node, endpoint_expression);
            } else {
                // todo: set exception here
                return Node(endpoints, URL_FAIL_NO_ENDPOINT);   // invalid NodeRef
            }
        }

        inline int error() const { return exception; }

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

        inline void attach(HttpMethod method, THandler handler ) {
            if(node!= nullptr && endpoints!= nullptr)
                node->handle(method, handler);
        }

        inline void attach(const char* expr, HttpMethod method, THandler handler ) {
            if (node != nullptr && endpoints != nullptr) {
                Node r = on(expr);
                r.attach(method, handler);
                if(r.exception!=0)
                    exception = r.exception;
            }
        }

        inline Node& katch(const std::function<void(Exception)>& endpoint_exception_handler) {
            if(exception>0) {
                endpoint_exception_handler(Exception(*this, exception));
                exception = 0;
            }
            return *this;
        }

        // todo: implement node::name()
        std::string name() const {
            return "todo:name";
        }

    protected:
        Endpoints* endpoints;
        NodeData* node;
        int exception;
        // todo: do we want endpoint->name? we can add it

        inline Node(Endpoints* _endpoints, int _exception) : endpoints(_endpoints), node(nullptr), exception(_exception) {}
        inline Node(Endpoints* _endpoints, NodeData* _node) : endpoints(_endpoints), node(_node), exception(0) {}
    };

    class Exception {
    public:
        Node node;
        int code;

        inline Exception(Node _node, int _code) : node(_node), code(_code) {}
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

    Node on(const char *endpoint_expression) {
        return on(nullptr, endpoint_expression);
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
            Handler& handler = ev.ep->handle(method);
            if(handler !=nullptr) {
                endpoint.status = URL_MATCHED;
                endpoint.handler = handler;
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
        Node on(NodeData* node, const char *endpoint_expression) {
            short rs;

            if(node==nullptr || (*endpoint_expression == '/')) {
                node = &parser.getRoot();
                if(*endpoint_expression == '/')
                    endpoint_expression++;
            }

            // determine if expression is a relative or absolute expression
            /*if(*endpoint_expression == '/') {
                // start at root node
                node.node = &parser.getRoot();
                endpoint_expression++;
            }*/

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
