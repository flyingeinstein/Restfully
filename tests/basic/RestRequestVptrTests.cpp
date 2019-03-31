
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

    using Handler = int(VptrTest::*)(RestRequest&);
    using Endpoints = Rest::Endpoints< Handler >;

    VptrTest() : greeting("Hello") {}

    virtual int echo(RestRequest& r) {
        r.response = greeting + " " + (const char*)r["msg"];
        return 200;
    }

    int echo2(RestRequest& r) {
        r.response = "Greeting is " + greeting;
        return 200;
    }
};

class Vptr2Test : public VptrTest {
public:
    using Handler = int(Vptr2Test::*)(RestRequest&);
    using Endpoints = Rest::Endpoints< Handler >;

    int echo(RestRequest& r) {
        r.response = "I say, " + greeting + " " + (const char*)r["msg"];
        return 200;
    }

    int eko(RestRequest& r) {
        r.response = "he said, " + greeting + " " + (const char*)r["msg"];
        return 200;
    }
};

//template<class Klass, class H> Rest::Handler<Klass*, H&> GET(int (Klass::*handler)(H&)) { return Rest::Handler<Klass*, H&>(HttpGet, std::function<int(Klass*, H&)>(handler)); }

TEST(endpoints_vptr_decl)
{
    VptrTest::Endpoints endpoints;
    return OK;
}

TEST(endpoints_vptr_on_echo)
{
    VptrTest::Endpoints endpoints;
    endpoints.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);
    return OK;
}

TEST(endpoints_vptr_resolve_echo_class_member)
{
    std::string response;
    VptrTest one;
    VptrTest::Endpoints endpoints;

    endpoints.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    auto endpoint = endpoints.resolve(HttpGet, "/api/echo/Maya");
    if(endpoint) {
        RestRequest rr(endpoint);
        VptrTest::Handler h = endpoint.handler;
        if( (one.*h)(rr) !=200)
            return FAIL;
        if(rr.response !="Hello Maya")
            return FAIL;
    } else
        return FAIL;    // handle returned fail

    return OK;
}

TEST(endpoints_vptr_resolve_derived_echo_class_member)
{
    std::string response;
    Vptr2Test one;
    VptrTest::Endpoints endpoints;

    endpoints.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    auto endpoint = endpoints.resolve(HttpGet, "/api/echo/Maya");
    if(endpoint) {
        RestRequest rr(endpoint);
        VptrTest::Handler h = endpoint.handler;
        if( (one.*h)(rr) !=200)
            return FAIL;
        if(rr.response !="I say, Hello Maya")
            return FAIL;
    } else
        return FAIL;    // handle returned fail

    return OK;
}

#if 1
TEST(endpoints_vptr_resolve_derived_eko)
{
    std::string response;
    Vptr2Test one;
    Vptr2Test::Endpoints endpoints;

    endpoints.on("/api/echo/:msg(string|integer)").GET(&Vptr2Test::eko);

    auto endpoint = endpoints.resolve(HttpGet, "/api/echo/Maya");
    if(endpoint) {
        RestRequest rr(endpoint);
        std::function<int(RestRequest&)> ff = std::bind(&Vptr2Test::eko, &one, std::placeholders::_1);
        //if( (one.*(endpoint.handler) )(rr) !=200)
        if( ff(rr) !=200)
            return FAIL;
        if(rr.response !="he said, Hello Maya")
            return FAIL;
    } else
        return FAIL;    // handle returned fail

    return OK;
}
#endif

//using Handler = int(VptrTest::*)(RestRequest&);
//using Endpoints = Rest::Endpoints< Handler >;

TEST(endpoints_curry_with_class_method)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest one;
    Endpoints1 endpoints1;
    VptrTest::Endpoints endpoints2;
    endpoints1
            .on("/api")
            .with(one, endpoints2)
            .on("echo/:msg(string|integer)")
            .GET(&VptrTest::echo);
    return OK;
}

TEST(endpoints_curry_with_class_method_resolve)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    Vptr2Test inst;
    inst.greeting = "Bonjour";

    Endpoints1 endpoints1;
    VptrTest::Endpoints endpoints2;
    endpoints1
        .on("/api")
        .with(inst, endpoints2)
            .on("echo/:msg(string|integer)")
            .GET(&VptrTest::echo2);

    Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/echo/johndoe");
    if(res.method!=Rest::HttpGet || res.status!=Rest::UriMatched)
        return FAIL;

    RestRequest r(res);
    if(res.handler(r) >=0) {
        if(r.response != "Greeting is Bonjour")
            return FAIL;
    }
    return OK;
}

TEST(endpoints_curry_with_anonymous_class_method_resolve)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    Endpoints1 endpoints1;
    endpoints1
            .on("/api")
            .with(cowboy)
            .on("echo/:msg(string|integer)")
            .GET(&VptrTest::echo2);

    Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/echo/johndoe");
    if(res.method!=Rest::HttpGet || res.status!=UriMatched)
        return FAIL;

    RestRequest r(res);
    if(res.handler(r) >=0) {
        if(r.response != "Greeting is Howdy")
            return FAIL;
    }

    return OK;
}

TEST(endpoints_curry_with_anonymous_class_method_inst_resolver)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy, pirate, damsel;
    cowboy.greeting = "Howdy";
    pirate.greeting = "Ahoy";
    damsel.greeting = "Big Boy";

    std::function<VptrTest&(Rest::UriRequest&)> resolve_instance = [&cowboy, &pirate, &damsel](Rest::UriRequest& rr) -> VptrTest& {
        int idx = rr["idx"];
        switch(idx) {
            case 0: return cowboy;
            case 1: return pirate;
            default: return damsel;
        }
    };

    Endpoints1 endpoints1;
    endpoints1
        .on("/api")
        .with(resolve_instance)
            .on("echo/:idx(integer)/:msg(string|integer)")
            .GET(&VptrTest::echo2);

    // Pirate
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/echo/1/johndoe");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Ahoy")
                return FAIL;
        }
    }

    // Cowboy
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/echo/0/johndoe");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Howdy")
                return FAIL;
        }
    }

    // Damsel
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/echo/2/johndoe");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Big Boy")
                return FAIL;
        }
    }
    return OK;
}

#if 0
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