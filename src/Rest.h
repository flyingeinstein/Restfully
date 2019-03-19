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
    using Endpoint = typename Node::Request;
    using Exception = typename Node::Exception;

public:
    /// \brief Initialize an empty UriExpression with a maximum number of code size.
    Endpoints()
            : sznodes(32), maxUriArgs(0)
    {
        ep_head = ep_tail =  (TNodeData*)calloc(sznodes, sizeof(TNodeData));
        ep_end = ep_head + sznodes;
        if(literals_index == nullptr) {
            literals_index = binbag_create(128, 1.5);
        }
        newNode();  // create the root node
    }

    /// \brief Destroys the RestUriExpression and releases memory
    virtual ~Endpoints() {
        ::free(ep_head);
    }

    Node on(const char* expression) {
        return getRoot().on(expression);
    }

    Endpoint resolve(HttpMethod method, const char* expression) {
        return getRoot().resolve(method, expression);
    }

    inline Node getRoot() { return Node(this, ep_head); }

    Node newNode() {
        assert(ep_tail < ep_end);
        return Node(this, new (ep_tail++) TNodeData());
    }

    ArgumentType* newArgumentType(const char* name, unsigned short typemask) {
        long nameid = binbag_insert_distinct(literals_index, name);
        return new ArgumentType(nameid, typemask);
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
        p->next = nullptr;

        return _new + _insert;
    }


    Literal* addLiteralString(TNodeData* ep, const char* literal_value)
    {
        Literal lit;
        lit.isNumeric = false;
        if((lit.id = binbag_find_nocase(literals_index, literal_value)) <0)
            lit.id = binbag_insert(literals_index, literal_value);  // insert value into the binbag, and record the index into the id field
        lit.next = nullptr;
        return newLiteral(ep, &lit);
    }

    Literal* addLiteralNumber(TNodeData* ep, ssize_t literal_value)
    {
        Literal lit;
        lit.isNumeric = true;
        lit.id = literal_value;
        lit.next = nullptr;
        return newLiteral(ep, &lit);
    }

public:
    // stores the expression as a chain of endpoint nodes
    // todo: this will need to use paged memory
    TNodeData *ep_head, *ep_tail, *ep_end;
    size_t sznodes;

    // some statistics on the endpoints
    // todo: move this into a stats structure that is public
    size_t maxUriArgs;       // maximum number of embedded arguments on any one endpoint expression
};


} // ns:Rest
