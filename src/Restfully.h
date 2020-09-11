
#pragma once

#include "Parser.h"

#include "Platforms/platform.h"

// The platform include should hopefully have detected what platform and hardware we are compiling on
// and automatically define the DefaultPlatform type.
#if defined(RESTFULLY_DEFAULT_PLATFORM)
using Endpoint = Rest::Platforms::Default::Endpoint;
using RestRequest = Rest::Platforms::Default::Request;
using WebServerRequestHandler = Rest::Platforms::Default::WebServerRequestHandler;
#endif
