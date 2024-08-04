#ifndef PT_AUDIO
#define PT_AUDIO

#include "Settings.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define BUSY D0
#define USE_AUDIO 1

SoftwareSerial mySoftwareSerial(RX, TX); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void InitAudio() {
  Settings settings = LoadSettings();
  pinMode(BUSY, INPUT);
  mySoftwareSerial.begin(9600);
  myDFPlayer.begin(mySoftwareSerial);
  delay(100);
  myDFPlayer.volume(settings.audioVolume);
}

#endif