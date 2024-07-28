//General
#include "Arduino.h"

//Network
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266httpUpdate.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include <AsyncElegantOTA.h>

//Storage
#include "LittleFS.h"

//Routines
#include <AceRoutine.h>

//Devices
#include <FastLED.h>
#include <Servo.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#include "Settings.h"

using namespace ace_routine;

//Tweak these according to servo speed
#define CLOSE_STOP_DELAY 100

#define USE_AUDIO 1

#define BUSY D0
#define CENTER_LED D3
#define GUN_LEDS D4
#define RING_LEDS D8
#define WING_SWITCH D5
#define PID A0

#define NUM_LEDS 8

#define FREQ 50           //one clock is 20 ms
#define FREQ_MINIMUM 205  //1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410  //2ms is 2/20, of 4096

bool useCaptive = false;
const byte DNS_PORT = 53;
DNSServer dnsServer;

CRGB leds[NUM_LEDS];

enum class TurretMode {
  Automatic = 0,
  Manual = 1
};

Servo wingServo;
Servo rotateServo;

Settings settings;

SoftwareSerial mySoftwareSerial(RX, TX);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;
int currentMoveSpeed = 0;

AsyncWebServer server = AsyncWebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

bool websocketStarted;
bool diagnoseMode = false;
unsigned long nextWebSocketUpdateTime = 0;
unsigned long nextLEDUpdateTime = 0;

bool wingsOpen;
bool wasOpen;
bool fullyOpened;
bool alarm;
bool needsSetup;
bool shouldUpdate = false;
int diagnoseAction = -1;

bool isOpen() {
  return digitalRead(WING_SWITCH) == HIGH;
}

//For some reason we need to cache this value, as checking it every loop causes the webserver to freeze. //https://github.com/me-no-dev/ESPAsyncWebServer/issues/944
bool isDetectingMotionCached = false;
unsigned long lastMotionCheck = 0;
bool isDetectingMotion() {
  unsigned long curMillis = millis();
  if (curMillis > lastMotionCheck + 50) {
    isDetectingMotionCached = analogRead(A0) > 512;
    lastMotionCheck = curMillis;
  }
  return isDetectingMotionCached;
}

bool isPlayingAudio() {
  return digitalRead(BUSY) == LOW;
}

#include "Accelerometer.h"
#include "Routines.h"
#include "StateBehaviour.h"

bool isConnected;

void UpdateLEDPreloader() {
  int t = floor(millis() / 10);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB((i + t) % 8 == 0 ? 255 : 0, 0, 0);
    FastLED.show();
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    while (true) {}
    return;
  }

  Serial.println("LittleFS Initialized, loading Settings");

  settings = loadSettings();

  Serial.println("Settings Volume:");
  Serial.println(settings.audioVolume);

  Serial.println("Setting up WLED");

  FastLED.addLeds<WS2812, RING_LEDS, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(84);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
    FastLED.show();
  }

  pinMode(GUN_LEDS, OUTPUT);
  pinMode(BUSY, INPUT);
  pinMode(WING_SWITCH, INPUT_PULLUP);

  wingServo.attach(settings.wingPin);
  rotateServo.attach(settings.rotatePin);

  Serial.println("Closing wings");

  rotateServo.write(settings.centerAngle);
  delay(250);
  fullyOpened = false;
  unsigned long closingStartTime = millis();
  wingServo.write(settings.idleAngle + 90);
  while (millis() < closingStartTime + 3000 && isOpen()) {
    delay(10);
  }
  delay(CLOSE_STOP_DELAY);
  wingServo.write(settings.idleAngle);

  WiFi.hostname("turret");
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.wifiSSID, settings.wifiPassword);

  unsigned long m = millis();
  Serial.println("Starting wifi");
  while (WiFi.status() != WL_CONNECTED) {
    UpdateLEDPreloader();
    Serial.print(".");
    delay(50);
    if (m + 10000 < millis()) {
      WiFi.disconnect();
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    needsSetup = true;

    //Preemtive scan of networks, just in case.
    WiFi.scanNetworks(true);

    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    useCaptive = true;

    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("Portal Turret");
    dnsServer.start(DNS_PORT, "*", local_IP);

  } else {
    Serial.println("Connected");
    Serial.println(WiFi.localIP());
  }

  UpdateLEDPreloader();

  currentState = TurretState::Idle;
  currentManualState = ManualState::Idle;
  currentTurretMode = TurretMode::Automatic;

  wasOpen = isOpen();

  AsyncElegantOTA.begin(&server);

  UpdateLEDPreloader();

  StartWebServer();
  StartWebSocket();
  SetupAccelerometer();

  UpdateLEDPreloader();

  Serial.end();

  UpdateLEDPreloader();

#ifdef USE_AUDIO
  mySoftwareSerial.begin(9600);
  myDFPlayer.begin(mySoftwareSerial);
  delay(100);
  myDFPlayer.volume(settings.audioVolume);
#endif

  UpdateLEDPreloader();

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    //Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
    }
  });
  ArduinoOTA.setHostname("turret");
  ArduinoOTA.begin();

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 0, 0);
    FastLED.show();
  }

  previousTime = millis();
}

void loop() {
  wingsOpen = isOpen();

  ArduinoOTA.handle();
  webSocket.loop();
  if(useCaptive) {
    dnsServer.processNextRequest();
  }

  if (!diagnoseMode) {
    stateBehaviour();
  } else {
    switch (diagnoseAction) {
      case 0:
        wingServo.write(settings.idleAngle - 90);
        delay(250);
        wingServo.write(settings.idleAngle);
        break;
      case 1:
        wingServo.write(settings.idleAngle + 90);
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

  if (currentMoveSpeed > 0 && wasOpen && !wingsOpen) {
    currentMoveSpeed = 0;
    delay(CLOSE_STOP_DELAY);
    wingServo.write(settings.idleAngle);
  }

  wasOpen = wingsOpen;

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

  if (websocketStarted && millis() > nextWebSocketUpdateTime) {

    nextWebSocketUpdateTime = millis() + 30;
    int a = analogRead(A0);

    UpdateAccelerometer();

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
      (isDetectingMotion() ? 1 : 0),
      ((uint8_t)(a >> 8)) & 0xFF,
      ((uint8_t)a) & 0xFF,
      (uint8_t)currentState,
      (isPlayingAudio() ? 1 : 0),
    };
    webSocket.broadcastBIN(values, 12);
  }
}
