//
// Created by guru on 9/10/20.
//

#ifndef RESTFULLY_URIREQUEST_H
#define RESTFULLY_URIREQUEST_H

#pragma once

#include "Token.h"
#include "HttpStatus.h"
#include "Exception.h"

#include <vector>
#include <functional>

namespace Rest {

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
        typedef enum {
            Accept,             // request is only checking if we should handle the Uri or pass on to the next handler
            Execute             // request is to fully execute and fulfill the request output
        } Intent;

    public:
        HttpMethod method;              // HTTP Request method: GET, POST, PUT, PATCH, DELETE, OPTIONS, etc
        const char *contentType;        // MIME content-type, typically application/json for Rest services
        Intent intent;
        std::vector<Token> words;       // parsed list of symbols in the Uri

        // if status >= 200 then returned as a standard http response code,
        // otherwise http response code is interpreted to 4xx or 5xx and the actual status code is returned as a
        // x-api-code response header.
        short status;

        // optional x-api-message header if value is non-empty
        StringType message;

    public:
        inline UriRequest() : method(HttpMethodAny), contentType(ApplicationJsonMimeType), intent(Execute), status(0)
        {}

        UriRequest(HttpMethod _method, const char *_uri);
        UriRequest(HttpMethod _method, std::vector<Token>&& _uri);
        UriRequest(const UriRequest &copy);

        UriRequest &operator=(const UriRequest &copy);

        /// @brief If the intention of the request was only to check if we should accept the Uri
        /// then this completes the request processing with the given return code.
        virtual short accept(short code = 200) {
            if(intent == Accept) {
                complete(code);
                return code;
            } else
                return 0;
        }

        /// @brief stops any further Uri processing and returns the given code to the requestor
        virtual void complete(short code) {
            switch(code) {
                case UriMatched:
                case UriMatchedWildcard:
                case UriAccepted:
                    status = OK;
                    break;

                case InvalidHandler:
                case NoHandler:
                    status = NotFound;
                    break;

                case InvalidParameterType:
                case MissingParameter:
                    status = BadRequest;
                    break;

                default:
                    if(code < 0)
                        status = InternalServerError;
                    break;

            }
            status = code;
        }

        inline bool isSuccessful() const { return status >= 200 && status < 300; }

    };


    class UriRequestMatch {
    public:
        HttpMethod method;
        const char* contentType;

        UriRequestMatch() : method(HttpMethodAny), contentType(nullptr) {}
        explicit UriRequestMatch(HttpMethod _method) : method(_method), contentType(nullptr) {}

        UriRequestMatch withContentType(const char* contentType) const;

        virtual bool matches(UriRequest& request) const;

    };


    template<HttpMethod METHOD, class TUriRequestHandler>
    class SimpleUriRequestHandler : public TUriRequestHandler {
    public:
#if 0
        SimpleUriRequestHandler(std::function<int()> _handler) : UriRequestHandler(METHOD, std::move(_handler)) {}
        SimpleUriRequestHandler(std::function<int(const UriRequest&)> _handler) : UriRequestHandler(METHOD, std::move(_handler)) {}
#else
        template<class THandler>
        SimpleUriRequestHandler(THandler _handler) : TUriRequestHandler(METHOD, std::move(_handler)) {}
#endif
    };


} //ns:Rest

#endif //RESTFULLY_URIREQUEST_H
