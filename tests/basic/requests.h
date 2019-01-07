//
// Created by Colin MacKenzie on 2018-12-18.
//

#ifndef NIMBLE_REQUESTS_H
#define NIMBLE_REQUESTS_H

#include <Rest.h>

class RestRequest
{
public:
    Rest::HttpMethod method;
    std::string uri;
    std::string request;
    std::string response;
    Rest::Arguments& args;

    RestRequest(Rest::Arguments& _args) : args(_args) {}
};


using Rest::HttpMethod;
using Rest::HttpGet;
using Rest::HttpPut;
using Rest::HttpPost;
using Rest::HttpPatch;
using Rest::HttpDelete;
using Rest::HttpOptions;

template<class TRestRequest>
class RestRequestHandler
{
public:
    // types
    typedef TRestRequest RequestType;

    typedef Rest::Handler< TRestRequest& > RequestHandler;
    typedef Rest::Endpoints<RequestHandler> Endpoints;

    // the collection of Rest handlers
    Endpoints endpoints;

    virtual bool handle(HttpMethod requestMethod, std::string requestUri, std::string* response_out=NULL) {
        Rest::HttpMethod method = (Rest::HttpMethod)requestMethod;
        typename Endpoints::Endpoint ep = endpoints.resolve(method, requestUri.c_str());
        if (ep) {
            RequestType request(ep);
            request.method = method;
            request.uri = requestUri;
            ep.handler(request);
            if(response_out)
                *response_out = request.response;
            return true;
        } else
            return false;
    }

#if 0
    RestRequestHandler& on(const char *endpoint_expression, Rest::Handler< TRestRequest& > methodHandler ) {
        //Rest::Handler< RequestHandler > h( methodHandler.method, RequestHandler( methodHandler.handler ) );
        endpoints.on(endpoint_expression, methodHandler);
        return *this;
    }

    RestRequestHandler& on(const char *endpoint_expression, std::function< int(TRestRequest&) > methodHandler ) {
        //Rest::Handler< RequestHandler > h( methodHandler.method, RequestHandler( methodHandler.handler ) );
        endpoints.on(endpoint_expression, methodHandler);
        return *this;
    }
#else
    template<class... Targs>
    RestRequestHandler& on(const char *endpoint_expression, Targs... rest ) {
        endpoints.on(endpoint_expression, rest...);   // add the rest (recursively)
        return *this;
    }
#endif

#if 0
    // c++11 using parameter pack expressions to recursively call add()
    template<class T, class... Targs>
    RestRequestHandler& on(const char *endpoint_expression, T h1, Targs... rest ) {
      on(endpoint_expression, h1);   // add first argument
      return on(endpoint_expression, rest...);   // add the rest (recursively)
    }
#endif
};

DEFINE_HTTP_METHOD_HANDLERS(RestRequest)

using Rest::uri_result_to_string;


#endif //NIMBLE_REQUESTS_H
