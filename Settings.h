#include <LittleFS.h>
#include <ArduinoJson.h>

// Define the file path for settings
const char* settingsFilePath = "settings.json";

// Structure to hold settings
struct Settings {
  String wifiSSID;
  String wifiPassword;
  uint8_t audioVolume = 10;
  int openDuration = 1000;
  int maxRotation = 50;
  int centerAngle = 90;
  int idleAngle = 90;
  int wingRotateDirection = 1;
  int wingPin = 12;
  int rotatePin = 13;
  float panicTreshold = 3;
  float restTreshold = 1;
  float tippedOverTreshold = 5;
};

String settingsToJSON(const Settings& settingsIn) {
  String json = "{";
  json += "\"audioVolume\":" + String(settingsIn.audioVolume) + ",";
  json += "\"centerAngle\":" + String(settingsIn.centerAngle) + ",";
  json += "\"idleAngle\":" + String(settingsIn.idleAngle) + ",";
  json += "\"wingRotateDirection\":" + String(settingsIn.wingRotateDirection) + ",";
  json += "\"wingPin\":" + String(settingsIn.wingPin) + ",";
  json += "\"rotatePin\":" + String(settingsIn.rotatePin) + ",";
  json += "\"openDuration\":" + String(settingsIn.openDuration) + ",";
  json += "\"maxRotation\":" + String(settingsIn.maxRotation) + ",";
  json += "\"panicTreshold\":" + String(settingsIn.panicTreshold) + ",";
  json += "\"restTreshold\":" + String(settingsIn.restTreshold) + ",";
  json += "\"tippedOverTreshold\":" + String(settingsIn.tippedOverTreshold);
  json += "}";
  return json;
}

// Function to save settings to the filesystem
void saveSettings(const Settings& settingsIn) {
  Serial.println("Saving settings");
  File file = LittleFS.open(settingsFilePath, "w");
  if (!file) {
    Serial.println("Failed to open settings file for writing");
    return;
  }

  // Create a JSON document to store the settings
  StaticJsonDocument<256> doc;
  doc["wifiSSID"] = settingsIn.wifiSSID;
  doc["wifiPassword"] = settingsIn.wifiPassword;
  doc["audioVolume"] = settingsIn.audioVolume;
  doc["centerAngle"] = settingsIn.centerAngle;
  doc["idleAngle"] = settingsIn.idleAngle;
  doc["wingRotateDirection"] = settingsIn.wingRotateDirection;
  doc["wingPin"] = settingsIn.wingPin;
  doc["rotatePin"] = settingsIn.rotatePin;
  doc["openDuration"] = settingsIn.openDuration;
  doc["maxRotation"] = settingsIn.maxRotation;
  doc["panicTreshold"] = settingsIn.panicTreshold;
  doc["restTreshold"] = settingsIn.restTreshold;
  doc["tippedOverTreshold"] = settingsIn.tippedOverTreshold;

  Serial.println("Updated JSON");

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to settings file");
  }

  Serial.println("Saved settings to file");

  file.close();
}

// Function to load settings from the filesystem
Settings loadSettings() {
  Settings settings;

  File file = LittleFS.open(settingsFilePath, "r");
  if (!file) {
    Serial.println("Failed to open settings file for reading");
    return settings;  // Return default settings
  }

  // Create a JSON document to read the settings
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse settings file");
    return settings;  // Return default settings
  }

  // Assign values from JSON to settings structure
  if (doc.containsKey("wifiSSID")) settings.wifiSSID = doc["wifiSSID"].as<String>();
  if (doc.containsKey("wifiPassword")) settings.wifiPassword = doc["wifiPassword"].as<String>();
  if (doc.containsKey("audioVolume")) settings.audioVolume = doc["audioVolume"].as<int>();
  if (doc.containsKey("centerAngle")) settings.centerAngle = doc["centerAngle"].as<int>();
  if (doc.containsKey("idleAngle")) settings.idleAngle = doc["idleAngle"].as<int>();
  if (doc.containsKey("wingRotateDirection")) settings.wingRotateDirection = doc["wingRotateDirection"].as<int>();
  if (doc.containsKey("wingPin")) settings.wingPin = doc["wingPin"].as<int>();
  if (doc.containsKey("rotatePin")) settings.rotatePin = doc["rotatePin"].as<int>();
  if (doc.containsKey("openDuration")) settings.openDuration = doc["openDuration"].as<int>();
  if (doc.containsKey("maxRotation")) settings.maxRotation = doc["maxRotation"].as<int>();
  if (doc.containsKey("panicTreshold")) settings.panicTreshold = doc["panicTreshold"].as<float>();
  if (doc.containsKey("restTreshold")) settings.restTreshold = doc["restTreshold"].as<float>();
  if (doc.containsKey("tippedOverTreshold")) settings.tippedOverTreshold = doc["tippedOverTreshold"].as<float>();

  file.close();
  return settings;
}