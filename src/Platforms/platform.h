//
// Created by Colin MacKenzie on 2019-03-21.
//

#ifndef RESTFULLY_PLATFORM_H
#define RESTFULLY_PLATFORM_H

// this header include some generic templates for Platform, Config and Request
#include "generics.h"

// support more Arduino hardware by adding a new file. Refer to the Esp8266.h file, it should
// be pretty simple to define a new Config type for the new hardware.
#if defined(ARDUINO)
#include "Esp8266.h"
#endif

#endif //RESTFULLY_PLATFORM_H
