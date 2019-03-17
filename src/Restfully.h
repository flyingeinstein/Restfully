
#pragma once

#include "Rest-Esp.h"

using RestRequestHandler = EspRestRequestHandler;
using RestRequest = EspRestRequest;  // RestRequestHandler::RequestType RestRequest;
using HandlerType = typename RestRequestHandler::HandlerType;
using Endpoints = typename RestRequestHandler::Endpoints;
