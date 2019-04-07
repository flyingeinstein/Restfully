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
        : ep_head(nullptr), ep_tail(nullptr), ep_end(nullptr), sznodes(32), maxUriArgs(0)
    {
    }

    /// \brief Move constructor
    /// moves endpoints and resources from one Endpoints instance to another.
    Endpoints(Endpoints&& other) noexcept
        : ep_head(other.ep_head), ep_tail(other.ep_tail), ep_end(other.ep_end), sznodes(other.sznodes), maxUriArgs(other.maxUriArgs) {
        other.free(false);
    }

    /// \brief Move assignment operator
    /// moves endpoints and resources from one Endpoints instance to another.
    Endpoints& operator=(Endpoints&& other) noexcept {
        ep_head = other.ep_head;
        ep_tail = other.ep_tail;
        ep_end = other.ep_end;
        sznodes = other.sznodes;
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
        free(true);
    }

    Node on(const char* expression) {
        if(ep_head == nullptr) alloc();
        return getRoot().on(expression);
    }

    Request resolve(HttpMethod method, const char* expression) {
        return (ep_head == nullptr)
            ? Request(method, expression, URL_FAIL_NULL_ROOT)
            : getRoot().resolve(method, expression);
    }

    Node newNode() {
        if(ep_head == nullptr) alloc();
        assert(ep_tail < ep_end);
        return Node(this, new (ep_tail++) TNodeData());
    }

    inline Node getRoot() {
        if(ep_head == nullptr) alloc();
        return Node(this, ep_head);
    }



    // todo: hide the Endpoints argument, node and literal building methods behind a Builder interface.
    
    ArgumentType* newArgumentType(int literal_id, unsigned short typemask) {
        return new ArgumentType(literal_id, typemask);
    }

    long findLiteral(const char* word) {
        return binbag_find_nocase(literals_index, word);
    }

    Literal* newLiteral(TNodeData* ep, Literal* literal)
    {
        Literal* _new, *p;
        int _insert;
        if(ep->literals) {
            // todo: this kind of realloc every Literal insert will cause memory fragmentation, use Endpoints shared mem
            // find the end of this list
            Literal *_list = ep->literals;
            while (_list->isValid())
                _list++;
            _insert = (int)(_list - ep->literals);

            // allocate a new list
            _new = (Literal*)realloc(ep->literals, (_insert + 2) * sizeof(Literal));
            //memset(_new+_insert+1, 0, sizeof(Literal));
        } else {
            _new = (Literal*)calloc(2, sizeof(Literal));
            _insert = 0;
        };

        // insert the new literal
        memcpy(_new + _insert, literal, sizeof(Literal));
        ep->literals = _new;

        p = &_new[_insert + 1];
        p->id = -1;
        p->isNumeric = false;
        p->nextNode = nullptr;

        return _new + _insert;
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
    // todo: this will need to use paged memory
    TNodeData *ep_head, *ep_tail, *ep_end;
    size_t sznodes;

    // some statistics on the endpoints
    size_t maxUriArgs;       // maximum number of embedded arguments on any one endpoint expression

protected:
    void alloc() {
        if(ep_head == nullptr) {
            ep_head = ep_tail = (TNodeData *) calloc(sznodes, sizeof(TNodeData));
            ep_end = ep_head + sznodes;

            if(literals_index == nullptr) {
                literals_index = binbag_create(128, 1.5);
            }

            // create the root node
            new (ep_tail++) TNodeData();
        }
    }

    void  free(bool dealloc=true) {
        if (dealloc && ep_head!= nullptr) {
            // call destructor on all nodes
            TNodeData *n = ep_head;
            while (n < ep_tail) {
                n->~TNodeData();
                n++;
            }
            // free all memory
            ::free(ep_head);
        }
        ep_head = ep_tail = ep_end = nullptr;
        sznodes = 32;
        maxUriArgs = 0;
    }
};


} // ns:Rest
