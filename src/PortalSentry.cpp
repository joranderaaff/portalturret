// General
#include <Arduino.h>
#include "pins.h"

#ifdef LEGACY
#include <Adafruit_PWMServoDriver.h>
#define FREQ 50 // one clock is 20 ms
#define FREQ_MINIMUM 205  // 1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410  // 2ms is 2/20, of 4096
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#endif

#ifndef HARDWARE_V3
#include <SoftwareSerial.h>
SoftwareSerial softwareSerial(AUDIO_RX, AUDIO_TX);
#endif

// Why do I have to include this here? Servo.h otherwise found twice?
#include <ESPAsyncWebServer.h>

#include "LEDs.h"
#include "Sensors.h"
#include "Servos.h"
#include "Settings.h"

Settings settings;

#ifdef USE_AUDIO_CARL
#include "Audio_carl.h"
Audio audio(settings, softwareSerial);

#elif HARDWARE_V3
#include "ESP32Audio.h"
Audio audio(settings);

#elif USE_SERIAL_MP3
#include "Audio_TD5580A.h"
Audio audio(settings, AUDIO_RX, AUDIO_TX);

#elif USE_AUDIO
#include "Audio.h"
Audio audio(settings, softwareSerial);

#else
#include "Audio_dummy.h"
Audio audio(settings);
#endif

Sensors sensors(settings);
Servos servos(settings, sensors);
LEDs leds;

#include "Routines.h"
#include "StateBehaviour.h"
#include "PortalServer.h"

void setup() {

#ifdef LEGACY
  pwm.begin();
  pwm.setPWMFreq(FREQ);
#endif

#if defined(ESP32)
  Serial.begin(115200);
#ifdef WAIT_FOR_SERIAL
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  while (!Serial) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
  }
#endif
#else
  Serial.begin(74880);
#endif
  settings.Begin();
  sensors.Begin();
  leds.Begin();
  servos.Begin();
  servos.CloseWings();
#if defined(USE_AUDIO) && not defined(LEGACY) && not defined(HARDWARE_V3)
  Serial.println("Ending serial communications, enabling audio");
  delay(300);
  Serial.end();
#endif
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
#ifdef HARDWARE_V3
  audio.Loop();
#endif
}
