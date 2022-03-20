void startWebServer()
{

  server.serveStatic("/", LittleFS, "/");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "index.html", String(), false, processor);
  });

  server.onNotFound([](AsyncWebServerRequest * request)
  {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404);
    }
  });


  server.on("/set_mode", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    if (request->hasParam("mode", true))
    {
      AsyncWebParameter *modeParam = request->getParam("mode", true);
      currentTurretMode = (TurretMode) modeParam->value().toInt();
      currentRotateAngle = 90;
      request->send(200, "text/html", "State set");
    } else {
      request->send(200, "text/html", "Failed to set mode");
    }
  });

  server.on("/set_state", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    if (request->hasParam("state", true))
    {
      AsyncWebParameter *stateParam = request->getParam("state", true);
      int state = stateParam->value().toInt();
      setState((TurretState) state);
      request->send(200, "text/html", "State set");
    }
    else
    {
      request->send(200, "text/html", "No state sent");
    }
  });

  server.on("/set_open", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (currentTurretMode == TurretMode::Manual) {
      if (request->hasParam("open", true))
      {
        AsyncWebParameter *openParam = request->getParam("open", true);
        if(openParam->value().toInt() == 1) {
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

  server.on("/set_angle", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    Serial.println("Set angle request");
    if (request->hasParam("angle", true))
    {
      AsyncWebParameter *angleParam = request->getParam("angle", true);
      AsyncWebParameter *servoParam = request->getParam("servo", true);
      int angle = angleParam->value().toInt();
      int servo = servoParam->value().toInt();
      currentMoveSpeed = angle;
      if (servo == 0) {
        pwm.setPWM(WING_SERVO, 0, map(STATIONARY_ANGLE + angle, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
      } else {
        pwm.setPWM(ROTATE_SERVO, 0, map(90 + angle, 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
      }

      request->send(200, "text/html", "Angle set");
    }
    else
    {
      request->send(200, "text/html", "No angle sent");
    }
  });

  server.on("/reset_wifi", HTTP_GET, [](AsyncWebServerRequest * request) {
    wifiManager.resetSettings();
    WiFi.disconnect();
    request->send(200, "text/html", "Wifi reset");
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}

String processor(const String &var)
{
  if (var == "IP")
    return WiFi.localIP().toString();
  return String();
}
