
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include <Rest.h>
#include "requests.h"

typedef Rest::Handler< RestRequest& > RequestHandler;
typedef Rest::Endpoints<RequestHandler> Endpoints;

using Rest::uri_result_to_string;

std::function<int(RestRequest&, std::string)> _dummy = [](RestRequest &request, std::string _response) -> int {
    request.response = _response;
    return 200;
};

int getbus(RestRequest &request) {
    request.response += uri_method_to_string(request.method);
    request.response += " i2c-bus";
    return 200;
};

int devices(RestRequest &request) {
    request.response += uri_method_to_string(request.method);
    request.response += " devices";
    return 200;
};

int slots(RestRequest &request) {
    request.response += uri_method_to_string(request.method);
    request.response += " slots";
    return 200;
};

int slot(RestRequest &request) {
    request.response += uri_method_to_string(request.method);
    request.response += " slot";
    return 200;
};

std::function<int(RestRequest&)> dummy(std::string _response) {
    return [_response](RestRequest &request) -> int {
        request.response = _response;
        return 200;
    };;
}

bool check_response(const std::function<int(RestRequest&)>& x, const std::function<int(RestRequest&)>& y)
{
    if(x==nullptr || y==nullptr)
        return false;
    Rest::Arguments args(1);
    RestRequest rx(args), ry(args);
    x(rx);
    y(ry);
    //printf("   response was %s\n", rx.response.c_str());
    return rx.response == ry.response;
}

TEST(endpoints_simple)
{
    Endpoints endpoints;

    // add some endpoints
    endpoints.on("/api/devices").GET(getbus);
    Endpoints::Request res = endpoints.resolve(Rest::HttpGet, "/api/devices");
    return (res.method==Rest::HttpGet && check_response(res.handler.handler, getbus) && res.status==URL_MATCHED)
       ? OK
       : FAIL;
}

TEST(endpoints_partial_match_returns_no_handler) {
    Endpoints endpoints;
    //Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices").GET(getbus);
    Endpoints::Request r = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c");
    return (r.status==URL_FAIL_NO_HANDLER)
        ? OK
        : FAIL;
}

TEST(endpoints_wildcard_match_returns_handler) {
    Endpoints endpoints;
    //Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/*").GET(getbus);
    Endpoints::Request r = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/5/config/display");
    return (r.status==URL_MATCHED_WILDCARD && check_response(r.handler.handler, getbus))
           ? OK
           : FAIL;
}

TEST(endpoints_int_argument)
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(integer)/devices").GET(getbus);
    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/3/devices");
    return (r_1.status==URL_MATCHED && check_response(r_1.handler.handler, getbus) && r_1["bus"].isInteger() && 3==(long)r_1["bus"])
       ? OK
       : FAIL;
}

TEST(endpoints_real_argument)
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(real)/devices").GET(getbus);
    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/3.14/devices");
    return (r_1.status==URL_MATCHED && check_response(r_1.handler.handler, getbus) && r_1["bus"].isNumber() && 3.14==(double)r_1["bus"])
        ? OK
        : FAIL;
}

TEST(endpoints_string_argument)
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(string)/devices").GET(getbus);
    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/default/devices");
    return (r_1.status==URL_MATCHED && check_response(r_1.handler.handler, getbus) && r_1["bus"].isString() && strcmp("default", (const char*)r_1["bus"])==0)
        ? OK
        : FAIL;
}

TEST_DISABLED(endpoints_many)
{
	Endpoints endpoints;
	//Endpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");

    // add some endpoints
	auto dev = endpoints.on("/api/devices");
    dev
        .GET(devices)
	    .GET(":dev(integer|string)/slots", slots)
        .PUT(":dev(integer|string)/slot/:slot(integer)/meta", slot);
	endpoints
        .on("/api/bus/i2c/:bus(integer)/devices")
                  .GET(getbus)
                  .PUT(getbus)
                  .PATCH(getbus)
                  .POST(getbus)
                  .DELETE(getbus)
                  //.GET(putbus)         // will cause a duplicate endpoint error
                  .OPTIONS(getbus)

        // any errors produced in the above sentences will get caught here
        .katch([](Endpoints::Exception ex) {
            std::cout << "exception occured adding endpoints: "
                << uri_result_to_string(ex.code) << ": "
                << ex.node.name();
            return FAIL;
        });

    // resolve some endpoints
    Endpoints::Request rb1 = endpoints.resolve(Rest::HttpGet, "/api/devices/5/slots");
    if(rb1.status!=URL_MATCHED)
        return FAIL;

    Endpoints::Request rb2 = endpoints.resolve(Rest::HttpGet, "/api/devices/i2c/slots");
    if(rb2.status!=URL_MATCHED)
        return FAIL;

    Endpoints::Request rc1 = endpoints.resolve(Rest::HttpPut, "/api/devices/i2c/slot/96/meta");
    if(rc1.status!=URL_MATCHED)
        return FAIL;
    const char* devid = rc1["dev"];
    if(!rc1["dev"].isString() || strcmp(rc1["dev"], "i2c")!=0)
        return FAIL;
    unsigned long slotid = rc1["slot"];
    if(!rc1["slot"].isInteger() || 96!=(long)rc1["slot"])
        return FAIL;

    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpPut, "/api/bus/i2c/3/devices");
    if(r_1.status!=URL_MATCHED || !r_1["bus"].isInteger() || 3!=(long)r_1["bus"])
        return FAIL;

    return OK;
}

TEST(endpoints_subnode_simple)
{
    Endpoints endpoints;

    // add some endpoints
    Endpoints::Node devs = endpoints.on("/api/devices");
    devs.on("lights").GET(getbus);

    Endpoints::Request res = endpoints.resolve(Rest::HttpGet, "/api/devices/lights");
    if(res.method!=Rest::HttpGet || !check_response(res.handler.handler, getbus) || res.status!=URL_MATCHED)
        return FAIL;

    return OK;
}

TEST(endpoints_subnode_three)
{
    Endpoints endpoints;

    // starting at the path /api/devices, add 4 new lights and doors endpoints.
    // if any of the additions fail then they will return an invalid NodeRef which we then return FAIL. Note,
    // calling on(...) on an invalid NodeRef just returns the invalid NodeRef again.
    if(endpoints.on("/api/devices")
        .PUT("lights", devices)
        .GET("lights/kitchen", getbus)
        .GET("lights/bedroom", getbus)
        .GET("doors/garage", getbus)
        .error() !=0)
            return FAIL;

    Endpoints::Request res = endpoints.resolve(Rest::HttpPut, "/api/devices/lights");
    if(res.method!=Rest::HttpPut || !check_response(res.handler.handler, devices) || res.status!=URL_MATCHED)
        return FAIL;

    res = endpoints.resolve(Rest::HttpGet, "/api/devices/lights/kitchen");
    if(res.method!=Rest::HttpGet || !check_response(res.handler.handler, getbus) || res.status!=URL_MATCHED)
        return FAIL;

    res = endpoints.resolve(Rest::HttpGet, "/api/devices/lights/bedroom");
    if(res.method!=Rest::HttpGet || !check_response(res.handler.handler, getbus) || res.status!=URL_MATCHED)
        return FAIL;

    res = endpoints.resolve(Rest::HttpGet, "/api/devices/doors/garage");
    if(res.method!=Rest::HttpGet || !check_response(res.handler.handler, getbus) || res.status!=URL_MATCHED)
        return FAIL;

    return OK;
}

TEST(endpoints_subnode_bad_path_fails)
{
    Endpoints endpoints;

    // starting at the path /api/devices, add 4 new lights and doors endpoints.
    // if any of the additions fail then they will return an invalid NodeRef which we then return FAIL. Note,
    // calling on(...) on an invalid NodeRef just returns the invalid NodeRef again.
    auto devs = endpoints.on("/api/devices");
    devs
        .PUT("lights", devices)
        .GET("kitchen", getbus)
        .GET("bedroom", getbus)
        .GET("doors/ &garage", getbus);
    if(devs.error())
        return OK;

    return FAIL; // failed FAIL test
}

TEST(endpoints_subnode_inner_exception_fails)
{
    Endpoints endpoints;

    // starting at the path /api/devices, add 4 new lights and doors endpoints.
    // if any of the additions fail then they will return an invalid NodeRef which we then return FAIL. Note,
    // calling on(...) on an invalid NodeRef just returns the invalid NodeRef again.
    if(endpoints.on("/api/devices")
            .PUT("lights", devices)
            .GET("/lights/ &kitchen", getbus)
            .GET("/lights/bedroom", getbus)
            .GET("doors/garage", getbus)
            .error()!=0 )
        return OK;  // correctly caused error
    else
        return FAIL; // failed FAIL test
}

TEST(endpoints_curry_using_bind) {
    Endpoints endpoints;
    bool ledState = false;

    auto SetLed = [&ledState](RestRequest &request, bool value) {
        ledState = value;
        return 200;
    };
    auto x = std::bind(SetLed,
                       std::placeholders::_1,     // RestRequest placeholder,
                       true                       // specified and bound as constant
    );

    endpoints.on("/api/led")
        .PUT("off", std::bind(SetLed,
                       std::placeholders::_1,     // RestRequest placeholder,
                       true                       // specified and bound as constant
    ))
        .PUT("on", std::bind(SetLed,
                      std::placeholders::_1,     // RestRequest placeholder,
                      false                      // specified and bound as constant
    ));
    return OK;
}

TEST(endpoints_curry_with_same)
{
    Endpoints endpoints1, endpoints2;
    endpoints1
            .on("/api")
            .with(endpoints2)
               .on("echo/:msg(string|integer)")
               .GET(getbus);
    return OK;
}

TEST(endpoints_with_same_resolve)
{
    Endpoints endpoints1, endpoints2;

    endpoints1
            .on("/api")
            .with(endpoints2)
            .on("echo/:msg(string|integer)")
            .PUT(getbus);

    Endpoints::Request res = endpoints1.resolve(Rest::HttpPut, "/api/echo/johndoe");
    if(res.method!=Rest::HttpPut || !check_response(res.handler.handler, getbus) || res.status!=URL_MATCHED)
        return FAIL;

    return OK;
}
