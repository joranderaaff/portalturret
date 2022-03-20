COROUTINE(openWingsRoutine) {
  COROUTINE_BEGIN();
  if (!isOpen()) {
    pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE - 90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
    COROUTINE_DELAY(900);
    pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  }
  COROUTINE_END();
}

COROUTINE(closeWingsRoutine) {
  COROUTINE_BEGIN();
  pwm.setPWM(ROTATE_SERVO, 0, map(90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_DELAY(250);
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE + 90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_AWAIT(!isOpen());
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_END();
}

COROUTINE(activatedRoutine) {
  COROUTINE_BEGIN();
  
  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  static unsigned long fromTime;
  static unsigned long toTime;
  static int fromAngle;
  static int toAngle;
  static bool closedAtStart;
  closedAtStart = !isOpen();

  myDFPlayer.playFolder(1, random(1, 9));
  COROUTINE_AWAIT(isPlayingAudio());
  COROUTINE_AWAIT(!isPlayingAudio());

  if (closedAtStart) {
    if (!isOpen()) {
      pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE - 90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
      COROUTINE_DELAY(900);
      pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
    }
  }

  COROUTINE_END();
}


COROUTINE(searchingRoutine) {
  COROUTINE_BEGIN();
  
  if(isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  
  static unsigned long nextAudioClipTime = 0;

  while (true) {
    if (millis() > nextAudioClipTime) {
      nextAudioClipTime = millis() + 5000;
      myDFPlayer.playFolder(7, random(1, 11));
    }
    float t = millis() / 1000.0;
    uint16_t s = t * 255;
    if (isOpen()) {
      pwm.setPWM(ROTATE_SERVO, 0, map(map(constrain(inoise8_raw(s) * 2, -100, 100), -100, 100, 30, 160), 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
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
    if (isPlayingAudio()) {
      myDFPlayer.stop();
      COROUTINE_AWAIT(!isPlayingAudio());
    }

    myDFPlayer.playFolder(9, 13);
    alarm = true;
    fromTime = millis();
    COROUTINE_AWAIT(isPlayingAudio() || (!isPlayingAudio() && millis() > fromTime + 1000));
    COROUTINE_AWAIT(!isPlayingAudio());
    alarm = false;

    myDFPlayer.playFolder(9, 8);

    fromTime = millis();
    toTime = fromTime + 1200;

    int whatSide = random(0, 2);
    fromAngle = whatSide == 0 ? 30 : 160;
    toAngle = whatSide == 0 ? 160 : 30;

    if (isOpen()) {
      pwm.setPWM(ROTATE_SERVO, 0, map(fromAngle, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
    }

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      if (isOpen()) {
        pwm.setPWM(ROTATE_SERVO, 0, map(map(millis(), fromTime, toTime, fromAngle, toAngle), 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
      }
      pwm.setPWM(GUN_RIGHT, 4096, 0);
      pwm.setPWM(GUN_LEFT, 4096, 0);
      COROUTINE_DELAY(10);
      pwm.setPWM(GUN_RIGHT, 0, 4096);
      pwm.setPWM(GUN_LEFT, 0, 4096);
      COROUTINE_DELAY(15);
    }

  }
  COROUTINE_END();
}


COROUTINE(targetLostRoutine) {
  COROUTINE_BEGIN();
  if(isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  
  myDFPlayer.playFolder(6, random(1, 8));
  COROUTINE_AWAIT(isPlayingAudio());
  COROUTINE_AWAIT(!isPlayingAudio());

  pwm.setPWM(ROTATE_SERVO, 0, map(90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_DELAY(250);
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE + 90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_AWAIT(!isOpen());
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));

  COROUTINE_END();
}

COROUTINE(pickedUpRoutine) {
  COROUTINE_BEGIN();
  
  if(isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }
  
  static unsigned long nextAudioClipTime = 0;

  while (true) {
    if (isOpen()) {
      float t = millis() / 1000.0 * 5.0;
      uint16_t s = t * 255;
      pwm.setPWM(ROTATE_SERVO, 0, map(map(constrain(inoise8_raw(s) * 2, -100, 100), -100, 100, 30, 160), 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
    }
    if (millis() > nextAudioClipTime) {
      nextAudioClipTime = millis() + 2500;
      myDFPlayer.playFolder(5, random(1, 11));
    }
    COROUTINE_YIELD();
  }

  COROUTINE_END();
}

COROUTINE(shutdownRoutine) {
  COROUTINE_BEGIN();

  static unsigned long fromTime;
  static unsigned long toTime;;
  static unsigned long t;

  if (isPlayingAudio()) {
    myDFPlayer.stop();
    COROUTINE_AWAIT(!isPlayingAudio());
  }

  myDFPlayer.playFolder(4, random(1, 9));

  pwm.setPWM(ROTATE_SERVO, 0, map(90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_DELAY(250);
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE + 90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  COROUTINE_AWAIT(!isOpen());
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));

  fromTime = t = millis();
  toTime = fromTime + 2000;
  while (t < toTime) {
    t = millis();
    if (t > toTime) t = toTime;
    uint16_t s = (t / 1000.0 * 15.0) * 255;
    uint8_t red = map(t, fromTime, toTime, inoise8(s), 0);
    pwm.setPWM(CENTER_LED, 0, map(red, 255, 0, 4096, 0));
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
    pwm.setPWM(CENTER_LED, 0, map(red, 255, 0, 4095, 0));
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

    if (isPlayingAudio()) {
      myDFPlayer.stop();
      COROUTINE_AWAIT(!isPlayingAudio());
    }
    static unsigned long fromTime;
    static unsigned long toTime;
    static int fromAngle;
    static int toAngle;

    myDFPlayer.playFolder(9, 8);

    fromTime = millis();
    toTime = fromTime + 1200;

    COROUTINE_DELAY(200);

    while (toTime > millis()) {
      pwm.setPWM(GUN_RIGHT, 4096, 0);
      pwm.setPWM(GUN_LEFT, 4096, 0);
      COROUTINE_DELAY(10);
      pwm.setPWM(GUN_RIGHT, 0, 4096);
      pwm.setPWM(GUN_LEFT, 0, 4096);
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
        pwm.setPWM(ROTATE_SERVO, 0, map(currentRotateAngle, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
      }
    }
    COROUTINE_DELAY(5);
  }
}
