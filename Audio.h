#ifndef PT_AUDIO
#define PT_AUDIO

#include "Arduino.h"
#include "Settings.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define BUSY D0
#define USE_AUDIO 1

class Audio {
public:
  Audio(Settings &settings, SoftwareSerial &softwareSerial)
    : settings(settings), softwareSerial(softwareSerial) {
#ifdef USE_AUDIO
    pinMode(BUSY, INPUT);
    softwareSerial.begin(9600);
    myDFPlayer.begin(softwareSerial);
    delay(100);
    myDFPlayer.volume(settings.audioVolume);
#endif
  }

  void PlaySound(uint8_t folder, uint8_t file) {
    myDFPlayer.playFolder(folder, file);
  }

  void Stop() {
    myDFPlayer.stop();
  }

  bool IsPlayingAudio() {
    return digitalRead(BUSY) == LOW;
  }
private:
  Settings &settings;
  SoftwareSerial &softwareSerial;
  DFRobotDFPlayerMini myDFPlayer;
};

#endif