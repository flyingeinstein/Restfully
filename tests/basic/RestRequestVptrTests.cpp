
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>

#include "requests.h"


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
    if(res.method!=Rest::HttpGet || res.status!=Rest::UriMatched)
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

TEST(endpoints_curry_with_anonymous_class_method_inst_resolver_bug)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest&(Rest::UriRequest&)> resolve_instance = [&cowboy](Rest::UriRequest& rr) -> VptrTest& {
        return cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1
            .on("/sensors/:id(string|integer)")
            .with(resolve_instance)
                .on("info")
                .GET(&VptrTest::echo2);

    // Cowboy
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/sensors/0/info");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Howdy")
                return FAIL;
        }
    }
    return OK;
}

TEST(endpoints_curry_with_anonymous_class_method_inst_resolver_at_root)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest&(Rest::UriRequest&)> resolve_instance = [&cowboy](Rest::UriRequest& rr) -> VptrTest& {
        return cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1
            .on("/sensors/:id(string|integer)")
            .with(resolve_instance)
            .GET(&VptrTest::echo2);

    // Cowboy
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/sensors/0");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Howdy")
                return FAIL;
        }
    }
    return OK;
}

TEST(endpoints_curry_with_anonymous_class_method_inst_resolver_devices)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest&(Rest::UriRequest&)> resolve_instance = [&cowboy](Rest::UriRequest& rr) -> VptrTest& {
        long dev = rr["device"];
        if(dev != 5)
            rr.abort(404);
        return cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1
            .on("/api/devices/:device(string|integer)")
            .with(resolve_instance)
            .GET(&VptrTest::echo2);

    // Cowboy
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/devices/5");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Howdy")
                return FAIL;
        }
    }
    return OK;
}

TEST(endpoints_curry_with_anonymous_class_method_inst_resolver_devices_404)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest&(Rest::UriRequest&)> resolve_instance = [&cowboy](Rest::UriRequest& rr) -> VptrTest& {
        long dev = rr["device"];
        if(dev != 5)
            rr.abort(404);
        return cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1
            .on("/api/devices/:device(string|integer)")
            .with(resolve_instance)
            .GET(&VptrTest::echo2);

    // Cowboy
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/devices/4");
        if (res.status == 404)
            return OK;
    }
    return FAIL;
}

TEST(endpoints_curry_with_anonymous_class_method_instptr_resolver_devices)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest*(Rest::UriRequest&)> resolve_instance = [&cowboy](Rest::UriRequest& rr) -> VptrTest* {
        long dev = rr["device"];
        if(dev != 5) {
            rr.abort(404);
            return nullptr;
        }
        return &cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1
            .on("/api/devices/:device(string|integer)")
            .with(resolve_instance)
            .GET(&VptrTest::echo2);

    // Cowboy
    {
        Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/devices/5");
        if (res.method != Rest::HttpGet || res.status != Rest::UriMatched)
            return FAIL;

        RestRequest r(res);
        if (res.handler(r) >= 0) {
            if (r.response != "Greeting is Howdy")
                return FAIL;
        }
    }
    return OK;
}

TEST(endpoints_curry_with_anonymous_class_method_instptr_resolver_devices_404)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest*(Rest::UriRequest&)> resolve_instance = [&cowboy](Rest::UriRequest& rr) -> VptrTest* {
        long dev = rr["device"];
        if(dev != 5) {
            rr.abort(404);
            return nullptr;
        }
        return &cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1
            .on("/api/devices/:device(string|integer)")
            .with(resolve_instance)
            .GET(&VptrTest::echo2);

    // Cowboy
    Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/devices/4");
    return (res.method == Rest::HttpGet && res.status != Rest::UriMatched && res.handler == nullptr)
        ? OK
        : FAIL;
}

TEST(endpoints_curry_nimble_test1)
{
    using Handler1 = Rest::Handler<RestRequest&>;
    using Endpoints1 = Rest::Endpoints< Handler1 >;

    VptrTest cowboy;
    cowboy.greeting = "Howdy";

    std::function<VptrTest*(Rest::UriRequest&)> device_resolver = [&cowboy](Rest::UriRequest& rr) -> VptrTest* {
        long dev = rr["xxx"];
        if(dev != 5) {
            rr.abort(404);
            return nullptr;
        }
        return &cowboy;
    };

    Endpoints1 endpoints1;
    endpoints1.on("/api/echo/:msg(string|integer)")
            .GET( [](RestRequest& request) { return 200; } );
    endpoints1.on("/api/devices")
            .GET([](RestRequest& request) { return 200; });
    endpoints1.on("/api/dev/:xxx(string|integer)/info")
            .with(device_resolver)
            .GET(&VptrTest::echo2);

    // Cowboy
    Endpoints1::Request res = endpoints1.resolve(Rest::HttpGet, "/api/dev/4/info");
    return (res.method == Rest::HttpGet && res.status != Rest::UriMatched && res.handler == nullptr)
           ? OK
           : FAIL;
}

TEST(endpoints_vptr_resolve_echo)
{
    using Endpoints = Rest::Endpoints< int(VptrTest::*)(RestRequest&) >;
    Endpoints rest;  // endpoints based on this member function type
    VptrTest one;

    rest.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    Endpoints::Request request = rest.resolve(HttpGet, "/api/echo/Maya");
    if(request) {
        // execute the handler and check result
        RestRequest req(request);
        //Endpoints::Handler = &VptrTest::echo;
        int x = (one.*request.handler)(req);
        if(x==200 && req.response !="Hello Maya")
            return FAIL;
    } else
        return FAIL;    // handle returned fail

    return OK;
}

TEST(endpoints_vptr_resolve_echo_instance)
{
    using Endpoints = Rest::Endpoints< int(VptrTest::*)(RestRequest&) >;
    Endpoints rest;  // endpoints based on this member function type

    VptrTest one;
    one.greeting = "Dzien Dobry";

    rest.on("/api/echo/:msg(string|integer)").GET(&VptrTest::echo);

    Endpoints::Request request = rest.resolve(HttpGet, "/api/echo/Maya");
    if(request) {
        // execute the handler and check result
        RestRequest req(request);
        //Endpoints::Handler = &VptrTest::echo;
        int x = (one.*request.handler)(req);
        if(x==200 && req.response !="Dzien Dobry Maya")
            return FAIL;
    } else
        return FAIL;    // handle returned fail

    return OK;
}

