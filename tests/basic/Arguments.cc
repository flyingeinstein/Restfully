//
// Created by Colin MacKenzie on 2019-04-02.
//

#define CATCH_CONFIG_FAST_COMPILE

#include <Argument.h>
#include <catch.hpp>


using namespace Rest;

Argument A(const char* _name, unsigned short _tm) {
    return Argument(Type(_name, _tm));
}

template<class I>
Argument A(const char* _name, unsigned short _tm, I v) {
    return Argument(Type(_name, _tm), v);
}

TEST_CASE("Arguments constructor", "[arguments]") {
    Arguments x(0);
    REQUIRE ( x.count()==0 );
}

TEST_CASE("Arguments reserve", "[arguments]") {
    Arguments x;
    x.reserve(5);
    REQUIRE (x.count()==0);
    REQUIRE (x.capacity()==5);
}

TEST_CASE("Arguments copy constructor ange", "[arguments]") {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args(_args, 2);
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==2);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (args[0].l==2l);
    REQUIRE (strcmp(args[1].name(), "y")==0);
    REQUIRE (args[1].l==6l);
}

TEST_CASE("Arguments copy constructor range with capacity", "[arguments]") {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args(_args, 2, 4);
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==4);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (args[0].l==2l);
    REQUIRE (strcmp(args[1].name(), "y")==0);
    REQUIRE (args[1].l==6l);
}

TEST_CASE("Arguments copy constructor with initializer array", "[arguments]") {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args_first(_args, 2);
    Arguments args(args_first);
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==2);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (args[0].l==2l);
    REQUIRE (strcmp(args[1].name(), "y")==0);
    REQUIRE (args[1].l==6l);
}

TEST_CASE("Arguments copy assignment", "[arguments]") {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args_first(_args, 2);
    Arguments args;
    args = args_first;
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==2);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (args[0].l==2l);
    REQUIRE (strcmp(args[1].name(), "y")==0);
    REQUIRE (args[1].l==6l);
}

TEST_CASE("Arguments plus operator", "[arguments]") {
    Argument _args1[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Argument _args2[] = {
            A("z", ARG_MASK_INTEGER, 4l)
    };
    Arguments args1(_args1, 2);
    Arguments args2(_args2, 1);
    Arguments args = args1 + args2;
    REQUIRE (args.count()==3);
    REQUIRE (args.capacity()==3);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (args[0].l==2l);
    REQUIRE (strcmp(args[1].name(), "y")==0);
    REQUIRE (args[1].l==6l);
    REQUIRE (strcmp(args[2].name(), "z")==0);
    REQUIRE (args[2].l==4l);
}

TEST_CASE("Arguments add types", "[arguments]") {
    Arguments args;
    args.add( Type("x", ARG_MASK_INTEGER) );
    args.add( Type("y", ARG_MASK_INTEGER) );
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==2);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (strcmp(args[1].name(), "y")==0);
}

TEST_CASE("Arguments add arguments", "[arguments]") {
    Arguments args;
    args.add( A("x", ARG_MASK_INTEGER, 4l) );
    args.add( A("y", ARG_MASK_INTEGER, 6l) );
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==2);
    REQUIRE (strcmp(args[0].name(), "x")==0);
    REQUIRE (args[0].l==4l);
    REQUIRE (strcmp(args[1].name(), "y")==0);
    REQUIRE (args[1].l==6l);
}

TEST_CASE("Arguments index operator", "[arguments]") {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args(_args, 2);
    REQUIRE (args.count()==2);
    REQUIRE (args.capacity()==2);
    REQUIRE (args["y"].l==6l);
    REQUIRE (args["x"].l==2l);
}

