
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
#include <mDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#endif

#include <Restfully.h>
#include "Sensor.h"

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
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
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
    virtual bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) {
      SendHeaders();
      server.send(200, "application/json; charset=utf-8", "");
      return true;
    }
} optionsRequestHandler;



// collection of sensors using shared pointers
// we must use shared pointers in std::vector because we will be adding derived objects of Sensor not actual Sensor objects
std::vector< std::shared_ptr<Sensor> > sensors;



//std::function< decltype(resolve_sensor) > resolve_sensor_func = resolve_sensor;
Sonar sonar;

std::function<Sensor&(UriRequest&)> resolve_sensor = [](UriRequest& request) -> Sensor& {
    auto id = request["id"];
    if(id.isString()) {
      auto name = id.toString();
      Serial.println(name);
      for(auto s=sensors.begin(), _s=sensors.end(); s!=_s; s++) {
        if( (*s)->name == name)
          return **s;
      }
    } else {
      return *(sensors[ (int)id ].get());
      Serial.println((int)id);
    }
    return *(sensors[0]).get();  // todo: ordinal value doesnt exist, not found
  };


void setup() {
  Serial.begin(115200);
  Serial.println("SimpleRestServer");

  server.addHandler(&optionsRequestHandler);
  server.addHandler(&restHandler);

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

  //server.onNotFound(handleNotFound);
  // our 404 handler tries to load documents from SPIFFS
  server.onNotFound(handleNotFound);

  // setup our sensors by adding a sonar and pH sensor
  sensors.insert(sensors.end(), std::make_shared<Sonar>() );
  sensors.insert(sensors.end(), std::make_shared<pH>() );

  // This API call returns sensor data from the Sensor class member
  // This method uses an instance resolver callback and demos Object Oriented Rest APIs
  restHandler.on("/sensors/:id(string|integer)") 
    .with(resolve_sensor)
      .GET(&Sensor::restGet);

  // returns the list of sensors by id & name
  restHandler.on("/sensors") 
    .GET([](RestRequest& request) {
      JsonArray list = request.response.createNestedArray("sensors"); 
      for(auto s=sensors.begin(), _s=sensors.end(); s!=_s; s++) {
        JsonObject sensor = list.createNestedObject();
          sensor["name"] = (*s)->name;
          sensor["type"] = (*s)->sensorType();
      }
      return 200;
    });


    // binding a regular static handler in the form of int(RestRequest&) to an endpoint
    // this endpoint simply says Hello and is a good test API call
  restHandler.on("/api/echo/:msg(string)")
    .GET([](RestRequest& request) {
      JsonObject root = request.body.as<JsonObject>();
      auto greeting = root.get<const char*>("greeting");
      request.response["reply"] = String(greeting ? greeting : "Hello ") + request["msg"].toString();
      return 200;
    });

  // this static call returns device runtime information
  restHandler.on("/sys/info")
    .GET([](RestRequest& request) {
      JsonObject root = request.response;
      
      JsonObject cpu = root.createNestedObject("cpu");
      cpu["id"] =  ESP.getChipId();
      cpu["MHz"] =  ESP.getCpuFreqMHz();
      cpu["vcc"] =  ESP.getVcc();

      JsonObject mem = root.createNestedObject("memory");
      mem["sketchSize"] =  ESP.getSketchSize();
      mem["free"] =  ESP.getFreeHeap();
      //mem["maxFreeBlockSize"] =  ESP.getMaxFreeBlockSize();
      //mem["heapFragmentation"] =  ESP.getHeapFragmentation();
      //mem["freeContStack"] =  ESP.getFreeContStack();

      JsonObject sdk = root.createNestedObject("versions");
      sdk["sdk"] =  ESP.getSdkVersion();
      sdk["core"] =  ESP.getCoreVersion();
      sdk["full"] =  ESP.getFullVersion();
      sdk["boot"] =  ESP.getBootVersion();

      JsonObject rst = root.createNestedObject("reset");
      rst["reason"] = ESP.getResetReason();
      rst["info"] = ESP.getResetInfo();
      return 200;
    });





  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
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
