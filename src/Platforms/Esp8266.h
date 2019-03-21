//
// Created by Colin MacKenzie on 2019-03-21.
//

#if !defined(RESTFULLY_ESP8266_H) && defined(ARDUINO_ARCH_ESP8266)
#define RESTFULLY_ESP8266_H

#include "Arduino.h"  // from Restfully platform

// Typical Arduino compilation defines
// -DARDUINO=10807 -DARDUINO_ESP8266_NODEMCU -DARDUINO_ARCH_ESP8266 "-DARDUINO_BOARD=\"ESP8266_NODEMCU\"" -DESP8266

namespace Rest {
    namespace Generics {

        namespace Configs {
            // Config [  TWebServer, TRequestFragment,TResponseFragment,TWebServerBaseRequestHandler  ]

            using Esp8266Config = Generics::Config<
                    ::ESP8266WebServer,             // using the Arduino Esp8266 based web server
                    ArduinoJson::Request,           // using a Request/Response structure based on the ArduinoJson dependancy
                    ArduinoJson::Response,
                    ::RequestHandler                // Arduino's generic RequestHandler class
            >;
        }
    }

    namespace Platforms {

        // create a new Arduino Restfully platform around our Esp8266 hardware config
        using Esp8266 = Rest::Generics::ArduinoPlatform< Rest::Generics::Configs::Esp8266Config >;

        // make this the default platform if one has not already been defined
#ifndef RESTFULLY_DEFAULT_PLATFORM
#define RESTFULLY_DEFAULT_PLATFORM
        using Default = Esp8266;
#endif
    }
}


#endif //RESTFULLY_ESP8266_H
