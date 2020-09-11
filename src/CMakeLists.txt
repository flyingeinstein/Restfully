cmake_minimum_required(VERSION 3.5)

# package up the Nimble files into a static library
set(SOURCE_FILES Restfully.h
        # Endpoints.h Endpoints.cpp Mixins.h handler.h
        Globals.cpp
        Exception.h Exception.cpp
        UriRequest.h UriRequest.cpp
        StringPool.h StringPool.cpp Pool.cpp
        Literal.h Argument.h Token.h Pool.h Parser.h
        Platforms/platform.h Platforms/generics.h)
add_library(restfully STATIC ${SOURCE_FILES})
set_property(TARGET restfully PROPERTY CXX_STANDARD 14)

