#ifndef PT_ROUTINES
#define PT_ROUTINES

#include <Arduino.h>
#include "LEDs.h"
#include "Servos.h"
#include "Settings.h"
#include <AceRoutine.h>

using namespace ace_routine;


static unsigned long fromTime = 0;
static unsigned long toTime = 0;
static int fromAngle = 0;
static int toAngle = 0;


static bool fullyOpened;

COROUTINE(openWingsRoutine) {
  COROUTINE_BEGIN();
  if (!sensors.WingsAreOpen()) {
    fullyOpened = false;
    servos.SetWingAngle(settings.idleAngle - settings.wingRotateDirection * 90);
    COROUTINE_DELAY(settings.openDuration);
    servos.SetWingAngle(settings.idleAngle);
    fullyOpened = true;
  }
  COROUTINE_END();
}

COROUTINE(closeWingsRoutine) {
  COROUTINE_BEGIN();
  static unsigned long startTime;
  servos.SetRotateAngle(settings.centerAngle);
  COROUTINE_DELAY(250);
  fullyOpened = false;
  startTime = millis();
  servos.SetWingAngle(settings.idleAngle + settings.wingRotateDirection * 90);
  COROUTINE_AWAIT(!sensors.WingsAreOpen() || millis() > startTime + 2000);
  COROUTINE_DELAY(CLOSE_STOP_DELAY);
  servos.SetWingAngle(settings.idleAngle);
  COROUTINE_END();
}

COROUTINE(activatedRoutine) {
  COROUTINE_BEGIN();

#ifdef USE_AUDIO
  if (audio.IsPlayingAudio()) {
    audio.Stop();
    COROUTINE_AWAIT(!audio.IsPlayingAudio());
  }
#endif
  static bool closedAtStart;
  closedAtStart = !sensors.WingsAreOpen();

#ifdef USE_AUDIO
  audio.PlaySound(1, random(1, 9));
  COROUTINE_AWAIT(audio.IsPlayingAudio());
  COROUTINE_AWAIT(!audio.IsPlayingAudio());
#endif

  if (closedAtStart) {
    if (!sensors.WingsAreOpen()) {
      fullyOpened = false;
      servos.SetWingAngle(settings.idleAngle -
                          settings.wingRotateDirection * 90);
      COROUTINE_DELAY(settings.openDuration);
      servos.SetWingAngle(settings.idleAngle);
      fullyOpened = true;
    }
  }

  COROUTINE_END();
}

COROUTINE(searchingRoutine) {
  COROUTINE_BEGIN();

#ifdef USE_AUDIO
  if (audio.IsPlayingAudio()) {
    audio.Stop();
    COROUTINE_AWAIT(!audio.IsPlayingAudio());
  }
#endif

  static unsigned long nextAudioClipTime = 0;

  while (true) {
    if (millis() > nextAudioClipTime) {
      nextAudioClipTime = millis() + 5000;
#ifdef USE_AUDIO
      audio.PlaySound(7, random(1, 11));
#endif
    }
    float t = millis() / 1000.0;
    uint16_t s = t * 255;
    if (fullyOpened) {
      servos.SetRotateAngle(map(constrain(inoise8_raw(s) * 2, -100, 100), -100,
                                100,
                                settings.centerAngle - settings.maxRotation,
                                settings.centerAngle + settings.maxRotation));
    }
    COROUTINE_YIELD();
  }

  COROUTINE_END();
}

COROUTINE(engagingRoutine) {
  COROUTINE_BEGIN();
  Serial.println("Zustand 1: Start"); 
  

  fromTime = millis();

#ifdef USE_AUDIO
  if (audio.IsPlayingAudio()) {
    audio.Stop();
    COROUTINE_AWAIT(!audio.IsPlayingAudio());
    audio.PlaySound(9, 13);
  }

#endif
  leds.alarm = true;
  fromTime = millis();
#ifdef USE_AUDIO
  COROUTINE_AWAIT(audio.IsPlayingAudio() ||
                  (!audio.IsPlayingAudio() && millis() > fromTime + 1000));
  COROUTINE_AWAIT(!audio.IsPlayingAudio());
  audio.PlaySound(9, 8);
#endif
  Serial.println("audio played");
  leds.alarm = false;
  fromTime = millis();
  toTime = fromTime + 1200;

  Serial.println("Fully opened: " + String(fullyOpened));

  if (fullyOpened) {
    int whatSide = random(0, 2);
    fromAngle = whatSide == 0 ? settings.centerAngle - settings.maxRotation
                              : settings.centerAngle + settings.maxRotation;
    toAngle = whatSide == 0 ? settings.centerAngle + settings.maxRotation
                            : settings.centerAngle - settings.maxRotation;

    if (fullyOpened) {
      servos.SetRotateAngle(fromAngle);
    }

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      if (fullyOpened) {
        servos.SetRotateAngle(
            map(millis(), fromTime, toTime, fromAngle, toAngle));
      }
      leds.ToggleGUNLEDs(true);
      COROUTINE_DELAY(5);
      leds.ToggleGUNLEDs(false);
      COROUTINE_DELAY(30);
    }
  }
  COROUTINE_END();
}

COROUTINE(targetLostRoutine) {
  COROUTINE_BEGIN();

#ifdef USE_AUDIO
  if (audio.IsPlayingAudio()) {
    audio.Stop();
    COROUTINE_AWAIT(!audio.IsPlayingAudio());
  }
  audio.PlaySound(6, random(1, 8));

  COROUTINE_AWAIT(audio.IsPlayingAudio());
  COROUTINE_AWAIT(!audio.IsPlayingAudio());
#endif

  servos.SetRotateAngle(settings.centerAngle);
  COROUTINE_DELAY(250);
  fullyOpened = false;
  servos.SetWingAngle(settings.idleAngle + settings.wingRotateDirection * 90);
  COROUTINE_AWAIT(!sensors.WingsAreOpen());
  COROUTINE_DELAY(CLOSE_STOP_DELAY);
  servos.SetWingAngle(settings.idleAngle);

  COROUTINE_END();
}

COROUTINE(pickedUpRoutine) {
  COROUTINE_BEGIN();
#ifdef USE_AUDIO
  if (audio.IsPlayingAudio()) {
    audio.Stop();
    COROUTINE_AWAIT(!audio.IsPlayingAudio());
  }
#endif

  while (true) {
    if (fullyOpened) {
      float t = millis() / 1000.0 * 5.0;
      uint16_t s = t * 255;
      servos.SetRotateAngle(map(constrain(inoise8_raw(s) * 2, -100, 100), -100,
                                100,
                                settings.centerAngle - settings.maxRotation,
                                settings.centerAngle + settings.maxRotation));
    }
#ifdef USE_AUDIO
  static unsigned long nextAudioClipTime = 0;

  if (millis() > nextAudioClipTime) {
    nextAudioClipTime = millis() + 2500;
    audio.PlaySound(5, random(1, 11));
  }
#endif
    COROUTINE_YIELD();
  }

  COROUTINE_END();
}

COROUTINE(shutdownRoutine) {
  COROUTINE_BEGIN();

  static unsigned long t;
  static unsigned long closingStartTime;
#ifdef USE_AUDIO
  if (audio.IsPlayingAudio()) {
    audio.Stop();
    COROUTINE_AWAIT(!audio.IsPlayingAudio());
  }

  audio.PlaySound(4, random(1, 9));
#endif

  servos.SetRotateAngle(settings.centerAngle);
  COROUTINE_DELAY(250);
  fullyOpened = false;
  servos.SetWingAngle(settings.idleAngle + settings.wingRotateDirection * 90);
  closingStartTime = millis();
  COROUTINE_AWAIT(!sensors.WingsAreOpen() ||
                  millis() > closingStartTime + 2000);
  COROUTINE_DELAY(CLOSE_STOP_DELAY);
  servos.SetWingAngle(settings.idleAngle);

  fromTime = t = millis();
  toTime = fromTime + 2000;
  while (t < toTime) {
    t = millis();
    if (t > toTime)
      t = toTime;
    uint16_t s = (t / 1000.0 * 15.0) * 255;
    uint8_t red = map(t, fromTime, toTime, inoise8(s), 0);

    leds.SetCenterLEDBrightness(red);

    leds.FillLEDRing();
    
    COROUTINE_YIELD();
  }

  FastLED.clear();
  FastLED.show();

  COROUTINE_DELAY(5000);

  COROUTINE_END();
}

COROUTINE(rebootRoutine) {
  COROUTINE_BEGIN();
  ;
  static unsigned long t;

  COROUTINE_DELAY(1000);

  fromTime = t = millis();
  toTime = fromTime + 500;
  while (t < toTime) {
    t = millis();
    if (t > toTime)
      t = toTime;
    uint16_t s = (t / 1000.0 * 15.0) * 255;
    uint8_t red = map(t, fromTime, toTime, 0, 255);
    leds.SetCenterLEDBrightness(red);
    leds.FillLEDRing();
    COROUTINE_YIELD();
  }

  COROUTINE_DELAY(1000);

  COROUTINE_END();
}

COROUTINE(manualEngagingRoutine) {
  COROUTINE_BEGIN();
  if (fullyOpened) {
#ifdef USE_AUDIO
    if (audio.IsPlayingAudio()) {
      audio.Stop();
      COROUTINE_AWAIT(!audio.IsPlayingAudio());
    }
#endif

#ifdef USE_AUDIO
    audio.PlaySound(9, 8);
#endif
    fromTime = millis();
    toTime = fromTime + 1200;

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      leds.ToggleGUNLEDs(true);
      COROUTINE_DELAY(5);
      leds.ToggleGUNLEDs(false);
      COROUTINE_DELAY(30);
    }
  }
  COROUTINE_END();
}

COROUTINE(manualMovementRoutine) {
  static int8_t currentRotateDirection = 0;
  static int currentRotateAngle = settings.centerAngle;
  COROUTINE_LOOP() {
    if (currentRotateDirection != 0) {
      if (fullyOpened) {
        currentRotateAngle += currentRotateDirection;
        currentRotateAngle = constrain(
            currentRotateAngle, settings.centerAngle - settings.maxRotation,
            settings.centerAngle + settings.maxRotation);
        servos.SetRotateAngle(currentRotateAngle);
      }
    }
    COROUTINE_DELAY(5);
  }
}
#endif