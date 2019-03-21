#pragma once

#include <ESP8266WebServer.h>
#include "Rest.h"




namespace Rest {
    namespace Generics {

        /// Config Template - Configure a package of types to build a Rest platform
        /// The types you define using this template can be used as a parameter to the Platform<> template to build a
        /// complete system of Rest classes around a platform-specific set of web server, Json or other custom
        /// implementations. The Restfully.h include will usually auto-detect and configure the right Rest platform
        /// based on your hardware and included libraries. For advanced features and custom types you may configure
        /// your own using a custom Config or custom Platform specification.
        ///
        /// The fragment types are all merged into a final Request object.
        /// \tparam TWebServer         - Arduino WebServer or derived class
        /// \tparam TRequestHandler    - class receives WebServer events and delegates request to Rest Endpoints (typically this is the Arduino platforms WebServer RequestHandler)
        /// \tparam TRequestFragment   - this type will receive our POST data (typically parsed int an ArduinoJSON document)
        /// \tparam TResponseFragment  - this type will collect our response output (typically based on ArduinoJSON)
        template<
                class TWebServer,
                class TRequestFragment,
                class TResponseFragment,
                class TWebServerBaseRequestHandler = decltype(TWebServer::_firstHandler)
        >
        struct Config
        {
            using WebServer = TWebServer;
            using WebServerBaseRequestHandler = TWebServerBaseRequestHandler;
            using RequestFragment = TRequestFragment;
            using ResponseFragment = TResponseFragment;
        };


        /// \brief Main argument to Rest Callbacks
        /// This structure is created and passed to Rest Method callbacks after the request Uri
        /// and Endpoint has been resolved. Any arguments in the Uri Endpoint expression will be
        /// added to the request json object and merged with POST data or other query parameters.
        template<
                class TWebServer,
                class TUriRequestFragment,
                class TRequestFragment,
                class TResponseFragment
        >
        class Request : public TUriRequestFragment, public TRequestFragment, public TResponseFragment {
          public:
            using WebServerType = TWebServer;
            using RequestType = TRequestFragment;
            using ResponseType = TResponseFragment;

            WebServerType& server;

            /// content type of the incoming request (should always be application/json)
            const char* contentType;

            unsigned long long timestamp;   // timestamp request was received, set by framework

            short httpStatus;               // return status sent in response

#if 0
            template<typename ... ReqArgs, typename ... UriArgs, typename ... RespArgs>
            Request(WebServerType& _server, UriArgs...uri_args, ReqArgs...request_args, RespArgs...response_args )
                : server(_server), TUriRequestFragment(uri_args...), RequestType(request_args...), ResponseType(response_args...),
                  contentType(nullptr), timestamp(0), httpStatus(0)
            {}
#endif
            Request(WebServerType& _server, const TUriRequestFragment& uri_request )
                    : server(_server), TUriRequestFragment(uri_request),
                      contentType(nullptr), timestamp(0), httpStatus(0)
            {}

            Request(Request& copy)
                : TRequestFragment(copy), TResponseFragment(copy),
                  server(copy.server), contentType(copy.contentType), timestamp(copy.timestamp), httpStatus(copy.httpStatus)
            {}
        };

    } // Rest::Generics

} // Rest

