
// SimpleRestServer - example project for Restfully library
// Written by Colin MacKenzie, MIT license
// (c)2018 FlyingEinstein.com

// hard code the node name of the device
const char* hostname = "myhome";

// if you dont use the Captive Portal for config you must define
// the SSID and Password of the network to connect to.
const char* ssid = "MyRouter";
const char* password = "mypassword";


#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

using WebServer = ESP8266WebServer;
#else
#include <WiFi.h>
//#include <mDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#endif

#include <Restfully.h>

WebServer server(80);
RestRequestHandler restHandler;


/*** Web Server

*/
void SendHeaders()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
}

void handleRoot() {
  String html = "<html><head><title>Simple Rest Server</title>";
  html += "</head>";
  html += "<body>";
  // header
  html += "<h1>Simple Rest Server</h1>";
  // title area
  html += "<div class='title'><h2><label>Site</label> ";
  html += hostname;
  html += "</h2></div>";
  // ... add more here ...
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  switch(server.method()) {
    case HTTP_ANY: message += "ANY"; break;
    case HTTP_GET: message += "GET"; break;
    case HTTP_POST: message += "POST"; break;
    case HTTP_PUT: message += "PUT"; break;
    case HTTP_PATCH: message += "PATCH"; break;
    case HTTP_DELETE: message += "DELETE"; break;
    case HTTP_OPTIONS: message += "OPTIONS"; break;
  }
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

/* Most browsers require a responst to OPTIONS requests or they will deny requests due to CORS policy.
 * We simply answer all requests with the appropriate Access-Control-* headers for testing.
*/
class OptionsRequestHandler : public RequestHandler
{
    virtual bool canHandle(HTTPMethod method, String uri) {
      return method == HTTP_OPTIONS;
    }
    virtual bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) {
      SendHeaders();
      server.send(200, "application/json; charset=utf-8", "");
      return true;
    }
} optionsRequestHandler;

int handleEcho(RestRequest& request) {
  String s("Hello ");

  // if we have a json body, perhaps we have a greeting parameter
  if(request.hasJson) {
    auto root = request.body.as<JsonObject>();  // expect request is a Json object not array
    const char* greeting = root["greeting"];  // NULL if this argument doesnt exist
    if(greeting) {
      s = greeting; // default to Hello if greeting argument wasnt supplied
      s += ' ';
    }
  }
  
  // add the greeting name
  auto msg = request["msg"];
  if(msg.isString())
    s += msg.toString();
  else {
    s += '#';
    s += (long)msg;
  }
  request.response["reply"] = s;
  return 200;
}

void setup() {
  Serial.begin(230400);
  Serial.println("SimpleRestServer");

  // have the server collect the content-type header for us
  const char * headerkeys[] = {"Content-Type"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders( headerkeys, headerkeyssize );

  server.on("/", handleRoot);

  // handle CORS requests from browsers
  // https://en.wikipedia.org/wiki/Cross-origin_resource_sharing
  // Without answering http OPTIONS requests most external browser clients/javascript will fail with a security
  // error. We could handle OPTIONS requests per API call using Restfully but if all you intend to do is to blanket 
  // allow API calls then a single OPTIONS handler that returns access-control headers will do fine.
  server.addHandler(&optionsRequestHandler);

  // Add the Restfully Web Server Request Handler
  // note: adding the Restfully handler should be done last, after all other server.on(...) statements because
  // if the handler doesnt find an endpoint it will trigger a 404 regardless if a later on(...) should catch it.
  server.addHandler(&restHandler);

  // A custom 404 handler
  server.onNotFound(handleNotFound);
  
  // binding a handler in the form of int(RestRequest&) to an endpoint
  restHandler.on("/api/echo/:msg(string|integer)")
    .GET(handleEcho)
    .POST(handleEcho);

  // read the state of a digital pin
  // uses a lambda function held within a std::function
  std::function<int(RestRequest&)> ReadDigitalPin = [](RestRequest& request) {
    int pin = request["pin"]; // note: automatic conversion from Argument to integer
    request.response["value"] = (digitalRead(pin)==HIGH) 
      ? "high"
      :"low";
    return 200;
  };
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
  // now bind the handlers to the digital pin endpoint
  restHandler.on("/api/digital/pin/:pin(integer)").GET(ReadDigitalPin);

  restHandler.on("/api/digital/pin/:pin(integer)/set/:value(integer|string)").PUT(WriteDigitalPin);

#if !defined(ARDUINO_ARCH_ESP32) // no analogWrite() on ESP32
  // read or write the state of an analog pin
  // uses direct lambda expression
  restHandler.on("/api/analog/pin/:pin(integer)") 
    .GET([](RestRequest& request) {
      int pin = request["pin"]; // note: automatic conversion from Argument to integer
      request.response["value"] = analogRead(pin);
      return 200;
    })
    .PUT([](RestRequest& request) {
      int pin = request["pin"]; // note: automatic conversion from Argument to integer
      int value = request["value"];
      analogWrite(pin, value);
      request.response["value"] = analogRead(pin);
      return 200;
    });
#endif

  // set the state of the BuiltIn User LED
  // uses a lambda function but in this case we have an extra argument 'value' which will be passed to digitalWrite, we
  // will use this lambda and the std::bind() function to set as a constant. 
  // LED is active low, so to turn on we write a LOW value to the pin.
  auto SetLed = [](RestRequest& request, int value) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, value);
    request.response["led"] = digitalRead(LED_BUILTIN) ? "off":"on";
    return 200;
  };
  restHandler.on("/api/led")
    .PUT("off", std::bind(SetLed, 
        std::placeholders::_1,     // RestRequest placeholder,
        HIGH                      // High=Off, specified and bound as constant
    ))  
    .PUT("on", std::bind(SetLed,
        std::placeholders::_1,     // RestRequest placeholder,
        LOW                      // Low=On, specified and bound as constant
    ))
    .PUT("toggle", [&SetLed](RestRequest& request) {
        pinMode(LED_BUILTIN, OUTPUT);
        return SetLed(request, ! digitalRead(LED_BUILTIN)); // toggles pin state
    })
    .GET([](RestRequest& request) {
        request.response["pin"] = LED_BUILTIN;
        request.response["led"] = digitalRead(LED_BUILTIN) ? "off":"on";
        return 200;
    });
  
  WiFi.mode(WIFI_STA);
#if !defined(ARDUINO_ARCH_ESP32)
  WiFi.hostname(hostname);
#endif
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  server.begin();

  Serial.print("Host: ");
  Serial.print(hostname);
  Serial.print("   IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  server.handleClient();

  // no further processing if we are not in station mode
  if(WiFi.getMode() != WIFI_STA)
    return;
}
