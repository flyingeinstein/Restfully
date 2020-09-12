cmake_minimum_required(VERSION 3.5)

# package up the Nimble files into a static library
set(SOURCE_FILES Restfully.h
        # Endpoints.h Endpoints.cpp Mixins.h handler.h Literal.h
        Globals.cpp
        Exception.h Exception.cpp
        UriRequest.h UriRequest.cpp
        Pool.h Pool.cpp
        StringPool.h StringPool.cpp
        Name.h Variant.h Parameter.h Argument.h
        Token.h Parser.h
        Platforms/platform.h Platforms/generics.h)
add_library(restfully STATIC ${SOURCE_FILES})
set_property(TARGET restfully PROPERTY CXX_STANDARD 14)

