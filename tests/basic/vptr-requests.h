//
// Created by Colin MacKenzie on 2018-12-18.
//

#ifndef NIMBLE_REQUESTS_VPTR_H
#define NIMBLE_REQUESTS_VPTR_H

#include "requests.h"

#include <vector>


template<class Klass, class TRestRequest>
class RestRequestVptrHandler
{
public:
    // types
    typedef TRestRequest RequestType;

    typedef Rest::Handler< Klass*, TRestRequest& > RequestHandler;
    typedef std::vector<RequestHandler> Handlers;

    typedef typename Handlers::size_type size_type;

protected:
    // when storing handlers in Endpoints tree we must attach the method to the handler
    // todo: maybe we can change this, do we really have to return the method when we pass it into Endpoints already?
    class IndexedHandler
    {
    public:
        HttpMethod method;
        size_type handler;

        IndexedHandler() : method(HttpGet), handler(0) {}
        IndexedHandler(std::nullptr_t np) : method(HttpGet), handler(0) {}
        IndexedHandler(size_type _index) : method(HttpGet), handler(_index) {}
        IndexedHandler(HttpMethod _method, size_type _index) : method(_method), handler(_index) {}
    };

    typedef Rest::Endpoints<IndexedHandler> Endpoints;

public:
    // the collection of Rest handlers
    Endpoints endpoints;
    Handlers handlers;

    Klass* instance;

    RestRequestVptrHandler() : instance(nullptr) {}

    virtual bool handle(HttpMethod requestMethod, std::string requestUri, std::string* response_out=NULL) {
        Rest::HttpMethod method = (Rest::HttpMethod)requestMethod;
        if(instance== nullptr)
            return false;

        typename Endpoints::Request ep = endpoints.resolve(method, requestUri.c_str());
        if (ep) {
            RequestType request(ep);
            request.method = method;
            request.uri = requestUri;
            RequestHandler handler = handlers[ ep.handler.handler ];
            handler(instance, request);
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


#endif //NIMBLE_REQUESTS_H
