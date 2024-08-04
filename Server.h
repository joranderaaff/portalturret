#ifndef PT_SERVER
#define PT_SERVER

#include <ArduinoOTA.h>
#include <AsyncElegantOTA.h>
#include <ESP8266httpUpdate.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

#include "Settings.h"
#include "StateBehaviour.h"
#include "index.html.gz.h"

bool useCaptive = false;
const byte DNS_PORT = 53;
DNSServer dnsServer;

AsyncWebServer server = AsyncWebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
bool websocketStarted;
unsigned long nextWebSocketUpdateTime = 0;

void InitServer() {
  Settings settings = LoadSettings();
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

    // Preemtive scan of networks, just in case.
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

  AsyncElegantOTA.begin(&server);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    // Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    // Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    // Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      // Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      // Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      // Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      // Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      // Serial.println("End Failed");
    }
  });
  ArduinoOTA.setHostname("turret");
  ArduinoOTA.begin();
}

void requestReboot() {
  while (true) {
    int i = 0;
  }
}

void StartWebServer() {
  Settings settings = LoadSettings();

  Serial.println("Start webserver");

  // server.serveStatic("/", LittleFS, "/");
  //  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //    request->send(LittleFS, "index.html", String(), false, processor);
  //  });

  ArRequestHandlerFunction siteHandler = [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
      200, "text/html", index_html_gz, index_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  };

  server.on("/", HTTP_GET, siteHandler);
  server.on("/diagnose", HTTP_GET, siteHandler);
  server.on("/setup", HTTP_GET, siteHandler);
  server.on("/settings", HTTP_GET, siteHandler);

  server.onNotFound(siteHandler);

  ArRequestHandlerFunction settingsHandler =
    [&](AsyncWebServerRequest *request) {
      if (request->hasParam("ssid", true) && request->hasParam("pw", true)) {
        settings.wifiSSID = request->getParam("ssid", true)->value();
        settings.wifiPassword = request->getParam("pw", true)->value();
      }
      if (request->hasParam("audioVolume", true)) {
        settings.audioVolume =
          request->getParam("audioVolume", true)->value().toInt() & 0xFF;
      }
      if (request->hasParam("centerAngle", true)) {
        settings.centerAngle =
          request->getParam("centerAngle", true)->value().toInt();
      }
      if (request->hasParam("idleAngle", true)) {
        settings.idleAngle =
          request->getParam("idleAngle", true)->value().toInt();
      }
      if (request->hasParam("wingRotateDirection", true)) {
        settings.wingRotateDirection =
          request->getParam("wingRotateDirection", true)->value().toInt();
      }
      if (request->hasParam("wingPin", true)) {
        settings.wingPin =
          request->getParam("wingPin", true)->value().toInt();
      }
      if (request->hasParam("rotatePin", true)) {
        settings.rotatePin =
          request->getParam("rotatePin", true)->value().toInt();
      }
      if (request->hasParam("openDuration", true)) {
        settings.openDuration =
          request->getParam("openDuration", true)->value().toInt();
      }
      if (request->hasParam("maxRotation", true)) {
        settings.maxRotation =
          request->getParam("maxRotation", true)->value().toInt();
      }
      if (request->hasParam("panicTreshold", true)) {
        settings.panicTreshold =
          request->getParam("panicTreshold", true)->value().toFloat();
      }
      if (request->hasParam("restTreshold", true)) {
        settings.restTreshold =
          request->getParam("restTreshold", true)->value().toFloat();
      }
      if (request->hasParam("tippedOverTreshold", true)) {
        settings.tippedOverTreshold =
          request->getParam("tippedOverTreshold", true)->value().toFloat();
      }

      SaveSettings(settings);
      request->send(200, "text/html", "ok");
      requestReboot();
    };

  server.on("/setup", HTTP_POST, settingsHandler);
  server.on("/settings", HTTP_POST, settingsHandler);

  server.on("/get_settings", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "application/json", SettingsToJSON(settings));
  });

  server.on("/scan", HTTP_GET, [&](AsyncWebServerRequest *request) {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
      WiFi.scanNetworks(true);
    } else if (n) {
      for (int i = 0; i < n; ++i) {
        if (i)
          json += ",";
        json += "{";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
        json += ",\"channel\":" + String(WiFi.channel(i));
        json += ",\"secure\":" + String(WiFi.encryptionType(i));
        json += ",\"hidden\":" + String(WiFi.isHidden(i) ? "true" : "false");
        json += "}";
      }
      WiFi.scanDelete();
      if (WiFi.scanComplete() == -2) {
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  server.on("/set_mode", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (request->hasParam("mode", true)) {
      AsyncWebParameter *modeParam = request->getParam("mode", true);
      currentTurretMode = (TurretMode)modeParam->value().toInt();
      //currentRotateAngle = 90;
      request->send(200, "text/html",
                    currentTurretMode == TurretMode::Automatic ? "Automatic"
                                                               : "Manual");
    } else {
      request->send(200, "text/html", "Failed to set mode");
    }
  });

  server.on("/set_state", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (request->hasParam("state", true)) {
      AsyncWebParameter *stateParam = request->getParam("state", true);
      int state = stateParam->value().toInt();
      setState((TurretState)state);
      request->send(200, "text/html", "State set");
    } else {
      request->send(200, "text/html", "No state sent");
    }
  });

  server.on("/diagnose", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (request->hasParam("action", true)) {
      AsyncWebParameter *stateParam = request->getParam("action", true);
      diagnoseMode = true;
      diagnoseAction = stateParam->value().toInt();
      request->send(200, "text/html", "Diagnose");
    } else {
      request->send(200, "text/html", "No Action Sent");
    }
  });

  server.on("/set_open", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (currentTurretMode == TurretMode::Manual) {
      if (request->hasParam("open", true)) {
        AsyncWebParameter *openParam = request->getParam("open", true);
        if (openParam->value().toInt() == 1) {
          setManualState(ManualState::Opening);
          request->send(200, "text/html", "Opening");
        } else {
          setManualState(ManualState::Closing);
          request->send(200, "text/html", "Closing");
        }
      } else {
        request->send(200, "text/html", "No state sent");
      }
    } else {
      request->send(200, "text/html", "Not in Manual mode");
    }
  });

  server.on("/set_angle", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (request->hasParam("angle", true)) {
      AsyncWebParameter *angleParam = request->getParam("angle", true);
      AsyncWebParameter *servoParam = request->getParam("servo", true);
      int angle = angleParam->value().toInt();
      int servo = servoParam->value().toInt();
      currentMoveSpeed = angle;
      if (servo == 0) {
        wingServo.write(settings.idleAngle + settings.wingRotateDirection * angle);
      } else {
        rotateServo.write(90 + angle);
      }
      request->send(200, "text/html", "Angle set");
    } else {
      request->send(200, "text/html", "No angle sent");
    }
  });

  server.on("/reset_wifi", HTTP_GET, [&](AsyncWebServerRequest *request) {
    WiFi.disconnect();
    settings.wifiSSID = "";
    settings.wifiPassword = "";
    SaveSettings(settings);
    request->send(200, "text/html", "Wifi reset");
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  Serial.println("server.begin()");
  server.begin();
}

void UpdateServer() {
  webSocket.loop();
  ArduinoOTA.handle();

  if (useCaptive) {
    dnsServer.processNextRequest();
  }

  if (websocketStarted && millis() > nextWebSocketUpdateTime) {

    nextWebSocketUpdateTime = millis() + 30;
    int a = analogRead(A0);

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

String processor(const String &var) {
  if (var == "IP")
    return WiFi.localIP().toString();
  return String();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t lenght) {
  uint8_t transferType;

  // When a WebSocket message is received
  switch (type) {
    case WStype_ERROR:
      // Serial.printf("Error: [%f]", payload);
      break;
    case WStype_BIN:
      switch (payload[0]) {
        case 0:
          if (isOpen()) {
            rotateServo.write(payload[1]);
          }
          break;
        case 1:
          setManualState(ManualState::Firing);
          break;
        case 2:
          if (payload[1] == 0) {
            //currentRotateDirection = 0;
          } else if (payload[1] == 1) {
            //currentRotateDirection = 1;
          } else if (payload[1] == 2) {
            //currentRotateDirection = -1;
          }
          break;
      }
      break;
    case WStype_DISCONNECTED:  // if the websocket is disconnected
      break;
    case WStype_CONNECTED:  // if a new websocket connection is established
      IPAddress ip = webSocket.remoteIP(num);
      // Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
      // ip[1], ip[2], ip[3], payload);
      break;
  }
}

void StartWebSocket() {
  // Start a WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);  // if there's an incomming websocket
                                      // message, go to function 'webSocketEvent'
  websocketStarted = true;
  // Serial.println("WebSocket server started.");
}

#endif