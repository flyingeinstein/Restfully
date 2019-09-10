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
#include "Mixins.h"
#include "StringPool.h"
#include "Parser.h"
#include "handler.h"

// format:    /api/test/:param_name(integer|real|number|string|boolean)/method

namespace Rest {

/// \brief Implements a compiled list of Rest Uri Endpoint expressions
/// Each UriExpression contains one or more Uri Endpoint expressions in a compiled form.
/// You can store expressions for all Rest methods in the application if desired. This
/// compiled byte-code machine can optimally compare and match a request Uri to an
/// Endpoint specification.
template<
        class THandler = std::function<short(void)>,
        class TNodeData = Rest::NodeData<THandler>
>
class Endpoints {
public:
    using Handler = THandler;
    using NodeData = TNodeData;

    using Literal = typename TNodeData::LiteralType;
    using ArgumentType = typename TNodeData::ArgumentType;
    using Argument = Rest::Argument;
    using Node = Rest::Node< Endpoints >;

    using HandlerTraits = Rest::function_traits<THandler>;

    template<class Klass>
    using ClassEndpoints = Endpoints< typename HandlerTraits::template CVFunctionType<Klass> >;

    template<class Klass>
    using ClassConstEndpoints = Endpoints< typename HandlerTraits::template CVConstFunctionType<Klass> >;

public:
    using Request = typename Node::Request;
    using Exception = typename Node::Exception;

public:
    /// \brief Initialize an empty UriExpression with a maximum number of code size.
    Endpoints()
        : pool( (sizeof(NodeData)+sizeof(Literal))*8 ),
          ep_head(nullptr), maxUriArgs(0)
    {
    }

    /// \brief Move constructor
    /// moves endpoints and resources from one Endpoints instance to another.
    Endpoints(Endpoints&& other) noexcept
        : pool(other.pool), ep_head(other.ep_head), maxUriArgs(other.maxUriArgs) {
          other.free(false);
    }

    /// \brief Move assignment operator
    /// moves endpoints and resources from one Endpoints instance to another.
    Endpoints& operator=(Endpoints&& other) noexcept {
        pool = other.pool;
        ep_head = other.ep_head;
        maxUriArgs = other.maxUriArgs;
        other.free(false);
    }

    /// \brief Copy constructor (not allowed)
    /// You should not be copying endpoints, this is a sign that you forgot to pass an Endpoints instance by reference
    /// and your code is probably going to fail desired behavior.
    Endpoints(const Endpoints& copy) = delete;

    /// \brief Copy assignment (not allowed)
    /// You should not be copying endpoints, this is a sign that you forgot to pass an Endpoints instance by reference
    /// and your code is probably going to fail desired behavior.
    Endpoints& operator=(const Endpoints&) = delete;

    /// \brief Destroys the RestUriExpression and releases memory
    virtual ~Endpoints() {
    }

    /// \brief Return or create a new Rest API namespace at the given URI
    /// You begin with this method to build your Rest API tree using method chaining. The on() method returns Node objects
    /// which represent Rest Endpoints where you can attach your function handlers using the methods GET, POST, PUT,
    /// PATCH, DELETE, etc. You can also use the on() from Nodes as well. The Node object has other useful methods such
    /// as with(...) and withContentType(...) for controlling your Rest API.
    /// ```
    ///    Endpoints endpoints;
    ///    endpoints.on("/api/rooms/:roomid(string|integer)")
    ///        .GET("lights", get_lights)       // subpath + get handler
    ///        .PUT("lights", set_lights)       // subpath + put handler
    ///        .GET(get_room_details);          // get handler attached to /api/rooms/:roomid
    /// ```
    Node on(const char* expression) {
        return getRoot().on(expression);
    }

    /// \brief Check if a URL should be accepted by the Rest API
    /// This function performs a check on a URI to see if it is within the Rest API namespace. It works most efficient
    /// if you have used the accept() method on short base paths to indicate your Rest API URI namespaces and therefor
    /// the queryAccept method can stop parsing the URI sooner in the parsing process. A URI that is accepted but later
    /// doesnt resolve to a handler would be considered 404.
    inline int queryAccept(HttpMethod method, const char* uri) {
        return (ep_head == nullptr)
               ? NoEndpoint
               : getRoot().queryAccept(method, uri);
    }


    /// \brief \deprecated Resolve the request handler for a URI
    /// Returns the Request handler for a URI, or returns a blank Request handler with status set to a ParseResult code.
    /// This method is deprecated. You should construct a Request object using your HttpMethod, URI and optionally other
    /// details such as contentType, then pass that Request object into the resolve(...) method.
    Request resolve(HttpMethod method, const char* uri) {
        if (ep_head == nullptr)
            return Request(method, uri, URL_FAIL_NULL_ROOT);
        typename Node::Request req(method, uri);
        resolve(req);
        return req;
    }

    /// \brief Resolve the request handler for a URI
    /// Resolves a Request object into a function handler. The Request contains the URI and HttpMethod that will be
    /// resolved. The status member of the Request object will be set upon return.
    /// \returns true if the Request matched to a valid handler.
    bool resolve(Request& req) {
        if (ep_head == nullptr) {
            req.status = URL_FAIL_NULL_ROOT;
            return false;
        }
        return getRoot().resolve(req);
    }

    /// \brief Returns the root node of the Rest API namespace.
    inline Node getRoot() {
        return Node(this,
            (ep_head == nullptr)
                ? ep_head = newNode()        // first node
                : ep_head                    // usual route
        );
    }



    // todo: hide the Endpoints argument, node and literal building methods behind a Builder interface.

    Node newNode() {
        return Node(this, pool.make<TNodeData>() );
    }

    ArgumentType* newArgumentType(int literal_id, unsigned short typemask) {
        return pool.make<ArgumentType>(literal_id, typemask);
    }

    long findLiteral(const char* word) {
        return literals_index.find_nocase(word);
    }

    Literal* newLiteral(TNodeData* ep, Literal* literal)
    {
        Literal* _new = pool.make<Literal>(*literal);
        if(ep->literals == nullptr) {
            ep->literals = _new;    // first literal in node
        } else {
            // not first literal, walk the literals list and add to the end
            Literal* l = ep->literals;
            while(l->next != nullptr)
                l = l->next;
            l->next = _new;
        }
        return _new;
    }

    Literal* newLiteralString(TNodeData* ep, const char* literal_value)
    {
        Literal lit;
        lit.isNumeric = false;
        lit.id = literals_index.insert_distinct(literal_value, strcasecmp);
        lit.nextNode = nullptr;
        return newLiteral(ep, &lit);
    }

    Literal* newLiteralNumber(TNodeData* ep, ssize_t literal_value)
    {
        Literal lit;
        lit.isNumeric = true;
        lit.id = literal_value;
        lit.nextNode = nullptr;
        return newLiteral(ep, &lit);
    }

public:
    // stores the expression as a chain of endpoint nodes
    PagedPool pool;
    TNodeData *ep_head;

    // some statistics on the endpoints
    size_t maxUriArgs;       // maximum number of embedded arguments on any one endpoint expression
};


} // ns:Rest
