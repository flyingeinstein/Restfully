/// \file


#include "pool.h"

#include <cstring>
#include <stdio.h>
#include <tests.h>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64) 
#include <windows.h>
#define SRAND  srand(GetTickCount());
#else
#define SRAND  srand(time(NULL));
#endif

typedef struct _point {
    int x;
    int y;
    int z;
} point;

#define POINT_POOL  (pool_element_ptr*)&head, (pool_element_ptr*)&tail, (pool_element_ptr*)&end, sizeof(point)

TEST(pool_allocate) {
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;
    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_get_an_object)
{
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
	if (pool_new_element(POINT_POOL, (pool_element_ptr*)&element) <= 0)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_get_an_object_three_times)
{
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
    for(int i=0; i<3; i++) {
		if (pool_new_element(POINT_POOL, (pool_element_ptr*)&element) <= 0)
            return FAIL;
        element->x = 54;
        element->y = i;
        element->z = i*i;

        if (element < head || element > tail || element > end)
            return FAIL;
    }

    int i=0;
    for(point *e=head, *_e=tail; e<_e; e++,i++) {
        if(e->x != 54 && e->y!=i && e->z!=i*i)
            return FAIL;
    }

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_free_causes_null)
{
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
	if (pool_new_element(POINT_POOL, (pool_element_ptr*)&element) <= 0)
        return FAIL;

    pool_free(POINT_POOL);
    return (head==NULL && tail==NULL && end==NULL)
        ? OK
        : FAIL;
}

TEST(pool_get_three_objects)
{
    size_t length=0;
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
	if (pool_new_elements(POINT_POOL, 3, (pool_element_ptr*)&element) <= 0)
        return FAIL;

    if((length=pool_length(POINT_POOL)) !=3)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_get_eighteen_objects)
{
	size_t length = 0, capacity = 0;
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
	if (pool_new_elements(POINT_POOL, 18, (pool_element_ptr*)&element) <= 0)
        return FAIL;

    if((length=pool_length(POINT_POOL)) !=18)
        return FAIL;

    if((capacity=pool_capacity(POINT_POOL)) <18 || capacity > 30) // 30 is at least 1.5*18
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_length_is_one)
{
    size_t length=0;
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
	if (pool_new_element(POINT_POOL, (pool_element_ptr*)&element) <= 0)
        return FAIL;

    if((length=pool_length(POINT_POOL)) !=1)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_capacity_is_five)
{
	size_t capacity = 0;
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* element = NULL;
	if (pool_new_element(POINT_POOL, (pool_element_ptr*)&element) <= 0)
        return FAIL;

    if((capacity=pool_capacity(POINT_POOL)) !=5)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_sanly_grows)
{
    size_t N=100, length, capacity;
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point *element = NULL;
    for(size_t i=0; i<N; i++) {
		if (pool_new_element(POINT_POOL, (pool_element_ptr*)&element) <= 0)
            return FAIL;
    }

    if((length=pool_length(POINT_POOL)) !=N)
        return FAIL;
    if((capacity=pool_capacity(POINT_POOL)) <N || capacity > N*2)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_push_an_object)
{
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point p = { 10, 20, 30 };

	if (pool_push(POINT_POOL, (pool_element_ptr)&p) <= 0)
        return FAIL;

    // check the top of the stack that we have our object
    point* element = NULL;
	if (!pool_top(POINT_POOL, (pool_element_ptr*)&element))
        return FAIL;

    if(memcmp(&p, element, sizeof(point))!=0)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(pool_push_pop_an_object)
{
	size_t length;
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point p = { 10, 20, 30 };

	if (pool_push(POINT_POOL, (pool_element_ptr)&p) <= 0)
        return FAIL;

    // check the top of the stack that we have our object
    point* element = NULL;
	if (!pool_top(POINT_POOL, (pool_element_ptr*)&element))
        return FAIL;

    if(memcmp(&p, element, sizeof(point))!=0)
        return FAIL;

    if(element<head || element > tail || element > end)
        return FAIL;

    if(!pool_pop(POINT_POOL))
        return FAIL;

    if((length=pool_length(POINT_POOL)) !=0)
        return FAIL;

    pool_free(POINT_POOL);
    return OK;
}

TEST(top_on_empty_pool_returns_null)
{
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* p=(point*)1;
	return (pool_top(POINT_POOL, (pool_element_ptr*)&p) == 0 && p == NULL) ? OK : FAIL;
}

TEST(pop_on_empty_pool_returns_zero)
{
    point *head = NULL, *tail = NULL, *end = NULL;
    if(pool_reserve(POINT_POOL, 5) <=0)
        return FAIL;

    point* p=(point*)1;
    return (pool_pop(POINT_POOL)==0) ? OK : FAIL;
}

int pool_insert_test(size_t N, size_t initial_capacity)
{
    int rv = OK;
    size_t i=0, length = 0, pagecount = 0;
    size_t capacity;
    point *head = NULL, *tail = NULL, *end = NULL;

    if(pool_reserve(POINT_POOL, initial_capacity) <=0)
        return FAIL;

    // create our C arrays to store check values
    point* check_elements = (point*)calloc(N, sizeof(point));

    // get a 1000 objects
    for(int i=0; i<N; i++) {
        point *pool_element, *check_element;
        if (pool_new_element(POINT_POOL, (pool_element_ptr *) &pool_element) <= 0)
            goto fail;
        check_element = &check_elements[i];

        // fill element with random numbers
        pool_element->x = check_element->x = 1000 + rand()%9000;
        pool_element->y = check_element->y = -1000 - rand()%9000;
        pool_element->z = check_element->z = -1000 + rand()%2000;

    }

    if((length=pool_length(POINT_POOL)) !=N)
        goto fail;
    if((capacity = pool_capacity(POINT_POOL)) < N)
        goto fail;

    // now check that no pointers have changed
    for(; i<N; i++) {
        point *check_element;
        point* expected = (point*)pool_get_element(POINT_POOL, i);
        point* expected2 = &head[i];

        check_element = &check_elements[i];

        // now check the pool_get_element accessor is working as expected
        if(expected->x != expected2->x || expected->y != expected2->y || expected->z != expected2->z)
            goto content_fail;

        // now check the values match
        if(expected->x != check_element->x || expected->y != check_element->y || expected->z != check_element->z)
            goto content_fail;

        // we can check the range of the values too
        if(expected->x < 1000 || expected->x > 10000)
            goto content_fail;
        if(expected->y < -10000 || expected->y > -1000)
            goto content_fail;
        if(expected->z < -1000 || expected->z > 1000)
            goto content_fail;
    }
ok:
    free(check_elements);
    pool_free(POINT_POOL);
    return rv;

content_fail:
    rv = FAIL;
    printf("    content verification failed at element %lu for pool of %lu items with initial capacity of %lu\n",
           i, length, initial_capacity);
    goto ok;    // not ok, but we continue to there

fail:
    rv = FAIL;
    printf("    test failed for pool of %lu items (with N=%lu, capacity=%lu)\n",
           length, N, initial_capacity);
    goto ok;    // not ok, but we continue to there
}


TEST(pool_N7_C64)
{
    return pool_insert_test(
            7,
            64
    );
}

TEST(pool_N64_C7)
{
    return pool_insert_test(
            64,
            7
    );
}

TEST(pool_equal_N7_C64)
{
    return pool_insert_test(
            7,
            64
    );
}

TEST(pool_equal_N64_C64)
{
    return pool_insert_test(
            64,
            64
    );
}

TEST(pool_near_N63_C64)
{
    return pool_insert_test(
            63,
            64
    );
}
TEST(pool_near_N65_C64)
{
    return pool_insert_test(
            7,
            7
    );
}

TEST(pool_equal_N7_C7)
{
    return pool_insert_test(
            7,
            7
    );
}

TEST(pool_near_N6_C7)
{
    return pool_insert_test(
            6,
            7
    );
}

TEST(pool_near_N8_C7)
{
    return pool_insert_test(
            8,
            7
    );
}

TEST(pool_zero_N0_C13)
{
    return pool_insert_test(
            0,
            13
    );
}

TEST(pool_one_N1_C13)
{
    return pool_insert_test(
            1,
            13
    );
}

TEST(pool_zero_N13_C0)
{
    return pool_insert_test(
            13,
            0
    );
}

TEST(pool_zero_N1_C0)
{
    return pool_insert_test(
            1,
            0
    );
}

TEST(pool_zero_N0_C1)
{
    return pool_insert_test(
            0,
            1
    );
}

TEST(pool_large_N1000_C112)
{
    return pool_insert_test(
            1000,
            112
    );
}

TEST(pool_massive_N1250000_C4)
{
    return pool_insert_test(
            1250000,
            4
    );
}

TEST(pool_massive_N8250000_C4096)
{
    return pool_insert_test(
            8250000,
            4096
    );
}

TEST(pool_many_randomly_sized_tests)
{
	SRAND;
    for(int n=0; n<100; n++) {
        size_t N = rand()%10000000;
        size_t capacity = rand()%4096; // should be 60
        //printf("   initiating random pool test with N=%lu, capacity=%lu\n", N, capacity);
        if(pool_insert_test(N, capacity) !=OK) {
            return FAIL;
        }
    }
    return OK;
}