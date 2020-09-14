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
            using Endpoint = Parser<TRequest>;

            typename Endpoint::Delegate* endpoints;

            // types
            using RequestType = TRequest;
            using WebServerType = TWebServer;

            WebServerRequestHandler()
                : endpoints(nullptr) {
            }

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

            virtual bool canHandle(HTTPMethod requestMethod, String requestUri) {
                if(!endpoints)
                    return false;

                //Rest::HttpMethod method = TranslateHttpMethod(requestMethod);
                // todo: find out a way to just check for acceptance before parsing
                //return endpoints.queryAccept(method, uri.c_str()) >0;
                Rest::HttpMethod method = TranslateHttpMethod(requestMethod);

                // todo: not liking that I am passing null for server!
                RequestType request(*(WebServerType*)nullptr, method, requestUri.c_str());

                // set the intent of the request only to check acceptance!
                request.intent = UriRequest::Accept;

                // convert the Endpoints::Request (UriRequest) into a Rest Request object (with request/response text, etc)
                request.timestamp = millis();

                Endpoint root(request);
                endpoints->delegate(root);

                return request.isSuccessful();
            }

            virtual bool handle(WebServerType &server, HTTPMethod requestMethod, String requestUri) {
                Rest::HttpMethod method = TranslateHttpMethod(requestMethod);
                RequestType request(server, method, requestUri.c_str());

                // add content-type header
                request.contentType = server.hasHeader("content-type")
                        ? server.header("content-type").c_str()
                        : Rest::ApplicationJsonMimeType;


                // convert the Endpoints::Request (UriRequest) into a Rest Request object (with request/response text, etc)
                request.timestamp = millis();

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

                Endpoint root(request);
                int code = (endpoints && endpoints->delegate(root)) || 404;
                if(code == HTTP_RESPONSE_SENT)
                    return true;    // handler sent its own response (probably non-Json)


                // send error code
                sendError(server, request.result);

                // send output to server
                String content;
                serializeJson(request.response, content);
                server.send(code, Rest::ApplicationJsonMimeType, content);
                return true;


                // handler or object not found
                //sendError(server, 404);
                //server.send(404, "text/plain", "Not found");
                //return true;
            }

            void sendError(WebServerType &server, const Error& error) {
                server.sendHeader("x-api-code", String(error.code) );
                if(error.message.length() >0)
                    server.sendHeader("x-api-message", error.message );
            }

#if 0
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
#endif
        };


        template<typename TConfig>
        class ArduinoPlatform {
        public:
            using WebServer = typename TConfig::WebServer;
            using RequestFragment = typename TConfig::RequestFragment;
            using ResponseFragment = typename TConfig::ResponseFragment;

            using Request = Rest::Generics::Request<
                    typename TConfig::WebServer,
                    typename TConfig::RequestFragment,
                    typename TConfig::ResponseFragment
            >;

            using WebServerRequestHandler = Rest::Generics::WebServerRequestHandler<
                    typename TConfig::WebServer,
                    Request,
                    typename TConfig::WebServerBaseRequestHandler
            >;

            using Endpoint = Parser<Request>;
        };

    }
}


#endif //RESTFULLY_ARDUINO_H
