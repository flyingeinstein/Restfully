#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <Endpoints.h>
#include "requests.h"

typedef Rest::Handler< RestRequest& > RequestHandler;
typedef Rest::Endpoints<RequestHandler> Endpoints;

using Rest::uri_result_to_string;

std::function<int(RestRequest&, std::string)> _dummy = [](RestRequest &request, std::string _response) -> int {
    request.response = _response;
    return 200;
};

int echo(RestRequest &request) {
    request.response += "Hello ";
    request.response += (const char*)request["msg"];
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

TEST_CASE("Endpoints simple")
{
    Endpoints endpoints;

    // add some endpoints
    endpoints.on("/api/devices").GET(getbus);
    Endpoints::Request res = endpoints.resolve(Rest::HttpGet, "/api/devices");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (check_response(res.handler.handler, getbus));
    REQUIRE (res.status==Rest::UriMatched);
}

TEST_CASE("Endpooints echo")
{
    Endpoints endpoints;

    // add some endpoints
    endpoints.on("/api/echo/:msg(string)").GET(echo);
    Endpoints::Request res = endpoints.resolve(Rest::HttpGet, "/api/echo/Colin MacKenzie");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (res.status==Rest::UriMatched);

    RestRequest rr(res);
    REQUIRE (res.handler(rr)==200);
    REQUIRE (rr.response=="Hello Colin MacKenzie");
}

TEST_CASE("Endpoints on() starts with url argument")
{
    Endpoints endpoints;
    endpoints
            .on("/api/echo")
            .on(":msg(string|integer)")     // starts with argument, this should be ok
            .GET(getbus);

    auto req = endpoints.resolve(Rest::HttpGet, "/api/echo/johndoe");
    REQUIRE(req.status == Rest::UriMatched);
}

TEST_CASE("Endpooints including dots in path")
{
    Endpoints endpoints;

    // add some endpoints
    endpoints.on("/api/v1.0/echo/:msg(string)").GET(echo);
    Endpoints::Request res = endpoints.resolve(Rest::HttpGet, "/api/v1.0/echo/Colin MacKenzie");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (res.status==Rest::UriMatched);

    RestRequest rr(res);
    REQUIRE (res.handler(rr)==200);
    REQUIRE (rr.response=="Hello Colin MacKenzie");
}

TEST_CASE("Endpooints partial match returns no handler") {
    Endpoints endpoints;
    //Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/devices").GET(getbus);
    Endpoints::Request r = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c");
    REQUIRE (r.status==Rest::NoHandler);
}

TEST_CASE("Endpooints wildcard match returns handler") {
    Endpoints endpoints;
    //Endpoints::Handler getbus("get i2c-bus");
    endpoints.on("/api/bus/i2c/:bus(integer)/*").GET(getbus);
    Endpoints::Request r = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/5/config/display");
    REQUIRE (r.status==Rest::UriMatchedWildcard);
    REQUIRE (check_response(r.handler.handler, getbus));
}

TEST_CASE("Endpooints int argument")
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(integer)/devices").GET(getbus);
    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/3/devices");
    REQUIRE (r_1.status==Rest::UriMatched);
    REQUIRE (check_response(r_1.handler.handler, getbus));
    REQUIRE (r_1["bus"].isInteger());
    REQUIRE (((long)r_1["bus"]) == 3);
}

TEST_CASE("Endpooints real argument")
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(real)/devices").GET(getbus);
    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/3.14/devices");
    REQUIRE (r_1.status==Rest::UriMatched);
    REQUIRE (check_response(r_1.handler.handler, getbus));
    REQUIRE (r_1["bus"].isNumber());
    REQUIRE (3.14==(double)r_1["bus"]);
}

TEST_CASE("Endpooints string argument")
{
    Endpoints endpoints;
    endpoints.on("/api/bus/i2c/:bus(string)/devices").GET(getbus);
    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpGet, "/api/bus/i2c/default/devices");
    REQUIRE (r_1.status==Rest::UriMatched);
    REQUIRE (check_response(r_1.handler.handler, getbus));
    REQUIRE (r_1["bus"].isString());
    REQUIRE (strcmp("default", (const char*)r_1["bus"])==0);
}

#if 0
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
                << uri_result_to_string(ex.code);
            REQUIRE(ex.status ==0);
        });

    // resolve some endpoints
    Endpoints::Request rb1 = endpoints.resolve(Rest::HttpGet, "/api/devices/5/slots");
    REQUIRE (rb1.status==Rest::UriMatched);

    Endpoints::Request rb2 = endpoints.resolve(Rest::HttpGet, "/api/devices/i2c/slots");
    REQUIRE (rb2.status==Rest::UriMatched);

    Endpoints::Request rc1 = endpoints.resolve(Rest::HttpPut, "/api/devices/i2c/slot/96/meta");
    REQUIRE (rc1.status==Rest::UriMatched);

    const char* devid = rc1["dev"];
    REQUIRE (c1["dev"].isString());
    REQUIRE (strcmp(rc1["dev"], "i2c")==0);

    unsigned long slotid = rc1["slot"];
    REQUIRE (rc1["slot"].isInteger());
    REQUIRE (96==(long)rc1["slot"]);

    Endpoints::Request r_1 = endpoints.resolve(Rest::HttpPut, "/api/bus/i2c/3/devices");
    REQUIRE (r_1.status==Rest::UriMatched);
    REQUIRE (r_1["bus"].isInteger());
    REQUIRE (3!=(long)r_1["bus"]);
}
#endif

TEST_CASE("Endpooints subnode simple")
{
    Endpoints endpoints;

    // add some endpoints
    Endpoints::Node devs = endpoints.on("/api/devices");
    devs.on("lights").GET(getbus);

    Endpoints::Request res = endpoints.resolve(Rest::HttpGet, "/api/devices/lights");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (check_response(res.handler.handler, getbus));
    REQUIRE (res.status==Rest::UriMatched);
}

TEST_CASE("Endpooints subnode three")
{
    Endpoints endpoints;

    // starting at the path /api/devices, add 4 new lights and doors endpoints.
    // if any of the additions fail then they will return an invalid NodeRef which we then return FAIL. Note,
    // calling on(...) on an invalid NodeRef just returns the invalid NodeRef again.
    REQUIRE(0 == endpoints.on("/api/devices")
        .PUT("lights", devices)
        .GET("lights/kitchen", getbus)
        .GET("lights/bedroom", getbus)
        .GET("doors/garage", getbus)
        .error());

    Endpoints::Request res = endpoints.resolve(Rest::HttpPut, "/api/devices/lights");
    REQUIRE (res.method==Rest::HttpPut);
    REQUIRE (check_response(res.handler.handler, devices));
    REQUIRE (res.status==Rest::UriMatched);

    res = endpoints.resolve(Rest::HttpGet, "/api/devices/lights/kitchen");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (check_response(res.handler.handler, getbus));
    REQUIRE (res.status==Rest::UriMatched);

    res = endpoints.resolve(Rest::HttpGet, "/api/devices/lights/bedroom");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (check_response(res.handler.handler, getbus));
    REQUIRE (res.status==Rest::UriMatched);

    res = endpoints.resolve(Rest::HttpGet, "/api/devices/doors/garage");
    REQUIRE (res.method==Rest::HttpGet);
    REQUIRE (check_response(res.handler.handler, getbus));
    REQUIRE (res.status==Rest::UriMatched);
}

TEST_CASE("Endpooints subnode fails with bad path")
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
    REQUIRE (0 != devs.error());
}

TEST_CASE("Endpooints subnode fails with inner exception")
{
    Endpoints endpoints;

    // starting at the path /api/devices, add 4 new lights and doors endpoints.
    // if any of the additions fail then they will return an invalid NodeRef which we then return FAIL. Note,
    // calling on(...) on an invalid NodeRef just returns the invalid NodeRef again.
    REQUIRE (0 != endpoints.on("/api/devices")
            .PUT("lights", devices)
            .GET("/lights/ &kitchen", getbus)
            .GET("/lights/bedroom", getbus)
            .GET("doors/garage", getbus)
            .error() ); // expect error!
}

TEST_CASE("Endpooints curry using std::bind")
{
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
}

TEST_CASE("Endpooints add curry with same endpoint type")
{
    Endpoints endpoints1, endpoints2;
    endpoints1
            .on("/api")
            .with(endpoints2)
               .on("echo/:msg(string|integer)")
               .GET(getbus);
}

TEST_CASE("Endpooints resolve curry with same endpoint type")
{
    Endpoints endpoints1, endpoints2;

    endpoints1
            .on("/api")
            .with(endpoints2)
            .on("echo/:msg(string|integer)")
            .PUT(getbus);

    Endpoints::Request res = endpoints1.resolve(Rest::HttpPut, "/api/echo/johndoe");
    REQUIRE (res.method==Rest::HttpPut);
    REQUIRE (check_response(res.handler.handler, getbus));
    REQUIRE (res.status==Rest::UriMatched);
}

TEST_CASE("Endpoints accepts all /api requests", "[uri-accept]")
{
    Endpoints endpoints;
    endpoints
            .on("/api")
            .accept()
            .on("echo/:msg(string|integer)")
            .GET(getbus);

    SECTION("accepts /api") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/api") > 0);
    }
    SECTION("accepts /api/echo/johndoe") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/api/echo/johndoe") > 0);
    }
    SECTION("resolves /api/echo/johndoe") {
        auto req = endpoints.resolve(Rest::HttpGet, "/api/echo/johndoe");
        REQUIRE(req.status == Rest::UriMatched);
    }

    SECTION("accepts /api/ping/johndoe") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/api/ping/johndoe") > 0);
    }
    SECTION("does not resolve /api/ping/johndoe") {
        auto req = endpoints.resolve(Rest::HttpGet, "/api/ping/johndoe");
        REQUIRE(req.status == Rest::NoEndpoint);
    }
    SECTION("does not accept /ping/echo/johndoe") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/ping/echo/johndoe") < 0);
    }
}

TEST_CASE("Endpoints accepts all /api/echo requests", "[uri-accept]")
{
    Endpoints endpoints;
    endpoints
            .on("/api/echo")
//            .on("echo")
            .accept()
            .on("name/:msg(string|integer)")
            .GET(getbus);

    SECTION("accepts /api/echo") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/api/echo") > 0);
    }
    SECTION("accepts /api/echo/johndoe") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/api/echo/johndoe") > 0);
    }
    SECTION("resolves /api/echo/johndoe") {
        auto req = endpoints.resolve(Rest::HttpGet, "/api/echo/name/johndoe");
        REQUIRE(req.status == Rest::UriMatched);
    }

    SECTION("accepts /api/echo/johndoe") {
        REQUIRE(endpoints.queryAccept(Rest::HttpGet, "/api/echo/johndoe") > 0);
    }
    SECTION("does not resolve /api/ping/johndoe") {
        auto req = endpoints.resolve(Rest::HttpGet, "/api/ping/johndoe");
        REQUIRE(req.status == Rest::NoEndpoint);
    }
}

TEST_CASE("Endpoint accepts only yaml content-type", "[uri-content-type]") {
    Endpoints endpoints;
    endpoints
            .on("/api/config")
            .withContentType("application/x-yaml", false)
            .on("cloud-init")
            .GET(getbus);

    SECTION("accepts application/x-yaml") {
        Endpoints::Request req(Rest::HttpGet, "/api/config/cloud-init");
        req.contentType = "application/x-yaml";
        REQUIRE( endpoints.resolve(req) );
        REQUIRE(req.status == Rest::UriMatched);
    }
    SECTION("doesnt accept application/json") {
        Endpoints::Request req(Rest::HttpGet, "/api/config/cloud-init");
        req.contentType = "application/json";
        REQUIRE( ! endpoints.resolve(req) );
        REQUIRE( req.status != Rest::UriMatched );
    }
}

TEST_CASE("Endpoint accepts yaml or json content-type", "[uri-content-type]") {
    Endpoints endpoints;
    endpoints
            .on("/api/config")
            .withContentType("application/x-yaml")
            .on("cloud-init")
            .GET(getbus);

    SECTION("accepts application/x-yaml") {
        Endpoints::Request req(Rest::HttpGet, "/api/config/cloud-init");
        req.contentType = "application/x-yaml";
        REQUIRE( endpoints.resolve(req) );
        REQUIRE(req.status == Rest::UriMatched);
    }
    SECTION("accepts application/json") {
        Endpoints::Request req(Rest::HttpGet, "/api/config/cloud-init");
        req.contentType = "application/json";
        REQUIRE( endpoints.resolve(req) );
        REQUIRE(req.status == Rest::UriMatched);
    }
}
