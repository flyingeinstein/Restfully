
# Feature Requests
* A method that adds a URI base path, and returns a reference to that base path endpoint (no handler attached). You can then add more endpoints starting at that base path. Would save program memory where we have many strings that start with "/api/etc/etc".


# Memory Optimizations
* All instances of Endpoints share the same text binbag. I can't see a downside, for endpoints objects that get created for each instance of a user's class this means the literals are not duplicated in memory.
* Could we split the Endpoints object into the search tree and handlers as separated so each user object instance only holds something like a virtual class ptr (vcptr) instead of a whole Endpoints object that is identical to other object instances?
* Alternative to above item, an Endpoints object can be shared statically with a class but have a vector of handler pointers. This is analaguous to C++ basically.

