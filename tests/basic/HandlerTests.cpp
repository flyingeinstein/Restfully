#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <Endpoints.h>


class FakeRequest
{
public:
    int value;

    inline FakeRequest(int x=0) : value(x) {}
};

typedef Rest::Handler<FakeRequest&> FakeHandler;


TEST_CASE("FakeRequest with std::function")
{
    FakeRequest r(1);
    std::function<int(FakeRequest&)> f = [](FakeRequest& r) { r.value = 2; return r.value; };
    FakeHandler h(f);
    REQUIRE ( h(r)==2 );
    REQUIRE ( r.value==2 );
}

TEST_CASE("FakeRequest with lambda")
{
    FakeRequest r(1);
    FakeHandler h([](FakeRequest& r) { r.value = 2; return r.value; });
    REQUIRE ( h(r)==2 );
    REQUIRE ( r.value==2 );
}

TEST_CASE("FakeRequest with lambda and capture")
{
    int rr = 4;
    FakeRequest r(1);
    FakeHandler h([&rr](FakeRequest& r) { r.value = 2; rr=2; return r.value;});
    REQUIRE ( h(r)==rr );
    REQUIRE ( r.value==rr );
}

int handler_func(FakeRequest& r) { return r.value = 2; }

TEST_CASE("FakeRequest with function ptr")
{
    FakeRequest r(1);
    FakeHandler h(handler_func);
    REQUIRE ( h(r)==2 );
    REQUIRE ( r.value==2 );
}

class handler_class
{
public:
    int rr;
    handler_class(): rr(1) {}
    int m(FakeRequest& r) { rr = 2; return r.value = 2; }
};

TEST_CASE("FakeRequest with instance handler")
{
    handler_class c;
    FakeRequest r(1);
    FakeHandler h(std::bind(&handler_class::m, &c, std::placeholders::_1));
    REQUIRE ( h(r)==2 );
    REQUIRE ( r.value==2 );
}

TEST_CASE("FakeRequest using std::bind on function with extra arguments")
{
    handler_class c;
    FakeRequest r(1);
    FakeHandler h(std::bind(&handler_class::m, &c, std::placeholders::_1));
    REQUIRE ( h(r)==2 );
    REQUIRE ( r.value==2 );
}


