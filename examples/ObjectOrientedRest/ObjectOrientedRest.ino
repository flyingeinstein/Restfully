
// SimpleRestServer - example project for Restfully library
// Written by Colin MacKenzie, MIT license
// (c)2018 FlyingEinstein.com

// hard code the node name of the device
const char* hostname = "nimbl";

// if you dont use the Captive Portal for config you must define
// the SSID and Password of the network to connect to.
const char* ssid = "Yogi";
const char* password = "cholera0625";

#include <Esp.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <Restfully.h>

#include "Device.h"

ESP8266WebServer server(80);
RestRequestHandler restHandler;

Animal* animals[2];

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

int handleEcho(RestRequest& request) {
  String s("Hello ");
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
  Serial.begin(115200);
  Serial.println("SimpleRestServer");

  server.addHandler(&optionsRequestHandler);
  server.addHandler(&restHandler);

  server.on("/", handleRoot);
  //server.on("/status", JsonSendStatus);
  //server.on("/devices", JsonSendDevices);

  // binding a handler in the form of int(RestRequest&) to an endpoint
  restHandler.on("/api/echo/:msg(string|integer)", GET(handleEcho) );

  //server.onNotFound(handleNotFound);
  // our 404 handler tries to load documents from SPIFFS
  server.onNotFound(handleNotFound);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  server.begin();

  /* setup our animals 
   */
  animals[0] = new Cat();
  animals[1] = new Dog();

  // read or write the state of an analog pin
  // uses direct lambda expression
  restHandler.on("/animals/:ordinal(integer)/*", 
    GET([](RestRequest& request) {
      unsigned int ord = request["ordinal"];
      return (ord < sizeof(animals)/sizeof(animals[0]) && animals[ord]!=nullptr)
        ? restHandler.defer(animals[ord]->endpoints, request)
        : 404;  // ordinal value doesnt exist, not found
    })
  );
  restHandler.on("/animals/:name(string)/*", 
    GET([](RestRequest& request) {
      auto name = request["name"];
      Serial.println(request["_url"].toString());
      for(int i=0, _i=sizeof(animals)/sizeof(animals[0]); i<_i; i++) {
        if(animals[i] && animals[i]->name==name.toString())
          return restHandler.defer(animals[i]->endpoints, request);
      }
      return 404;  // ordinal value doesnt exist, not found
    })
  );  
  restHandler.on("/animals/list", 
    GET([](RestRequest& request) {
      size_t count = sizeof(animals)/sizeof(animals[0]);
      JsonArray list = request.response.createNestedArray("animals"); 
      for(size_t i=0; i<count; i++) {
        if(animals[i]!=nullptr)
          list.add(animals[i]->name.c_str());
      }
      return 200;
    })
  );
  restHandler.on("/sys/info", 
    GET([](RestRequest& request) {
      JsonObject root = request.response;
      
      JsonObject cpu = root.createNestedObject("cpu");
      cpu["id"] =  ESP.getChipId();
      cpu["MHz"] =  ESP.getCpuFreqMHz();
      cpu["vcc"] =  ESP.getVcc();

      JsonObject mem = root.createNestedObject("memory");
      mem["sketchSize"] =  ESP.getSketchSize();
      mem["free"] =  ESP.getFreeHeap();
      mem["maxFreeBlockSize"] =  ESP.getMaxFreeBlockSize();
      mem["heapFragmentation"] =  ESP.getHeapFragmentation();
      mem["freeContStack"] =  ESP.getFreeContStack();

      JsonObject sdk = root.createNestedObject("versions");
      sdk["sdk"] =  ESP.getSdkVersion();
      sdk["core"] =  ESP.getCoreVersion();
      sdk["full"] =  ESP.getFullVersion();
      sdk["boot"] =  ESP.getBootVersion();

      JsonObject rst = root.createNestedObject("reset");
      rst["reason"] = ESP.getResetReason();
      rst["info"] = ESP.getResetInfo();
      return 200;
    })
  );

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
