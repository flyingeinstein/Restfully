//
// Created by Colin MacKenzie on 2019-02-04.
//

#ifndef RESTFULLY_NODE_H
#define RESTFULLY_NODE_H

#include "Link.h"
#include "Exception.h"
#include "handler.h"

namespace Rest {

    template<class TLiteral, class TArgumentType, class THandler>
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


    template<class TNodeData, class THandler>
    class Node {
    public:
        using Exception = Exception<Node>;

        inline Node() : node(nullptr), exception(0) {}
        inline Node(int _exception) : node(nullptr), exception(_exception) {}
        inline Node(TNodeData* _node) : node(_node), exception(0) {}

        inline Node(const Node& copy) : node(copy.node), exception(copy.exception) {}

        Node& operator=(const Node& copy) {
            node=copy.node;
            exception = copy.exception;
            return *this;
        }

        inline operator bool() const { return node!=nullptr; }

        inline int error() const { return exception; }

        template<typename H> inline Node& GET(H handler) { attach(HttpGet, handler); return *this; }
        template<typename H> inline Node& PUT(H handler) { attach(HttpPut, handler); return *this; }
        template<typename H> inline Node& PATCH(H handler) { attach(HttpPatch, handler); return *this; }
        template<typename H> inline Node& POST(H handler) { attach(HttpPost, handler); return *this; }
        template<typename H> inline Node& DELETE(H handler) { attach(HttpDelete, handler); return *this; }
        template<typename H> inline Node& OPTIONS(H handler) { attach(HttpOptions, handler); return *this; }
        template<typename H> inline Node& ANY(H handler) { attach(HttpMethodAny, handler); return *this; }

        inline void attach(HttpMethod method, THandler handler ) {
            if(node!= nullptr)
                node->handle(method, handler);
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
        TNodeData* node;
        int exception;
        // todo: do we want endpoint->name? we can add it
    };

}

#endif //RESTFULLY_NODE_H
