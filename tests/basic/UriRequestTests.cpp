//
// Created by guru on 9/10/20.
//
#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <cstring>
#include <stdio.h>
#include <iostream>
#include <map>

#include <UriRequest.h>

void RequireToken(Rest::Token& t, const char* s) {
    REQUIRE(t.id == TID_STRING);
    REQUIRE( strcmp(t.s, s) == 0 );
}

TEST_CASE("parsed URI contains token words", "UriRequest") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/echo");
    REQUIRE(request.words.size() == 3);
    REQUIRE(request.words[0] == "api");
    REQUIRE(request.words[1] == "dev");
    REQUIRE(request.words[2] == "echo");
}

TEST_CASE("parsed URI handles integer nodes", "UriRequest") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/1/echo/john.doe");
    REQUIRE(request.words.size() == 5);
    REQUIRE(request.words[0] == "api");
    REQUIRE(request.words[1] == "dev");
    REQUIRE(request.words[2] == 1);
    REQUIRE(request.words[3] == "echo");
    REQUIRE(request.words[4] == "john.doe");
}

