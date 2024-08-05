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

  Sensors(Settings &settingsIn) : settings(settingsIn) {
  }
  
  void Begin() {
    accel = Adafruit_ADXL345_Unified();
    accel.begin();
    wasOpen = WingsAreOpen();
    pinMode(WING_SWITCH, INPUT_PULLUP);
  }

  bool WingsAreOpen() { return digitalRead(WING_SWITCH) == HIGH; }

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
  }

  bool IsDetectingMotion() {
    unsigned long curMillis = millis();
    if (curMillis > lastMotionCheck + 50) {
      isDetectingMotionCached = analogRead(A0) > 512;
      lastMotionCheck = curMillis;
    }
    return isDetectingMotionCached;
  }

private:
  Settings &settings;
  Adafruit_ADXL345_Unified accel;

  int currentMeasurement;
  int16_t accelX[MEASUREMENTS];
  int16_t accelY[MEASUREMENTS];
  int16_t accelZ[MEASUREMENTS];

  bool wingsOpen;
  bool wasOpen;

  // For some reason we need to cache this value, as checking it every loop
  // causes the webserver to freeze.
  // //https://github.com/me-no-dev/ESPAsyncWebServer/issues/944
  bool isDetectingMotionCached;
  unsigned long lastMotionCheck;
};
#endif