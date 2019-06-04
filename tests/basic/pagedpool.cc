//
// Created by Colin MacKenzie on 2019-04-06.
//
#define CATCH_CONFIG_FAST_COMPILE
#include <catch.hpp>

#include <Pool.h>
#include <vector>


struct Point {
    long x;
    long y;
    short z;

    inline Point() : x(0), y(1), z(2) {}
    inline Point(long _x, long _y, short _z) : x(_x), y(_y), z(_z) {}
};

TEST_CASE("PagedPool create")
{
    Rest::PagedPool pool;
    auto info = pool.info();
    REQUIRE (info.available ==0);
}

TEST_CASE("PagedPool alloc single object")
{
    Rest::PagedPool pool;
    Point* x = pool.make<Point>();
    REQUIRE (x != nullptr);
}

TEST_CASE("PagedPool alloc two objects")
{
    Rest::PagedPool pool;
    Point* i = pool.make<Point>();
    Point* j = pool.make<Point>(25600,128000,(short)27);

    REQUIRE (i != nullptr);
    REQUIRE (j != nullptr);
    REQUIRE (i->x==0);
    REQUIRE (i->y==1);
    REQUIRE (i->z==2);
    REQUIRE (j->x==25600);
    REQUIRE (j->y==128000);
    REQUIRE (j->z==27);
}

std::vector<Point*> paged_pool_add_n(Rest::PagedPool& pool, size_t n)
{
    std::vector<Point*> points;

    for(int i=0; i<n; i++) {
        Point* p = pool.make<Point>(-i * 100, i * 10, i);
        if(p== nullptr)
            goto error;
        points.insert(points.end(), p);
    }

    for(int i=0; i<n; i++) {
        Point* j = points[i];
        if (j->x != -i*100 || j->y != i*10 || j->z != i)
            goto error;
    }

    return points;
error:
    return std::vector<Point*>();
}

Point* paged_pool_add_n_array(Rest::PagedPool& pool, size_t n)
{
    Point* points = pool.makeArray<Point>(n, 100, 10, 1);
    if(points == nullptr)
        goto error;

    for(int i=0; i<n; i++) {
        Point* j = &points[i];
        if (j->x != 100 || j->y != 10 || j->z != 1)
            goto error;
    }

    return points;
error:
    return nullptr;
}

TEST_CASE("PagedPool alloc 50 single objects")
{
    Rest::PagedPool pool(64);
    auto points = paged_pool_add_n(pool, 50);
    auto info = pool.info();
    REQUIRE (info.available>0);
}

TEST_CASE("PagedPool alloc 50 as array")
{
    Rest::PagedPool pool(64);
    auto points = paged_pool_add_n_array(pool, 50);
    auto info = pool.info();
    REQUIRE (info.available ==0); // should allocate 1 single large page
}
