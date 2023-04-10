/*
   Read audio data from MSGEQ7 chip and send as UDP data in the same format as used in https://github.com/atuline/WLED
   This allows for sending FFT data from either ESP8266 or ESP32 to other devices running the audio responsive WLED
*/

#if defined(ESP8266) // ESP8266
#include <ESP8266WiFi.h>
#else // ESP32
#include <WiFi.h>
#endif
#include <ArduinoOTA.h>
#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync

#include "wifi.h"
// Create file with the following
// *************************************************************************
// #define SECRET_SSID "";  /* Replace with your SSID */
// #define SECRET_PSK "";   /* Replace with your WPA2 passphrase */
// *************************************************************************
const char ssid[] = SECRET_SSID;
const char passphrase[] = SECRET_PSK;


// MSGEQ7
#include "MSGEQ7.h"
#define pinAnalogLeft A0 // "VP"
#define pinAnalogRight A0
#define pinReset 22
#define pinStrobe 19
#define MSGEQ7_INTERVAL ReadsPerSecond(50)
#define MSGEQ7_SMOOTH false

CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinAnalogLeft, pinAnalogRight> MSGEQ7;

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

  // This will set the IC ready for reading
  MSGEQ7.begin();

  ArduinoOTA.setHostname("WLED-MSGEQ7");
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  
  sync.begin();
}

void loop() {
  ArduinoOTA.handle();
  // Analyze without delay every interval
  bool newReading = MSGEQ7.read(MSGEQ7_INTERVAL);
  if (newReading) {
    audioSyncPacket transmitData;

    for (int b = 0; b < NUM_GEQ_CHANNELS; b+) {
      int val = MSGEQ7.get(map(b, 0, (NUM_GEQ_CHANNELS - 1), 0, 6));
      val = mapNoise(val);
      Serial.printf("%u ", val);
      transmitData.fftResult[b] = val;
    }
    Serial.println();

    int v = map(MSGEQ7.getVolume(), 0, MSGEQ7_OUT_MAX, 0, 1023); // TODO: not sure this is right
    transmitData.sampleRaw = v; // Current sample
    
    sync.send(transmitData);
  }

}
