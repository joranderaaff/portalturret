#ifndef PT_ROUTINES
#define PT_ROUTINES

#include "Arduino.h"
#include "Servos.h"
#include "Settings.h"
#include "LEDs.h"
#include <AceRoutine.h>

using namespace ace_routine;

static bool fullyOpened;

COROUTINE(openWingsRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();
  if (!isOpen()) {
    fullyOpened = false;
    wingServo.write(settings.idleAngle - settings.wingRotateDirection * 90);
    COROUTINE_DELAY(settings.openDuration);
    wingServo.write(settings.idleAngle);
    fullyOpened = true;
  }
  COROUTINE_END();
}

COROUTINE(closeWingsRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();
  static unsigned long startTime;
  rotateServo.write(settings.centerAngle);
  COROUTINE_DELAY(250);
  fullyOpened = false;
  startTime = millis();
  wingServo.write(settings.idleAngle + settings.wingRotateDirection * 90);
  COROUTINE_AWAIT(!isOpen() || millis() > startTime + 2000);
  COROUTINE_DELAY(CLOSE_STOP_DELAY);
  wingServo.write(settings.idleAngle);
  COROUTINE_END();
}

COROUTINE(activatedRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();

#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
#endif

  static unsigned long fromTime;
  static unsigned long toTime;
  static int fromAngle;
  static int toAngle;
  static bool closedAtStart;
  closedAtStart = !isOpen();

#ifdef USE_AUDIO
  myDFPlayer.playFolder(1, random(1, 9));
  COROUTINE_AWAIT(isPlayingAudio());
  COROUTINE_AWAIT(!isPlayingAudio());
#endif

  if (closedAtStart) {
    if (!isOpen()) {
      fullyOpened = false;
      wingServo.write(settings.idleAngle - settings.wingRotateDirection * 90);
      COROUTINE_DELAY(settings.openDuration);
      wingServo.write(settings.idleAngle);
      fullyOpened = true;
    }
  }

  COROUTINE_END();
}


COROUTINE(searchingRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();

#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
#endif

  static unsigned long nextAudioClipTime = 0;

  while (true) {
    if (millis() > nextAudioClipTime) {
      nextAudioClipTime = millis() + 5000;
#ifdef USE_AUDIO
      myDFPlayer.playFolder(7, random(1, 11));
#endif
    }
    float t = millis() / 1000.0;
    uint16_t s = t * 255;
    if (fullyOpened) {
      rotateServo.write(map(constrain(inoise8_raw(s) * 2, -100, 100), -100, 100, settings.centerAngle - settings.maxRotation, settings.centerAngle + settings.maxRotation));
    }
    COROUTINE_YIELD();
  }

  COROUTINE_END();
}

COROUTINE(engagingRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();

  static unsigned long fromTime;
  static unsigned long toTime;
  static int fromAngle;
  static int toAngle;

  fromTime = millis();
#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  myDFPlayer.playFolder(9, 13);
#endif
  alarm = true;
  fromTime = millis();
#ifdef USE_AUDIO
  COROUTINE_AWAIT(isPlayingAudio() || (!isPlayingAudio() && millis() > fromTime + 1000));
  COROUTINE_AWAIT(!isPlayingAudio());
  myDFPlayer.playFolder(9, 8);
#endif
  alarm = false;
  fromTime = millis();
  toTime = fromTime + 1200;

  if (fullyOpened) {
    int whatSide = random(0, 2);
    fromAngle = whatSide == 0 ? settings.centerAngle - settings.maxRotation : settings.centerAngle + settings.maxRotation;
    toAngle = whatSide == 0 ? settings.centerAngle + settings.maxRotation : settings.centerAngle - settings.maxRotation;

    if (fullyOpened) {
      rotateServo.write(fromAngle);
    }

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      if (fullyOpened) {
        rotateServo.write(map(millis(), fromTime, toTime, fromAngle, toAngle));
      }
      analogWrite(GUN_LEDS, 255);
      COROUTINE_DELAY(5);
      analogWrite(GUN_LEDS, 0);
      COROUTINE_DELAY(30);
    }
  }
  COROUTINE_END();
}


COROUTINE(targetLostRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();

#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  myDFPlayer.playFolder(6, random(1, 8));

  COROUTINE_AWAIT(isPlayingAudio());
  COROUTINE_AWAIT(!isPlayingAudio());
#endif

  rotateServo.write(settings.centerAngle);
  COROUTINE_DELAY(250);
  fullyOpened = false;
  wingServo.write(settings.idleAngle + settings.wingRotateDirection * 90);
  COROUTINE_AWAIT(!isOpen());
  COROUTINE_DELAY(CLOSE_STOP_DELAY);
  wingServo.write(settings.idleAngle);

  COROUTINE_END();
}

COROUTINE(pickedUpRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();
#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
#endif

  static unsigned long nextAudioClipTime = 0;

  while (true) {
    if (fullyOpened) {
      float t = millis() / 1000.0 * 5.0;
      uint16_t s = t * 255;
      rotateServo.write(map(constrain(inoise8_raw(s) * 2, -100, 100), -100, 100, settings.centerAngle - settings.maxRotation, settings.centerAngle + settings.maxRotation));
    }
#ifdef USE_AUDIO
    if (millis() > nextAudioClipTime) {
      nextAudioClipTime = millis() + 2500;
      myDFPlayer.playFolder(5, random(1, 11));
    }
#endif
    COROUTINE_YIELD();
  }

  COROUTINE_END();
}

COROUTINE(shutdownRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();

  static unsigned long fromTime;
  static unsigned long toTime;
  static unsigned long t;
  static unsigned long closingStartTime;
#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }

  myDFPlayer.playFolder(4, random(1, 9));
#endif

  rotateServo.write(settings.centerAngle);
  COROUTINE_DELAY(250);
  fullyOpened = false;
  wingServo.write(settings.idleAngle + settings.wingRotateDirection * 90);
  closingStartTime = millis();
  COROUTINE_AWAIT(!isOpen() || millis() > closingStartTime + 2000);
  COROUTINE_DELAY(CLOSE_STOP_DELAY);
  wingServo.write(settings.idleAngle);

  fromTime = t = millis();
  toTime = fromTime + 2000;
  while (t < toTime) {
    t = millis();
    if (t > toTime) t = toTime;
    uint16_t s = (t / 1000.0 * 15.0) * 255;
    uint8_t red = map(t, fromTime, toTime, inoise8(s), 0);

    analogWrite(CENTER_LED, red);

    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(red, 0, 0);
      FastLED.show();
    }
    COROUTINE_YIELD();
  }

  FastLED.clear();
  FastLED.show();

  COROUTINE_DELAY(5000);

  COROUTINE_END();
}


COROUTINE(rebootRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();

  static unsigned long fromTime;
  static unsigned long toTime;
  ;
  static unsigned long t;

  COROUTINE_DELAY(1000);

  fromTime = t = millis();
  toTime = fromTime + 500;
  while (t < toTime) {
    t = millis();
    if (t > toTime) t = toTime;
    uint16_t s = (t / 1000.0 * 15.0) * 255;
    uint8_t red = map(t, fromTime, toTime, 0, 255);
    analogWrite(CENTER_LED, red);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(red, 0, 0);
      FastLED.show();
    }
    COROUTINE_YIELD();
  }

  COROUTINE_DELAY(1000);

  COROUTINE_END();
}


COROUTINE(manualEngagingRoutine) {
  COROUTINE_BEGIN();
  static Settings settings = LoadSettings();
  if (fullyOpened) {
#ifdef USE_AUDIO
    if (isPlayingAudio()) {
      myDFPlayer.stop();
      COROUTINE_AWAIT(!isPlayingAudio());
    }
#endif
    static unsigned long fromTime;
    static unsigned long toTime;
    static int fromAngle;
    static int toAngle;
#ifdef USE_AUDIO
    myDFPlayer.playFolder(9, 8);
#endif
    fromTime = millis();
    toTime = fromTime + 1200;

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      analogWrite(GUN_LEDS, 255);
      COROUTINE_DELAY(5);
      analogWrite(GUN_LEDS, 0);
      COROUTINE_DELAY(30);
    }
  }
  COROUTINE_END();
}

COROUTINE(manualMovementRoutine) {
  Settings settings = LoadSettings();
  static int8_t currentRotateDirection = 0;
  static int currentRotateAngle = settings.centerAngle;
  COROUTINE_LOOP() {
    if (currentRotateDirection != 0) {
      if (fullyOpened) {
        currentRotateAngle += currentRotateDirection;
        currentRotateAngle = constrain(currentRotateAngle, settings.centerAngle - settings.maxRotation, settings.centerAngle + settings.maxRotation);
        rotateServo.write(currentRotateAngle);
      }
    }
    COROUTINE_DELAY(5);
  }
}
#endif