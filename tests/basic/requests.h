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
    Rest::Arguments args;

    template<class TEndpoint>
    RestRequest(TEndpoint ep)
        : method(ep.method), args(ep)
    {}

    RestRequest(const Rest::Arguments& _args) : args(_args), method(Rest::HttpMethodAny) {}

    inline const Rest::Argument& operator[](size_t idx) const { return args[idx]; }
    inline const Rest::Argument& operator[](const char* name) const { return args[name]; }
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

    typename Endpoints::Node on(const char *endpoint_expression ) {
        return endpoints.on(endpoint_expression);   // add the rest (recursively)
    }

};


using Rest::uri_result_to_string;


#endif //NIMBLE_REQUESTS_H
