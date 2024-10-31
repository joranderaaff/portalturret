#ifdef HARDWARE_V3
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
WiFiClient* wifiClient;

#define PREFIX "https://joranderaaff.nl/portal-sentry/audio/english"

void downloadFile(const char* urlPath, const char* filePath) {
  String fullUrl = String(PREFIX) + String(urlPath);
  if (wifiClient == NULL) {
    WiFiClientSecure* sslclient = new WiFiClientSecure();
    sslclient->setInsecure();
    wifiClient = sslclient;
  }
  
  Serial.print("Requesting: ");
  Serial.println(fullUrl);

  // Start the request to download the file
  HTTPClient http;
  http.begin(*wifiClient, fullUrl);
  int statusCode = http.GET();

  if (statusCode == 200) {
    Serial.println("HTTP request successful, downloading file...");

    // Open file for writing
    File file = LittleFS.open(filePath, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
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
  }
}
#endif