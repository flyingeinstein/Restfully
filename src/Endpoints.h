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

public:
    using Request = typename Node::Request;
    using Exception = typename Node::Exception;

public:
    /// \brief Initialize an empty UriExpression with a maximum number of code size.
    Endpoints()
        : pool( (sizeof(NodeData)+sizeof(Literal))*8 ),
          ep_head(nullptr), maxUriArgs(0)
    {
        if(literals_index == nullptr) {
            literals_index = binbag_create(128, 1.5);
        }
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

    Node on(const char* expression) {
        return getRoot().on(expression);
    }

    Request resolve(HttpMethod method, const char* expression) {
        return (ep_head == nullptr)
            ? Request(method, expression, URL_FAIL_NULL_ROOT)
            : getRoot().resolve(method, expression);
    }

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
        return binbag_find_nocase(literals_index, word);
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
        if((lit.id = binbag_find_nocase(literals_index, literal_value)) <0)
            lit.id = binbag_insert(literals_index, literal_value);  // insert value into the binbag, and record the index into the id field
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
