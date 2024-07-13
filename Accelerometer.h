#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

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

void SetupAccelerometer() {
  accel.begin();
}

void UpdateAccelerometer() {
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
