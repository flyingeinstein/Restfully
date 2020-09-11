//
// Created by guru on 9/10/20.
//

#include "UriRequest.h"
#include "Parser.h"

namespace Rest {

    const char *uri_method_to_string(HttpMethod method) {
        switch (method) {
            case HttpGet: return "GET";
            case HttpPost: return "POST";
            case HttpPut: return "PUT";
            case HttpPatch: return "PATCH";
            case HttpDelete: return "DELETE";
            case HttpOptions: return "OPTIONS";
            case HttpMethodAny: return "ANY";
            default: return "GET";
        }
    }

    UriRequest::UriRequest(HttpMethod _method, std::vector<Token> _words)
        : method(_method), contentType(ApplicationJsonMimeType), words(_words)
    {
    }

    UriRequest::UriRequest(HttpMethod _method, const char *_uri)
        : method(_method), contentType(ApplicationJsonMimeType)
    {
        Rest::Token t;
        do {
            if(t.scan(&_uri, false) >0) {
                if(t.id != '/')
                    words.push_back(t);
            }
        } while(t.id != TID_EOF);
    }

    UriRequest::UriRequest(const UriRequest &copy)
        : method(copy.method), contentType(copy.contentType), words(copy.words)
    {
    }

    UriRequest& UriRequest::operator=(const UriRequest &copy) {
        method = copy.method;
        contentType = copy.contentType;
        words = copy.words;
        return *this;
    }



    UriRequestMatch UriRequestMatch::withContentType(const char* contentType) const {
        UriRequestMatch m(*this);
        m.contentType = contentType;
        return m;
    }

    bool UriRequestMatch::matches(UriRequest& request) const {
        return (method == HttpMethodAny || request.method == method)
            && (contentType == nullptr || strcmp(contentType, request.contentType) ==0);
    };


#if 0
    UriRequestHandler::UriRequestHandler(const UriRequestHandler& copy)
        : UriRequestMatch(copy), handlerType(copy.handlerType)
    {
        // using the "placement" copy constructor to properly initialize our union object
        switch(handlerType) {
            case 0: new (&handler) std::function<int()>(copy.handler); break;
            case 1: new (&handler_req) std::function<int(const UriRequest&)>(copy.handler_req); break;
            case 2: new (&handler_parser) std::function<int(const Parser&)>(copy.handler_parser); break;
        }
    }

    UriRequestHandler::~UriRequestHandler() {
        // using the "placement" copy constructor to properly initialize our union object
        switch(handlerType) {
            case 0: handler.~function<int()>(); break;
            case 1: handler_req.~function<int(const UriRequest&)>(); break;
            case 2: handler_parser.~function<int(const Parser&)>(); break;
        }
    }

    int UriRequestHandler::call(const Parser& parser) {
        switch(handlerType) {
            case 0: return handler();
            case 1: return handler_req(parser.request);
            case 2: return handler_parser(parser);
        }
        return 0;
    }
#endif

} //ns:Rest