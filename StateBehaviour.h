#ifndef PT_STATE_BEHAVIOUR
#define PT_STATE_BEHAVIOUR

#include "Arduino.h"
#include "Routines.h"
#include "Servos.h"
#include "Settings.h"

enum class TurretMode { Automatic = 0,
                        Manual = 1 };

enum class TurretState {
  Idle = 0,
  Activated = 1,
  Searching = 2,
  Engaging = 3,
  TargetLost = 4,
  PickedUp = 5,
  Shutdown = 6,
  ManualEngaging = 7,
  Rebooting = 8,
};

enum class ManualState {
  Idle,
  Opening,
  Closing,
  Firing,
};

static TurretMode currentTurretMode;
static TurretState currentState = TurretState::Idle;
static ManualState currentManualState = ManualState::Idle;

static unsigned long detectTime = 0;
static unsigned long undetectTime = 0;
static unsigned long previousTime = 0;
static unsigned long stateStartTime = 0;
static unsigned long lastMovementTime = 0;

static bool diagnoseMode;
static int diagnoseAction;

void InitStates() {
  currentState = TurretState::Idle;
  currentManualState = ManualState::Idle;
  currentTurretMode = TurretMode::Automatic;
}

void setState(TurretState nextState) {
  Settings settings = LoadSettings();
  // Stop the Wing Servos just in case;
  wingServo.write(settings.idleAngle);

  if (currentTurretMode == TurretMode::Automatic) {
    switch (nextState) {
      case TurretState::Activated:
        activatedRoutine.reset();
        break;
      case TurretState::Engaging:
        engagingRoutine.reset();
        break;
      case TurretState::Searching:
        searchingRoutine.reset();
        break;
      case TurretState::TargetLost:
        targetLostRoutine.reset();
        break;
      case TurretState::PickedUp:
        pickedUpRoutine.reset();
        break;
      case TurretState::Shutdown:
        shutdownRoutine.reset();
        break;
      case TurretState::ManualEngaging:
        manualEngagingRoutine.reset();
        break;
      case TurretState::Rebooting:
        rebootRoutine.reset();
        break;
    }
    stateStartTime = millis();
    currentState = nextState;
  }
}

void setManualState(ManualState nextState) {
  Settings settings = LoadSettings();
  // Stop the Wing Servos just in case;
  wingServo.write(settings.idleAngle);

  if (currentTurretMode == TurretMode::Manual) {
    switch (nextState) {
      case ManualState::Opening:
        openWingsRoutine.reset();
        break;
      case ManualState::Closing:
        closeWingsRoutine.reset();
        break;
      case ManualState::Firing:
        manualEngagingRoutine.reset();
        break;
    }
    currentManualState = nextState;
  }
}

void manualRotation(unsigned long deltaTime) {
  manualMovementRoutine.runCoroutine();
}

void UpdateStateBehaviour() {
  Settings settings = LoadSettings();
  if (!diagnoseMode) {
    unsigned long deltaTime = millis() - previousTime;
    previousTime = millis();

    if (currentTurretMode == TurretMode::Manual) {
      switch (currentManualState) {
        case ManualState::Idle:
          manualRotation(deltaTime);
          break;
        case ManualState::Opening:
          openWingsRoutine.runCoroutine();
          if (openWingsRoutine.isDone()) {
            setManualState(ManualState::Idle);
          }
          break;
        case ManualState::Closing:
          closeWingsRoutine.runCoroutine();
          if (closeWingsRoutine.isDone()) {
            setManualState(ManualState::Idle);
          }
          break;
        case ManualState::Firing:
          manualRotation(deltaTime);
          manualEngagingRoutine.runCoroutine();
          if (manualEngagingRoutine.isDone()) {
            setManualState(ManualState::Idle);
          }
          break;
      }
    }
    if (currentTurretMode == TurretMode::Automatic) {

      bool motionDetected = isDetectingMotion();

      float zMovement = (smoothZ / measurements * SENSORS_GRAVITY_STANDARD * ADXL345_MG2G_MULTIPLIER);
      float xMovement = (smoothX / measurements * SENSORS_GRAVITY_STANDARD * ADXL345_MG2G_MULTIPLIER);
      float yMovement = (smoothY / measurements * SENSORS_GRAVITY_STANDARD * ADXL345_MG2G_MULTIPLIER);

      bool pickedUp =
        accelerometerBuffered && (xMovement < -settings.panicTreshold || xMovement > settings.panicTreshold || yMovement < -settings.panicTreshold || yMovement > settings.panicTreshold);
      bool movedAround =
        accelerometerBuffered && (xMovement < -settings.restTreshold || xMovement > settings.restTreshold || yMovement < -settings.restTreshold || yMovement > settings.restTreshold);
      bool onItsSide =
        accelerometerBuffered && (zMovement < settings.tippedOverTreshold);

      if (movedAround) {
        lastMovementTime = millis();
      }

      if (pickedUp && currentState != TurretState::PickedUp && currentState != TurretState::Shutdown && currentState != TurretState::Rebooting) {
        setState(TurretState::PickedUp);
      }
      switch (currentState) {
        case TurretState::Idle:
          if (motionDetected) {
            setState(TurretState::Activated);
          }
          break;
        case TurretState::Activated:
          activatedRoutine.runCoroutine();
          if (activatedRoutine.isDone()) {
            if (motionDetected) {
              setState(TurretState::Engaging);
            } else {
              setState(TurretState::Searching);
            }
          }
          break;
        case TurretState::Searching:
          searchingRoutine.runCoroutine();
          if (millis() > stateStartTime + 10000) {
            setState(TurretState::TargetLost);
          }
          if (motionDetected && millis() > stateStartTime + 100) {
            setState(TurretState::Engaging);
          }
          break;
        case TurretState::TargetLost:
          targetLostRoutine.runCoroutine();
          if (targetLostRoutine.isDone()) {
            setState(TurretState::Idle);
          }
          break;
        case TurretState::Engaging:
          engagingRoutine.runCoroutine();
          if (engagingRoutine.isDone()) {
            if (wingsOpen) {
              setState(TurretState::Searching);
            } else {
              setState(TurretState::Idle);
            }
          }
          break;
        case TurretState::PickedUp:
          pickedUpRoutine.runCoroutine();
          if (onItsSide) {
            setState(TurretState::Shutdown);
          } else if (!movedAround && millis() > lastMovementTime + 5000) {
            setState(TurretState::Activated);
          }
          break;
        case TurretState::Shutdown:
          shutdownRoutine.runCoroutine();
          if (shutdownRoutine.isDone() && !onItsSide) {
            setState(TurretState::Rebooting);
          }
          break;
        case TurretState::Rebooting:
          rebootRoutine.runCoroutine();
          if (rebootRoutine.isDone()) {
            setState(TurretState::Idle);
          }
          break;
      }
    }
  } else {
    switch (diagnoseAction) {
      case 0:
        wingServo.write(settings.idleAngle - settings.wingRotateDirection * 90);
        delay(250);
        wingServo.write(settings.idleAngle);
        break;
      case 1:
        wingServo.write(settings.idleAngle + settings.wingRotateDirection * 90);
        delay(250);
        wingServo.write(settings.idleAngle);
        break;
      case 2:
        rotateServo.write(50);
        delay(1000);
        rotateServo.write(90);
        break;
      case 3:
        rotateServo.write(130);
        delay(1000);
        rotateServo.write(90);
        break;
      case 4:
        analogWrite(GUN_LEDS, 255);
        delay(1000);
        analogWrite(GUN_LEDS, 0);
        break;
      case 5:
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
        break;
      case 6:
        myDFPlayer.playFolder(1, random(1, 9));
        break;
    }
    diagnoseAction = -1;
  }
}
#endif