/**
 * @brief WLED Audio Sync
 * Author: Will Tatam
 * https://creativecommons.org/licenses/by/4.0/
 */
#ifndef WLEDSYNC_H_
#define WLEDSYNC_H_

#include "Arduino.h"
#include <WiFiUdp.h>

#define UDP_SYNC_HEADER_v1 "00001"
#define UDP_SYNC_HEADER "00002"
#define UDP_SYNC_PORT 11988
#define NUM_GEQ_CHANNELS 16  // number of frequency channels. Don't change !!

struct __attribute__ ((packed)) audioSyncPacket {  // WLEDMM "packed" ensures that there are no additional gaps
  char    header[6];      //  06 Bytes  offset 0
  uint8_t gap1[2];        // gap added by compiler: 02 Bytes, offset 6
  float   sampleRaw;      //  04 Bytes  offset 8  - either "sampleRaw" or "rawSampleAgc" depending on soundAgc setting
  float   sampleSmth;     //  04 Bytes  offset 12 - either "sampleAvg" or "sampleAgc" depending on soundAgc setting
  uint8_t samplePeak;     //  01 Bytes  offset 16 - 0 no peak; >=1 peak detected. In future, this will also provide peak Magnitude
  uint8_t frameCounter;   //  01 Bytes  offset 17 - track duplicate/out of order packets
  uint8_t fftResult[16];  //  16 Bytes  offset 18
  uint8_t gap2[2];        // gap added by compiler: 02 Bytes, offset 34
  float  FFT_Magnitude;   //  04 Bytes  offset 36
  float  FFT_MajorPeak;   //  04 Bytes  offset 40
};

// old "V1" audiosync struct - 83 Bytes - for backwards compatibility
struct audioSyncPacket_v1 {
  char header[6];         //  06 Bytes
  uint8_t myVals[32];     //  32 Bytes
  int sampleAgc;          //  04 Bytes
  int sampleRaw;          //  04 Bytes
  float sampleAvg;        //  04 Bytes
  bool samplePeak;        //  01 Bytes
  uint8_t fftResult[16];  //  16 Bytes
  double FFT_Magnitude;   //  08 Bytes
  double FFT_MajorPeak;   //  08 Bytes
};

class WLEDSync {

  private:
    WiFiUDP fftUdp;
    bool udpSyncConnected;
    bool lastWiFiConnected;  // Track last WiFi connection state

    bool audioSyncEnabled = true; // not actually an option, but kept here to keep code in sync with UserMod

    // peak detection
    bool samplePeak = false;      // Boolean flag for peak - used in effects. Responding routine may reset this flag. Auto-reset after strip.getMinShowDelay()
    unsigned long timeOfPeak = 0; // time of last sample peak detection.
    bool udpSamplePeak = false;   // Boolean flag for peak. Set at the same tiem as samplePeak, but reset by transmitAudioData


    void decodeAudioData(int packetSize, uint8_t *fftBuff);
    void decodeAudioData_v1(int packetSize, uint8_t *fftBuff);
    void autoResetPeak(void);
    void handleWiFiStateChange(void);
    void initializeUdp(void);

  public:
    int receivedFormat = -1;            // last received UDP sound sync format - 0=none, 1=v1 (0.13.x), 2=v2 (0.14.x)
    unsigned long lastPacketTime = 0;
    IPAddress sourceIP;
    uint8_t fftResult[NUM_GEQ_CHANNELS]= {0};// Our calculated freq. channel result table to be used by effects
    float FFT_MajorPeak = 1.0f;   // FFT: strongest (peak) frequency
    float FFT_Magnitude = 0.0f;   // FFT: volume (magnitude) of peak frequency
    // variables used in effects
    float    volumeSmth = 0.0f;   // either sampleAvg or sampleAgc depending on soundAgc; smoothed sample
    int16_t  volumeRaw = 0;       // either sampleRaw or rawSampleAgc depending on soundAgc
    int16_t  sampleRaw = 0;       // Current sample. Must only be updated ONCE!!! (amplified mic value by sampleGain and inputLevel)
    float    my_magnitude =0.0f;     // FFT_Magnitude, scaled by multAgc
    int16_t  rawSampleAgc = 0;    // not smoothed AGC sample
    float    sampleAgc = 0.0f;    // Smoothed AGC sample
    float    multAgc = 1.0f;      // sample * multAgc = sampleAgc. Our AGC multiplier
    float    sampleAvg = 0.0f;    // Smoothed Average sample - sampleAvg < 1 means "quiet" (simple noise gate)

    WLEDSync();

    void begin();
    void send(audioSyncPacket transmitData);
    bool read();
};
#endif
