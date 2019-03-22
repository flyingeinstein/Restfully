//
// Created by Colin MacKenzie on 2019-03-21.
//

#if !defined(RESTFULLY_ESP32_H) && (defined(ARDUINO_ARCH_ESP32) || defined(ESP32))
#define RESTFULLY_ESP32_H

#include "ArduinoPlatform.h"  // from Restfully platform
#include <WebServer.h>


namespace Rest {
    namespace Generics {

        namespace Configs {
            // Config [  TWebServer, TRequestFragment,TResponseFragment,TWebServerBaseRequestHandler  ]

            using Esp32Config = Generics::Config<
                    ::WebServer,             // using the Arduino Esp8266 based web server
                    ArduinoJson::Request,           // using a Request/Response structure based on the ArduinoJson dependancy
                    ArduinoJson::Response,
                    ::RequestHandler                // Arduino's generic RequestHandler class
            >;
        }
    }

    namespace Platforms {

        // create a new Arduino Restfully platform around our Esp8266 hardware config
        using Esp32 = Rest::Generics::ArduinoPlatform< Rest::Generics::Configs::Esp32Config >;

        // make this the default platform if one has not already been defined
#ifndef RESTFULLY_DEFAULT_PLATFORM
#define RESTFULLY_DEFAULT_PLATFORM
        using Default = Esp32;
#endif
    }
}


#endif //RESTFULLY_ESP8266_H
