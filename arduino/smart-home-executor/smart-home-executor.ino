#include <ArduinoJson.h>
#include <DHT.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#define MQTT_BAUDRATE 9600

// actuator settings
#define ACTUATOR_COUNT 3
#define LIGHT_ACTUATOR_IDX 0
#define DOOR_ACTUATOR_IDX 1
#define AUTO_LIGHT_ACTUATOR_IDX 2

#define LIGHT_ACTUATOR_PIN 5
#define DOOR_ACTUATOR_PIN 6

#define NUMPIXELS 12

// sensor settings
#define SENSOR_COUNT 4
#define LIGHT_SENSOR_IDX 0
#define TEMPERATURE_SENSOR_IDX 1
#define HUMIDITY_SENSOR_IDX 2
#define HUMAN_DETECT_SENSOR_IDX 3

#define LIGHT_SENSOR_PIN A0
#define DHT11_PIN 7
#define HUMAN_DETECT_SENSOR_PIN 8

// timing settings
#define REGISTER_INTERVAL 100 // 0.1 seconds
#define REGISTER_RETRY_INTERVAL 5000
#define SENSOR_UPDATE_INTERVAL 2000
/*
==================================================================
    액추에이터 정의
==================================================================
*/

// actuator structure
typedef struct {
    String name;
    int level;
    int state;
    boolean isRegistered;
    unsigned long lastRegisterSentMillis;
    unsigned long lastStateSentMillis;
} Actuator;
Actuator actuators[ACTUATOR_COUNT];

// method declarations
void registerActuators();
void onRecievedAction(JsonDocument& doc);
void updateActuatorState(int index, int value);
void sendActuatorState(int index);

// NeoPixel object for light actuator
Adafruit_NeoPixel pixels(NUMPIXELS, LIGHT_ACTUATOR_PIN, NEO_GRB + NEO_KHZ800);
// servo object for door actuator
Servo doorServo;

/*
==================================================================
    센서 정의
==================================================================
*/

// sensor structure
typedef struct {
    String name;
    String data_type;
    String state;
    boolean isRegistered;
    unsigned long lastRegisterSentMillis;
    unsigned long lastStateSentMillis;
} Sensor;
Sensor sensors[SENSOR_COUNT];

// method declarations
void registerSensors();
void updateSensorState(int index, String& value);
void sendSensorState(int index);

// DHT11 sensor object
DHT dht(DHT11_PIN, DHT11);

/*==================================================================
    기반 변수 및 함수
====================================================================
*/

boolean mqttConnected = false;

unsigned long globalRegisterLastMillis = 0;
String proccessingCommandBuffer = "";
boolean isProccessing;

void proccessCommand(JsonDocument& doc);

/*==================================================================
    셋업 함수
====================================================================
*/

void setup() {
    Serial.begin(9600);
    Serial1.begin(MQTT_BAUDRATE, SERIAL_8N1);
    
    
    actuators[LIGHT_ACTUATOR_IDX].name = "light";
    actuators[LIGHT_ACTUATOR_IDX].level = 256; // 0-255
    actuators[LIGHT_ACTUATOR_IDX].state = 0;
    actuators[LIGHT_ACTUATOR_IDX].isRegistered = false;
    actuators[LIGHT_ACTUATOR_IDX].lastRegisterSentMillis = 0;
    actuators[LIGHT_ACTUATOR_IDX].lastStateSentMillis = 0;
  
    pixels.begin();
    for (int i=0; i<NUMPIXELS; ++i) pixels.setPixelColor(i, pixels.Color(0,0,0));
    pixels.show();

    actuators[DOOR_ACTUATOR_IDX].name = "door";
    actuators[DOOR_ACTUATOR_IDX].level = 181; // 0-180
    actuators[DOOR_ACTUATOR_IDX].state = 0;
    actuators[DOOR_ACTUATOR_IDX].isRegistered = false;
    actuators[DOOR_ACTUATOR_IDX].lastRegisterSentMillis = 0;
    actuators[DOOR_ACTUATOR_IDX].lastStateSentMillis = 0;
    doorServo.attach(DOOR_ACTUATOR_PIN);
    doorServo.write(actuators[DOOR_ACTUATOR_IDX].state);


    actuators[AUTO_LIGHT_ACTUATOR_IDX].name = "auto_light";
    actuators[AUTO_LIGHT_ACTUATOR_IDX].level = 2; // on/off
    actuators[AUTO_LIGHT_ACTUATOR_IDX].state = 0; // default off
    actuators[AUTO_LIGHT_ACTUATOR_IDX].isRegistered = false;
    actuators[AUTO_LIGHT_ACTUATOR_IDX].lastRegisterSentMillis = 0;
    actuators[AUTO_LIGHT_ACTUATOR_IDX].lastStateSentMillis = 0;

    sensors[LIGHT_SENSOR_IDX].name = "light_sensor";
    sensors[LIGHT_SENSOR_IDX].data_type = "integer";
    sensors[LIGHT_SENSOR_IDX].state = "0";
    sensors[LIGHT_SENSOR_IDX].isRegistered = false;
    sensors[LIGHT_SENSOR_IDX].lastRegisterSentMillis = 0;
    sensors[LIGHT_SENSOR_IDX].lastStateSentMillis = 0;

    dht.begin();
    sensors[TEMPERATURE_SENSOR_IDX].name = "temperature_sensor";
    sensors[TEMPERATURE_SENSOR_IDX].data_type = "float";
    sensors[TEMPERATURE_SENSOR_IDX].state = "0.0";
    sensors[TEMPERATURE_SENSOR_IDX].isRegistered = false;
    sensors[TEMPERATURE_SENSOR_IDX].lastRegisterSentMillis = 0;
    sensors[TEMPERATURE_SENSOR_IDX].lastStateSentMillis = 0;

    sensors[HUMIDITY_SENSOR_IDX].name = "humidity_sensor";
    sensors[HUMIDITY_SENSOR_IDX].data_type = "float";
    sensors[HUMIDITY_SENSOR_IDX].state = "0.0";
    sensors[HUMIDITY_SENSOR_IDX].isRegistered = false;
    sensors[HUMIDITY_SENSOR_IDX].lastRegisterSentMillis = 0;
    sensors[HUMIDITY_SENSOR_IDX].lastStateSentMillis = 0;

    sensors[HUMAN_DETECT_SENSOR_IDX].name = "human_detect_sensor";
    sensors[HUMAN_DETECT_SENSOR_IDX].data_type = "boolean";
    sensors[HUMAN_DETECT_SENSOR_IDX].state = "false";
    sensors[HUMAN_DETECT_SENSOR_IDX].isRegistered = false;
    sensors[HUMAN_DETECT_SENSOR_IDX].lastRegisterSentMillis = 0;
    sensors[HUMAN_DETECT_SENSOR_IDX].lastStateSentMillis = 0;

    globalRegisterLastMillis = millis();
}

/*==================================================================
    메인 루프
====================================================================
*/

void loop() {
    JsonDocument doc;
    proccessCommand(doc);
    if (mqttConnected) {
        // 등록되지 않은 액추에이터 및 센서 등록 시도
        registerActuators();
        registerSensors();

        for (int i = 0; i < SENSOR_COUNT; i++) {
            unsigned long currentMillis = millis();
            if (currentMillis - sensors[i].lastStateSentMillis >= SENSOR_UPDATE_INTERVAL) {
                switch(i) {
                    case LIGHT_SENSOR_IDX: {
                        int lightValue = analogRead(LIGHT_SENSOR_PIN);
                        sensors[i].state = String(lightValue);
                        break;
                    }
                    case TEMPERATURE_SENSOR_IDX: {
                        float temperature = dht.readTemperature();
                        sensors[i].state = String(temperature);
                        break;
                    }
                    case HUMIDITY_SENSOR_IDX: {
                        float humidity = dht.readHumidity();
                        sensors[i].state = String(humidity);
                        break;
                    }
                    case HUMAN_DETECT_SENSOR_IDX: {
                        int humanDetectValue = digitalRead(HUMAN_DETECT_SENSOR_PIN);
                        sensors[i].state = humanDetectValue == HIGH ? "true" : "false";
                        break;
                    }
                }
                sendSensorState(i);
            }
        }
        unsigned long currentMillis = millis();
        // 인체 감지 시 자동 조명 제어
        if (actuators[LIGHT_ACTUATOR_IDX].isRegistered && 
            actuators[AUTO_LIGHT_ACTUATOR_IDX].isRegistered && 
            sensors[HUMAN_DETECT_SENSOR_IDX].isRegistered &&
            actuators[AUTO_LIGHT_ACTUATOR_IDX].state > 0 &&
            currentMillis - actuators[LIGHT_ACTUATOR_IDX].lastStateSentMillis  >= SENSOR_UPDATE_INTERVAL
        ) {
            if (sensors[HUMAN_DETECT_SENSOR_IDX].state == "true") {
                updateActuatorState(LIGHT_ACTUATOR_IDX, 255);
            } else {
                updateActuatorState(LIGHT_ACTUATOR_IDX, 0);
            }
        }
    } else {
        unsigned long currentMillis = millis();
        if (currentMillis - globalRegisterLastMillis >= REGISTER_INTERVAL) {
            doc["command"] = "connection";
            serializeJsonPretty(doc, Serial1);
            Serial1.println();
            globalRegisterLastMillis = currentMillis;
        }
    }
}

/*==================================================================
    함수 구현 
====================================================================
*/


void registerActuators() {
    unsigned long currentMillis = millis();

    if (currentMillis - globalRegisterLastMillis < REGISTER_INTERVAL) {
        return; // 등록 간격이 지나지 않음
    }
    globalRegisterLastMillis = currentMillis;

    for (int i = 0; i < ACTUATOR_COUNT; i++) {
        if (!actuators[i].isRegistered && (currentMillis - actuators[i].lastRegisterSentMillis >= REGISTER_RETRY_INTERVAL)) {
            JsonDocument doc;
            doc["command"] = "register";
            doc["type"] = "actuator";
            doc["name"] = actuators[i].name;
            doc["level"] = actuators[i].level;
            serializeJsonPretty(doc, Serial1);
            Serial1.println();
            Serial.println("Register Sent(actuator): " +  actuators[i].name);
            actuators[i].lastRegisterSentMillis = currentMillis;
            return; // 한 번에 하나씩만 등록 시도
        }
    }
}

void registerSensors() {
    unsigned long currentMillis = millis();
    if (currentMillis - globalRegisterLastMillis < REGISTER_INTERVAL) {
        return; // 등록 간격이 지나지 않음
    }

    globalRegisterLastMillis = currentMillis;
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensors[i].isRegistered && (currentMillis - sensors[i].lastRegisterSentMillis >= REGISTER_RETRY_INTERVAL)) {
            JsonDocument doc;
            doc["command"] = "register";
            doc["type"] = "sensor";
            doc["name"] = sensors[i].name;
            doc["data_type"] = sensors[i].data_type;
            serializeJsonPretty(doc, Serial1);
            Serial1.println();
            Serial.println("Register Sent(sensor): " +  sensors[i].name);
            sensors[i].lastRegisterSentMillis = currentMillis;
            return; // 한 번에 하나씩만 등록 시도
        }
    }
}

void onRecievedAction(JsonDocument& doc) {
    String name = doc["name"].as<String>();
    int value = doc["value"].as<int>();
    Serial.println("Action received for " + name + ": " + String(value));

    int recievedIndex = -1;
    for (int i = 0; i < ACTUATOR_COUNT; i++) {
        if (actuators[i].name == name) {
            recievedIndex = i;
            break;
        }
    }
    if (recievedIndex == -1 || recievedIndex >= ACTUATOR_COUNT) {
        Serial.println("Recieved action for invalid actuator: " + name);
        return;
    }
    updateActuatorState(recievedIndex, value);
}

void proccessCommand(JsonDocument& doc) {
    if (Serial1.available()) {
        char ch = Serial1.read();
        Serial.print(ch);
        if (ch == '\n' || ch == '\r') {
            return; // 줄바꿈 문자 무시
        }
        proccessingCommandBuffer += ch;
        if (ch == '{') {
            isProccessing = true;
        }
        if (ch == '}') {
            isProccessing = false;
            Serial.println();
        }
        if (isProccessing) {
            return;
        }

        JsonDocument recieved_doc;
        String command = proccessingCommandBuffer;
        proccessingCommandBuffer = "";
        DeserializationError error = deserializeJson(recieved_doc, command);
        if (error) {
            Serial.println("Failed to parse JSON: " + String(error.c_str()));
            return;
        }
        Serial.println("command Recieved : " + command);
        String type = recieved_doc["type"].as<String>();
        if (type == "connection") {
            bool status = recieved_doc["status"].as<bool>();
            if (status) {
                mqttConnected = true;
                Serial.println("MQTT connected");
            } else {
                mqttConnected = false;
                Serial.println("MQTT failed to connect.. retrying");
            }
        }
        else if (type == "info") {
            Serial.println("Info(mqtt client): " + recieved_doc["message"].as<String>());
        }
        else if (type == "error") {
            Serial.println("Error(mqtt client): " + recieved_doc["message"].as<String>());
        }
        else if (type == "register") {
            // 등록 완료 알림 처리
            String name = recieved_doc["name"].as<String>();
            String entity = recieved_doc["entity"].as<String>();
            
            // 로컬 배열에서 해당 모듈 찾아서 isRegistered 업데이트
            if (entity == "actuator") {
                for (int i = 0; i < ACTUATOR_COUNT; i++) {
                    if (actuators[i].name == name) {
                        actuators[i].isRegistered = true;
                        sendActuatorState(i);
                        Serial.println(name + "(actuator) Registered!");
                        break;
                    }
                }
            } else if (entity == "sensor") {
                for (int i = 0; i < SENSOR_COUNT; i++) {
                    if (sensors[i].name == name) {
                        sensors[i].isRegistered = true;
                        Serial.println(name + "(sensor) Registered!");
                        sendSensorState(i);
                        break;
                    }
                }
            } else {
                Serial.println("invalid entity type! :" + entity);
            }
            Serial.println("Registered: " + name);
        }
        else if (type == "actuator") {
            onRecievedAction(recieved_doc);
        }
    }
}

void sendActuatorState(int index) {
    if (!actuators[index].isRegistered) return;

    JsonDocument doc;
    doc["command"] = "update";
    doc["type"] = "actuator";
    doc["name"] = actuators[index].name;
    doc["state"] = actuators[index].state;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
    actuators[index].lastStateSentMillis = millis();
    Serial.println("Acutator: " + actuators[index].name + ", State: " + String(actuators[index].state));
}

void sendSensorState(int index) {
    if (!actuators[index].isRegistered) return;
    
    JsonDocument doc;
    doc["command"] = "update";
    doc["type"] = "sensor";
    doc["name"] = sensors[index].name;
    doc["state"] = sensors[index].state;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
    sensors[index].lastStateSentMillis = millis();
    Serial.println("Sensor: " + sensors[index].name + ", State: " + sensors[index].state);
}

void updateActuatorState(int index, int value) {
    if (!actuators[index].isRegistered) {
        Serial.println("Actuator not registered: " + actuators[index].name);
        return;
    }
    if (index == LIGHT_ACTUATOR_IDX) {
        actuators[index].state = value;
        for (int i=0; i<NUMPIXELS; ++i) pixels.setPixelColor(i, pixels.Color(value, value, value));
        pixels.show();
        sendActuatorState(index);
        actuators[AUTO_LIGHT_ACTUATOR_IDX].state = 0; // 자동 조명 끄기
        sendActuatorState(AUTO_LIGHT_ACTUATOR_IDX);
    } else if (index == DOOR_ACTUATOR_IDX) {
        actuators[index].state = value;
        doorServo.write(value);
        sendActuatorState(index);
    } else if (index == AUTO_LIGHT_ACTUATOR_IDX) {
        actuators[index].state = value;
        sendActuatorState(index);
    }
}