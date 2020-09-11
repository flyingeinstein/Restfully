//
// Created by guru on 9/10/20.
//

#ifndef RESTFULLY_URIREQUEST_H
#define RESTFULLY_URIREQUEST_H

#pragma once

#include "Token.h"

#include <vector>
#include <functional>

namespace Rest {

    class Parser;

    extern const char* ApplicationJsonMimeType;

    // indicates a default Rest handler that matches any http verb request
    // this enum belongs with the web servers HTTP_GET, HTTP_POST, HTTP_xxx constants
    typedef enum {
        HttpMethodAny = 0,
        HttpGet,
        HttpPost,
        HttpPut,
        HttpPatch,
        HttpDelete,
        HttpOptions,
    } HttpMethod;

    /// \brief Convert a http method enum value to a string.
    const char* uri_method_to_string(HttpMethod method);


    class UriRequest {
    public:
        HttpMethod method;              // HTTP Request method: GET, POST, PUT, PATCH, DELETE, OPTIONS, etc
        const char *contentType;        // MIME content-type, typically application/json for Rest services
        std::vector<Token> words;       // parsed list of symbols in the Uri

    public:
        inline UriRequest() : method(HttpMethodAny), contentType(ApplicationJsonMimeType){}

        UriRequest(HttpMethod _method, const char *_uri);
        UriRequest(HttpMethod _method, std::vector<Token> _uri);
        UriRequest(const UriRequest &copy);

        UriRequest &operator=(const UriRequest &copy);
    };


    class UriRequestMatch {
    public:
        HttpMethod method;
        const char* contentType;

        UriRequestMatch() : method(HttpMethodAny), contentType(nullptr) {}
        UriRequestMatch(HttpMethod _method) : method(_method), contentType(nullptr) {}

        UriRequestMatch withContentType(const char* contentType) const;

        virtual bool matches(UriRequest& request) const;

    };

    class UriRequestHandler: public UriRequestMatch {
    public:
        UriRequestHandler(HttpMethod _method, std::function<int()> _handler)
                : UriRequestMatch(_method), handler(std::move(_handler)), handlerType(0)
        {}

        UriRequestHandler(HttpMethod _method, std::function<int(const UriRequest&)> _handler)
                : UriRequestMatch(_method), handler_req(std::move(_handler)), handlerType(1)
        {}

        UriRequestHandler(HttpMethod _method, std::function<int(const Parser&)> _handler)
                : UriRequestMatch(_method), handler_parser(std::move(_handler)), handlerType(2)
        {}

        UriRequestHandler(const UriRequestHandler& copy);

        virtual ~UriRequestHandler();

        virtual int call(const Parser& parser);

        short handlerType;
        union {
            std::function<int()> handler;
            std::function<int(const UriRequest&)> handler_req;
            std::function<int(const Parser&)> handler_parser;
        };
    };


    template<HttpMethod METHOD>
    class SimpleUriRequestHandler : public UriRequestHandler {
    public:
#if 0
        SimpleUriRequestHandler(std::function<int()> _handler) : UriRequestHandler(METHOD, std::move(_handler)) {}
        SimpleUriRequestHandler(std::function<int(const UriRequest&)> _handler) : UriRequestHandler(METHOD, std::move(_handler)) {}
#else
        template<class THandler>
        SimpleUriRequestHandler(THandler _handler) : UriRequestHandler(METHOD, std::move(_handler)) {}
#endif
    };

    using GET = SimpleUriRequestHandler<HttpGet>;
    using POST = SimpleUriRequestHandler<HttpPost>;
    using PUT = SimpleUriRequestHandler<HttpPut>;
    using PATCH = SimpleUriRequestHandler<HttpPatch>;
    using DELETE = SimpleUriRequestHandler<HttpPut>;
    using OPTIONS = SimpleUriRequestHandler<HttpOptions>;


} //ns:Rest

#endif //RESTFULLY_URIREQUEST_H
