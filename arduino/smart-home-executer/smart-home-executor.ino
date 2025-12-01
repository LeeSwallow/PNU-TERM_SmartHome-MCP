#include <ArduinoJSON.h>

#define MQTT_RX_PIN 16
#define MQTT_TX_PIN 17
#define MQTT_BAUDRATE 9600
#define MAX_ACTUATORS 10
#define MAX_SENSORS 10

// 센서 인덱스 정의 (sensors 배열에서 사용)
#define LIGHT_SENSOR_IDX 0
#define TEMPERATURE_SENSOR_IDX 1
#define HUMIDITY_SENSOR_IDX 2


// 액추에이터 인덱스 정의 (actuators 배열에서 사용)
#define LIGHT_ACTUATOR_IDX 0
#define DOOR_ACTUATOR_IDX 1


typedef struct {
    String name;
    int level;
    int state;
    boolean isRegistered;
    unsigned int lastRegisterSentMillis;
    unsigned int lastStateSentMillis;
} Actuator;

int actuatorIndex = 0;

typedef struct {
    String name;
    String data_type;
    String state;
    boolean isRegistered;
    unsigned int lastRegisterSentMillis;
    unsigned int lastStateSentMillis;
} Sensor;

int sensorIndex = 0;

Actuator actuators[MAX_ACTUATORS];
Sensor sensors[MAX_SENSORS];

boolean mqttConnected = false;



void registerActuator(String& name, int level);
void registerSensor(String& name, String& data_type);
void onRecievedAction(JsonDocument& doc);
void proccessCommand(JsonDocument& doc);
void sendActuatorState(int index);
void sendSensorState(int index);
void updateActuatorState(int index, int value);


void setup() {
    Serial.begin(115200);
    Serial1.begin(MQTT_BAUDRATE, SERIAL_8N1, MQTT_RX_PIN, MQTT_TX_PIN);
}


void loop() {
    JsonDocument doc;
    proccessCommand(doc);
    if (mqttConnected) {

    }
}

void registerActuator(String& name, int level) {
    // 인덱스 기반으로 배열에 저장 (순차적으로 저장)
    if (actuatorIndex < MAX_ACTUATORS) {
        actuators[actuatorIndex].name = name;
        actuators[actuatorIndex].level = level;
        actuators[actuatorIndex].state = 0;
        actuators[actuatorIndex].isRegistered = false;
        actuators[actuatorIndex].lastRegisterSentMillis = 0;
        actuators[actuatorIndex].lastStateSentMillis = 0;
        actuatorIndex++;
    }
    // Client로 등록 요청 전송
    JsonDocument doc;
    doc["command"] = "register";
    doc["type"] = "actuator";
    doc["name"] = name;
    doc["level"] = level;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
    if (actuatorIndex > 0) {
        actuators[actuatorIndex-1].lastRegisterSentMillis = millis();
    }
}

void registerSensor(String& name, String& data_type) {
    // 인덱스 기반으로 배열에 저장 (순차적으로 저장)
    if (sensorIndex < MAX_SENSORS) {
        sensors[sensorIndex].name = name;
        sensors[sensorIndex].data_type = data_type;
        sensors[sensorIndex].state = "";
        sensors[sensorIndex].isRegistered = false;
        sensors[sensorIndex].lastRegisterSentMillis = 0;
        sensors[sensorIndex].lastStateSentMillis = 0;
        sensorIndex++;
    }
    // Client로 등록 요청 전송
    JsonDocument doc;
    doc["command"] = "register";
    doc["type"] = "sensor";
    doc["name"] = name;
    doc["data_type"] = data_type;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
    if (sensorIndex > 0) {
        sensors[sensorIndex-1].lastRegisterSentMillis = millis();
    }
}

void onRecievedAction(JsonDocument& doc) {
    String name = doc["name"].as<String>();
    int value = doc["value"].as<int>();
    Serial.println("Action received for " + name + ": " + String(value));

    int recievedIndex = -1;
    for (int i = 0; i < MAX_ACTUATORS; i++) {
        if (actuators[i].name == name) {
            recievedIndex = i;
            break;
        }
    }
    if (recievedIndex == -1 || recievedIndex >= MAX_ACTUATORS) {
        Serial.println("Recieved action for invalid actuator: " + name);
        return;
    }
    updateActuatorState(recievedIndex, value);
}

void proccessCommand(JsonDocument& doc) {
    if (Serial1.available()) {
        String command = Serial1.readStringUntil('\n');
        doc.clear();
        DeserializationError error = deserializeJson(doc, command);
        if (error) {
            Serial.println("Failed to parse JSON: " + String(error.c_str()));
            return;
        }
        
        String type = doc["type"].as<String>();
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
        else if (type == "info") {
            Serial.println("Info(mqtt client): " + doc["message"].as<String>());
        }
        else if (type == "error") {
            Serial.println("Error(mqtt client): " + doc["message"].as<String>());
        }
        else if (type == "register") {
            // 등록 완료 알림 처리
            String name = doc["name"].as<String>();
            Serial.println("Registered: " + name);
            
            // 로컬 배열에서 해당 모듈 찾아서 isRegistered 업데이트
            for (int i = 0; i < MAX_ACTUATORS; i++) {
                if (actuators[i].name == name) {
                    actuators[i].isRegistered = true;
                    break;
                }
            }
            for (int i = 0; i < MAX_SENSORS; i++) {
                if (sensors[i].name == name) {
                    sensors[i].isRegistered = true;
                    break;
                }
            }
        }
        else if (type == "actuator") {
            onRecievedAction(doc);
        }
    }
}

void sendActuatorState(int index) {
    JsonDocument doc;
    doc["command"] = "update";
    doc["type"] = "actuator";
    doc["name"] = actuators[index].name;
    doc["state"] = actuators[index].state;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
    actuators[index].lastStateSentMillis = millis();
}

void sendSensorState(int index) {
    JsonDocument doc;
    doc["command"] = "update";
    doc["type"] = "sensor";
    doc["name"] = sensors[index].name;
    doc["state"] = sensors[index].state;
    serializeJsonPretty(doc, Serial1);
    Serial1.println();
    sensors[index].lastStateSentMillis = millis();
}

void updateActuatorState(int index, int value) {
    /*
    ===================================================================
    여기에 액추에이터 상태 업데이트 로직을 추가합니다.
    ===================================================================
    예시:
    - LED 제어: digitalWrite(LED_PIN, value > 0 ? HIGH : LOW);
    - PWM 제어: analogWrite(PWM_PIN, value);
    - 모터 제어: motor.setSpeed(value);
    
    필요에 따라 updatedValue를 수정하세요.
    ===================================================================
    */
}