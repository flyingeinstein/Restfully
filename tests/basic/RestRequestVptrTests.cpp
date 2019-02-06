
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "vptr-requests.h"



class VptrTest {
public:
    std::string greeting;

    VptrTest() : greeting("Hello") {}

    int echo(RestRequest& r) {
        r.response = greeting + " " + (const char*)r.args["msg"];
        return 200;
    }
};

//template<class Klass, class H> Rest::Handler<Klass*, H&> GET(int (Klass::*handler)(H&)) { return Rest::Handler<Klass*, H&>(HttpGet, std::function<int(Klass*, H&)>(handler)); }


TEST(endpoints_vptr_on_echo)
{
    RestRequestVptrHandler<VptrTest, RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);
    return OK;
}

TEST(endpoints_vptr_resolve_echo)
{
    std::string response;
    VptrTest one;
    RestRequestVptrHandler<VptrTest, RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    rest.instance = &one;
    if(rest.handle(HttpGet, "/api/echo/Maya", &response)) {
        if(response !="Hello Maya")
            return FAIL;
    } else
        return FAIL;    // handle returned fail

    return OK;
}

TEST(endpoints_vptr_resolve_with_null_instance)
{
    std::string response;
    RestRequestVptrHandler<VptrTest, RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    // no instance set on 'rest' object
    if(rest.handle(HttpGet, "/api/echo/Maya", &response)) {
        if(response !="Hello Maya")
            return FAIL;
    } else
        return OK;    // handle returned fail (which it should)

    return FAIL;
}

TEST(endpoints_vptr_resolve_echo_instance)
{
    std::string response;
    VptrTest one;
    one.greeting = "Dzien Dobry";
    RestRequestVptrHandler<VptrTest, RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    rest.instance = &one;
    if(rest.handle(HttpGet, "/api/echo/Maya", &response)) {
        //printf("response: %s\n", response.c_str());
        if(response !="Dzien Dobry Maya")
            return FAIL;
    }

    return OK;
}

//template<class H> Rest::Handler<H&> GETT(std::function< int(H&) > handler) { return Rest::Handler<H&>(HttpGet, handler); }
//Rest::Handler<RestRequest&> GETT(std::function< int(RestRequest&) > handler) { return Rest::Handler<RestRequest&>(HttpGet, handler); }

#if 0
TEST(endpoints_vptr_lambda)
{
    RestRequestVptrHandler<VptrTest, RestRequest> rest;
    rest.on("/api/echo/:msg(string|integer)", GET([](RestRequest &request) {
        request.response = "Hello World!";
        return 200;
    }));
    return OK;
}

TEST(endpoints_vptr_split_collection)
{
    char msg[256];
    RestRequestVptrHandler<RestRequest> rest;
    RestRequestVptrHandler<RestRequest> dev1;
    RestRequestVptrHandler<RestRequest> dev2;
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
#endif