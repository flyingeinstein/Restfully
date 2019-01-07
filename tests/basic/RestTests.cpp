
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <utility>

#include <tests.h>
#include <Rest.h>


class FakeRequest
{
public:
    int value;

    inline FakeRequest(int x=0) : value(x) {}
};
DEFINE_HTTP_METHOD_HANDLERS(FakeRequest)
typedef Rest::Handler<FakeRequest&> FakeHandler;


TEST(handler_std_function)
{
    FakeRequest r(1);
    std::function<int(FakeRequest&)> f = [](FakeRequest& r) { r.value = 2; return r.value; };
    FakeHandler h(f);
    return (h(r)==2 && r.value==2)
        ? OK
        : FAIL;
}

TEST(handler_lambda)
{
    FakeRequest r(1);
    FakeHandler h([](FakeRequest& r) { r.value = 2; return r.value; });
    return (h(r)==2 && r.value==2)
           ? OK
           : FAIL;
}

TEST(handler_lambda_wcapture)
{
    int rr = 4;
    FakeRequest r(1);
    FakeHandler h([&rr](FakeRequest& r) { r.value = 2; rr=2; return r.value;});
    return (h(r)==rr && r.value==rr)
           ? OK
           : FAIL;
}

int handler_func(FakeRequest& r) { return r.value = 2; }

TEST(handler_function)
{
    FakeRequest r(1);
    FakeHandler h(handler_func);
    return (h(r)==2 && r.value==2)
           ? OK
           : FAIL;
}

class handler_class
{
public:
    int rr;
    handler_class(): rr(1) {}
    int m(FakeRequest& r) { rr = 2; return r.value = 2; }
};

TEST(handler_instance_function)
{
    handler_class c;
    FakeRequest r(1);
    FakeHandler h(std::bind(&handler_class::m, &c, std::placeholders::_1));
    return (h(r)==2 && r.value==2)
           ? OK
           : FAIL;
}

TEST(handler_get_std_function)
{
    FakeRequest r(1);
    std::function<int(FakeRequest&)> f = [](FakeRequest& r) { r.value = 2; return r.value; };
    FakeHandler h = GET(f);
    return (h(r)==2 && r.value==2)
           ? OK
           : FAIL;
}

TEST(handler_get_instance_function)
{
    handler_class c;
    FakeRequest r(1);
    FakeHandler h = GET(std::bind(&handler_class::m, &c, std::placeholders::_1));
    return (h(r)==2 && r.value==2)
           ? OK
           : FAIL;
}


