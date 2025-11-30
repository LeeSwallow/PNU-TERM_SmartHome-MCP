#include <ArduinoJSON.h>

#define MQTT_RX_PIN 16
#define MQTT_TX_PIN 17
#define MQTT_BAUDRATE 9600

boolean mqttConnected = false;
boolean isRegistered = false;

void registerActuator(String& name, int level);
void registerSensor(String& name, String& data_type);
void onRecievedAction(JsonDocument& doc);
void proccessCommand(JsonDocument& doc);
void sendActuatorState(String& name, int value);
void sendSensorState(String& name, String& state);


void setup() {
    Serial.begin(115200);
    Serial1.begin(MQTT_BAUDRATE, SERIAL_8N1, MQTT_RX_PIN, MQTT_TX_PIN);
}


void loop() {
    JsonDocument doc;
    proccessCommand(doc);

    if (mqttConnected) {
        // registerActuator("light1", 100);
        // registerSensor("temp1", "float");
        isRegistered = true;
    }

    if (isRegistered) {
        sendActuatorState("light1", 75);
        sendSensorState("temp1", "23.5");
    }
}

void registerActuator(String& name, int level) {
    JsonDocument doc;
    doc["command"] = "register";
    doc["type"] = "actuator";
    doc["name"] = name;
    doc["level"] = level;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
}

void registerSensor(String& name, String& data_type) {
    JsonDocument doc;
    doc["command"] = "register";
    doc["type"] = "sensor";
    doc["name"] = name;
    doc["data_type"] = data_type;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
}

void onRecievedAction(JsonDocument& doc) {
    String name = doc["name"].as<String>();
    int value = doc["value"];
    Serial.println("Action recieved for " + name + ": " + action);
    // Implement action handling logic here
}

void proccessCommand(JsonDocument& doc) {
    if (Serial1.available()) {
        doc.clear();
        deserializeJson(doc, Serial1);
        String type = doc["type"];
        if (type == "connection") {
            bool status = doc["status"].as<bool>();
            if (status) {
                mqttConnected = true;
                Serial.println("MQTT connected");
            } else {
                mqttConnected = false;
                Serial.println("MQTT failed to connect.. retrying");
            }
        }
        if (type == "info") {
            Serial.println("Info(mqtt client): " + doc["message"].as<String>());
        }
        if (type == "error") {
            Serial.println("Error(mqtt client): " + doc["message"].as<String>());
        }
        if (type == "actuator") {
            // proccess actuator
        }
    }
}

void sendActuatorState(String& name, int value) {
    JsonDocument doc;
    doc["command"] = "update";
    doc["type"] = "actuator";
    doc["name"] = name;
    doc["state"] = value;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
}

void sendSensorState(String& name, String& state) {
    JsonDocument doc;
    doc["command"] = "update";
    doc["type"] = "sensor";
    doc["name"] = name;
    doc["state"] = state;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
}