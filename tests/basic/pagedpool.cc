//
// Created by Colin MacKenzie on 2019-04-06.
//

#include <Pool.h>
#include <tests.h>
#include <vector>

struct Point {
    long x;
    long y;
    short z;

    inline Point() : x(0), y(1), z(2) {}
    inline Point(long _x, long _y, short _z) : x(_x), y(_y), z(_z) {}
};

TEST(paged_pool_create)
{
    Rest::PagedPool pool;
    auto info = pool.info();
    return (info.available ==0)
        ? OK
        : FAIL;
}

TEST(paged_pool_one_object)
{
    Rest::PagedPool pool;
    Point* x = pool.make<Point>();
    return (x != nullptr)
        ? OK
        : FAIL;
}

TEST(paged_pool_two_objects)
{
    Rest::PagedPool pool;
    Point* i = pool.make<Point>();
    Point* j = pool.make<Point>(25600,128000,(short)27);

    if (i == nullptr && j == nullptr)
        return FAIL;
    if(i->x!=0 || i->y!=1 || i->z!=2)
        return FAIL;
    if(j->x!=25600 || j->y!=128000 || j->z!=27)
        return FAIL;

    return OK;
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

TEST(paged_pool_fifty_objects)
{
    Rest::PagedPool pool(64);
    auto points = paged_pool_add_n(pool, 50);
    auto info = pool.info();
    return (info.available>0)
           ? OK
           : FAIL;
}

TEST(paged_pool_array_fifty_objects)
{
    Rest::PagedPool pool(64);
    auto points = paged_pool_add_n_array(pool, 50);
    auto info = pool.info();
    return (info.available ==0) // should allocate 1 single large page
           ? OK
           : FAIL;
}
