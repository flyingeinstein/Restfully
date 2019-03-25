#include "Device.h"

Animal::Animal(const char* _name)
  : name(_name)
{
  endpoints.on("name", GET(std::bind(&Animal::getName, this, std::placeholders::_1)) );
}

int Animal::getName(RestRequest& request)
{
  request.response["name"] = name;
  return 200;
}


Dog::Dog() 
  : Animal("Dog") 
{
  endpoints.on("scratch", GET([](RestRequest& request) { 
    request.response["itchy"] = true;
    return 200;
  }));
}
