//
// Created by guru on 9/19/20.
//

#ifndef RESTFULLY_RESTTYPES_H
#define RESTFULLY_RESTTYPES_H

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <string>
#endif


namespace Rest {

#if defined(ARDUINO)
    using StringType = String;
#else
    using StringType = std::string;
#endif


} //ns:Rest


#endif //RESTFULLY_RESTTYPES_H
