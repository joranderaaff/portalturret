#ifndef PT_STATE_BEHAVIOUR
#define PT_STATE_BEHAVIOUR

#include <Arduino.h>
#include "Servos.h"
#include "Settings.h"

enum class TurretMode { Automatic = 0, Manual = 1, Idle = 2 };

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

TurretMode currentTurretMode = TurretMode::Automatic;
TurretState currentState = TurretState::Idle;
ManualState currentManualState = ManualState::Idle;

unsigned long detectTime = 0;
unsigned long undetectTime = 0;
unsigned long previousTime = 0;
unsigned long stateStartTime = 0;
unsigned long lastMovementTime = 0;
bool diagnoseMode;
int diagnoseAction;

void setState(TurretState nextState) {
  // Stop the Wing Servos just in case;
  servos.SetWingAngle(settings.idleAngle);

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
  // Stop the Wing Servos just in case;
  servos.SetWingAngle(settings.idleAngle);

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

      bool motionDetected = sensors.IsDetectingMotion();

      float zMovement = (sensors.smoothZ / MEASUREMENTS *
                         SENSORS_GRAVITY_STANDARD * ADXL345_MG2G_MULTIPLIER);
      float xMovement = (sensors.smoothX / MEASUREMENTS *
                         SENSORS_GRAVITY_STANDARD * ADXL345_MG2G_MULTIPLIER);
      float yMovement = (sensors.smoothY / MEASUREMENTS *
                         SENSORS_GRAVITY_STANDARD * ADXL345_MG2G_MULTIPLIER);

      bool pickedUp = sensors.accelerometerBuffered &&
                      (xMovement < -settings.panicTreshold ||
                       xMovement > settings.panicTreshold ||
                       yMovement < -settings.panicTreshold ||
                       yMovement > settings.panicTreshold);
      bool movedAround = sensors.accelerometerBuffered &&
                         (xMovement < -settings.restTreshold ||
                          xMovement > settings.restTreshold ||
                          yMovement < -settings.restTreshold ||
                          yMovement > settings.restTreshold);
      bool onItsSide = sensors.accelerometerBuffered &&
                       (zMovement < settings.tippedOverTreshold);

      if (movedAround) {
        lastMovementTime = millis();
      }

      if (pickedUp && currentState != TurretState::PickedUp &&
          currentState != TurretState::Shutdown &&
          currentState != TurretState::Rebooting) {
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
          if (sensors.WingsAreOpen()) {
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
  } else if(currentTurretMode == TurretMode::Manual) {
    switch (diagnoseAction) {
    case 0:
      servos.SetWingAngle(settings.idleAngle -
                          settings.wingRotateDirection * 90);
      delay(250);
      servos.SetWingAngle(settings.idleAngle);
      break;
    case 1:
      servos.SetWingAngle(settings.idleAngle +
                          settings.wingRotateDirection * 90);
      delay(250);
      servos.SetWingAngle(settings.idleAngle);
      break;
    case 2:
      servos.SetRotateAngle(50);
      delay(1000);
      servos.SetRotateAngle(90);
      break;
    case 3:
      servos.SetRotateAngle(130);
      delay(1000);
      servos.SetRotateAngle(90);
      break;
    case 4:
      leds.ToggleGUNLEDs(true);
      delay(1000);
      leds.ToggleGUNLEDs(false);
      break;
    case 5:
      leds.TestLEDs();
      break;
    case 6:
      audio.PlaySound(1, random(1, 9));
      break;
    }
    diagnoseAction = -1;
  }
}
#endif