/*
   Read audio data from MSGEQ7 chip and send as UDP data in the same format as used in https://github.com/atuline/WLED
   This allows for sending FFT data from either ESP8266 or ESP32 to other devices running the audio responsive WLED
*/

#if defined(ESP8266) // ESP8266
#include <ESP8266WiFi.h>
#else // ESP32
#include <WiFi.h>
#endif
#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync

#include "wifi.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";  /* Replace with your SSID */
// #define SECRET_PSK "";   /* Replace with your WPA2 passphrase */
// *************************************************************************
const char ssid[] = SECRET_SSID;
const char passphrase[] = SECRET_PSK;

WLEDSync sync;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passphrase);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());
  
  sync.begin();

  Serial.println("Setup complete");
}

void loop() {

  if (sync.read()) {
    for (int b = 0; b < NUM_GEQ_CHANNELS; b++) {
      uint8_t val = sync.fftResult[b];
      Serial.printf("%u ", val);
    }
    Serial.println();

  }
}
