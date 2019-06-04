#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include "requests.h"


TEST_CASE("Endpoints echo request with std::function lambda")
{
    RestRequestHandler<RestRequest> rest;
    std::function<int(RestRequest & )> func = [](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    };
    rest.on("/api/echo/:msg(string|integer)")
        .GET(func);
    auto req ( rest.endpoints.resolve(Rest::HttpGet, "/api/echo/colin") );
    REQUIRE( req );
}

int handler_func(RestRequest& r) { r.response = "hello world"; return 2; }

TEST_CASE("Endpoints echo request with function ptr")
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)").GET(handler_func);
    auto req ( rest.endpoints.resolve(Rest::HttpGet, "/api/echo/colin") );
    REQUIRE( req );
}

TEST_CASE("Endpoints request with lambda")
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)").GET([](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    });
    auto req ( rest.endpoints.resolve(Rest::HttpGet, "/api/echo/colin") );
    REQUIRE( req );
}

TEST_CASE("Endpoints split collection")
{
    char msg[256];
    RestRequestHandler<RestRequest> rest;
    RestRequestHandler<RestRequest> dev1;
    RestRequestHandler<RestRequest> dev2;
    rest.on("/api/device/:dev(integer)/*").GET([&msg,&dev1,&dev2](RestRequest &request) {
        auto url = (const char*)request.args["_url"];
        auto devid = (int)request.args["dev"];
        switch(devid) {
            case 1: dev1.handle(HttpGet, url, &request.response); break;
            case 2: dev2.handle(HttpGet, url, &request.response); break;
            default:
                sprintf(msg, "no device %d", devid);
                request.response = msg;
                break;
        }
        return 200;
    });
    dev1.on("echo/:msg(string|integer)").GET([&msg](RestRequest &request) {
        sprintf(msg, "Hello %s from Device1", (const char*)request.args["msg"]);
        request.response = msg;
        return 200;
    });
    dev2.on("echo/:msg(string|integer)").GET([&msg](RestRequest &request) {
        sprintf(msg, "Hello %s from Device2", (const char*)request.args["msg"]);
        request.response = msg;
        return 200;
    });

    std::string response;
    if(rest.handle(HttpGet, "/api/device/2/echo/Colin", &response)) {
        //printf("response: %s\n", response.c_str());
        REQUIRE (response =="Hello Colin from Device2" );
    }

    if(rest.handle(HttpGet, "/api/device/1/echo/Maya", &response)) {
        //printf("response: %s\n", response.c_str());
        REQUIRE (response !="Hello Maya from Device1" );
    }

    if(rest.handle(HttpGet, "/api/device/3/echo/Maya", &response)) {
        REQUIRE (response !="no device 3");
    }
}

TEST_CASE("Endpoints path containing dot (like versions)")
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/v1.0/echo/:msg(string|integer)").GET([](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    });
    auto req ( rest.endpoints.resolve(Rest::HttpGet, "/api/v1.0/echo/colin") );
    REQUIRE( req );
}
