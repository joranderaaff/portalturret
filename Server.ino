#include "index.html.gz.h";

void StartWebServer() {
  Serial.println("Start webserver");

  //server.serveStatic("/", LittleFS, "/");
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(LittleFS, "index.html", String(), false, processor);
  // });

  ArRequestHandlerFunction siteHandler = [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  };

  server.on("/", HTTP_GET, siteHandler);
  server.on("/diagnose", HTTP_GET, siteHandler);
  server.on("/setup", HTTP_GET, siteHandler);
  server.on("/settings", HTTP_GET, siteHandler);

  server.onNotFound(siteHandler);

  ArRequestHandlerFunction settingsHandler = [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("pw", true)) {
      settings.wifiSSID = request->getParam("ssid", true)->value();
      settings.wifiPassword = request->getParam("pw", true)->value();
    }
    if (request->hasParam("audioVolume", true)) {
      settings.audioVolume = request->getParam("audioVolume", true)->value().toInt() & 0xFF;
    }
    if (request->hasParam("centerAngle", true)) {
      settings.centerAngle = request->getParam("centerAngle", true)->value().toInt();
    }
    if (request->hasParam("idleAngle", true)) {
      settings.idleAngle = request->getParam("idleAngle", true)->value().toInt();
    }
    if (request->hasParam("wingPin", true)) {
      settings.wingPin = request->getParam("wingPin", true)->value().toInt();
    }
    if (request->hasParam("rotatePin", true)) {
      settings.rotatePin = request->getParam("rotatePin", true)->value().toInt();
    }
    if (request->hasParam("openDuration", true)) {
      settings.openDuration = request->getParam("openDuration", true)->value().toInt();
    }
    if (request->hasParam("maxRotation", true)) {
      settings.maxRotation = request->getParam("maxRotation", true)->value().toInt();
    }
    if (request->hasParam("panicTreshold", true)) {
      settings.panicTreshold = request->getParam("panicTreshold", true)->value().toFloat();
    }
    if (request->hasParam("restTreshold", true)) {
      settings.restTreshold = request->getParam("restTreshold", true)->value().toFloat();
    }
    if (request->hasParam("tippedOverTreshold", true)) {
      settings.tippedOverTreshold = request->getParam("tippedOverTreshold", true)->value().toFloat();
    }

    saveSettings(settings);
    request->send(200, "text/html", "ok");
    requestReboot();
  };

  server.on("/setup", HTTP_POST, settingsHandler);
  server.on("/settings", HTTP_POST, settingsHandler);

  server.on("/get_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", settingsToJSON(settings));
  });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
      WiFi.scanNetworks(true);
    } else if (n) {
      for (int i = 0; i < n; ++i) {
        if (i) json += ",";
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

  server.on("/set_mode", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("mode", true)) {
      AsyncWebParameter *modeParam = request->getParam("mode", true);
      currentTurretMode = (TurretMode)modeParam->value().toInt();
      currentRotateAngle = 90;
      request->send(200, "text/html", currentTurretMode == TurretMode::Automatic ? "Automatic" : "Manual");
    } else {
      request->send(200, "text/html", "Failed to set mode");
    }
  });

  server.on("/set_state", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("state", true)) {
      AsyncWebParameter *stateParam = request->getParam("state", true);
      int state = stateParam->value().toInt();
      setState((TurretState)state);
      request->send(200, "text/html", "State set");
    } else {
      request->send(200, "text/html", "No state sent");
    }
  });

  server.on("/diagnose", HTTP_POST, [](AsyncWebServerRequest *request) {
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

  server.on("/set_angle", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("angle", true)) {
      AsyncWebParameter *angleParam = request->getParam("angle", true);
      AsyncWebParameter *servoParam = request->getParam("servo", true);
      int angle = angleParam->value().toInt();
      int servo = servoParam->value().toInt();
      currentMoveSpeed = angle;
      if (servo == 0) {
        wingServo.write(settings.idleAngle + angle);
      } else {
        rotateServo.write(90 + angle);
      }
      request->send(200, "text/html", "Angle set");
    } else {
      request->send(200, "text/html", "No angle sent");
    }
  });

  server.on("/reset_wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    WiFi.disconnect();
    settings.wifiSSID = "";
    settings.wifiPassword = "";
    saveSettings(settings);
    request->send(200, "text/html", "Wifi reset");
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  Serial.println("server.begin()");
  server.begin();
}

void requestReboot() {
  while (true) {
    int i = 0;
  }
}

String processor(const String &var) {
  if (var == "IP")
    return WiFi.localIP().toString();
  return String();
}
