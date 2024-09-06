#ifndef PT_AUDIO
#define PT_AUDIO

#include <Arduino.h>
#include "Settings.h"
#include <SoftwareSerial.h>
#include "mp3_driver.h"
#include "mp3_driver_factory.h"

#include "Pins.h"

class Audio {
public:
  Audio(Settings &settings, SoftwareSerial &softwareSerial)
    : settings(settings), softwareSerial(softwareSerial) {
  }

  void Begin() {
    softwareSerial.begin(9600);
    mp3_driver = new_mp3_driver(&softwareSerial, AUDIO_BUSY);
    delay(100);
    mp3_driver->setVolume(settings.audioVolume);
    mp3_driver->playSongFromFolder(1, 1+random(mp3_driver->getFileCountInFolder(1)));
  }

  void PlaySound(uint8_t folder, uint8_t file) {
    mp3_driver->playSongFromFolder(folder, file);
  }

  void Stop() {
    mp3_driver->stop();
  }

  bool IsPlayingAudio() {
    return mp3_driver->isBusy();
  }
private:
  Settings &settings;
  SoftwareSerial &softwareSerial;
  Mp3Driver* mp3_driver;
};

#endif