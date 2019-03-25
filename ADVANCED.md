## Joining Endpoint collections

We can join two Endpoints collection using the `.with(...)` method. The with method will return the right-hand Endpoints 
collection so you can keep using `.on(), .GET(), .PUT(), etc` in a method invoke chain. I am using Restfully in another 
project (Nimble) where the Rest interface is key to interfacing with the internal sensor devices, etc. in a plug-and-play
way so splitting the Rest endpoints into multiple units was very important. 


First, the simple example to join two Endpoints containing the same Handler type. We partition our Rest system into a 
System Info collection and use .with(sysinfo) to attach to our Endpoints collection from within the RequestHandler.
```C
Endpoints sysinfo;
restHandler.on("/api/sys")
    .with(sysinfo)                // attaching our sysinfo endpoints to main handler (returns sysinfo object)
    .on("status", StatusInfo) .   // attaching status endpoint to sysinfo endpoints
    .on("network", NetworkInfo);  // attaching network endpoint to sysinfo endpoints
```

We can now build Rest endpoints that terminate at Class methods but we do need to supply the object instance in some way. The `.with()` method supports supplying the object instance as one of the arguments. The `with` method has overloads that make it very flexible. It can:
* join different Endpoint collections
    * Example: Have multiple endpoint collections, "sub-systems", that join to the root collection in restHandler.
* supports class methods as handlers
    * Works great with first feature. Have a parent collection with static handlers, then attach instance handlers associated with an object instance. 
    * Also, the instance object can instead be a callback function that does a lookup and returns the instance object. For example, a rest endpoint like `/devices/<device-id>/control/start` might use the _device-id_ parameter to lookup and return the right device object instance to invoke the "start" endpoint handler.

Actually, the sysinfo variable is optional here since we never actually use ep2 outside the chain. The Endpoints class can construct it as an anonymous type and store it within the parent collection. This example defines Rest endpoints on the Sys class methods and we dont have to hold the Endpoints collection in a variable.
```
class Sys {  status(); statistics() }
Sys mysys;

restHandler
    .on("/api/sys")
    .with(mysys)      // this creates a new anonymous Endpoints collection of Sys class method handlers and returns it
        .GET("status", &Sys::status)            // status handler will call mysys.status
        .GET("statistics", &Sys::statistics) .  // statistics handler on will call mysys.statistics
```

The `with(...)` method has overloads that support these _args_:
- `<Endpoints>`  - join Endpoints at the current node. (both left and right Endpoints collections must use the same handler prototype.)
- `<object>, <Endpoints>`  - join class method Endpoints at current node, but use `object` as the 'this' pointer.
- `<object>`  - construct an anonymous class method Endpoints collection and store within the parent collection. The `object` is still used as the 'this' pointer during invokes. Since with() returns the anonymous type you can add the handlers by method chaining.
- `<callback_function>, <Endpoints>` - join class method Endpoints at current node. At Request time, the callback_function is invoked to lookup and return the `object` reference. callback_function prototype would be `Klass cb_func(Rest::UriRequest& r)` where Klass is your `object` type and UriRequest contains the URL and parameters parsed so far and your code may use a parameter as the key to lookup your object instance dynamically.

## Class Method Handlers vs Static Handlers
In all cases, when you use `with(...)` to attach an object instance to a member function the `this` object is bound to the member function at Request time to produce a statically invokable function call (think std::bind). So in the above examples resolving from the parent (containing static handlers) will always return a static function handler regardless of joined instance member based Endpoints. 

You can also create stand-alone Endpoints<Klass> that return class-method handlers as well but all method handlers in an Endpoints collection must be from the same class type. You would resolve a URL to get a class-method handler, and would perform the invoke yourself using the .* operator.

