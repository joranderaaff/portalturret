#ifndef PT_SERVOS
#define PT_SERVOS

#include <Arduino.h>
#ifdef ESP32
#include "ESP32Servo.h"
#else
#include <Servo.h>
#endif
#include "Pins.h"
#include "Sensors.h"
#include "Settings.h"

// Tweak these according to servo speed
#define CLOSE_STOP_DELAY 100

class Servos {
public:
  Servos(Settings &settings, Sensors &sensors)
    : settings(settings), sensors(sensors) {}

  void Begin() {
#ifdef ESP32
  wingServo.attach(SERVO_WING, 1, 50, 13);
  rotateServo.attach(SERVO_ROTATE, 2, 50, 13);
#else
#ifndef LEGACY
  wingServo.attach(settings.wingPin);
  rotateServo.attach(settings.rotatePin);
#endif
#endif
  }

  void SetWingAngle(int angle) {
#ifdef LEGACY
    pwm.setPWM(SERVO_WING, 0, map(angle, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
#else
    wingServo.write(angle);
#endif
  }

  void SetRotateAngle(int angle) {
#ifdef LEGACY
    pwm.setPWM(SERVO_ROTATE, 0, map(angle, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
#else
    rotateServo.write(angle);
#endif
  }

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
  Servo wingServo;
  Servo rotateServo;
  int currentMoveSpeed = 0;
};
#endif