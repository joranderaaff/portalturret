#ifndef PT_SETTINGS
#define PT_SETTINGS

#include "Arduino.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

class Settings {
public:
  String wifiSSID;
  String wifiPassword;
  uint8_t audioVolume = 10;
  int startInManualMode = 0;
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

  Settings() {}
  void Begin() {
    if (!LittleFS.begin()) {
      while (true) {
      }
      return;
    }

    // static Settings settings;
    Serial.println("Settings not yet loaded, doing that now.");

    File file = LittleFS.open(settingsFilePath, "r");
    if (!file) {
      Serial.println("Failed to open settings file for reading");
      return;  // Return default settings
    }

    // Create a JSON document to read the settings
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      Serial.println("Failed to parse settings file");
      return;  // Return default settings
    }

    // Assign values from JSON to settings structure
    if (doc.containsKey("wifiSSID"))
      wifiSSID = doc["wifiSSID"].as<String>();
    if (doc.containsKey("wifiPassword"))
      wifiPassword = doc["wifiPassword"].as<String>();
    if (doc.containsKey("audioVolume"))
      audioVolume = doc["audioVolume"].as<int>();
    if (doc.containsKey("centerAngle"))
      centerAngle = doc["centerAngle"].as<int>();
    if (doc.containsKey("idleAngle"))
      idleAngle = doc["idleAngle"].as<int>();
    if (doc.containsKey("wingRotateDirection"))
      wingRotateDirection = doc["wingRotateDirection"].as<int>();
    if (doc.containsKey("wingPin"))
      wingPin = doc["wingPin"].as<int>();
    if (doc.containsKey("rotatePin"))
      rotatePin = doc["rotatePin"].as<int>();
    if (doc.containsKey("openDuration"))
      openDuration = doc["openDuration"].as<int>();
    if (doc.containsKey("maxRotation"))
      maxRotation = doc["maxRotation"].as<int>();
    if (doc.containsKey("panicTreshold"))
      panicTreshold = doc["panicTreshold"].as<float>();
    if (doc.containsKey("restTreshold"))
      restTreshold = doc["restTreshold"].as<float>();
    if (doc.containsKey("tippedOverTreshold"))
      tippedOverTreshold = doc["tippedOverTreshold"].as<float>();
    if (doc.containsKey("startInManualMode"))
      startInManualMode = doc["startInManualMode"].as<int>();

    file.close();
  }

  // Function to save settings to the filesystem
  void SaveSettings() {
    Serial.println("Saving settings");
    File file = LittleFS.open(settingsFilePath, "w");
    if (!file) {
      Serial.println("Failed to open settings file for writing");
      return;
    }

    // Create a JSON document to store the settings
    StaticJsonDocument<256> doc;
    doc["wifiSSID"] = wifiSSID;
    doc["wifiPassword"] = wifiPassword;
    doc["audioVolume"] = audioVolume;
    doc["centerAngle"] = centerAngle;
    doc["idleAngle"] = idleAngle;
    doc["wingRotateDirection"] = wingRotateDirection;
    doc["wingPin"] = wingPin;
    doc["rotatePin"] = rotatePin;
    doc["openDuration"] = openDuration;
    doc["maxRotation"] = maxRotation;
    doc["panicTreshold"] = panicTreshold;
    doc["restTreshold"] = restTreshold;
    doc["tippedOverTreshold"] = tippedOverTreshold;
    doc["startInManualMode"] = startInManualMode;

    Serial.println("Updated JSON");

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
      Serial.println("Failed to write to settings file");
    }

    Serial.println("Saved settings to file");

    file.close();
  }

  String SettingsToJSON() {
    String json = "{";
    json += "\"audioVolume\":" + String(audioVolume) + ",";
    json += "\"centerAngle\":" + String(centerAngle) + ",";
    json += "\"idleAngle\":" + String(idleAngle) + ",";
    json += "\"wingRotateDirection\":" + String(wingRotateDirection) + ",";
    json += "\"wingPin\":" + String(wingPin) + ",";
    json += "\"rotatePin\":" + String(rotatePin) + ",";
    json += "\"openDuration\":" + String(openDuration) + ",";
    json += "\"maxRotation\":" + String(maxRotation) + ",";
    json += "\"panicTreshold\":" + String(panicTreshold) + ",";
    json += "\"restTreshold\":" + String(restTreshold) + ",";
    json += "\"startInManualMode\":" + String(startInManualMode) + ",";
    json += "\"tippedOverTreshold\":" + String(tippedOverTreshold);
    json += "}";
    return json;
  }

private:
  // Define the file path for settings
  const char *settingsFilePath = "settings.json";
};
#endif
