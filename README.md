
# Restfully
Arduino library for making Rest compliant APIs on the ESP8266 and ESP32. This libary currently supports the
generic WebServer and ESP8266WebServer along with the ArduinoJSON library for handling the parsing of the Request 
and building of the Response. The core primarily handles parsing of the URI and extraction of the embedded arguments
and provides a simple interface to the coder using callback functions, class methods, std::functions or lambdas to 
handle the Rest endpoint request.

The core library is WebServer, protocol and json library agnostic so future versions will support more alternatives 
and other platforms. There is currently a CMake file for compiling on GCC and Clang so you could use for any C++ project 
if you write the interface between your web server and Restfully core (see src/Platforms folder for examples). 

## Installation

#### Arduino Library Manager

Restfully can be installed from within the Arduino IDE menu at Sketch | Include Library | Manage Libraries...

#### Installation from Github

If you want to edit the source code you can clone this repo into your Arduino Libraries folder.

1. Clone the repository to your Arduino libriaries folder. This is typically ~/Documents/Arduino/libraries.
2. Restart Arduino application
3. Review example sketches in File | Examples | Examples from Custom Library | Restfully

## Dependancies

Currently this library only supports the Esp8266 and Esp32 hardware ready-to-code. The core code is not hardware dependant 
so other platforms will follow soon. You may also need to install the following libraries (if you dont already have them):
* Esp8266 or your platform WebServer libraries (requires Wifi, mDNS and WebServer)
* ArduinoJSON  (Use latest 6.5 or greater)

## Usage

To add Restfully to an existing Esp8266WebServer based application add the include for the Restfully.h header
```C
#include <Restfully.h>
```

Add a declaration of RestRequestHandler where you declare your WebServer. The RestRequestHandler class is derived from 
the standard RequestHandler class and intercepts WebServer requests that have a Rest endpoint. something like this:
```C
ESP8266WebServer server(80);
RestRequestHandler restHandler;   // add this
```

Add the restHandler to the web server instance in your setup() function. You must add this after any server.on(...) 
statements you have.
```
  server.addHandler(&restHandler);
```

## Rest Endpoint Declarations

Now similar to http server.on(...) calls, we can add our Rest API endpoints. We can however specify embedded arguments 
and thier types and Restfully will match and extract these arguments for us. Some samples:
```C
// provide a simple echo API method that returns in Json the string or integer given in the 'msg' argument. A handler is bound to the GET http verb.
restHandler.on("/api/echo/:msg(string|integer)").GET(handleEcho);

// declare a method that reads a GPIO pin on the core. The pin must be an integer, otherwise no match is made and the web server's default handler is called
restHandler.on("/api/digital/pin/:pin(integer)").GET(ReadDigitalPin);

// A Rest method to write a value to a GPIO pin. Argument 'pin' must be an integer, and the value can be either a string or integer.
restHandler.on("/api/digital/pin/:pin(integer)/set/:value(integer|string)").PUT(WriteDigitalPin);
```

You bind a Rest URI endpoint declaration with a function handler. You can bind a seperate function to any of the HTTP 
verbs GET, POST, PUT, PATCH, DELETE, OPTIONS or ANY. You can specify the same handler function to multiple verbs. 
```C
restHandler.on("/api/digital/pin/:pin(integer)")
   .GET(GpioGetHandler)
   .PUT(GpioPutHandler)
   .OPTIONS(GpioOptionsHandler);
```

Internally these endpoint URIs are parsed into a search tree so matching URIs during run-time are efficient. The following
code would produce the same URI search tree as above and therefor the same performance. Only difference being
the extra flash memory used by the redundant strings.
```C
restHandler.on("/api/digital/pin/:pin(integer)").GET(GpioGetHandler);
restHandler.on("/api/digital/pin/:pin(integer)").PUT(GpioPutHandler);
restHandler.on("/api/digital/pin/:pin(integer)").OPTIONS(GpioOptionsHandler);
```
Using the same Endpoint declaration is fine if the bound HTTP verb is different but it would be invalid to try to attach
a method handler to the same Endpoint and to the same HTTP verb.

Since matching of URIs are efficient, you can use this to make code for your handler simpler; especially for arguments we 
typically think of as as an enumeration or command, such as {start, stop, status} or {on, off, toggle}. Take this example
that controls the User LED:
```C
restHandler.on("/api/led/:command(string)").GET(GetLED).PUT(SetLED);
```
Although brief there are a few issues here. Assuming we expect the :command argument to be one of either {on, off, status)
then Restfully would have no issue passing the "status" command to SetLED if the user did a PUT request when it should be a
GET. Or vice versa, you could pass the "on" command for a GET request and this isnt good Rest form. Also /api/led/explode
would still invoke your handler leaving you to require coding guards against invalid values. You would have to handle the
logic of returning a 404 or 400 error code in your method logic along with a performance hit of some string comparisons.
Instead, use multiple on(...) calls to let Restfully do the logic for you.
```C
restHandler
    .on("/api/led")                // start all subsequent calls at this Endpoint node
       .PUT("on", SetLED_On);      // ON action (PUT)
       .PUT("off", SetLED_Off);    // OFF action (PUT)
       .GET(GetLED);               // LED status (GET)
```
The Restfully parser will create the /api/led node of the URI, then bind the on and off actions to their handlers, and
finally the status handler to the /api/led node. No logic needed in the method handlers, they are one-liners. Also see
the use of std::bind() in *Better Enumerations* for a way to combine the SetLED on/off methods into one handler.

## Handler functions

The function handler has the following prototype, for example the 'echo' handler:
```C
int handleEcho(RestRequest& request)
```
The 'request' argument provides access to the embedded arguments, web server (and therefor POST or query string 
parameters), and the Json request and response objects (via ArduinoJson library). Let's take a more detailed look at
the 'echo' handler:
```C
int handleEcho(RestRequest& request) {
  String s("Hello ");
    auto msg = request["msg"];    // retrieve the :msg parameter from the URI
    if(msg.isString())
      s += msg.toString();
    else {
      // must be an integer
      s += '#';
      s += (long)msg;           // convert the argument object to an integer
    }
    request.response["reply"] = s;  // set the response to {"reply": "Hello <string>"}
    return 200;
}
```

Handlers are not limited to being regular functions. You can attach lambdas or instances of std::function as long as it
matches the int(RestRequest&) prototype.

### Method Chaining
The Endpoints support method chaining. See https://en.wikipedia.org/wiki/Method_chaining. The on(...) method returns a
Node object, which supports GET, PUT, PATCH, etc and also the same on(...) method for relative node addressing. The GET,
PUT, POST methods are also overloaded to take a relative node path and handler or just a handler. The LED example above
shows both usages.

### Lambda Expressions
The following implements an API interface to getting or setting an analog pin value (0-1023). We are using method
chaining to attach both handlers to the _/api/analog/pin/:pin(integer)_ node. 
```C
  restHandler
    .on("/api/analog/pin/:pin(integer)")
        .GET([](RestRequest& request) {
          int pin = request["pin"]; // note: automatic conversion from Argument to integer
          request.response["value"] = analogRead(pin);
          return 200;
        })
        .PUT(":value(integer)", [](RestRequest& request) {
          int pin = request["pin"]; // note: automatic conversion from Argument to integer
          int value = request["value"];
          analogWrite(pin, value);
          request.response["value"] = analogRead(pin);
          return 200;
        });
```

### std::function handlers
We are implementing the code as a lambda, but assigning it to a std::function.
```C
std::function<int(RestRequest&)> WriteDigitalPin = [](RestRequest& request) {
    int pin = request["pin"]; // note: automatic conversion from Argument to integer
    auto value = request["value"];
    pinMode(pin, OUTPUT);
    if(value.isString()) {
      const char* strval = (const char*)value;
      digitalWrite(pin, (strcasecmp(strval, "high")==0) ? HIGH : LOW);  // expect "low" or "high"
    } else
      digitalWrite(pin, ((int)value > 0) ? HIGH : LOW);   // any non-zero value is HIGH
    request.response["value"] = (digitalRead(pin)==HIGH) 
      ? "high"
      :"low";
    return 200;
  };
```

## Better Enumerations
Since we can use std::bind(), we can improve our handling of endpoints with commands or 
enumerations. Take the LED example above. We don't need to split the SetLED into SetLED_On 
and SetLED_Off, we can instead write the function once and use std::bind to create the 
two versions of the method call. This produces efficient code between Restfully's URI
search tree and the constants bound to simple functions.
```C
// our SetLED handler, but with an added _value_ argument to the handler
auto SetLED = [](RestRequest& request, int value) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, value);
  return 200;
};

// we now use std::bind to create the two different versions
restHandler.on("/api/led")
    .PUT("off", std::bind(SetLED, 
        std::placeholders::_1,     // RestRequest placeholder,
        HIGH                      // High=Off, specified and bound as constant
    )) 
    .PUT("on", std::bind(SetLED,
        std::placeholders::_1,     // RestRequest placeholder,
        LOW                      // Low=On, specified and bound as constant
    ));
```


### Handlers bound to class method using std::function and std::bind handlers
You can also bind handlers to a member of a class instance using std::bind. This means that the object instance must
stay alive the entire run-time. std::bind() allows us to bind a _this_ pointer to a function and returns a static 
function. 
```C
restHandler.on("/api/system/status")
    .ANY( std::bind(
        &SystemAPI::status,     // the class method including the class name scope
        system_obj,             // a pointer to an instance of a SystemAPI class (the _this_ pointer)
        std::placeholders::_1   // required, this is a placeholder for the RestRequest& argument
    ));
```


### Class Instance Rest Handlers

You can also define an Endpoints class that holds class method handlers instead of static functions. This example code 
creates endpoints for turning LEDs on and off and the LED's identifier is given as part of the URI.
```C
// define a simple LED class
class LED {
public:
    int on(Rest::Request& r);
    int off(Rest::Request& r);
    
    int pin;    // GPIO pin for this LED indicator
    
    inline LED(int p) : pin(p) {}
}

// define the type of our handlers
using LEDHandlerType = decltype(Klass::echo); // defined as int(LED::*)(Rest::Request&) but easy to just use decltype operator

// Define our Rest endpoints as method handlers of the LED class
Rest::Endpoints< LEDHandlerType > indicator_endpoints;
endpoints.on('/led/:id')
    .PUT("on", &LED::on)
    .PUT("off", &LED::off);
```

However the restHandler that is pre-defined by Restfully has already defined Endpoints with static handlers and trying
to mix endpoints types would obviously be a C++ type mismatch. If only there was a way to attach method endpoints to
static ones, like this:
```C
// create a collection of LED indicators
std::map<int, LED> indicators;
indicators.insert(indicators.begin(), LED(5) );  // LED0 on pin 5
indicators.insert(indicators.begin(), LED(7) );  // LED1 on pin 7

// implement a function callback that given a Rest request (with 'id' argument) returns an LED object instance
auto indicator_context = [&indicators](Rest::UriRequest& rr) -> LED& { 
    // lookup id in collection and return a reference
    // FYI we should be gaurding against the returned iterator being invalid
    return *indicators.find( (int)rr["id"] );   // id argument is found in URL. ex. /led/5/on
}

// now attach our LED endpoints to the restHandler's static endpoint node using `.with(<callback>, <method-endpoints>)`
restHandler.on('/api')
    .with( 
        indicator_context,      // a function that will resolve the ID in a list of LEDs
        indicator_endpoints     // the LED endpoints that return a class member as handler
    )
```

In the example above we used a function to resolve the LED object instance based on the :id URI parameter into a std::map
collection. You can also simply supply a single class instance instead of a callback function. The class instance is bound
to the handler function during API request time and the bound function can then be called with the restHandler like a 
static handler. Obviously, Objects must stay alive but like in this example the indicators collection is free to grow or shrink.

If you really need to store class method handlers and not join like above you'll need to handle the resolving of URIs 
using the `Node.resolve(HttpMethod, const char* Uri)` function and then call the handler using the C++ .* operator. 
( `<obj>.*<method-ptr>(...)` syntax). Honestly though, I can't see how this would be useful but maybe you'll find a way.

See more [advanced usages](ADVANCED.md) with Class Instance Rest Handlers. 

