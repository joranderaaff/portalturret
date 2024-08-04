//General
#include "Arduino.h"

//Why do I have to include this here? Servo.h otherwise found twice?
#include <ESPAsyncWebServer.h>

#include "Settings.h"
#include "Sensors.h"
#include "Audio.h"
#include "LEDs.h"
#include "Server.h"
#include "StateBehaviour.h"


void setup() {
  // Init

  InitServos();
  CloseWings();
  
  InitLEDs();
  InitSensors();
  InitAudio();
  InitServer();
  InitStates();

  // Setup
  StartWebServer();
  StartWebSocket();

  FillLEDRing();
}

void loop() {
  UpdateSensors();
  UpdateServer();
  UpdateStateBehaviour();
  UpdateLEDs();
}