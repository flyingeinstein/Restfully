//
// Created by Colin MacKenzie on 2019-03-21.
//

#ifndef RESTFULLY_ARDUINO_H
#define RESTFULLY_ARDUINO_H

#include <ArduinoJson.h>        // from ArduinoJson library

namespace Rest {
    class Error {
    public:
        short code;
        String message;

        inline Error(short _code=0) : code(_code) {}
        inline Error(short _code, const char* _message) : code(_code), message(_message) {}

        Error(const Error& copy) : code(copy.code), message(copy.message) {}

        Error& operator=(const Error& copy) {
            code = copy.code;
            message = copy.message;
            return *this;
        }
    };

    namespace ArduinoJson {
        class Request {
        public:
            Request() {
            }

            Request(const Request &copy) = default;

            /// The parsed POST as Json if the content-type was application/json
            DynamicJsonDocument body { 256 };
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
            DynamicJsonDocument responseDoc { 512 };
            JsonObject response;

            // we can set an error code and it will be returned as a response header
            Error result;
            inline void error(short code) { result = Rest::Error(code); }
            inline void error(short code, const char* message) { result = Rest::Error(code, message); }
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
                    request.timestamp = millis();

                    if(request.server.hasHeader("content-type"))
                        request.contentType = request.server.header("content-type");

                    // check for POST data and parse if it exists
                    if(request.contentType=="application/json" && server.hasArg("plain")) {
                        DeserializationError error = deserializeJson(
                                request.body,
                                server.arg("plain")
                        );
                        if(error) {
                            // generate an error response
                            request.httpStatus = 400;
                            request.server.send(
                                    400,              // Bad Request
                                    "text/plain",     // plain text error
                                    String("expected Json in POST data : ")+error.c_str()     // error string from json parse
                                    );
                            return true;
                        } else
                            request.hasJson = true;
                    }

                    int rs = ep.handler(request);
                    if (request.httpStatus == 0) {
                        if (rs == 0)
                            request.httpStatus = 200;
                        else if (rs < 200)
                            request.httpStatus = 400;
                        else
                            request.httpStatus = rs;
                    }

                    // send error code
                    sendError(server, request.result);

                    // send output to server
                    String content;
                    serializeJson(request.response, content);
                    server.send(request.httpStatus, "application/json", content);
                    return true;
                }
                return false;
            }

            void sendError(WebServerType &server, const Error& error) {
                server.sendHeader("x-api-code", String(error.code) );
                if(error.message.length() >0)
                    server.sendHeader("x-api-message", error.message );
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
