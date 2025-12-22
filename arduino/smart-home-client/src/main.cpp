#include <Arduino.h>
#include <ArduinoJson.h>
#include <WifiManager.h>
#include <WiFi.h>
#include <SmartHome.h>

#define SERIAL2_RX_PIN 16
#define SERIAL2_TX_PIN 17

String DEVICE_ID = "SMART-HOME-0001"; // Unique device ID
int TIMEOUT_PORTAL = 180; //seconds
unsigned long previousMillis = 0;

// AP credentials
WiFiManager wifiManager;
WiFiClient espClient;
SmartHomeClient* smartHomeClient = nullptr;

// MQTT Broker settings
String mqttServer;
int mqttPort;
String mqttUsername;
String mqttPassword;

void setup_WiFi();
void read_json_from_executor();
void log_callback(const LogType& log);
void register_callback(const String& name);
void actuator_callback(const String& actuator_name, int value);
void setup_smart_home();
void processSerialCommands();
void prㅍ  occessOnConnectQuery();
void processOnRegister(const JsonDocument& doc);
void processOnUpdate(const JsonDocument& doc);
void sendSerialString(const String& message);
void sendSerialJson(const JsonDocument& doc);

void setup() {
  Serial.begin(115200); // for debugging
  Serial2.begin(9600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN); // for communication with executor
  Serial2.setRxBufferSize(1024);
  Serial2.setTimeout(200); // 200 milliseconds timeout for reading
  setup_WiFi();
  setup_smart_home();
  previousMillis = millis();
}

void loop() {
  processSerialCommands();
  if (smartHomeClient != nullptr) {
    smartHomeClient->loop();
  }
}

void setup_WiFi() {
  // 커스텀 파라미터 추가
  JsonDocument doc;
  char mqttServerBuf[64] = "swallow104.gonetis.com";
  char mqttPortBuf[6] = "11883";
  char mqttUserBuf[32] = "test_user";
  char mqttPassBuf[32] = "testpass1212";

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqttServerBuf, 64);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqttPortBuf, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", mqttUserBuf, 32);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqttPassBuf, 32); 

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);

  wifiManager.setTimeout(TIMEOUT_PORTAL);

  if (!wifiManager.autoConnect(DEVICE_ID.c_str())) {
    delay(3000);
    Serial.println("Failed to connect and hit timeout");
    ESP.restart();
  }
  mqttServer = String(custom_mqtt_server.getValue());
  mqttPort = String(custom_mqtt_port.getValue()).toInt();
  mqttUsername = String(custom_mqtt_user.getValue());
  mqttPassword = String(custom_mqtt_pass.getValue());
  Serial.println("Connected to WiFi!");
}

void log_callback(const LogType& log) {
  JsonDocument doc;
  Serial.println("Log( " + log.type + " ): " + log.message);
  sendSerialString(log.getLogMessage());
}

void register_callback(const String& name, const String& entity) {
  JsonDocument doc;
  doc["type"] = "register";
  doc["entity"] = entity;
  doc["name"] = name;
  sendSerialJson(doc);
  log_callback(LogType("info", "Register Callback: " + entity + " -> " + name));
}

void actuator_callback(const String& actuator_name, int value) {
  JsonDocument doc;
  doc["type"] = "actuator";
  doc["name"] = actuator_name;
  doc["value"] = value;
  sendSerialJson(doc);
  log_callback(LogType("info", "Actuator Callback: " + actuator_name + " -> " + String(value)));
}

void setup_smart_home() {
  smartHomeClient = new SmartHomeClient(espClient, DEVICE_ID.c_str());
  if (smartHomeClient == nullptr) {
      Serial.println("ERROR: Failed to create SmartHomeClient");
      ESP.restart();
      return;
  }
  smartHomeClient->setLogCallback(log_callback);
  smartHomeClient->setRegisterCallback(register_callback);
  smartHomeClient->setActuatorCallback(actuator_callback);
  smartHomeClient->setupMQTT(mqttServer.c_str(), mqttPort, mqttUsername.c_str(), mqttPassword.c_str());
}

void processSerialCommands() {
  if (Serial2.available()) {
      String command = Serial2.readStringUntil('\n');
      command.trim();
      if (command.length() == 0) {
          return;
      }
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, command);
      if (error) {
          log_callback(LogType("error", "Failed to parse command JSON"));
          Serial.println("Failed to parse command JSON: " + command);
          return;
      }
      String commandType = doc["command"].as<String>();
      if (commandType == "connection") {
          proccessOnConnectQuery();
      }
      else if (commandType == "register") {
          processOnRegister(doc);
      } else if (commandType == "update") {
          processOnUpdate(doc);
      }  else {
          log_callback(LogType("error", "Unknown command: " + commandType));
      } 
  }
}

void proccessOnConnectQuery() {
  JsonDocument doc;
  // if not connected to MQTT broker
  if (smartHomeClient == nullptr) {
      doc["type"] = "connection";
      doc["status"] = false;
      sendSerialJson(doc);
      return;
  }
  // connected to MQTT broker
  doc["type"] = "connection";
  doc["status"] = true;
  doc["ip"] = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["mqtt_server"] = mqttServer;
  doc["mqtt_port"] = mqttPort;
  doc["mqtt_username"] = mqttUsername;
  sendSerialJson(doc);
}

void processOnRegister(const JsonDocument& doc) {
  if (smartHomeClient == nullptr) {
      log_callback(LogType("error", "SmartHomeClient not initialized"));
      return;
  }
  String entityType = doc["type"].as<String>();
  String name = doc["name"].as<String>();

  // actuator Registration
  if (entityType == "actuator") {
      int level = doc["level"].as<int>();
      if (level == 0) {
          log_callback(LogType("error", "Missing level for actuator: " + name));
          return;
      }
      Serial.println("Registered actuator: " + name + " with level " + String(level));
      smartHomeClient->addActuator(name, level);
  // sensor Registration
  } else if (entityType == "sensor") {
      String dataType = doc["data_type"].as<String>();
      if (dataType == "") {
          log_callback(LogType("error", "Missing data_type for sensor: " + name));
          return;
      } else if (dataType != "boolean" && dataType != "integer" && dataType != "float" && dataType != "string") {
          log_callback(LogType("error", "Unsupported data_type for sensor: " + name + " (" + dataType + ")"));
          return;
      }
      smartHomeClient->addSensor(name, dataType);
      Serial.println("Registered sensor: " + name + " with data type " + dataType);
  // unknown entity type
  } else {
      log_callback(LogType("error", "Unknown entity type: " + entityType));
  }
}

void processOnUpdate(const JsonDocument& doc) {
  if (smartHomeClient == nullptr) {
      log_callback(LogType("error", "SmartHomeClient not initialized"));
      return;
  }
  String type = doc["type"].as<String>();
  String name = doc["name"].as<String>();

  // sensor state update
  if (type == "sensor") {
      log_callback(LogType("info", "Processing sensor update for: " + name));
      String state = doc["state"].as<String>();
      smartHomeClient->publishSensorState(name, state);

  // actuator state update
  } else if (type == "actuator") {
      log_callback(LogType("info", "Processing actuator update for: " + name));
      int state = doc["state"].as<int>();
      smartHomeClient->publishActuatorState(name, state);

  // unknown update type
  } else {
      log_callback(LogType("error", "Unknown update type: " + type));
  }
}

void sendSerialString(const String& message) {
  Serial2.println('\n' + message + '\n');
}

void sendSerialJson(const JsonDocument& doc) {
  Serial2.println();
  serializeJson(doc, Serial2);
  Serial2.println('\n');
}
