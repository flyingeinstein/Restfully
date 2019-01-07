# Restfully
Arduino library for making Rest compliant APIs on the ESP8266 (ESP32). This libary currently supports
the ESP8266WebServer and the ArduinoJSON library for handling the parsing of the Request and building 
of the Response. The core library is WebServer, protocol and json library agnostic so future versions
will support more alternatives and other platforms. At it's core this library mainly handles parsing
of the URI and extraction of the embedded arguments, and then calling the bound function or lambda to
handle the request.

# Installation

1. Clone the repository to your Arduino libriaries folder. This is typically ~/Documents/Arduino/libraries.
2. Restart Arduino application
3. Review example sketches in File | Examples | Examples from Custom Library | Restfully

# Dependancies

Currently this library only supports the Esp8266 hardware. The core code is not hardware dependant so other platforms will follow soon. You may also need to install the following libraries (if you dont already have them):
* Esp8266 (requires Wifi, mDNS and WebServer)
* ArduinoJSON  (Use latest 6.5)

# Usage

To add Restfully to an existing Esp8266WebServer based application add the include for the Restfully.h header
```C
#include <Restfully.h>
```

Add a declaration of RestRequestHandler where you declare your WebServer. The RestRequestHandler class is derived from the standard RequestHandler class and intercepts WebServer requests that have a Rest endpoint. something like this:
```C
ESP8266WebServer server(80);
RestRequestHandler restHandler;   // add this
```

Add the restHandler to the web server instance in your setup() function:
```
  server.addHandler(&restHandler);
```

# Rest Endpoint Declarations

Now similar to http server.on(...) calls, we can add our Rest API endpoints. We can however specify embedded arguments and thier types and Restfully will match and extract these arguments for us. Some samples:
```C
// provide a simple echo API method that returns in Json the string or integer given in the 'msg' argument. A handler is bound to the GET http verb.
restHandler.on("/api/echo/:msg(string|integer)", GET(handleEcho) );

// declare a method that reads a GPIO pin on the core. The pin must be an integer, otherwise no match is made and the web server's default handler is called
restHandler.on("/api/digital/pin/:pin(integer)", GET(ReadDigitalPin) );

// A Rest method to write a value to a GPIO pin. Argument 'pin' must be an integer, and the value can be either a string or integer.
restHandler.on("/api/digital/pin/:pin(integer)/set/:value(integer|string)", PUT(WriteDigitalPin) );
```

You bind a Rest URI endpoint declaration with a function handler. You can bind a seperate function to any of the HTTP verbs GET, POST, PUT, PATCH, DELETE, OPTIONS or ANY. You can specify the same handler function to multiple verbs. 
```C
restHandler.on("/api/digital/pin/:pin(integer)", 
   GET(GpioGetHandler), 
   PUT(GpioPutHandler), 
   OPTIONS(GpioOptionsHandler) 
);
```

Internally these endpoint URIs are parsed into a search tree so matching URIs during run-time are efficient. The following code would produce the same URI search tree as above and therefor the same performance. 
```C
restHandler.on("/api/digital/pin/:pin(integer)", GET(GpioGetHandler) );
restHandler.on("/api/digital/pin/:pin(integer)", PUT(GpioPutHandler) );
restHandler.on("/api/digital/pin/:pin(integer)", OPTIONS(GpioOptionsHandler) );
```
Using the same Endpoint declaration is fine if the bound HTTP verb is different but it would be invalid to try to attach a method handler to the same Endpoint and to the same HTTP verb.

Since matching of URIs are efficient, you can use this to make code your method simpler especially for arguments we typically think of as as an enumeration or command, such as {start, stop, status} or {on, off, toggle}. Take this example that controls the User LED:
```C
restHandler.on("/api/led/:command(string)", GET(GetLED), PUT(SetLED));
```
Although brief there are a few issues here. Assuming we expect the :command argument to be one of either {on, off, status) then Restfully would have no issue passing /api/led/status to SetLED if the user did a PUT request. Or vice versa, it could pass /api/led/on for a GET request. Also /api/led/explode would invoke your handler. You would have to handle the logic of returning a 404 or 400 error code in your method logic along with a performance hit of some string comparisons. Instead, use multiple on(...) calls to let Restfully do the logic for you.
```C
restHandler.on("/api/led/on", PUT(SetLED_On));
restHandler.on("/api/led/off", PUT(SetLED_Off));
restHandler.on("/api/led/status", GET(GetLED));
```
The Restfully parser will match the /api/led part of the URI, then conditionally compare the next part of the URI to the 3 literals on, off and status. No logic needed in the method handlers, they are one-liners. For more complex handlers than just setting an LED you might not like that the SetLED function is split/duplicated, see the use of std::bind() in *Better Enumerations* below for an example of getting around this.

## Handler functions

The function handler has the following prototype, for example the 'echo' handler:
```C
int handleEcho(RestRequest& request)
```
The 'request' argument provides access to the embedded arguments, web server (and therefor POST or query string parameters), and the Json request and response objects (via ArduinoJson library). Let's take a  more detailed look at the 'echo' handler:
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

Handlers are not limited to being regular functions. You can attach lambdas or instances of std::function as long as it matches the int(RestRequest&) prototype.

### Lambda Expressions
The following implements an API interface to getting or setting an analog pin value (0-1023).
```C
  restHandler.on("/api/analog/pin/:pin(integer)", 
    GET([](RestRequest& request) {
      int pin = request["pin"]; // note: automatic conversion from Argument to integer
      request.response["value"] = analogRead(pin);
      return 200;
    })
  );
  restHandler.on("/api/analog/pin/:pin(integer)/:value(integer)", 
    PUT([](RestRequest& request) {
      int pin = request["pin"]; // note: automatic conversion from Argument to integer
      int value = request["value"];
      analogWrite(pin, value);
      request.response["value"] = analogRead(pin);
      return 200;
    })
  );
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

### handlers bound to class method using std::function and std::bind handlers
You can also bind handlers to a member of a class instance using std::bind. This means that the object instance must stay alive the entire run-time. std::bind() allows us to bind a _this_ pointer to a function and returns a static function. 
```C
restHandler.on("/api/system/status", ANY(std::bind(
  &SystemAPI::status,     // the class method including the class name scope
  system_obj,             // a pointer to an instance of a SystemAPI class (the _this_ pointer)
  std::placeholders::_1   // required, this is a placeholder for the RestRequest& argument
)));
```

## Better Enumerations
Since we can use std::bind(), we can improve our handling of endpoints with commands or enumerations. Take the LED example above. We don't need to split the SetLED into SetLED_On and SetLED_Off, we can instead write the function once and use std::bind to create the two versions of the method call. This produces efficient code.
```C
// our SetLED handler, but with an added _value_ argument to the handler
auto SetLED = [](RestRequest& request, int value) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, value);
  return 200;
};

// we now use std::bind to create the two different versions
restHandler.on("/api/led/off", PUT(std::bind(SetLED, 
    std::placeholders::_1,     // RestRequest placeholder,
    HIGH                      // High=Off, specified and bound as constant
)));  
restHandler.on("/api/led/on", PUT(std::bind(SetLED,
    std::placeholders::_1,     // RestRequest placeholder,
    LOW                      // Low=On, specified and bound as constant
)));
```

