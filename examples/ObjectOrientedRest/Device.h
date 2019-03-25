
#pragma once

/* This is the classic Object Oriented "Animal" example extended to a Rest API. Each animal
 * has some common methods and some methods of their own. 
 */
#include <Restfully.h>

class Animal
{
  public:
    String name;
    bool hasFur;
    bool hasWings;
    bool hasTail;
    bool friendly;
    int legs;
    
  public:
    Endpoints endpoints;

    Animal(const char* _name);

    int getName(RestRequest& request);
};

class Cat : public Animal
{
  public:
    Cat() : Animal("Cat") {}
};

class Dog : public Animal
{
  public:
    Dog();
};
