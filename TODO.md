
# Feature Roadmap

#### on(...) returns relative nodes &#9745;
on(...) method that given a URI base path returns a reference to that base path. You
can then add more endpoints starting at this base path. Would save program memory 
where we have many strings that start with "/api/etc/etc".
- [x] Node class now supports on(...)
- [x] Endpoints.on(...) now just delegates to on() from the root node

#### Member Function Handlers &#9745;
Goal: Endpoints template supports handlers that are class member functions. These 
handlers have the prototype `int(Klass::*)(RestRequest&)` and require an object 
instance to invoke.
- [x] Endpoints template supports member functions
- [x] User invokes the resolved handler using the `obj.*handler(args...)` syntax

#### Split Endpoint collections
One instance of an Endpoints class can delegate into another instance. So your
application's endpoints can be broken out into seperate groups by function, etc.

For Endpoints<> that have the same handler type, this is trivial since the handler 
type is the same.
- [x] Node now contains `bindings` collection that will be searched if no local 
handler found.
- [x] The Node will store a lambda which serves as a resolve() delegate (to external 
endpoint collections). The lambda stores the reference to object instance and sub-endpoints.

#### Static Endpoints calling member function endpoints
Goal: Endpoints whose handlers are inside a class can be added to a parent static Endpoints
and resolved from the parent. This means the parent Endpoints must hold a reference
to the class object and also know how to invoke the class method.

*Questions*
- is the object bound during on(...) or during resolve(...)?
- binding during on(...) means we cant bind depending on URL arguments for example.
- how is method handler returned as a static function? using std::bind()?

*Implementation*
- [x] For instances Y whose handler is a class method, the node must store a lambda that
converts the external handler into a static one that is bound to the object instance.
I used std::bind to convert the member function into a static invokable.
- [x] User can supply a data instance object or a callback function that returns a data
instance. Callback has access to URL arguments up to the point of callback so it can 
return a data instance dynamically based on a URL argument.
- [x] SFINAE should be able to provide compatible `with(...)` methods
- [x] _Auto owned Class Endpoints_ - the with(instance) could take the existing Endpoints
type and construct one with a cv-ptr and return it. Thus something like the following 
would create essentially anonymous types stored in the parent Endpoints. The fact no 
Endpoints argument is given implies the NodeData must hold ownership of the EP not a ref 
to it. 
```cpp
Endpoints.on("...")
    .with( obj )             // object instance
        .on("...").GET(x)
    .with( callback_func )   // callback returns object instance
        .on("...").GET(y)
```

# Memory Optimizations
- [x] All instances of Endpoints share the same text binbag. I can't see a downside, for endpoints objects that get created for each instance of a user's class this means the literals are not duplicated in memory.
- [ ] Improve the Pool template class. It's currently just malloc'ing a fixed size.
    - beware memory fragmentation on small devices. Possibly have Pool template for large and micro hardware.
    - add a pack function?
    - allocate in small pages?
    
# Notes
- The Handler from Endpoints<> is not a direct function but the Rest::Handler<> type.
    - Could that hold function_traits?
    - For simplicity, can we remove it and Endpoints<> takes any object or function as handler.

# Todo
- [x] In Node resolve, Change `const char* url` into UriRequest that contains Url string and arguments. Endpoints::resolve() can still be a const char*