//
// Created by Colin MacKenzie on 2019-03-21.
//

#ifndef RESTFULLY_ARDUINO_H
#define RESTFULLY_ARDUINO_H

#include <ArduinoJson.h>        // from ArduinoJson library

namespace Rest {
    namespace ArduinoJson {
        class Request {
        public:
            Request() {
                request = requestDoc.to<JsonObject>();
            }

            Request(const Request &copy) = default;

            /// The parsed POST as Json if the content-type was application/json
            DynamicJsonDocument requestDoc { 2048 };
            JsonObject request;
        };

        class Response {
        public:
            Response() {
                response = responseDoc.to<JsonObject>();
                // todo: errors array?
            }

            Response(const Response &copy) = default;

            /// output response object built by Rest method handler.
            /// This object is empty when the method handler is called and is up to the method handler to populate with the
            /// exception of standard errors, warnings and status output which is added when the method handler returns.
            DynamicJsonDocument responseDoc { 2048 };
            JsonObject response;

            // todo: we should have a << operator here to output to server Stream
        };
    }


    namespace Generics {

        template<
                class TWebServer,
                class TRequest,
                class TWebServerRequestHandler
        >
        class WebServerRequestHandler : public TWebServerRequestHandler {
        public:
            // types
            using RequestType = TRequest;
            using WebServerType = TWebServer;

            using HandlerType = Handler<TRequest &>;
            using Endpoints = Rest::Endpoints<HandlerType>;  // is really correct (it was RequestHandler)? endpoints reference self type???
            using EndpointNode = typename Endpoints::Node;

            // the collection of Rest handlers
            Endpoints endpoints;

            WebServerRequestHandler() = default;

            virtual bool canHandle(HTTPMethod method, String uri) {
                return true;
            }

            virtual bool handle(WebServerType &server, HTTPMethod requestMethod, String requestUri) {
                // convert our Http method enumeration
                Rest::HttpMethod method;
                switch(requestMethod) {
                    case HTTP_ANY: method = HttpMethodAny; break;
                    case HTTP_GET: method = HttpGet; break;
                    case HTTP_POST: method = HttpPost; break;
                    case HTTP_PUT: method = HttpPut; break;
                    case HTTP_PATCH: method = HttpPatch; break;
                    case HTTP_DELETE: method = HttpDelete; break;
                    case HTTP_OPTIONS: method = HttpOptions; break;
                    default: return false;
                }

                typename Endpoints::Request ep = endpoints.resolve(method, requestUri.c_str());
                if (ep) {
                    RequestType request(server, ep);
                    request.contentType = "application/json";
                    request.timestamp = millis();

                    int rs = ep.handler(request);
                    if (request.httpStatus == 0) {
                        if (rs == 0)
                            request.httpStatus = 200;
                        else if (rs < 200)
                            request.httpStatus = 400;
                        else
                            request.httpStatus = rs;
                    }

                    // make a << operator to send output to server response
                    String content;
                    serializeJson(request.response, content);
                    server.send(request.httpStatus, "application/json", content);
                    return true;
                }
                return false;
            }

            virtual int defer(Endpoints &endpoints, TRequest &parent) {
                String _uri_rest = parent["_url"];  // contains the remaining part of the URL
                typename Endpoints::Request ep = endpoints.resolve(parent.method, _uri_rest.c_str());
                if (ep) {
                    RequestType request(parent);
#if 1
                    request.contentType = "application/json";
                    request.timestamp = millis();
                    //request.args = request.args + parent.args;
#elif 0
                    request.args = request.args + ep;
#endif
                    int rv = ep.handler(request);
                    //parent.response = request.response;
                    //parent.requestDoc = request.requestDoc;
                    return rv;
                }
                return 404;
            }

            // deprecated: this operator will probably disappear soon
            Endpoints *operator->() { return &endpoints; }

            // inline delegate calls to Endpoints class
            inline EndpointNode on(const char* expression) { return endpoints.on(expression); }

        };


        template<typename TConfig>
        class ArduinoPlatform {
        public:
            using WebServer = typename TConfig::WebServer;
            using RequestFragment = typename TConfig::RequestFragment;
            using ResponseFragment = typename TConfig::ResponseFragment;

            using Request = Rest::Generics::Request<
                    WebServer,
                    Rest::UriRequest,
                    RequestFragment,
                    ResponseFragment
            >;

            using Endpoints = Rest::Endpoints<Rest::Handler<Request> >;

            using WebServerRequestHandler = Rest::Generics::WebServerRequestHandler<
                    WebServer,
                    Request,
                    typename TConfig::WebServerBaseRequestHandler
            >;
        };

    }
}


#endif //RESTFULLY_ARDUINO_H
