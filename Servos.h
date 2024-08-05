#ifndef PT_SERVOS
#define PT_SERVOS

#include "Arduino.h"
#include "Sensors.h"
#include "Settings.h"
#include <Servo.h>

#define FREQ 50          // one clock is 20 ms
#define FREQ_MINIMUM 205 // 1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410 // 2ms is 2/20, of 4096

// Tweak these according to servo speed
#define CLOSE_STOP_DELAY 100

class Servos {
public:
  Servo wingServo;
  Servo rotateServo;

  Servos(Settings &setings, Sensors &sensors)
      : settings(settings), sensors(sensors) {}
      
  void Begin() {
    wingServo.attach(settings.wingPin);
    rotateServo.attach(settings.rotatePin);
  }

  void SetWingAngle(int angle) { wingServo.write(angle); }

  void SetRotateAngle(int angle) { rotateServo.write(angle); }

  void CloseWings() {
    SetRotateAngle(settings.centerAngle);
    delay(250);
    unsigned long closingStartTime = millis();
    SetWingAngle(settings.idleAngle + settings.wingRotateDirection * 90);
    while (millis() < closingStartTime + 3000 && sensors.WingsAreOpen()) {
      delay(10);
    }
    delay(CLOSE_STOP_DELAY);
    SetWingAngle(settings.idleAngle);
  }

private:
  Settings &settings;
  Sensors &sensors;
  int currentMoveSpeed = 0;
};
#endif