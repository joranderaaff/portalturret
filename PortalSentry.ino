// General
#include "Arduino.h"

// Why do I have to include this here? Servo.h otherwise found twice?
#include <ESPAsyncWebServer.h>

#include "Audio.h"
#include "LEDs.h"
#include "Sensors.h"
#include "Servos.h"
#include "Settings.h"
#include "SoftwareSerial.h"

Settings settings;
Sensors sensors(settings);
Servos servos(settings, sensors);
LEDs leds(settings);
SoftwareSerial softwareSerial(RX, TX);
Audio audio(settings, softwareSerial);

#include "Routines.h"
#include "StateBehaviour.h"
#include "Server.h"

void setup() {
  settings.Begin();
  sensors.Begin();
  leds.Begin();
  
  servos.Begin();
  servos.CloseWings();

  StartServer();
  InitStates();

  leds.FillLEDRing();
}

void loop() {
  sensors.UpdateSensors();
  leds.UpdateLEDs();
  UpdateStateBehaviour();
  UpdateServer();
}
