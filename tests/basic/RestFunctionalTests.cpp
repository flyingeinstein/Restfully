
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include "../../Rest.h"

class Functional {
public:
    typedef std::function<void()> F0;
    typedef std::function<void(Rest::Endpoints<Functional>::Endpoint&)> F1;

    Functional() : args(0), f0(NULL) {}
    Functional(const F0& _f) : args(0), f0(new F0(_f)) {
    }
    Functional(const F1& _f) : args(1), f1(new F1(_f)) {
    }
    Functional(const Functional& copy) : args(copy.args) {
        switch(args) {
            case 0: f0=new F0(*copy.f0); break;
            case 1: f1=new F1(*copy.f1); break;
            default: assert(false);
        }
    }
    virtual ~Functional() {
        free();
    }

    Functional& operator=(const Functional& copy) {
        free();
        args = copy.args;
        switch(args) {
            case 0: f0=new F0(*copy.f0); break;
            case 1: f1=new F1(*copy.f1); break;
            default: assert(false);
        }
        return *this;
    }

    void operator()(Rest::Endpoints<Functional>::Endpoint& ep) {
        switch(ep.handler.args) {
            case 0: (*f0)(); break;
            case 1: (*f1)(ep); break;
            default: assert(false);
        }
    }

    void free() {
        switch(args) {
            case 0: delete f0; break;
            case 1: delete f1; break;
            default: assert(false);
        }
    }
    //inline bool operator==(const FunctionalHandler& rhs) const { return _name==rhs._name; }

    int args;
    union {
        void* f;
        F0* f0;  // non-rest handler doesnt prepare json request/response
        F1* f1;  // rest handler using Json request/response
    };
};

typedef Rest::Endpoints<Functional> FunctionalEndpoints;


using Rest::GET;
using Rest::PUT;
using Rest::POST;
using Rest::PATCH;
using Rest::DELETE;
using Rest::OPTIONS;

using Rest::HttpGet;
using Rest::HttpPut;
using Rest::HttpPost;
using Rest::HttpPatch;
using Rest::HttpDelete;
using Rest::HttpOptions;

using Rest::uri_result_to_string;


TEST(endpoints_int_argument_func)
{
    FunctionalEndpoints endpoints;
    int func_num = 0;
    Functional def( [&func_num]() { func_num = 2; });
    Functional getbus( [&func_num]() { func_num = 1; });

    endpoints
            .onDefault( def )
            .on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    FunctionalEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3/devices");
    if (r_1.status==URL_MATCHED && r_1["bus"].isInteger() && 3==(long)r_1["bus"]) {
        r_1.handler(r_1);
        return (func_num==1) ? OK : FAIL;
    } else
        return FAIL;
}

TEST(endpoints_int_argument_func_w_1arg)
{
    FunctionalEndpoints endpoints;
    int func_num = 0;
    Functional def( [&func_num]() { func_num = 2; });
    Functional getbus( [&func_num](Rest::Endpoints<Functional>::Endpoint& ep) { func_num = (ep.name=="api/bus/i2c/<int>/devices") ? 2 : 1; });

    endpoints
            .onDefault( def )
            .on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    FunctionalEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/i2c/3/devices");
    if (r_1.status==URL_MATCHED && r_1["bus"].isInteger() && 3==(long)r_1["bus"]) {
        r_1.handler(r_1);
        return (func_num==2) ? OK : FAIL;
    } else
        return FAIL;
}

TEST(endpoints_default_handler_called)
{
    FunctionalEndpoints endpoints;
    int func_num = 0;
    Functional def( [&func_num]() { func_num = 2; });
    Functional getbus( [&func_num]() { func_num = 1; });

    endpoints
            .onDefault( def )
            .on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    FunctionalEndpoints::Endpoint r_1 = endpoints.resolve(HttpGet, "/api/bus/something/random");
    r_1.handler(r_1);
    return (func_num == 2)
            ? OK
            : FAIL;
}
