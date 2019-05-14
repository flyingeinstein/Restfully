//
// Created by Colin MacKenzie on 2019-02-04.
//

#ifndef RESTFULLY_NODE_H
#define RESTFULLY_NODE_H

#include "Mixins.h"
#include "Exception.h"
#include "Literal.h"
#include "Parser.h"


namespace Rest {

    template<class TNode> struct NodeLink {
        TNode* nextNode;
    };

    template<class THandler, class TLiteral = Rest::Literal, class TArgumentType = Rest::Type>
    class NodeData {
    public:
        /// links to other nodes are of this type. These links are used in Literals, Arguments and Externals to traverse
        /// to the next node.
        using Link = NodeLink<NodeData>;

        /// Construct a type for a linked list of Literals that are non-argument part of a URI.
        /// If a literal is matched then parsing follows the NodeLink to the next part of the path matching.
        using LiteralType = LinkedMixin<TLiteral, Link >;

        // Custruct a type is an argument of a specific type
        using ArgumentType = Mixin<TArgumentType, Link >;
        using HandlerType = THandler;

        using External = Linked< std::function<THandler(ParserState&)> >;

        // if there is more input in parse stream
        LiteralType *literals;    // first try to match one of these literals

        // if no literal matches, then try to match based on token type
        ArgumentType *string, *numeric, *boolean;

        // if no match is made, we can optionally call a wildcard handler
        NodeData *wild;

        External *externals;

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

        using Exception = Rest::Exception<Node>;
        using Parser = Rest::Parser< NodeData, Endpoints, Rest::Token >;

        // test to ensure the base class has an attach(HttpMethod, THandler) method
        //static_assert(
        //        has_method<Super, attach_caller, void(HttpMethod method, THandler handler) >::value,
        //        "Node class should have attach method with signature void(HttpMethod,THandler)");

        class Request : public UriRequest {
        public:
            Handler handler;
            // todo: possibly make this derived class contain the conversions from class instance to static?

            inline Request(HttpMethod _method, const char* _uri, int _status=0) : UriRequest(_method, _uri, _status), handler(nullptr) {}
            inline Request(const UriRequest& req) : UriRequest(req), handler(nullptr) {}

            inline explicit operator bool() const { return status==UriMatched && handler!=nullptr; }
        };

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

        inline explicit operator bool() const { return _node!=nullptr; }

        // can convert directly to the nodedata we point to
        inline operator const NodeData*() const { return _node; }
        inline operator NodeData*() { return _node; }

        // smartptr like operation
        inline const NodeData* operator->() const { return _node; }
        inline NodeData* operator->() { return _node; }

        void otherwise(std::function<Handler(Rest::ParserState&)> external) {
            auto ext = _endpoints->pool.template make<typename NodeData::External>(external);
            if(_node->externals != nullptr)
                _node->externals->append(ext);
            else
                _node->externals = ext;
        }

        // resolve an external Endpoints collection and apply the instance object to the resolve handler
        template<class I, class EP>
        typename EP::Node with(I& inst, EP& ep) { // here klass is the handler's class type
            // store the klass reference and endpoints reference together
            // we resolve the klass member handler via endpoints, then bind the resolved handler to the klass reference
            // thus making it a static call
            otherwise(
                   [&inst,&ep](ParserState& lhs_request) -> Handler {
                       typename EP::Node rhs_node = ep.getRoot();

                       ParserState rhs_request(lhs_request);
                       typename EP::Handler handler = rhs_node.resolve(rhs_request);
                       return (handler!=nullptr)
                            ? std::bind(handler, inst, std::placeholders::_1)    // todo: what if there is more than 1 argument in handler?
                            : Handler();
                       //return [&inst](RestRequest& rr) -> int { return inst.*h(rr); };
                   }
            );
            return ep.getRoot();
        }

        // resolve an external Endpoints collection and apply the instance object to the resolve handler
        template<class I, class EP=typename TEndpoints::template ClassEndpoints<I> >
        typename EP::Node with(I& inst) { // here klass is the handler's class type
            // store the klass reference and endpoints reference together using std::shared_ptr which gets stored in
            // the lambda class type.
            auto ep = std::make_shared<EP>();
            otherwise(
                    [&inst, ep](ParserState& lhs_request) -> Handler {
                        typename EP::Node rhs_node = ep->getRoot();

                        ParserState rhs_request(lhs_request);
                        typename EP::Handler handler = rhs_node.resolve(rhs_request);
                        return (handler!=nullptr)
                               ? std::bind(handler, inst, std::placeholders::_1)    // todo: what if there is more than 1 argument in handler?
                               : Handler();
                    }
            );

            // would make some sense to return shared_ptr here, but then the with().on() method chaining changes
            // to -> operator, which is inconsistent. The life of the shared_ptr is assured for as long as the chain.
            return ep->getRoot();
        }

        // resolve an external Endpoints collection and apply the instance object to the resolve handler
        template<class I, class EP=typename TEndpoints::template ClassEndpoints<I> >
        typename EP::Node with( std::function< I&(Rest::UriRequest&) > resolver) { // here klass is the handler's class type
            // store the klass reference and endpoints reference together using std::shared_ptr which gets stored in
            // the lambda class type.
            auto ep = std::make_shared<EP>();
            otherwise(
                    // this lambda is the 'external', it holds the internally stored Endpoints object, along with the
                    // resolver function and when invoked will call resolve() on the this internal Endpoints to get
                    // the instance handler and convert it to static using the instance resolver function.
                    [resolver, ep](ParserState& lhs_request) -> Handler {
                        typename EP::Node rhs_node = ep->getRoot();
                        ParserState rhs_request(lhs_request);

                        // try to resolve the rest of the endpoint Uri and get an instance handler
                        typename EP::Handler handler = rhs_node.resolve(rhs_request);
                        if(handler!=nullptr) {
                            // resolved an instance handler, now call the instance resolver to get an object instance (this pointer)
                            I& inst = resolver(rhs_request.request);
                            if(!rhs_request.request.isSuccessful() || &inst == nullptr ) {
                                lhs_request.request.status = rhs_request.request.status;
                                return Handler();   // failed to resolve due to instance callback
                            }

                            // now bind the instance to the handler thus creating a static invokable function
                            return std::bind(handler, inst,
                                      std::placeholders::_1);    // todo: what if there is more than 1 argument in handler?
                        }
                        else return Handler();  // external did not resolve handler
                    }
            );

            // would make some sense to return shared_ptr here, but then the with().on() method chaining changes
            // to -> operator, which is inconsistent. The life of the shared_ptr is assured for as long as the chain.
            return ep->getRoot();
        }

        // resolve an external Endpoints collection and apply the instance object to the resolve handler
        template<class I, class EP=typename TEndpoints::template ClassEndpoints<I> >
        typename EP::Node with( std::function< I*(Rest::UriRequest&) > resolver) { // here klass is the handler's class type
            // store the klass reference and endpoints reference together using std::shared_ptr which gets stored in
            // the lambda class type.
            auto ep = std::make_shared<EP>();
            otherwise(
                    // this lambda is the 'external', it holds the internally stored Endpoints object, along with the
                    // resolver function and when invoked will call resolve() on the this internal Endpoints to get
                    // the instance handler and convert it to static using the instance resolver function.
                    [resolver, ep](ParserState& lhs_request) -> Handler {
                        typename EP::Node rhs_node = ep->getRoot();
                        ParserState rhs_request(lhs_request);

                        // try to resolve the rest of the endpoint Uri and get an instance handler
                        typename EP::Handler handler = rhs_node.resolve(rhs_request);
                        if(handler!=nullptr) {
                            // resolved an instance handler, now call the instance resolver to get an object instance (this pointer)
                            I* inst = resolver(rhs_request.request);
                            if(inst == nullptr || !rhs_request.request.isSuccessful()) {
                                lhs_request.request.status = rhs_request.request.status;
                                return Handler();   // failed to resolve due to instance callback
                            }

                            // now bind the instance to the handler thus creating a static invokable function
                            return std::bind(handler, *inst,
                                             std::placeholders::_1);    // todo: what if there is more than 1 argument in handler?
                        }
                        else return Handler();  // external did not resolve handler
                    }
            );

            // would make some sense to return shared_ptr here, but then the with().on() method chaining changes
            // to -> operator, which is inconsistent. The life of the shared_ptr is assured for as long as the chain.
            return ep->getRoot();
        }

        // resolve an external Endpoints collection (that has the same handler type)
        //template<typename HH, typename NN>
        typename TEndpoints::Node with(TEndpoints& ep) { // here klass is the handler's class type
            // store the klass reference and endpoints reference together
            // we resolve the klass member handler via endpoints, then bind the resolved handler to the klass reference
            // thus making it a static call
            otherwise(
                    [&ep](ParserState& lhs_request) -> Handler {
                        typename TEndpoints::Node rhs_node = ep.getRoot();
                        ParserState rhs_request(lhs_request);

                        // try to resolve the rest of the endpoint Uri
                        return rhs_node.resolve(rhs_request);
                    }
            );
            return ep.getRoot();
        }

        inline int error() const { return _exception; }

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
        void attach(const char* expr, HttpMethod method, HandlerT handler ) {
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

            if(_node==nullptr || _endpoints==nullptr)
                return Node(_endpoints, _exception = URL_FAIL_NULL_ROOT);   // invalid NodeRef
            else if(_exception!=0)
                return Node(_endpoints, _exception );   // invalid NodeRef
            else if(*endpoint_expression == '/')
                // change to expression from absolute root node
                return _endpoints->getRoot().on(endpoint_expression+1);

            // create new parser state
            ParserState ev( UriRequest(HttpMethodAny, endpoint_expression) );
            ev.mode = ParserState::expand;         // tell the parser we are adding this endpoint

            // parse the Uri expression
            Parser parser(_node, _endpoints);
            if((rs = parser.parse(&ev)) <UriMatched) {
                return Node(_endpoints, rs);
            } else {
                // if we encountered more args than we did before, then save the new value
                if(ev.nargs > _endpoints->maxUriArgs)
                    _endpoints->maxUriArgs = ev.nargs;

                // attach the handler to this endpoint
                return Node(_endpoints, parser.context);
            }
        }

        Request resolve(HttpMethod method, const char* uri) {
            Request request(method, uri);
            resolve(request);
            return request;
        }

        bool resolve(Request& request) {

            // initialize new parser state
            ParserState ev(request);
            if (ev.state < 0)
                return URL_FAIL_SYNTAX;
            ev.mode = ParserState::resolve;
            ev.request.args.reserve(_endpoints->maxUriArgs);

            Handler h = resolve(ev);
            request.args = ev.request.args; // todo: can we get rid of this Args copy?
            request.handler = h;
            request.status = ev.result;
            return ev.result >=0;
        }

        Handler resolve(ParserState& ev) {
            // parse the input
            Parser parser(_node, _endpoints);
            if((ev.result=parser.parse( &ev )) >=UriMatched) {
                // successfully resolved the endpoint
                Handler handler = parser.context->handle(ev.request.method);
                if(handler != nullptr)
                    return handler;

                ev.result = NoHandler;
            }

            if((ev.result == NoEndpoint || ev.result == NoHandler) && parser.context->externals != nullptr) {
                // try any externals
                auto external = parser.context->externals;
                while(external != nullptr) {
                    // call into the external
                    Handler h = (*external)(ev);

                    // check result for error
                    if(ev.request.status <0 || ev.request.status>299) {
                        // handler reported error
                        ev.result = ev.request.status;
                        return Handler();
                    }

                    // if we have a handler then return success
                    if(h!=nullptr) {
                        ev.result = UriMatched;
                        return h;
                    }

                    // try next external
                    external = external->next;
                }
            }

            return Handler();    // cannot resolve
        }

    protected:
        Endpoints* _endpoints;
        NodeData* _node;
        int _exception;
    };

}

#endif //RESTFULLY_NODE_H
