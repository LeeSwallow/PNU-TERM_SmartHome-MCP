#include "SmartHome.h"

Actuator::Actuator(String name, int level) : name(name), level(level) {
    isRegistered = false;
    state = 0;
}

String Actuator::getRegisterMessage() {
    String msg = "{";
    msg += "\"type\":\"actuator\",";
    msg += "\"name\":\"" + name + "\",";
    msg += "\"level\":" + String(level);
    msg += "}";
    return msg;
}

String Actuator::getStateMessage() {
    String msg = "{";
    msg += "\"type\":\"actuator\",";
    msg += "\"name\":\"" + name + "\",";
    msg += "\"state\":" + String(state);
    msg += "}";
    return msg;
}

Sensor::Sensor(String name, String type) : name(name), type(type) {
    isRegistered = false;
    state = "";
}

String Sensor::getRegisterMessage() {
    String msg = "{";
    msg += "\"type\":\"sensor\",";
    msg += "\"name\":\"" + name + "\",";
    msg += "\"data_type\":\"" + type + "\"";
    msg += "}";
    return msg;
}

String Sensor::getStateMessage() {
    String msg = "{";
    msg += "\"type\":\"sensor\",";
    msg += "\"name\":\"" + name + "\",";
    msg += "\"state\":\"" + state + "\"";
    msg += "}";
    return msg;
}

LogType::LogType(String type, String message) : type(type), message(message) {}

String LogType::getLogMessage() const {
    String msg = "{";
    msg += "\"type\":\"" + type + "\",";
    msg += "\"message\":\"" + message + "\"";
    msg += "}";
    return msg;
}

SmartHomeClient* SmartHomeClient::instance = nullptr;
SmartHomeClient::ActuatorCallback SmartHomeClient::actuatorCallback = nullptr;
SmartHomeClient::LogCallback SmartHomeClient::logCallback = nullptr;

SmartHomeClient* SmartHomeClient::getInstance(WiFiClient& wifiClient, const String& deviceId) {
    if (instance == nullptr) {
        instance = new SmartHomeClient(wifiClient, deviceId);
    }
    return instance;
}

SmartHomeClient::SmartHomeClient(WiFiClient& wifiClient, const String& deviceId)
    : mqttClient(wifiClient) {
    if (instance != nullptr) {
        throw std::runtime_error("SmartHomeClient instance already exists!");
    }
    SmartHomeClient::instance = this;
    this->deviceId = deviceId;
}

SmartHomeClient::~SmartHomeClient() {
    if (instance == this) {
        instance = nullptr;
    }
}

// 내부 헬퍼 구현
void SmartHomeClient::_publishRegister(Actuator& actuator) {
    if (!mqttClient.connected() || registerPub.length() == 0) return;
    String msg = actuator.getRegisterMessage();
    mqttClient.publish(registerPub.c_str(), msg.c_str());
    actuator.lastRegisterSentMillis = millis();
}

void SmartHomeClient::_publishRegister(Sensor& sensor) {
    if (!mqttClient.connected() || registerPub.length() == 0) return;
    String msg = sensor.getRegisterMessage();
    mqttClient.publish(registerPub.c_str(), msg.c_str());
    sensor.lastRegisterSentMillis = millis();
}

// public API 구현
void SmartHomeClient::setupMQTT(const char* server, uint16_t port, const char* username, const char* password) {
    mqttClient.setServer(server, port);
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceId.c_str(), username, password)) {
            Serial.println("connected");
            registerSub = String("devices/") + deviceId + "/response";
            actionSub = String("devices/") + deviceId + "/action";
            registerPub = String("devices/") + deviceId + "/register";
            actionPub = String("devices/") + deviceId + "/update";

            mqttClient.setCallback(_mqttCallback);
            mqttClient.subscribe(registerSub.c_str());
            mqttClient.subscribe(actionSub.c_str());
        } else {
            if (logCallback != nullptr) {
                logCallback(LogType("error", "failed to connect to MQTT broker, rc=" + String(mqttClient.state())));
            }
            delay(5000);
        }
    }
}

void SmartHomeClient::addActuator(String name, int level) {
    actuators[name.c_str()] = Actuator(name, level);
}

void SmartHomeClient::addSensor(String name, String type) {
    sensors[name.c_str()] = Sensor(name, type);
}

void SmartHomeClient::setActuatorCallback(ActuatorCallback callback) {
    actuatorCallback = callback;
}

void SmartHomeClient::setLogCallback(LogCallback callback) {
    logCallback = callback;
}

void SmartHomeClient::publishSensorState(String name, String state) {
    std::string sensorName = name.c_str();
    if (sensors.find(sensorName) != sensors.end()) {
        // 등록 되지 않은 센서가 있다면 등록 요청 전송
        if (sensors[sensorName].isRegistered == false) {
            _publishRegister(sensors[sensorName]);
            return;
        }
        sensors[sensorName].state = state;
        if (!mqttClient.connected() || actionPub.length() == 0) return;
        String msg = sensors[sensorName].getStateMessage();
        mqttClient.publish(actionPub.c_str(), msg.c_str());
    }
}

void SmartHomeClient::publishActuatorState(String name, int state) {
    std::string actuatorName = name.c_str();
    if (actuators.find(actuatorName) != actuators.end()) {
        if (actuators[actuatorName].isRegistered == false) {
            _publishRegister(actuators[actuatorName]);
            return;
        }
        actuators[actuatorName].state = state;
        String msg = actuators[actuatorName].getStateMessage();
        mqttClient.publish(actionPub.c_str(), msg.c_str());
    }
}

void SmartHomeClient::loop() {
    if (!mqttClient.connected()) {
        return;
    }
    mqttClient.loop();
    for (auto& pair : actuators) {
        Actuator& actuator = pair.second;
        if (!actuator.isRegistered && (millis() - actuator.lastRegisterSentMillis > RESENT_INTERVAL_MS)) {
            _publishRegister(actuator);
        }
    }
    for (auto& pair : sensors) {
        Sensor& sensor = pair.second;
        if (!sensor.isRegistered && (millis() - sensor.lastRegisterSentMillis > RESENT_INTERVAL_MS)) {
            _publishRegister(sensor);
        }
    }
}

void SmartHomeClient::_mqttCallback(char* topic, byte* payload, unsigned int length) {
    SmartHomeClient* client = SmartHomeClient::instance;
    if (!client) return;
    String topicStr = String(topic);
    String payloadStr = String((char*)payload, length);

    if (topicStr == client->registerSub) {
        _onRegister(payloadStr);
    } else if (topicStr == client->actionSub) {
        _onAction(payloadStr);
    } else if (logCallback != nullptr) {
        logCallback(LogType("warning", "Unknown topic received: " + topicStr));
    }
}

void SmartHomeClient::_onRegister(String payload) {
    SmartHomeClient* client = SmartHomeClient::instance;
    if (!client) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload.c_str());
    if (error) {
        if (logCallback != nullptr) logCallback(LogType("error", "Failed to parse register response: " + String(error.c_str())));
        return;
    }
    String name = doc["name"].as<String>();
    std::string key = name.c_str();

    auto ait = client->actuators.find(key);
    if (ait != client->actuators.end()) {
        ait->second.isRegistered = true;
        if (logCallback != nullptr) logCallback(LogType("info", "Actuator registered: " + name));
        return;
    }

    auto sit = client->sensors.find(key);
    if (sit != client->sensors.end()) {
        sit->second.isRegistered = true;
        if (logCallback != nullptr) logCallback(LogType("info", "Sensor registered: " + name));
        return;
    }
}

void SmartHomeClient::_onAction(String payload) {
    SmartHomeClient* client = SmartHomeClient::instance;
    if (!client) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload.c_str());
    if (error) {
        return;
    }
    String name = doc["name"].as<String>();
    int state = doc["state"].as<int>();

    std::string key = name.c_str();
    auto it = client->actuators.find(key);
    if (it != client->actuators.end()) {
        if (it->second.isRegistered == false) return;
        if (actuatorCallback != nullptr) actuatorCallback(name, state);
    }
}