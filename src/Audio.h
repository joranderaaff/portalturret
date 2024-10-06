#ifndef PT_AUDIO
#define PT_AUDIO

#include <Arduino.h>
#include "Settings.h"
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#include "Pins.h"

class Audio {
public:
  Audio(Settings &settings, SoftwareSerial &softwareSerial)
    : settings(settings), softwareSerial(softwareSerial) {
  }

  void Begin() {
    pinMode(AUDIO_BUSY, INPUT);

    softwareSerial.begin(9600);
    myDFPlayer.begin(softwareSerial);
    delay(100);
    myDFPlayer.volume(settings.audioVolume);
  }

  void PlaySound(uint8_t folder, uint8_t file) {
    myDFPlayer.playFolder(folder, file);
  }

  void Stop() {
    myDFPlayer.stop();
  }

  bool IsPlayingAudio() {
#ifdef LEGACY
    return analogRead(AUDIO_BUSY) < 0XFF;
#else
    return digitalRead(AUDIO_BUSY) == LOW;
#endif
  }

private:
  Settings &settings;
  SoftwareSerial &softwareSerial;
  DFRobotDFPlayerMini myDFPlayer;
};

#endif