//
// Created by Colin MacKenzie on 2019-03-21.
//

#ifndef RESTFULLY_ARDUINO_H
#define RESTFULLY_ARDUINO_H

#include <ArduinoJson.h>        // from ArduinoJson library

#define HTTP_RESPONSE_SENT  -99

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

    namespace Arduino {
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

            Rest::HttpMethod TranslateHttpMethod(HTTPMethod requestMethod) {
                // convert our Http method enumeration
                switch(requestMethod) {
                    case HTTP_GET: return HttpGet;
                    case HTTP_POST: return HttpPost;
                    case HTTP_PUT: return HttpPut;
                    case HTTP_PATCH: return HttpPatch;
                    case HTTP_DELETE: return HttpDelete;
                    case HTTP_OPTIONS: return HttpOptions;
                    default: return HttpMethodAny;
                }
            }

            virtual bool canHandle(HTTPMethod requestMethod, String uri) {
                Rest::HttpMethod method = TranslateHttpMethod(requestMethod);
                return endpoints.queryAccept(method, uri.c_str()) >0;
            }

            virtual bool handle(WebServerType &server, HTTPMethod requestMethod, String requestUri) {
                Rest::HttpMethod method = TranslateHttpMethod(requestMethod);
                typename Endpoints::Request req = typename Endpoints::Request(method, requestUri.c_str());

                // add content-type header
                req.contentType = server.hasHeader("content-type")
                        ? server.header("content-type").c_str()
                        : Rest::ApplicationJsonMimeType;

                if (endpoints.resolve(req)) {
                    // convert the Endpoints::Request (UriRequest) into a Rest Request object (with request/response text, etc)
                    RequestType request(server, req);
                    request.timestamp = millis();
                    request.contentType = req.contentType;

                    // check for POST data and parse if it exists
                    if(request.contentType==Rest::ApplicationJsonMimeType && server.hasArg("plain")) {
                        DeserializationError error = deserializeJson(
                                request.body,
                                server.arg("plain")
                        );
                        if(error) {
                            // generate an error response
                            request.httpStatus = 400;
                            server.send(
                                    400,              // Bad Request
                                    "text/plain",     // plain text error
                                    String("expected Json in POST data : ")+error.c_str()     // error string from json parse
                                    );
                            return true;
                        } else
                            request.hasJson = true;
                    }

                    int rs = req.handler(request);
                    if(rs == HTTP_RESPONSE_SENT)
                        return true;    // handler sent its own response (probably non-Json)

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
                    server.send(request.httpStatus, Rest::ApplicationJsonMimeType, content);
                    return true;
                }

                // handler or object not found
                sendError(server, 404);
                server.send(404, "text/plain", "Not found");
                return true;
            }

            void sendError(WebServerType &server, const Error& error) {
                server.sendHeader("x-api-code", String(error.code) );
                if(error.message.length() >0)
                    server.sendHeader("x-api-message", error.message );
            }

            virtual int defer(Endpoints &endpoints, TRequest &parent) {
                String _uri_rest = (String)parent["_url"];  // contains the remaining part of the URL
                typename Endpoints::Request ep = endpoints.resolve(parent.method, _uri_rest.c_str());
                if (ep) {
                    RequestType request(parent);
#if 1
                    request.contentType = Rest::ApplicationJsonMimeType;
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

            using WebServerRequestHandler = Rest::Generics::WebServerRequestHandler<
                    WebServer,
                    Request,
                    typename TConfig::WebServerBaseRequestHandler
            >;

            using HandlerType = typename WebServerRequestHandler::HandlerType;
            using Endpoints = typename WebServerRequestHandler::Endpoints;
        };

    }
}


#endif //RESTFULLY_ARDUINO_H
