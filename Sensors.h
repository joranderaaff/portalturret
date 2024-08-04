#ifndef PT_SENSORS
#define PT_SENSORS

#include "Arduino.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

#define WING_SWITCH D5
#define PID A0

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();

const int measurements = 10;
int currentMeasurement = 0;
bool accelerometerBuffered = false;
int16_t accelX[measurements];
int16_t accelY[measurements];
int16_t accelZ[measurements];
int32_t smoothX = 0;
int32_t smoothY = 0;
int32_t smoothZ = 0;

bool wingsOpen;
bool wasOpen;

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
  if (currentMeasurement >= measurements) {
    accelerometerBuffered = true;
    currentMeasurement = 0;
  }
}

bool isOpen() {
  return digitalRead(WING_SWITCH) == HIGH;
}

void InitSensors() {
  accel.begin();
  wasOpen = isOpen();
  pinMode(WING_SWITCH, INPUT_PULLUP);
}

// For some reason we need to cache this value, as checking it every loop causes
// the webserver to freeze.
// //https://github.com/me-no-dev/ESPAsyncWebServer/issues/944
bool isDetectingMotionCached = false;
unsigned long lastMotionCheck = 0;
bool isDetectingMotion() {
  unsigned long curMillis = millis();
  if (curMillis > lastMotionCheck + 50) {
    isDetectingMotionCached = analogRead(A0) > 512;
    lastMotionCheck = curMillis;
  }
  return isDetectingMotionCached;
}

bool isPlayingAudio() {
  return digitalRead(BUSY) == LOW;
}

#endif