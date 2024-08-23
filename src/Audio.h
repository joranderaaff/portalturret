#ifndef PT_AUDIO
#define PT_AUDIO

#include <Arduino.h>
#include "Settings.h"
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#ifdef LEGACY
#define AUDIO_BUSY A0
#else
#define AUDIO_BUSY D0
#endif

class Audio {
public:
  Audio(Settings &settings, SoftwareSerial &softwareSerial)
    : settings(settings), softwareSerial(softwareSerial) {
  }

  void Begin() {
#ifdef USE_AUDIO
    pinMode(BUSY, INPUT);
    softwareSerial.begin(9600);
    myDFPlayer.begin(softwareSerial);
    delay(100);
    myDFPlayer.volume(settings.audioVolume);
#endif
  }

  void PlaySound(uint8_t folder, uint8_t file) {
#ifdef USE_AUDIO
    myDFPlayer.playFolder(folder, file);
#endif
  }

  void Stop() {
#ifdef USE_AUDIO
    myDFPlayer.stop();
#endif
  }

  bool IsPlayingAudio() {
#ifdef USE_AUDIO
#ifdef LEGACY
    return analogRead(AUDIO_BUSY) < 0XFF;
#else
    return digitalRead(AUDIO_BUSY) == LOW;
#endif
#else
    return false;
#endif
  }
private:
  Settings &settings;
  SoftwareSerial &softwareSerial;
  DFRobotDFPlayerMini myDFPlayer;
};

#endif