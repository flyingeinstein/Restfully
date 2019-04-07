cmake_minimum_required(VERSION 3.5)

# package up the Nimble files into a static library
set(SOURCE_FILES Restfully.h
        Endpoints.h Endpoints.cpp binbag.h binbag.cpp Pool.cpp Link.h Literal.h Argument.h Token.h Pool.h Parser.h
        handler.h Platforms/platform.h Platforms/generics.h)
add_library(restfully STATIC ${SOURCE_FILES})
set_property(TARGET restfully PROPERTY CXX_STANDARD 14)

