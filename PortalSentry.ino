//General
#include "Arduino.h"

//Network
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> // Include the mDNS library
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

using namespace ace_routine;

//Tweak these according to servo speed
#define OPEN_DURATION 2300
#define CLOSE_STOP_DELAY 200


#define WIFI_CRED_FILE "settings.txt"

#define USE_AUDIO 1

#define BUSY D0
#define CENTER_LED D3
#define GUN_LEDS D4
#define RING_LEDS D8
#define SERVO_A D6
#define SERVO_B D7
#define WING_SWITCH D5
#define PID A0

#define CENTER_ANGLE 90
#define STATIONARY_ANGLE 90
#define NUM_LEDS 8

CRGB leds[NUM_LEDS];

enum class TurretMode {
  Automatic = 0,
  Manual = 1
};

Servo wingServo;
Servo rotateServo;

SoftwareSerial mySoftwareSerial(RX, TX); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

#define FREQ 50     //one clock is 20 ms
#define FREQ_MINIMUM 205 //1ms is 1/20, of 4096
#define FREQ_MAXIMUM 410 //2ms is 2/20, of 4096
int currentMoveSpeed = 0;

AsyncWebServer server = AsyncWebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

bool websocketStarted;
unsigned long nextWebSocketUpdateTime = 0;

bool wingsOpen;
bool wasOpen;
bool fullyOpened;
bool alarm;
bool needsSetup;
bool myDFPlayerSetup = false;
bool shouldUpdate = false;

bool isOpen() {
  return digitalRead(WING_SWITCH) == HIGH;
}

//For some reason we need to cache this value, as checking it every loop causes the webserver to freeze.
//https://github.com/me-no-dev/ESPAsyncWebServer/issues/944
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

void setup()
{
  Serial.begin(9600);
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    while (true) { }
    return;
  }
  Serial.println("LittleFS Initialized");

  FastLED.addLeds<WS2812, RING_LEDS, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(84);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
    FastLED.show();
  }

  pinMode(GUN_LEDS, OUTPUT);

  wingServo.attach(SERVO_A);
  rotateServo.attach(SERVO_B);

  pinMode(BUSY, INPUT);
  pinMode(WING_SWITCH, INPUT_PULLUP);

  File wifiCreds = LittleFS.open(WIFI_CRED_FILE, "r");
  String esid = wifiCreds.readStringUntil('\r'); wifiCreds.read();
  String epass = wifiCreds.readStringUntil('\r'); wifiCreds.read();
  
  WiFi.hostname("turret");
  WiFi.mode(WIFI_STA);
  WiFi.begin(esid, epass);

  unsigned long m = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Starting wifi");
    UpdateLEDPreloader();
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

    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("Portal Turret");

  } else {
    Serial.println("Connected");
    Serial.println(WiFi.localIP());
    delay(500);
  }
  
  UpdateLEDPreloader();

  currentState = TurretState::Idle;
  currentManualState = ManualState::Idle;
  currentTurretMode = TurretMode::Automatic;

  wasOpen = isOpen();

  AsyncElegantOTA.begin(&server);
  
  UpdateLEDPreloader();

  startWebServer();
  startWebSocket();
  setupAccelerometer();
  
  UpdateLEDPreloader();

  Serial.println("Begin MDNS");
  if (MDNS.begin("turret", WiFi.localIP())) {
    Serial.println("MDSN setup");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("http", "tcp", 81);
  } else {
    Serial.println("MDSN failed");
  }
  delay(200);
  Serial.end();
  
  UpdateLEDPreloader();

#ifdef USE_AUDIO
  mySoftwareSerial.begin(9600);
  delay(200);
  myDFPlayerSetup = myDFPlayer.begin(mySoftwareSerial);
  if (myDFPlayerSetup) myDFPlayer.volume(15);
#endif
  
  UpdateLEDPreloader();

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
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

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(255, 0, 0);
    FastLED.show();
  }

  previousTime = millis();
}

void preloader(uint8_t led) {
  FastLED.clear();
  leds[led] = CRGB(255, 0, 0);
  FastLED.show();
}

void loop()
{
  MDNS.update();
  //if (!isConnected) return;

  wingsOpen = isOpen();

  ArduinoOTA.handle();
  webSocket.loop();
  stateBehaviour();
  //This helps the webserver function when AceRoutines are running..?
  //delay(10);

  if (currentMoveSpeed > 0 && wasOpen && !wingsOpen) {
    currentMoveSpeed = 0;
    delay(CLOSE_STOP_DELAY);
    wingServo.write(STATIONARY_ANGLE);
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
      (isDetectingMotion() ? 1 : 0),
      ((uint8_t)(a >> 8)) & 0xFF,
      ((uint8_t)a) & 0xFF,
      (uint8_t) currentState,
      (isPlayingAudio() ? 1 : 0),
    };
    webSocket.broadcastBIN(values, 12);
  }
}
