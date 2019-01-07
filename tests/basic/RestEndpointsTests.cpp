
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

bool check_response(std::function<int(RestRequest&)> x, std::function<int(RestRequest&)> y)
{
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
    endpoints.on("/api/devices", GET(getbus));
    Endpoints::Endpoint res = endpoints.resolve(Rest::HttpGet, "/api/devices");
    return res.method==Rest::HttpGet && check_response(res.handler.handler, getbus) && res.status==URL_MATCHED;
}

TEST(endpoints_partial_match_returns_no_handler) {
    Endpoints endpoints;
    //Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    Endpoints::Endpoint r = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c");
    return (r.status==URL_FAIL_NO_HANDLER)
        ? OK
        : FAIL;
}

TEST(endpoints_wildcard_match_returns_handler) {        // todo: implement URL wildcards!!!
    Endpoints endpoints;
    //Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/*", GET(getbus));
    Endpoints::Endpoint r = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/5/config/display");
    return (r.status==URL_MATCHED && check_response(r.handler.handler, getbus))
           ? OK
           : FAIL;
}

TEST(endpoints_int_argument)
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(integer)/devices", GET(getbus));
    Endpoints::Endpoint r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/3/devices");
    return (r_1.status==URL_MATCHED && check_response(r_1.handler.handler, getbus) && r_1["bus"].isInteger() && 3==(long)r_1["bus"])
       ? OK
       : FAIL;
}

TEST(endpoints_real_argument)
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(real)/devices", GET(getbus));
    Endpoints::Endpoint r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/3.14/devices");
    return (r_1.status==URL_MATCHED && check_response(r_1.handler.handler, getbus) && r_1["bus"].isNumber() && 3.14==(double)r_1["bus"])
        ? OK
        : FAIL;
}

TEST(endpoints_string_argument)
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(string)/devices", GET(getbus));
    Endpoints::Endpoint r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/default/devices");
    return (r_1.status==URL_MATCHED && check_response(r_1.handler.handler, getbus) && r_1["bus"].isString() && strcmp("default", (const char*)r_1["bus"])==0)
        ? OK
        : FAIL;
}

TEST(endpoints_many)
{
	Endpoints endpoints;
	//Endpoints::Handler devices("devices"), slots("dev:slots"), slot("dev:slot"), getbus("get i2c-bus"), putbus("put i2c-bus");

    // add some endpoints
	endpoints
	    .on("/api/devices", GET(devices))
	    .on("/api/devices/:dev(integer|string)/slots", GET(slots))
        .on("/api/devices/:dev(integer|string)/slot/:slot(integer)/meta", PUT(slot));
	endpoints
        .on("/api/bus/i2c/:bus(integer)/devices",
                  GET(getbus),
                  PUT(getbus),
                  PATCH(getbus),
                  POST(getbus),
                  DELETE(getbus),
                  //GET(putbus),         // will cause a duplicate endpoint error
                  OPTIONS(getbus)
                  )
        // any errors produced in the above sentences will get caught here
        .katch([](Endpoints::Endpoint p) {
            std::cout << "exception occured adding endpoints: "
                << uri_result_to_string(p.status) << ": "
                << p.name;
            return FAIL;
        });

    // resolve some endpoints
    Endpoints::Endpoint rb1 = endpoints.resolve(Rest::HttpGet, "/api/devices/5/slots");
    if(rb1.status!=URL_MATCHED)
        return FAIL;

    Endpoints::Endpoint rb2 = endpoints.resolve(Rest::HttpGet, "/api/devices/i2c/slots");
    if(rb2.status!=URL_MATCHED)
        return FAIL;

    Endpoints::Endpoint rc1 = endpoints.resolve(Rest::HttpPut, "/api/devices/i2c/slot/96/meta");
    if(rc1.status!=URL_MATCHED)
        return FAIL;
    const char* devid = rc1["dev"];
    if(!rc1["dev"].isString() || strcmp(rc1["dev"], "i2c")!=0)
        return FAIL;
    unsigned long slotid = rc1["slot"];
    if(!rc1["slot"].isInteger() || 96!=(long)rc1["slot"])
        return FAIL;

    Endpoints::Endpoint r_1 = endpoints.resolve(Rest::HttpPut, "/api/bus/i2c/3/devices");
    if(r_1.status!=URL_MATCHED || !r_1["bus"].isInteger() || 3!=(long)r_1["bus"])
        return FAIL;

    return OK;
}