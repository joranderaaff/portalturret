// General
#include "Arduino.h"
#include "SoftwareSerial.h"

#define LEGACY 1

#ifdef LEGACY
#include <Adafruit_PWMServoDriver.h>
#define FREQ 50 // one clock is 20 ms
#define FREQ_MINIMUM 205  // 1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410  // 2ms is 2/20, of 4096
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
SoftwareSerial softwareSerial(D5, D6);
#else
SoftwareSerial softwareSerial(RX, TX);
#endif

// Why do I have to include this here? Servo.h otherwise found twice?
#include <ESPAsyncWebServer.h>

#include "Audio.h"
#include "LEDs.h"
#include "Sensors.h"
#include "Servos.h"
#include "Settings.h"

Settings settings;
Sensors sensors(settings);
Servos servos(settings, sensors);
LEDs leds;

Audio audio(settings, softwareSerial);

#include "Routines.h"
#include "StateBehaviour.h"
#include "Server.h"

void setup() {

#ifdef LEGACY
  pwm.begin();
  pwm.setPWMFreq(FREQ);
#endif

  Serial.begin(115200);
  settings.Begin();
  sensors.Begin();
  leds.Begin();
  servos.Begin();
  servos.CloseWings();
  Serial.end();
  audio.Begin();

  StartServer();

  currentTurretMode = settings.startInManualMode == 1 ? TurretMode::Manual
                                                      : TurretMode::Automatic;

  leds.SetCenterLEDBrightness(255);
  leds.FillLEDRing();
}

void loop() {
  sensors.UpdateSensors();
  leds.UpdateLEDs();
  UpdateStateBehaviour();
  UpdateServer();
}
