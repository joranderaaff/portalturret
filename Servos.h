#ifndef PT_SERVOS
#define PT_SERVOS

#include "Arduino.h"
#include "Settings.h"
#include <Servo.h>

#define FREQ 50           // one clock is 20 ms
#define FREQ_MINIMUM 205  // 1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410  // 2ms is 2/20, of 4096

// Tweak these according to servo speed
#define CLOSE_STOP_DELAY 100

Servo wingServo;
Servo rotateServo;

int currentMoveSpeed = 0;

void InitServos() {
  Settings settings = LoadSettings();
  wingServo.attach(settings.wingPin);
  rotateServo.attach(settings.rotatePin);
}

void CloseWings() {
  Settings settings = LoadSettings();
  rotateServo.write(settings.centerAngle);
  delay(250);
  unsigned long closingStartTime = millis();
  wingServo.write(settings.idleAngle + settings.wingRotateDirection * 90);
  while (millis() < closingStartTime + 3000 && isOpen()) {
    delay(10);
  }
  delay(CLOSE_STOP_DELAY);
  wingServo.write(settings.idleAngle);
}

#endif