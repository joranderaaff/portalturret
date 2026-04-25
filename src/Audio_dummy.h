#ifndef PT_AUDIO
#define PT_AUDIO

#include <Arduino.h>
#include "Settings.h"

class Audio {
public:
  Audio(Settings &settings)
    : settings(settings) {
  }

  void Begin() {
  }

  void PlaySound(uint8_t folder, uint8_t file) {
  }

  void Stop() {
  }

  bool IsPlayingAudio() {
    return false;
  }

private:
  Settings &settings;
};

#endif