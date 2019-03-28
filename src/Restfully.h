
#pragma once

#include "Platforms/platform.h"

// The platform include should hopefully have detected what platform and hardware we are compiling on
// and automatically define the DefaultPlatform type.
#if defined(RESTFULLY_DEFAULT_PLATFORM)
using Endpoints = Rest::Platforms::Default::Endpoints;
using RestRequest = Rest::Platforms::Default::Request;
using UriRequest = Rest::UriRequest;
using RestRequestHandler = Rest::Platforms::Default::WebServerRequestHandler;
#endif
