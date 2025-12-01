#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <unordered_map>
#include <string>

#define RESENT_INTERVAL_MS 500

struct Actuator {
    String name; // unique name of the actuator
    int level; // level of the actuator
    bool isRegistered;
    int state;
    unsigned int lastRegisterSentMillis;

    Actuator(String name = "", int level = 0);
    String getRegisterMessage();
    String getStateMessage();
};

struct Sensor {
    String name; // unique name of the sensor
    String type; // "boolean", "integer", "float", "string"
    String state;
    bool isRegistered;
    unsigned int lastRegisterSentMillis;

    Sensor(String name = "", String type = "");
    String getRegisterMessage();
    String getStateMessage();
};

struct LogType {
    String type;
    String message;
    LogType(String type = "", String message = "");
    String getLogMessage() const;
};

class SmartHomeClient {
public:
    using ActuatorCallback = void(*)(const String& name, int state);
    using LogCallback = void(*) (const LogType& log);
    using RegisterCallback = void(*) (const String& name);
    // constructor
    static SmartHomeClient* getInstance(WiFiClient& wifiClient, const String& deviceId);
    SmartHomeClient(WiFiClient& wifiClient, const String& deviceId);
    SmartHomeClient(const SmartHomeClient& other) = delete;
    SmartHomeClient& operator=(const SmartHomeClient& other) = delete;
    // destructor
    ~SmartHomeClient();
    // public API
    void setupMQTT(const char* server, uint16_t port, const char* username, const char* password);
    void addActuator(String name, int level);
    void addSensor(String name, String type);
    void setLogCallback(LogCallback callback);
    void setRegisterCallback(RegisterCallback callback);
    void setActuatorCallback(ActuatorCallback callback);
    void publishSensorState(String name, String state);
    void publishActuatorState(String name, int state);
    void loop();
private:
    static SmartHomeClient* instance;
    static LogCallback logCallback;
    static RegisterCallback registerCallback;
    static ActuatorCallback actuatorCallback;

    String deviceId;
    PubSubClient mqttClient;
    String registerSub, actionSub, registerPub, actionPub;
    std::unordered_map<std::string, Actuator> actuators;
    std::unordered_map<std::string, Sensor> sensors;
    // internal helpers
    void _publishRegister(Actuator& actuator);
    void _publishRegister(Sensor& sensor);
    
    // MQTT callback handlers
    static void _mqttCallback(char* topic, byte* payload, unsigned int length);
    static void _onRegister(String payload);
    static void _onAction(String payload);
};