//
// Created by guru on 9/9/20.
//
#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <cstring>
#include <stdio.h>
#include <iostream>
#include <map>

#include <Parser.h>


TEST_CASE("parses 3 alpha terms", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/echo");
    Rest::Parser parser(request);

    std::string hello_msg = "Greetings!";

    auto good = parser / "api" / "dev" / "echo";
    auto bad = parser / "api" / "dev" / 1;

    REQUIRE(good.status == 0);
    REQUIRE_FALSE(bad.status == 0);
}

TEST_CASE("parses a numeric path", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/1/echo");
    Rest::Parser parser(request);
    auto good = parser / "api" / "dev" / 1;
    REQUIRE(good.status == 0);
}

TEST_CASE("parses a direct numeric argument", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo");
    Rest::Parser parser(request);
    int id = 0;
    auto good = parser / "api" / "dev" / &id;
    REQUIRE(good.status == 0);
    REQUIRE(id == 6);
}

TEST_CASE("parses a direct string argument", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Parser parser(request);
    int id = 0;
    std::string name = "jane";
    auto good = parser / "api" / "dev" / &id / "echo" / &name;
    REQUIRE(good.status == 0);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
}

TEST_CASE("mismatched Uri does not fill direct arguments", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Parser parser(request);
    int id = 0;
    std::string name = "jane";
    auto bad = parser / "api" / "device" / &id / "echo" / &name;
    REQUIRE(bad.status != 0);
    REQUIRE(id != 6);
    REQUIRE(name == "jane");
}


TEST_CASE("parsing logic with two branches", "Parser") {
    int id = 0;
    std::string name;
    bool echo_greeted = false, exported_config = false;

    auto api_eval = [&id, &name, &echo_greeted, &exported_config](const char* req_uri) {
        Rest::UriRequest request(Rest::HttpGet, req_uri);
        Rest::Parser parser(request);
        echo_greeted = false;
        exported_config = false;
        if (auto device_endpoint = parser / "api" / "dev" / &id) {
            if (device_endpoint / "echo" / &name)
                echo_greeted = true;
            if (device_endpoint / "system" / "config")
                exported_config = true;
        }
    };

    api_eval("/api/dev/6/echo/the.brown.fox");
    REQUIRE(echo_greeted);
    REQUIRE_FALSE(exported_config);
    REQUIRE(id == 6);
    REQUIRE(name == "the.brown.fox");

    api_eval("/api/dev/8/system/config");
    REQUIRE_FALSE(echo_greeted);
    REQUIRE(exported_config);
    REQUIRE(id == 8);
}


TEST_CASE("matched Uri calls GET handler", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Parser parser(request);
    bool hit = false;
    int id = 0;
    std::string name = "jane";
    auto good = parser / "api" / "dev" / &id / "echo" / &name / Rest::GET([&]() { hit = true; return 200; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri calls GET handler amongst PUT handler", "Parser") {
    Rest::UriRequest request(Rest::HttpGet, "/api/dev/6/echo/john.doe");
    Rest::Parser parser(request);
    bool hit = false;
    int id = 0;
    std::string name = "jane";
    auto good = parser / "api" / "dev" / &id / "echo" / &name
            / Rest::PUT([&]() { hit = true; return 500; })
            / Rest::GET([&]() { hit = true; return 200; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri calls PUT handler before GET handler", "Parser") {
    Rest::UriRequest request(Rest::HttpPut, "/api/dev/6/echo/john.doe");
    Rest::Parser parser(request);
    bool hit = false;
    int id = 0;
    std::string name = "jane";
    auto good = parser / "api" / "dev" / &id / "echo" / &name
                / Rest::PUT([&]() { hit = true; return 200; })
                / Rest::GET([&]() { hit = true; return 500; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit);
}

TEST_CASE("matched Uri calls PUT handler with UriRequest arg", "Parser") {
    Rest::UriRequest request(Rest::HttpPut, "/api/dev/6/echo/john.doe/and/jane");
    Rest::Parser parser(request);
    int hit = 0;
    int id = 0;
    std::string name = "jane";
    auto good = parser / "api" / "dev" / &id / "echo" / &name
                / Rest::PUT([&](const Rest::UriRequest& req) {
                    hit = req.words.size();
                    return 200;
                })
                / Rest::GET([&]() { hit = 1; return 500; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit == 7);
}

TEST_CASE("matched Uri calls PUT handler with Parser arg", "Parser") {
    Rest::UriRequest request(Rest::HttpPut, "/api/dev/6/echo/john.doe/and/jane");
    Rest::Parser parser(request);
    int hit = 0;
    int id = 0;
    std::string name = "jane";
    auto good = parser / "api" / "dev" / &id / "echo" / &name
                / Rest::PUT([&](const Rest::Parser& p) {
                    hit = p.token;
                    return 200;
                })
                / Rest::GET([&]() { hit = 1; return 500; });
    REQUIRE(good.status == 200);
    REQUIRE(id == 6);
    REQUIRE(name == "john.doe");
    REQUIRE(hit == 5);
}

#if 0
    // each parser function returns an iterator into the parsing, therefor parsing can be re-continued
    // the parser itself would be an iterator then right?
    // the parsing would stop though if UriRequest status becomes non-zero
    // the iterator evaluates to false if unexpected token or if UriRequest::status becomes non-zero
    parser
        .on("api")
        .on("dev")
        .on(1)      // const number
        .on("echo")

        .GET([&hello_msg]() { std::cout << hello_msg; })        // if its a GET request, then run this function
        .PUT(hello_msg);        // if its a PUT request, then run this function

    int devid;
    std::string varname;
    std::map<std::string, std::string> config;
    if(auto config_endpoint = parser >> "api" >> "dev" >> devid >> "configure") {
        config_endpoint >> "set" >> varname
                >> Rest::Get([&config, &varname]() { std::cout << config[varname]; })
                >> Rest::Put([&config, &varname]() { config[varname] = "post data"; })

    }


#endif
