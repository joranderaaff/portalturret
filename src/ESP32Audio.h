#ifndef PT_AUDIO
#define PT_AUDIO

#include <Arduino.h>
#include "Settings.h"
#include "Pins.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include "pins.h"

class Audio {
public:
  Audio(Settings &settings)
    : settings(settings) {
    mp3 = NULL;
  }

  void Begin() {
    out = new AudioOutputI2S();
    out->SetPinout(BCK, WS, DOUT);
    out->SetGain(settings.audioVolume / 10.0);
    mp3 = new AudioGeneratorMP3();
  }

  void PlaySound(uint8_t folder, uint8_t filenum) {
    char filename[32];
    snprintf(filename, 31, "/%02i/%03i.mp3", folder, filenum);
    file = new AudioFileSourceLittleFS(filename);
    if (!mp3->begin(file, out)) {
      Serial.println("MP3 failed to start");
    } else {
      Serial.print("MP3 playback started ");
      Serial.println(filename);
    }
  }

  void Stop() {
    if (mp3)
      mp3->stop();
  }

  bool IsPlayingAudio() {
    return mp3 && mp3->isRunning();
  }

  void Loop() {
    if (mp3 && mp3->isRunning()) {
      if (!mp3->loop()) {
        mp3->stop();
        Serial.println("MP3 playback stopped");
        file->close();
      }
    }
  }
private:
  Settings &settings;
  AudioGeneratorMP3 *mp3;
  AudioFileSourceLittleFS *file;
  AudioOutputI2S *out;
};

#endif