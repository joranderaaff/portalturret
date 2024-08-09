#ifndef PT_SENSORS
#define PT_SENSORS

#include "Arduino.h"
#include "Settings.h"
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>

#define WING_SWITCH D5
#define PID A0
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
    accel = Adafruit_ADXL345_Unified();
    accel.begin();
    wasOpen = WingsAreOpen();
    pinMode(WING_SWITCH, INPUT_PULLUP);
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
    // So we check every 100ms
    // https://github.com/me-no-dev/ESPAsyncWebServer/issues/944
    unsigned long curMillis = millis();
    if (curMillis > lastMotionCheckMillis + 100) {
      float deltaTime = (curMillis - lastMotionCheckMillis) / 1000.0;
      bool pirActive = analogRead(A0) > 512;
      float previousMotionLerp = motionLerp;
      if (pirActive) {
        motionLerp += deltaTime / 2.0;
      } else {
        motionLerp -= deltaTime / 0.25;
      }
      if (motionLerp >= 1.0 && previousMotionLerp < 1.0) {
        isDetectingMotion = true;
      }
      if (motionLerp <= 0.0 && previousMotionLerp > 0.0) {
        isDetectingMotion = false;
      }
      motionLerp = constrain(motionLerp, 0.0, 1.0);
      lastMotionCheckMillis = curMillis;
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