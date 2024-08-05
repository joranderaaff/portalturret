#ifndef PT_LEDS
#define PT_LEDS

#include <FastLED.h>

// Pins
#define CENTER_LED D3
#define RING_LEDS D8
#define GUN_LEDS D4

#define NUM_LEDS 8
class LEDs {
public:
  bool alarm;

  LEDs() {}

  void Begin() {
    pinMode(GUN_LEDS, OUTPUT);
    FastLED.addLeds<WS2812, RING_LEDS, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(84);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();
    }
  }

  void UpdateLEDPreloader() {
    int t = floor(millis() / 10);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB((i + t) % 8 == 0 ? 255 : 0, 0, 0);
      FastLED.show();
    }
  }

  void FillLEDRing() {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(255, 0, 0);
      FastLED.show();
    }
  }

  void UpdateLEDs() {
    if (millis() > nextLEDUpdateTime + 16) {
      nextLEDUpdateTime = millis();
      if (alarm) {
        uint8_t phase = ((millis() * 2) % 255) & 0xFF;
        uint8_t red = cubicwave8(phase);
        fill_solid(leds, NUM_LEDS, CRGB(red, 0, 0));
        FastLED.show();
      } else {
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.show();
      }
    }
  }

private:
  CRGB leds[NUM_LEDS];
  unsigned long nextLEDUpdateTime = 0;
};

#endif