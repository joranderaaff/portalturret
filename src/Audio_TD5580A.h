#ifndef PT_AUDIO
#define PT_AUDIO

#include <Arduino.h>
#include "Settings.h"
#include <SerialMP3.h>

#include "Pins.h"

class Audio {
public:
  Audio(Settings &settings, uint8_t rx, uint8_t tx)
    : settings(settings), mp3(rx, tx)
    {
    }

  void Begin() {
    pinMode(AUDIO_BUSY, INPUT);

    mp3.init(); 
    delay(100);
    mp3.setVolume(settings.audioVolume);
  }

  void PlaySound(uint8_t folder, uint8_t file) {
    mp3.playFolderFile(folder, file);
  }

  void Stop() {
    mp3.stop();
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
  SerialMP3 mp3;
};

#endif