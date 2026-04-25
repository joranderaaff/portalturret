#ifdef HARDWARE_V3
#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include "config.h"

#include <WiFiClientSecure.h>

#define PREFIX "https://joranderaaff.nl/portal-sentry/audio/english"

void downloadFile(const char* urlPath, const char* filePath) {
  static WiFiClientSecure sslclient;
  static bool initialized = false;
  if (!initialized) {
    sslclient.setInsecure();
    initialized = true;
  }

  String fullUrl = String(PREFIX) + String(urlPath);
  Serial.print("Requesting: ");
  Serial.println(fullUrl);

  // Start the request to download the file
  HTTPClient http;
  http.begin(sslclient, fullUrl);
  int statusCode = http.GET();

  if (statusCode == 200) {
    Serial.println("HTTP request successful, downloading file...");

    // Open file for writing
    File file = LittleFS.open(filePath, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      http.end();
      return;
    }

    int len = http.getSize();
    // create buffer for read
    uint8_t buff[128] = { 0 };

    // get tcp stream
    WiFiClient * stream = http.getStreamPtr();

    // read all data from server
    while(http.connected() && (len > 0 || len == -1)) {
      // get available data size
      size_t size = stream->available();

      if(size) {
        // read up to 128 byte
        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

        // write it to Serial
        file.write(buff, c);

        if(len > 0) {
          len -= c;
        }
      }
      delay(1);
    }

    file.close();
  }

  http.end();
}
#endif

