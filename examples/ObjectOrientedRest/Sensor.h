
#pragma once


class Sensor
{
  public:
    String name;
    int value;

    Sensor(const char* _name) : name(_name), value(0) {}

    virtual String sensorType() const { return "Generic"; };

    virtual int restGet(RestRequest& request) {
        JsonObject sensor = request.response.createNestedObject("sensor");
        sensor["name"] = name;
        sensor["type"] = sensorType();
        sensor["value"] = value;
        request.error(3, "sensor ok");
        return 200;
    }
};

class Calibration
{
  public:
    std::vector<int> calib_data;
    
  Calibration(std::initializer_list<int> _data) : calib_data(_data) {}
};

class Sonar : public Sensor
{
  public:
    Sonar() : Sensor("distance") {}

    String sensorType() const { return "Sonar"; }
};

class pH : public Sensor, public Calibration
{
  public:
    pH() : Sensor("acidity"), Calibration({13,8,3,160}) {}

    String sensorType() const { return "pH"; }    
};
