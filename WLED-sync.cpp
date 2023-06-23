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
}

void WLEDSync::begin() {
  #ifndef ESP8266
    udpSyncConnected = fftUdp.beginMulticast(IPAddress(239, 0, 0, 1), UDP_SYNC_PORT);
  #else
    udpSyncConnected = fftUdp.beginMulticast(WiFi.localIP(), IPAddress(239, 0, 0, 1), UDP_SYNC_PORT);
  #endif
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
}

bool WLEDSync::read()   // check & process new data. return TRUE in case that new audio data was received. 
{
  if (!udpSyncConnected) return false;
  bool haveFreshData = false;

  size_t packetSize = fftUdp.parsePacket();
  if ((packetSize > 0) && ((packetSize < 5) || (packetSize > UDPSOUND_MAX_PACKET))) fftUdp.flush(); // discard invalid packets (too small or too big)
  if ((packetSize > 5) && (packetSize <= UDPSOUND_MAX_PACKET)) {
    //DEBUGSR_PRINTLN("Received UDP Sync Packet");
    fftUdp.read(fftUdpBuffer, packetSize);

    // VERIFY THAT THIS IS A COMPATIBLE PACKET
    if (packetSize == sizeof(audioSyncPacket) && (isValidUdpSyncVersion((const char *)fftUdpBuffer))) {
      decodeAudioData(packetSize, fftUdpBuffer);
      //DEBUGSR_PRINTLN("Finished parsing UDP Sync Packet v2");
      haveFreshData = true;
      receivedFormat = 2;
    } else {
      if (packetSize == sizeof(audioSyncPacket_v1) && (isValidUdpSyncVersion_v1((const char *)fftUdpBuffer))) {
        decodeAudioData_v1(packetSize, fftUdpBuffer);
        //DEBUGSR_PRINTLN("Finished parsing UDP Sync Packet v1");
        haveFreshData = true;
        receivedFormat = 1;
      } else receivedFormat = 0; // unknown format
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

