#ifndef PT_LEDS
#define PT_LEDS

#include <FastLED.h>
#include "Pins.h"

#define NUM_LEDS 8

class LEDs {
public:
  bool alarm;

  LEDs() {}

  void Begin() {
#ifndef LEGACY
    pinMode(GUN_LEDS, OUTPUT);
#endif
    FastLED.addLeds<WS2812, RING_LEDS, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(84);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();
    }
  }

  void UpdateLEDSystem() {
    static char led_system = 0;
    led_system = (led_system+1)%NUM_LEDS;
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = i==led_system?CRGB::Blue:CRGB::Black;
    }
    FastLED.show();
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

  void ToggleGUNLEDs(bool on) {
#ifdef LEGACY
    if (on) {
      pwm.setPWM(GUN_RIGHT, 4096, 0);
      pwm.setPWM(GUN_LEFT, 4096, 0);
    } else {
      pwm.setPWM(GUN_RIGHT, 0, 4096);
      pwm.setPWM(GUN_LEFT, 0, 4096);
    }
#else
    analogWrite(GUN_LEDS, on ? 255 : 0);
#endif
  }

  void SetCenterLEDBrightness(uint8_t brightness) {
#ifdef LEGACY
    int br = brightness << 4;
    pwm.setPWM(CENTER_LED, 0, br);
#else
    analogWrite(CENTER_LED, brightness);
#endif
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

  void TestLEDs() {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(1000);
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(1000);
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    delay(1000);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }

private:
  CRGB leds[NUM_LEDS];
  unsigned long nextLEDUpdateTime = 0;
};

#endif