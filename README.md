# Restfully
Arduino library for making Rest compliant APIs on the ESP8266 (ESP32). This libary currently supports
the ESP8266WebServer and the ArduinoJSON library for handling the parsing of the Request and building 
of the Response. The core library is WebServer, protocol and json library agnostic so future versions
will support more alternatives and other platforms. At it's core this library mainly handles parsing
of the URI and extraction of the embedded arguments, and then calling the bound function or lambda to
handle the request.
