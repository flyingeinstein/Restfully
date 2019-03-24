



### Joining Endpoint collections

We can join two Endpoints collection using the `.with(...)` method. The with method will return the right-hand Endpoints 
collection so you can keep using `.on(), .GET(), .PUT(), etc` in a method invoke chain. I am using Restfully in another 
project (Nimble) where the Rest interface is key to interfacing with the internal sensor devices, etc. in a plug-and-play
way so splitting the Rest endpoints into multiple units was very important. 


First, the simple example to join two Endpoints containing the same Handler type. We partition our Rest system into a 
System Info collection and use .with(sysinfo) to attach to our Endpoints collection from within the RequestHandler.
```C
Endpoints sysinfo;
restHandler.on("/api/sys")
    .with(sysinfo)
    .on("status", StatusInfo)
    .on("network", NetworkInfo);     
```

Going back to the Class Method handlers collection, we can now build Rest endpoints that terminate at Class methods but
we do need to supply the object instance in some way. The `.with()` method supports supplying the object instance as one
of the arguments.


- Can join different Endpoint collections. 
    - Example: Have a main collection that branches off to multiple "sub-systems".
- Now supports class methods as handlers. 
    - Works great with first feature. Have a parent collection with static handlers, then attach instance handlers associated with a 'this' object. 
    - Also, the instance object can instead be a callback function that returns the instance object. For example, a rest endpoint like `/devices/<device-id>/control/start` might use the _device-id_ parameter to lookup and return the right device object instance to invoke the "start" endpoint handler.
- (Mostly) less cryptic error messages
    - though I wish it was still better but the old ones might as well have been machine code.


Actually, ep2 is optional here since we never actually use ep2 outside the chain. The Endpoints class can construct it as an anonymous type and store it within the parent collection.
```
class Sys {  status(); statistics() }
Sys mysys;

Endpoints ep1;     // this type is actually system dependent
ep1
    .on("/api/sys")
    .with(mysys)      // this creates a new anonymous collection with Sys class and returns it
        .on("status")
            .GET(&Sys::status)
        .on("statistics")
            .GET(&Sys::statistics)
```

The `with(...)` method has overloads that support these _args_:
- `<Endpoints>`  - join Endpoints at the current node. (both left and right Endpoints collections must be the same type.)
- `<object>, <Endpoints>`  - join Endpoints at current node, but use `object` as the 'this' pointer.
- `<object>`  - construct an anonymous Endpoints collection and store within the parent collection. The `object` is still used as the 'this' pointer during invokes.
- `<callback_function>, <Endpoints>` - join Endpoints at current node. At Request time, the callback_function is invoked to lookup and return the `object` reference. callback_function prototype would be `Klass cb_func(Rest::UriRequest& r)` where Klass is your `object` type and UriRequest contains the URL  and parameters parsed so far and your code may use a parameter as the key to lookup your object instance dynamically.

## Class Method Handlers vs Static Handlers
In all cases, when you use `with(...)` to attach an object instance to a member function the `this` object is bound to the member function at Request time to produce a statically invokable function call (think std::bind). So in the above examples resolving from the parent (containing static handlers) will always return a static function handler regardless of joined instance member based Endpoints. 

You can also create stand-alone Endpoints<Klass> that return class-method handlers as well but all method handlers in an Endpoints collection must be from the same class type. You would resolve a URL to get a class-method handler, and would perform the invoke yourself using the .* operator.

