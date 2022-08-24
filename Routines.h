#define OPEN_DURATION 2300

COROUTINE(openWingsRoutine) {
  COROUTINE_BEGIN();
  if (!isOpen()) {
    wingServo.write(STATIONARY_ANGLE - 90);
    COROUTINE_DELAY(OPEN_DURATION);
    wingServo.write(STATIONARY_ANGLE);
  }
  COROUTINE_END();
}

COROUTINE(closeWingsRoutine) {
  COROUTINE_BEGIN();
  rotateServo.write(90);
  COROUTINE_DELAY(250);
  wingServo.write(STATIONARY_ANGLE + 90);
  COROUTINE_AWAIT(!isOpen());
  wingServo.write(STATIONARY_ANGLE);
  COROUTINE_END();
}

COROUTINE(activatedRoutine) {
  COROUTINE_BEGIN();

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
      wingServo.write(STATIONARY_ANGLE - 90);
      COROUTINE_DELAY(OPEN_DURATION);
      wingServo.write(STATIONARY_ANGLE);
    }
  }

  COROUTINE_END();
}


COROUTINE(searchingRoutine) {
  COROUTINE_BEGIN();

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
    if (isOpen()) {
      rotateServo.write(map(constrain(inoise8_raw(s) * 2, -100, 100), -100, 100, 30, 160));
    }
    COROUTINE_YIELD();
  }

  COROUTINE_END();
}

COROUTINE(engagingRoutine) {
  COROUTINE_BEGIN();
  if (isOpen()) {

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

    int whatSide = random(0, 2);
    fromAngle = whatSide == 0 ? 30 : 160;
    toAngle = whatSide == 0 ? 160 : 30;

    if (isOpen()) {
      rotateServo.write(fromAngle);
    }

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      if (isOpen()) {
        rotateServo.write(map(millis(), fromTime, toTime, fromAngle, toAngle));
      }
      analogWrite(GUN_LEDS, 255);
      COROUTINE_DELAY(10);
      analogWrite(GUN_LEDS, 0);
      COROUTINE_DELAY(15);
    }

  }
  COROUTINE_END();
}


COROUTINE(targetLostRoutine) {
  COROUTINE_BEGIN();

#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  myDFPlayer.playFolder(6, random(1, 8));

  COROUTINE_AWAIT(isPlayingAudio());
  COROUTINE_AWAIT(!isPlayingAudio());
#endif

  rotateServo.write(90);
  COROUTINE_DELAY(250);
  wingServo.write(STATIONARY_ANGLE + 90);
  COROUTINE_AWAIT(!isOpen());
  wingServo.write(STATIONARY_ANGLE);

  COROUTINE_END();
}

COROUTINE(pickedUpRoutine) {
  COROUTINE_BEGIN();
#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
#endif

  static unsigned long nextAudioClipTime = 0;

  while (true) {
    if (isOpen()) {
      float t = millis() / 1000.0 * 5.0;
      uint16_t s = t * 255;
      rotateServo.write(map(constrain(inoise8_raw(s) * 2, -100, 100), -100, 100, 30, 160));
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

  static unsigned long fromTime;
  static unsigned long toTime;;
  static unsigned long t;
#ifdef USE_AUDIO
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }

  myDFPlayer.playFolder(4, random(1, 9));
#endif

  rotateServo.write(90);
  COROUTINE_DELAY(250);
  wingServo.write(STATIONARY_ANGLE + 90);
  COROUTINE_AWAIT(!isOpen());
  wingServo.write(STATIONARY_ANGLE);

  fromTime = t = millis();
  toTime = fromTime + 2000;
  while (t < toTime) {
    t = millis();
    if (t > toTime) t = toTime;
    uint16_t s = (t / 1000.0 * 15.0) * 255;
    uint8_t red = map(t, fromTime, toTime, inoise8(s), 0);

    analogWrite(CENTER_LED, red);

    for (int i = 0; i < NUM_LEDS; i++)
    {
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

  static unsigned long fromTime;
  static unsigned long toTime;;
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
    for (int i = 0; i < NUM_LEDS; i++)
    {
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
  if (isOpen()) {
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
      COROUTINE_DELAY(10);
      analogWrite(GUN_LEDS, 0);
      COROUTINE_DELAY(15);
    }
  }
  COROUTINE_END();
}

int8_t currentRotateDirection = 0;
int currentRotateAngle = 90;

COROUTINE(manualMovementRoutine) {
  COROUTINE_LOOP() {
    if (currentRotateDirection != 0) {
      if (isOpen()) {
        currentRotateAngle += currentRotateDirection;
        currentRotateAngle = constrain(currentRotateAngle, 30, 150);
        rotateServo.write(currentRotateAngle);
      }
    }
    COROUTINE_DELAY(5);
  }
}
