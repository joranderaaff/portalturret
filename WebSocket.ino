void startWebSocket()
{
  // Start a WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent); // if there's an incomming websocket message, go to function 'webSocketEvent'
  websocketStarted = true;
  Serial.println("WebSocket server started.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{
  uint8_t transferType;

  // When a WebSocket message is received
  switch (type)
  {
  case WStype_ERROR:
    Serial.printf("Error: [%f]", payload);
    break;
  case WStype_BIN:
    switch(payload[0]) {
      case 0:
        if(isOpen()) {
          pwm.setPWM(ROTATE_SERVO, 0, map(payload[1], 0, 180, FREQ_MINIMUM, FREQ_MAXIMUM));
        }
        break;
      case 1:
        setManualState(ManualState::Firing);
        break;
      case 2:
        if(payload[1] == 0) {
          currentRotateDirection = 0;
        } else if(payload[1] == 1) {
          currentRotateDirection = 1;
        } else if(payload[1] == 2) {
          currentRotateDirection = -1;
        }
        break;
    }
    break;
  case WStype_DISCONNECTED: // if the websocket is disconnected
    break;
  case WStype_CONNECTED: // if a new websocket connection is established
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    break;
  }
}
