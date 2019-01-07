//
// Created by ineoquest on 3/28/17.
//

#ifndef ANALYTICSAPI_TESTS_H
#define ANALYTICSAPI_TESTS_H

#define OK      (short)0
#define FAIL    (short)-1
#define LEAKY	(short)-2
#define CORRUPT_HEAP		(short)-3

extern bool enable_header;
extern bool enable_reporting;
extern int _ok, _fail;

typedef int (*test_func)();

typedef struct _Test {
    const char* name;
    const char* module;
	bool module_static;
    test_func test;
} Test;


#define CHECK(test) { int r=test_ ## test(); report(#test, r); }
//#define TEST(test) if(!testname || strcmp(testname, #test)==0) CHECK( test );

typedef struct json_object json_object;
json_object* test_json_load_asset(const char* filename);

void test_add(Test test);

void test_all();

int report(const char* testname, int result);

#ifdef __cplusplus
#define constructor(f) \
        static void f(void); \
        struct f##_t_ { f##_t_(void) { f(); } }; static f##_t_ f##_; \
        static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU",read)
#define constructor2_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)
#ifdef _WIN64
#define constructor(f) constructor2_(f,"")
#else
#define constructor(f) constructor2_(f,"_")
#endif
#else
#define INITIALIZER(f) \
        static void f(void) __attribute__((constructor)); \
        static void f(void)
#endif

#define TEST_PROTOTYPE(test_name) static int test_ ## test_name()

#define TEST_INITIALIZER(test_name) constructor(_init_test_ ## test_name) { Test t = { \
		/* name: */ #test_name, \
		/* module: */ __FILE__, \
		/* module_static: */ true, \
		/* test: */ test_ ## test_name \
	}; test_add(t); }

#define TEST(test_name)   \
            TEST_PROTOTYPE(test_name);  \
            TEST_INITIALIZER(test_name) \
            TEST_PROTOTYPE(test_name)

#define TEST_DISABLED(test_name)   \
            TEST_PROTOTYPE(test_name)



#endif //ANALYTICSAPI_TESTS_H
