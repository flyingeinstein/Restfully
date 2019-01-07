//
// Created by ineoquest on 3/28/17.
//

#include "tests.h"

#include <iostream>
#include <iomanip>
#include <memory>
#include <cstring>
#include <map>
#include <vector>
#include <stdlib.h>
#include <stdio.h>


#ifdef __linux
#include <malloc.h>
#endif

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#define ENABLE_WINDOWS_CRT_DEBUG
#endif

#define MAX_TESTS 100000

bool enable_header = false;
bool enable_reporting = true;
int _ok = 0, _fail = 0;


int report(const char* testname, int result)
{
	const char* status;
	switch (result) {
        case 1: status = "OK+"; break;
		case OK: status = "OK"; break;
		case FAIL: status = "FAIL"; break;
		case LEAKY: status = "LEAKY"; break;
		case CORRUPT_HEAP: status = "!HEAP"; break;
        default: status = "N/A"; break;
	}
    if (enable_reporting)
        std::cout << "  " << std::left << std::setw(8) << status << std::setw(8) << testname << std::endl;
    if(result==OK)
        _ok++;
    else
        _fail++;
    return result;
}

Test *head_test = NULL, *end_test = NULL, *tail_test = NULL;

void test_add(Test test)
{
    if(head_test ==NULL) {
        head_test = tail_test = (Test*)calloc(MAX_TESTS, sizeof(Test));
        end_test = head_test + MAX_TESTS;
    }

    // find just the file base name
    const char* bn_end = test.module + strlen(test.module);
    while(bn_end > test.module && *bn_end!='.')
        bn_end--;
    if(bn_end > test.module) {
        const char *bn_begin = bn_end;
        while (bn_begin > test.module && *bn_begin != '/')
            bn_begin--;
        if (bn_begin > test.module) {
            bn_begin++; // advance past slash
            char *b = (char*)calloc(bn_end - bn_begin + 1, 1);
            memcpy(b, bn_begin, bn_end - bn_begin);
            test.module = b;
			test.module_static = false;
        }
    }

    // convert any aliases datatype
    memcpy(tail_test++, &test, sizeof(Test));
}

void free_tests()
{
    Test* t = head_test;
    while(t < tail_test) {
        if(t->module) {
			if (!t->module_static)
				free((void *)t->module);
            t->module = NULL;
        }
        t++;
    }
    free(head_test);
    head_test = end_test = tail_test = NULL;
}

void list_all_ctest_format(const char* program_name)
{
    if(head_test==NULL)
        return;
    typedef std::map<std::string /* module */, std::vector<std::string> /* testname */> tests_map;
    tests_map tests;

    // ensure program_name has no path
    const char* p = strrchr(program_name, '/');
    if(p!=NULL) program_name = p+1;

    Test* first = head_test;
    while(first < tail_test) {
        tests[first->module].insert(tests[first->module].end(), first->name);
        first++;
    }

    if(!tests.empty()) {
        printf("### %s unit tests", program_name);
        // now output the tests by group
        for (tests_map::const_iterator m = tests.begin(), _m = tests.end(); m != _m; m++) {
            printf("\n\n#  %s module\n", m->first.c_str());
            for (std::vector<std::string>::const_iterator t = m->second.begin(), _t = m->second.end(); t != _t; t++) {
                printf("add_test(%s %s %s)\n", t->c_str(), program_name, t->c_str());
            }
        }
    }
    exit(0);
}

int test(Test* t)
{
#if defined(ENABLE_WINDOWS_CRT_DEBUG)
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif

	int result = t->test();

#if defined(ENABLE_WINDOWS_CRT_DEBUG)
	if (result == OK) {
		_CrtMemCheckpoint(&s2);
		if (_CrtMemDifference(&s3, &s1, &s2))  {
			_CrtMemDumpStatistics(&s3);
			result = LEAKY;
		}

		if (!_CrtCheckMemory())
			result = CORRUPT_HEAP;
	}
#endif
	return result;
}

void test_all()
{
    if(head_test==NULL)
        return;
    Test* first = head_test;
    while(first < tail_test) {
        int r=test(first);
        report(first->name, r);
        first++;
    }
}

int test_by_name(const char* name)
{
    int tested=0;
    if(head_test==NULL)
        return 0;
    Test* first = head_test;
    while(first < tail_test) {
        if(strncmp(name, first->name, strlen(name)) ==0 || strcmp(name, first->module) ==0) {
            int r = test(first);
            report(first->name, r);
            tested++;
        }
        first++;
    }
    return tested;
}


#if !defined(NO_STANDARD_MAIN)
int main(int argc, const char* argv[])
{
    const char* testname = NULL;    // null for all tests
    int do_all_tests = 1;

    for(int i=1; i<argc; i++) {
        const char* arg = argv[i];
        size_t arglen = strlen(arg);

        if(arglen>2 && arg[0]=='-' && arg[1]=='-')
        {
            arg+=2;
            if(strcmp(arg, "silent")==0)
                enable_reporting = false;
            else if(strcmp(arg, "no-header")==0)
                enable_header = false;
            else if(strcmp(arg, "header")==0)
                enable_header = true;
            else if(strcmp(arg, "ctest-list")==0)
                list_all_ctest_format(argv[0]);
        } else {
            if(test_by_name(arg))
                do_all_tests = 0;
        }
    }

    if(do_all_tests)
        test_all();

#if defined(WIN32)
	printf("\ndone.\n");
	getchar();
#endif

    free_tests();
    return _fail || _ok==0;
}
#endif

