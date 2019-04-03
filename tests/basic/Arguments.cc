//
// Created by Colin MacKenzie on 2019-04-02.
//

#include <tests.h>
#include <Argument.h>

using namespace Rest;

Argument A(const char* _name, unsigned short _tm) {
    return Argument(Type(_name, _tm));
}

template<class I>
Argument A(const char* _name, unsigned short _tm, I v) {
    return Argument(Type(_name, _tm), v);
}

TEST(arguments_constructor) {
    if(literals_index == nullptr)
        literals_index = binbag_create(128, 1.2);
    Arguments x(0);
    return (x.count()==0)
        ? OK
        : FAIL;
}

TEST(arguments_reserve) {
    Arguments x;
    x.reserve(5);
    return (x.count()==0) && (x.capacity()==5)
           ? OK
           : FAIL;
}

TEST(arguments_constructor_copy_range) {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args(_args, 2);
    return (args.count()==2) && (args.capacity()==2)
            && (strcmp(args[0].name(), "x")==0) && args[0].l==2l
            && (strcmp(args[1].name(), "y")==0) && args[1].l==6l
        ? OK
        : FAIL;
}

TEST(arguments_constructor_copy_range_with_capacity) {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args(_args, 2, 4);
    return (args.count()==2) && (args.capacity()==4)
           && (strcmp(args[0].name(), "x")==0) && args[0].l==2l
           && (strcmp(args[1].name(), "y")==0) && args[1].l==6l
       ? OK
       : FAIL;
}

TEST(arguments_constructor_copy_constructor) {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args_first(_args, 2);
    Arguments args(args_first);
    return (args.count()==2) && (args.capacity()==2)
           && (strcmp(args[0].name(), "x")==0) && args[0].l==2l
           && (strcmp(args[1].name(), "y")==0) && args[1].l==6l
       ? OK
       : FAIL;
}

TEST(arguments_constructor_copy_assignment) {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args_first(_args, 2);
    Arguments args;
    args = args_first;
    return (args.count()==2) && (args.capacity()==2)
           && (strcmp(args[0].name(), "x")==0) && args[0].l==2l
           && (strcmp(args[1].name(), "y")==0) && args[1].l==6l
       ? OK
       : FAIL;
}

TEST(arguments_plus_operator) {
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
    return (args.count()==3) && (args.capacity()==3)
           && (strcmp(args[0].name(), "x")==0) && args[0].l==2l
           && (strcmp(args[1].name(), "y")==0) && args[1].l==6l
           && (strcmp(args[2].name(), "z")==0) && args[2].l==4l
       ? OK
       : FAIL;
}

TEST(arguments_add_types) {
    Arguments args;
    args.add( Type("x", ARG_MASK_INTEGER) );
    args.add( Type("y", ARG_MASK_INTEGER) );
    return (args.count()==2) && (args.capacity()==2)
           && (strcmp(args[0].name(), "x")==0)
           && (strcmp(args[1].name(), "y")==0)
       ? OK
       : FAIL;
}

TEST(arguments_add_arguments) {
    Arguments args;
    args.add( A("x", ARG_MASK_INTEGER, 4l) );
    args.add( A("y", ARG_MASK_INTEGER, 6l) );
    return (args.count()==2) && (args.capacity()==2)
           && (strcmp(args[0].name(), "x")==0) && args[0].l==4l
           && (strcmp(args[1].name(), "y")==0) && args[1].l==6l
           ? OK
           : FAIL;
}

TEST(arguments_constructor_operator_idx) {
    Argument _args[] = {
            A("x", ARG_MASK_INTEGER, 2l),
            A("y", ARG_MASK_INTEGER, 6l)
    };
    Arguments args(_args, 2);
    return (args.count()==2) && (args.capacity()==2)
           && args["y"].l==6l
           && args["x"].l==2l
           ? OK
           : FAIL;
}

