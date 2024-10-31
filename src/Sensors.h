#ifndef PT_SENSORS
#define PT_SENSORS

#include <Arduino.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
#include "Pins.h"
#include "Settings.h"

#define MEASUREMENTS 10

class Sensors {
public:
  int32_t smoothX;
  int32_t smoothY;
  int32_t smoothZ;
  bool accelerometerBuffered;

  Sensors(Settings &settingsIn)
    : settings(settingsIn) {
  }

  void Begin() {
    Serial.println("Starting up: sensors");
    pinMode(WING_SWITCH, INPUT_PULLUP);
#ifndef LEGACY
    pinMode(PID, INPUT);
#endif
#ifdef SDA
    Wire.setPins(SDA, SCL);
#endif
    accel = Adafruit_ADXL345_Unified();
    if (!accel.begin()) {
        Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    }
    wasOpen = WingsAreOpen();
  }

  bool WingsAreOpen() {
    return digitalRead(WING_SWITCH) == HIGH;
  }

  void UpdateSensors() {
    sensors_event_t event;
    accel.getEvent(&event);

    smoothX -= accelX[currentMeasurement];
    accelX[currentMeasurement] = accel.getX();
    smoothX += accelX[currentMeasurement];

    smoothY -= accelY[currentMeasurement];
    accelY[currentMeasurement] = accel.getY();
    smoothY += accelY[currentMeasurement];

    smoothZ -= accelZ[currentMeasurement];
    accelZ[currentMeasurement] = accel.getZ();
    smoothZ += accelZ[currentMeasurement];

    currentMeasurement++;
    if (currentMeasurement >= MEASUREMENTS) {
      accelerometerBuffered = true;
      currentMeasurement = 0;
    }


    // For some reason we need to cache this value, as checking it every loop
    // causes the webserver to freeze.
    // So we check every 500ms
    // https://github.com/me-no-dev/ESPAsyncWebServer/issues/944
    unsigned long curMillis = millis();
    if (curMillis > lastMotionCheckMillis + 500) {
#ifdef LEGACY
      isDetectingMotion = digitalRead(PID) == HIGH;
#else
      isDetectingMotion = analogRead(PID) > 0xFF;
#endif
    }
  }

  bool IsDetectingMotion() {
    return isDetectingMotion;
  }

private:
  Settings &settings;
  Adafruit_ADXL345_Unified accel;

  const float moveStartDuration = 1;
  const float moveEndDuration = 0.3;

  int currentMeasurement;
  int16_t accelX[MEASUREMENTS];
  int16_t accelY[MEASUREMENTS];
  int16_t accelZ[MEASUREMENTS];

  bool wingsOpen;
  bool wasOpen;

  bool isDetectingMotion;
  unsigned long lastMotionCheckMillis;
  float motionLerp = 0;
};
#endif