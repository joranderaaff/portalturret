#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <FastLED.h>
#include "LittleFS.h"
#include <ESP8266mDNS.h> // Include the mDNS library
#include <ESP8266httpUpdate.h>
#include <ArduinoOTA.h>
#include <AceRoutine.h>
#include <AsyncElegantOTA.h>
#include <ESPAsyncWiFiManager.h>

using namespace ace_routine;

#define CENTER_LED 0
#define GUN_RIGHT 13
#define GUN_LEFT 12
#define ROTATE_SERVO 8
#define WING_SERVO 7
#define CENTER_ANGLE 90
#define STATIONARY_ANGLE 110
#define NUM_LEDS 8

CRGB leds[NUM_LEDS];

enum class TurretMode {
  Automatic = 0,
  Manual = 1
};

SoftwareSerial mySoftwareSerial(D5, D6); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define FREQ 50     //one clock is 20 ms
#define FREQ_MINIMUM 205 //1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410 //2ms is 2/20, of 4096
int currentMoveSpeed = 0;

AsyncWebServer server = AsyncWebServer(80);
DNSServer dns;
WebSocketsServer webSocket = WebSocketsServer(81);
AsyncWiFiManager wifiManager(&server, &dns);

bool websocketStarted;
unsigned long nextWebSocketUpdateTime = 0;

bool wingsOpen;
bool wasOpen;
bool alarm;
bool shouldUpdate = false;

bool isOpen() {
  return digitalRead(D0) == HIGH;
}
bool isPlayingAudio() {
  return analogRead(A0) < 512;
}

#include "Accelerometer.h"
#include "Routines.h"
#include "StateBehaviour.h"

bool isConnected;

void setup()
{
  pwm.begin();
  pwm.setPWMFreq(FREQ);
  pwm.setPWM(ROTATE_SERVO, 0, map(90, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  pwm.setPWM(CENTER_LED, 4096, 0);

  Serial.begin(19200);

  Serial.println("trying wifiManager");
  wifiManager.setDebugOutput(true);
  isConnected = wifiManager.autoConnect("Portal Turret");

  //if (isConnected) {

    FastLED.addLeds<WS2812, D8, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(84);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(0, 0, 0);
      FastLED.show();
    }

    currentState = TurretState::Idle;
    currentManualState = ManualState::Idle;
    currentTurretMode = TurretMode::Automatic;

    pinMode(D0, INPUT);
    pinMode(D7, INPUT);

    wasOpen = isOpen();

    preloader(0);

    Serial.println(WiFi.localIP());

    AsyncElegantOTA.begin(&server);

    MDNS.begin("portal");
    MDNS.addService("http", "tcp", 80);

    // Initialize LittleFS
    if (!LittleFS.begin())
    {
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
    }
    preloader(1);

    startWebServer();
    preloader(2);

    startWebSocket();
    preloader(3);

    setupAccelerometer();

    preloader(4);

    mySoftwareSerial.begin(9600);
    delay(200);
    if (!myDFPlayer.begin(mySoftwareSerial))
    {
      //while (true);
    }

    delay(1000);
    myDFPlayer.volume(15);

    preloader(5);

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_FS
        type = "filesystem";
      }

      // NOTE: if updating FS this would be the place to unmount FS using FS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();

    preloader(6);

    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB(255, 0, 0);
      FastLED.show();
    }

    previousTime = millis();
  //}
}

void preloader(uint8_t led) {
  FastLED.clear();
  leds[led] = CRGB(255, 0, 0);
  FastLED.show();
}

void loop()
{
  //if (!isConnected) return;

  wingsOpen = isOpen();

  ArduinoOTA.handle();
  webSocket.loop();
  MDNS.update();

  stateBehaviour();

  if (currentMoveSpeed > 0 && wasOpen && !wingsOpen) {
    currentMoveSpeed = 0;
    pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
  }

  wasOpen = wingsOpen;

  if (websocketStarted && millis() > nextWebSocketUpdateTime)
  {

    nextWebSocketUpdateTime = millis() + 30;
    int a = analogRead(A0);

    updateAccelerometer();

    int16_t x = smoothX / measurements;
    int16_t y = smoothY / measurements;
    int16_t z = smoothZ / measurements;

    uint8_t values[] = {
      (x >> 8),
      (x & 0xFF),
      (y >> 8),
      (y & 0xFF),
      (z >> 8),
      (z & 0xFF),
      (!isOpen() ? 1 : 0),
      (digitalRead(D7) == HIGH ? 1 : 0),
      ((uint8_t)(a >> 8)) & 0xFF,
      ((uint8_t)a) & 0xFF,
      (uint8_t) currentState,
      (isPlayingAudio() ? 1 : 0),
    };
    webSocket.broadcastBIN(values, 12);
  }
}
