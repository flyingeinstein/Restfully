cmake_minimum_required(VERSION 3.5)

# package up the Nimble files into a static library
set(SOURCE_FILES Restfully.h
        Rest.h Rest.cpp binbag.h binbag.cpp Link.h Literal.h Argument.h Token.h Pool.h Pool.cpp Parser.h
        handler.h
        Rest-Esp8266.h)
add_library(restfully STATIC ${SOURCE_FILES})
set_property(TARGET restfully PROPERTY CXX_STANDARD 11)

