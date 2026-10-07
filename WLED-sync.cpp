/**
 * @brief WLED Audio Sync
 * Author: Will Tatam
 * https://creativecommons.org/licenses/by/4.0/
 */

#include "WLED-sync.h"
#if defined(ESP8266) // ESP8266
#include <ESP8266WiFi.h>
#endif

#define UDPSOUND_MAX_PACKET 96 // max packet size for audiosync, with a bit of "headroom"
uint8_t fftUdpBuffer[UDPSOUND_MAX_PACKET+1] = { 0 }; // static buffer for receiving

static bool isValidUdpSyncVersion(const char *header) {
  return strncmp_P(header, PSTR(UDP_SYNC_HEADER), 6) == 0;
}
static bool isValidUdpSyncVersion_v1(const char *header) {
  return strncmp_P(header, PSTR(UDP_SYNC_HEADER_v1), 6) == 0;
}

WLEDSync::WLEDSync() {
  lastWiFiConnected = false;
}

void WLEDSync::begin() {
  initializeUdp();
  lastWiFiConnected = (WiFi.status() == WL_CONNECTED);
  lastPacketTime = 0;
}

void WLEDSync::send(audioSyncPacket transmitData) {
  #ifndef ESP8266
    fftUdp.beginMulticastPacket();
  #else
    fftUdp.beginPacketMulticast(IPAddress(239, 0, 0, 1), UDP_SYNC_PORT, WiFi.localIP());
  #endif
    strncpy_P(transmitData.header, UDP_SYNC_HEADER, 6);
    fftUdp.write(reinterpret_cast<uint8_t *>(&transmitData), sizeof(transmitData));
    fftUdp.endPacket();
    lastPacketTime = millis();
}

bool WLEDSync::read() // check & process new data. return TRUE in case that new audio data was received.
{
  // Handle WiFi state changes (reconnection/disconnection)
  handleWiFiStateChange();
  
  // Check if WiFi is connected and UDP is initialized
  if (!udpSyncConnected || WiFi.status() != WL_CONNECTED) return false;
  
  bool haveFreshData = false;
  size_t packetSize = 0;
  static uint8_t fftUdpBuffer[UDPSOUND_MAX_PACKET + 1] = { 0 };
  size_t lastValidPacketSize = 0;

  // Drain all packets, keep only the last valid one
  while (true) {
    packetSize = fftUdp.parsePacket();
    if (packetSize == 0) break; // no more packets

    if ((packetSize > 5) && (packetSize <= UDPSOUND_MAX_PACKET)) {
      fftUdp.read(fftUdpBuffer, packetSize);
      sourceIP = fftUdp.remoteIP();   // track sender of last packet
      lastValidPacketSize = packetSize;
    } else {
      #if ESP_ARDUINO_VERSION_MAJOR >= 3
      fftUdp.clear(); // discard invalid packets (too small or too big)
      #else
      fftUdp.flush(); // discard invalid packets (too small or too big)
      #endif
    }
  }

  if (lastValidPacketSize > 0) {
    //DEBUGSR_PRINTLN("Received UDP Sync Packet");
    if (lastValidPacketSize == sizeof(audioSyncPacket) &&
      isValidUdpSyncVersion((const char*)fftUdpBuffer)) {
      decodeAudioData(lastValidPacketSize, fftUdpBuffer);
      //DEBUGSR_PRINTLN("Finished parsing UDP Sync Packet v2");
      receivedFormat = 2;
      haveFreshData = true;
      lastPacketTime = millis();
    } else if (lastValidPacketSize == sizeof(audioSyncPacket_v1) &&
      isValidUdpSyncVersion_v1((const char*)fftUdpBuffer)) {
      decodeAudioData_v1(lastValidPacketSize, fftUdpBuffer);
      //DEBUGSR_PRINTLN("Finished parsing UDP Sync Packet v1");
      receivedFormat = 1;
      haveFreshData = true;
      lastPacketTime = millis();
    } else {
      receivedFormat = 0; // unknown format
    }
  }
  return haveFreshData;
}

void WLEDSync::decodeAudioData_v1(int packetSize, uint8_t *fftBuff) {
  audioSyncPacket_v1 *receivedPacket = reinterpret_cast<audioSyncPacket_v1*>(fftBuff);
  // update samples for effects
  volumeSmth   = fmaxf(receivedPacket->sampleAgc, 0.0f);
  volumeRaw    = volumeSmth;   // V1 format does not have "raw" AGC sample
  // update internal samples
  sampleRaw    = fmaxf(receivedPacket->sampleRaw, 0.0f);
  sampleAvg    = fmaxf(receivedPacket->sampleAvg, 0.0f);;
  sampleAgc    = volumeSmth;
  rawSampleAgc = volumeRaw;
  multAgc      = 1.0f;   
  // Only change samplePeak IF it's currently false.
  // If it's true already, then the animation still needs to respond.
  autoResetPeak();
  if (!samplePeak) {
        samplePeak = receivedPacket->samplePeak >0 ? true:false;
        if (samplePeak) timeOfPeak = millis();
        //userVar1 = samplePeak;
  }
  //These values are only available on the ESP32
  for (int i = 0; i < NUM_GEQ_CHANNELS; i++) fftResult[i] = receivedPacket->fftResult[i];
  my_magnitude  = fmaxf(receivedPacket->FFT_Magnitude, 0.0);
  FFT_Magnitude = my_magnitude;
  FFT_MajorPeak = constrain(receivedPacket->FFT_MajorPeak, 1.0, 11025.0);  // restrict value to range expected by effects
}

void WLEDSync::decodeAudioData(int packetSize, uint8_t *fftBuff) {
  audioSyncPacket *receivedPacket = reinterpret_cast<audioSyncPacket*>(fftBuff);
  // update samples for effects
  volumeSmth   = fmaxf(receivedPacket->sampleSmth, 0.0f);
  volumeRaw    = fmaxf(receivedPacket->sampleRaw, 0.0f);
  // update internal samples
  sampleRaw    = volumeRaw;
  sampleAvg    = volumeSmth;
  rawSampleAgc = volumeRaw;
  sampleAgc    = volumeSmth;
  multAgc      = 1.0f;   
  // Only change samplePeak IF it's currently false.
  // If it's true already, then the animation still needs to respond.
  autoResetPeak();
  if (!samplePeak) {
        samplePeak = receivedPacket->samplePeak >0 ? true:false;
        if (samplePeak) timeOfPeak = millis();
        //userVar1 = samplePeak;
  }
  //These values are only available on the ESP32
  for (int i = 0; i < NUM_GEQ_CHANNELS; i++) {
    fftResult[i] = receivedPacket->fftResult[i];
    // Serial.printf("%u ", fftResult[i]);
  }
  // Serial.println(" 100");

  my_magnitude  = fmaxf(receivedPacket->FFT_Magnitude, 0.0f);
  FFT_Magnitude = my_magnitude;
  FFT_MajorPeak = constrain(receivedPacket->FFT_MajorPeak, 1.0f, 11025.0f);  // restrict value to range expected by effects
}

void WLEDSync::autoResetPeak(void) {
  uint16_t MinShowDelay = 50; // MAX(50, strip.getMinShowDelay());  // Fixes private class variable compiler error. Unsure if this is the correct way of fixing the root problem. -THATDONFC
  if (millis() - timeOfPeak > MinShowDelay) {          // Auto-reset of samplePeak after a complete frame has passed.
    samplePeak = false;
    if (audioSyncEnabled == 0) udpSamplePeak = false;  // this is normally reset by transmitAudioData
  }
}

void WLEDSync::initializeUdp(void) {
  #ifndef ESP8266
    udpSyncConnected = fftUdp.beginMulticast(IPAddress(239, 0, 0, 1), UDP_SYNC_PORT);
  #else
    udpSyncConnected = fftUdp.beginMulticast(WiFi.localIP(), IPAddress(239, 0, 0, 1), UDP_SYNC_PORT);
  #endif
}

void WLEDSync::handleWiFiStateChange(void) {
  bool currentWiFiConnected = (WiFi.status() == WL_CONNECTED);
  
  // WiFi just reconnected - reinitialize UDP
  if (currentWiFiConnected && !lastWiFiConnected) {
    fftUdp.stop();  // Stop the old UDP connection
    initializeUdp();
  }
  // WiFi just disconnected - stop UDP
  else if (!currentWiFiConnected && lastWiFiConnected) {
    fftUdp.stop();
    udpSyncConnected = false;
  }
  
  lastWiFiConnected = currentWiFiConnected;
}


