/**
 * @brief WLED Audio Sync
 * 
 */
#ifndef WLEDSYNC_H_
#define WLEDSYNC_H_

#include "Arduino.h"
#include <WiFiUdp.h>

#define UDP_SYNC_HEADER "00002"
#define UDP_SYNC_PORT 11988

struct audioSyncPacket {
  char    header[6];      //  06 Bytes
  float   sampleRaw;      //  04 Bytes  - either "sampleRaw" or "rawSampleAgc" depending on soundAgc setting
  float   sampleSmth;     //  04 Bytes  - either "sampleAvg" or "sampleAgc" depending on soundAgc setting
  uint8_t samplePeak;     //  01 Bytes  - 0 no peak; >=1 peak detected. In future, this will also provide peak Magnitude
  uint8_t reserved1;      //  01 Bytes  - for future extensions - not used yet
  uint8_t fftResult[16];  //  16 Bytes
  float  FFT_Magnitude;   //  04 Bytes
  float  FFT_MajorPeak;   //  04 Bytes
};

class WLEDSync {

  private:
    WiFiUDP fftUdp;

  public:
    
    WLEDSync();

    void send(audioSyncPacket transmitData);
};
#endif