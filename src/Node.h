//
// Created by Colin MacKenzie on 2019-02-04.
//

#ifndef RESTFULLY_NODE_H
#define RESTFULLY_NODE_H

#include "Link.h"
#include "Exception.h"
#include "handler.h"
#include "Literal.h"
#include "Argument.h"
#include "Parser.h"


namespace Rest {

    template<class THandler, class TLiteral = Rest::Literal, class TArgumentType = Rest::Type>
    class NodeData {
    public:
        using LiteralType = Link<TLiteral, NodeData>;
        using ArgumentType = Link<TArgumentType, NodeData>;
        using HandlerType = THandler;

        // if there is more input in parse stream
        LiteralType *literals;    // first try to match one of these literals

        // if no literal matches, then try to match based on token type
        ArgumentType *string, *numeric, *boolean;

        // if no match is made, we can optionally call a wildcard handler
        NodeData *wild;

        // if we are at the end of the URI then we can pass to one of the http verb handlers
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

        void attach(HttpMethod method, const NodeData::HandlerType& handler) {
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

    template<class TEndpoints>
    class Node {
    public:
        using Endpoints = TEndpoints;
        using NodeData = typename TEndpoints::NodeData;
        using Handler = typename TEndpoints::Handler;
        using Endpoint = typename TEndpoints::Endpoint;

        using Exception = Exception<Node>;
        using Parser = Rest::Parser< NodeData, Endpoints, Rest::Token >;

        // test to ensure the base class has an attach(HttpMethod, THandler) method
        //static_assert(
        //        has_method<Super, attach_caller, void(HttpMethod method, THandler handler) >::value,
        //        "Node class should have attach method with signature void(HttpMethod,THandler)");

        inline Node() : _endpoints(nullptr), _node(nullptr), _exception(URL_FAIL_NULL_ROOT) {}
        explicit inline Node(Endpoints* endpoints) : _endpoints(endpoints), _node(nullptr), _exception(0) {}
        explicit inline Node(Endpoints* endpoints, NodeData* node) : _endpoints(endpoints), _node(node), _exception(0) {}
        explicit inline Node(Endpoints* endpoints, int _exception) : _endpoints(endpoints), _node(nullptr), _exception(_exception) {}

        inline Node(const Node& copy) : _endpoints(copy._endpoints), _node(copy._node), _exception(copy._exception) {}

        Node& operator=(const Node& copy) {
            _node=copy._node;
            _endpoints = copy._endpoints;
            _exception = copy._exception;
            return *this;
        }

        inline operator bool() const { return _node!=nullptr; }

        // can convert directly to the nodedata we point to
        inline operator const NodeData*() const { return _node; }
        inline operator NodeData*() { return _node; }

        // smartptr like operation
        inline const NodeData* operator->() const { return _node; }
        inline NodeData* operator->() { return _node; }

        inline int error() const { return _exception; }

        // todo: implement node::name()
        std::string name() const {
            return "todo:name";
        }

        inline const Endpoints* endpoints() const { return _endpoints; }
        inline Endpoints* endpoints() { return _endpoints; }

        inline Node& katch(const std::function<void(Exception)>& endpoint_exception_handler) {
            if(_exception>0) {
                endpoint_exception_handler(Exception(*this, _exception));
                _exception = 0;
            }
            return *this;
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

        inline void attach(HttpMethod method, Handler handler ) {
            if(_node!= nullptr)
                _node->attach(method, handler);
        }

        template<class HandlerT>
        inline void attach(const char* expr, HttpMethod method, HandlerT handler ) {
            if (_node != nullptr) {
                Node r = on(expr);
                r.attach(method, handler);
                if(r._exception!=0)
                    _exception = r._exception;
            }
        }

        /// \brief Parse and add single Uri Endpoint expressions to our list of Endpoints
        Node on(const char *endpoint_expression) {
            short rs;
            Parser parser(_endpoints);

            if(_node==nullptr || _endpoints==nullptr)
                return Node(_endpoints, _exception = URL_FAIL_NULL_ROOT);   // invalid NodeRef
            else if(_exception!=0)
                return Node(_endpoints, _exception );   // invalid NodeRef
            else if(*endpoint_expression == '/')
                // change to expression from absolute root node
                return _endpoints->getRoot().on(endpoint_expression+1);


            typename Parser::EvalState ev(&parser, _node, &endpoint_expression);
            ev.szargs = 20;
            ev.mode = Parser::expand;         // tell the parser we are adding this endpoint

            if((rs = parser.parse(&ev)) <URL_MATCHED) {
                return Node(_endpoints, URL_FAIL_SYNTAX);
            } else {
                // if we encountered more args than we did before, then save the new value
                if(ev.nargs > _endpoints->maxUriArgs)
                    _endpoints->maxUriArgs = ev.nargs;

                // attach the handler to this endpoint
                return Node(_endpoints, ev.ep);
            }
        }

        /// \brief Match a Uri against the compiled list of Uri expressions.
        /// If a match is found with an associated http method handler, the resolved UriEndpoint object is filled in.
        Endpoint resolve(HttpMethod method, const char *uri) {
            short rs;
            Parser parser(_endpoints);

            Argument args[_endpoints->maxUriArgs+1];
            typename Parser::EvalState ev(&parser, _node, &uri);
            if(ev.state<0)
                return Endpoint(method, URL_FAIL_INTERNAL);
            ev.mode = Parser::resolve;
            ev.szargs = _endpoints->maxUriArgs+1;
            ev.args = args;

            // parse the input
            if((rs=parser.parse( &ev )) >=URL_MATCHED) {
                // successfully resolved the endpoint
                Handler& handler = ev.ep->handle(method);
                if(handler !=nullptr) {
                    Endpoint endpoint(method, handler, URL_MATCHED, ev.args, ev.nargs);
                    endpoint.name = ev.methodName;
                    return endpoint;
                } else {
                    return Endpoint(method, URL_FAIL_NO_HANDLER);
                }
            } else {
                // cannot resolve
                return Endpoint(method, rs);
            }
        }

    protected:
        Endpoints* _endpoints;
        NodeData* _node;
        int _exception;
    };

}

#endif //RESTFULLY_NODE_H
