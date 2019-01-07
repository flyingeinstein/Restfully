
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "requests.h"


#if 0
template<class H> Rest::MethodHandler<std::function<H> > PUTR(const H& handler) {
    auto f = std::function<H>(handler);
    return Rest::MethodHandler<std::function<H> >(HttpPut, f);
}
#endif

TEST(endpoints_std_function)
{
    RestRequestHandler<RestRequest> rest;
    std::function<int(RestRequest & )> func = [](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    };
    rest.on("/api/echo/:msg(string|integer)", GET(func));
    return OK;
}

int handler_func(RestRequest& r) { r.response = "hello world"; return 2; }

TEST(endpoints_function)
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)", GET(handler_func));
    return OK;
}

//template<class H> Rest::Handler<H&> GETT(std::function< int(H&) > handler) { return Rest::Handler<H&>(HttpGet, handler); }
//Rest::Handler<RestRequest&> GETT(std::function< int(RestRequest&) > handler) { return Rest::Handler<RestRequest&>(HttpGet, handler); }

TEST(endpoints_lambda)
{
    RestRequestHandler<RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)", GET([](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    }));
    return OK;
}

TEST(endpoints_split_collection)
{
    char msg[256];
    RestRequestHandler<RestRequest> rest;
    RestRequestHandler<RestRequest> dev1;
    RestRequestHandler<RestRequest> dev2;
    rest.on("/api/device/:dev(integer)/*", GET([&msg,&dev1,&dev2](RestRequest &request) {
        const char* url = (const char*)request.args["_url"];
        int devid = (long)request.args["dev"];
        switch(devid) {
            case 1: dev1.handle(HttpGet, url, &request.response); break;
            case 2: dev2.handle(HttpGet, url, &request.response); break;
            default:
                sprintf(msg, "no device %d", devid);
                request.response = msg;
                break;
        }
        return 200;
    }));
    dev1.on("echo/:msg(string|integer)", GET([&msg](RestRequest &request) {
        sprintf(msg, "Hello %s from Device1", (const char*)request.args["msg"]);
        request.response = msg;
        return 200;
    }));
    dev2.on("echo/:msg(string|integer)", GET([&msg](RestRequest &request) {
        sprintf(msg, "Hello %s from Device2", (const char*)request.args["msg"]);
        request.response = msg;
        return 200;
    }));

    std::string response;
    if(rest.handle(HttpGet, "/api/device/2/echo/Colin", &response)) {
        //printf("response: %s\n", response.c_str());
        if(response !="Hello Colin from Device2")
            return FAIL;
    }

    if(rest.handle(HttpGet, "/api/device/1/echo/Maya", &response)) {
        //printf("response: %s\n", response.c_str());
        if(response !="Hello Maya from Device1")
            return FAIL;
    }

    if(rest.handle(HttpGet, "/api/device/3/echo/Maya", &response)) {
        //printf("response: %s\n", response.c_str());
        if(response !="no device 3")
            return FAIL;
    }

    return OK;
}
