//
// Created by Colin MacKenzie on 2019-03-21.
//

#ifndef RESTFULLY_PLATFORM_H
#define RESTFULLY_PLATFORM_H

// this header include some generic templates for Platform, Config and Request
#include "generics.h"

#include "../Parser.h"

// support more Arduino hardware by adding a new file. Refer to the Esp8266.h file, it should
// be pretty simple to define a new Config type for the new hardware.
#if defined(ARDUINO)
#include "Esp8266.h"
#include "Esp32.h"

// unknown target platform, use general types
namespace Rest {
    using Endpoint = Platforms::Default::Endpoint;
}

#else

// unknown target platform, use general types
namespace Rest {
    using Endpoint = Parser<UriRequest>;
}

#endif

namespace Rest {
    using GET = SimpleUriRequestHandler<HttpGet, typename Endpoint::Handler>;
    using POST = SimpleUriRequestHandler<HttpPost, typename Endpoint::Handler>;
    using PUT = SimpleUriRequestHandler<HttpPut, typename Endpoint::Handler>;
    using PATCH = SimpleUriRequestHandler<HttpPatch, typename Endpoint::Handler>;
    using DELETE = SimpleUriRequestHandler<HttpPut, typename Endpoint::Handler>;
    using OPTIONS = SimpleUriRequestHandler<HttpOptions, typename Endpoint::Handler>;
}
#endif //RESTFULLY_PLATFORM_H
