#include "WLED-sync.h"

    
  bool isValidUdpSyncVersion(char header[6]) {
    return (header == UDP_SYNC_HEADER);
  }

WLEDSync::WLEDSync() {
}

void WLEDSync::begin() {
  #ifndef ESP8266
    fftUdp.beginMulticast(IPAddress(239, 0, 0, 1), UDP_SYNC_PORT);
  #else
    fftUdp.beginMulticast(WiFi.localIP(), IPAddress(239, 0, 0, 1), UDP_SYNC_PORT);
  #endif
}

void WLEDSync::send(audioSyncPacket transmitData) {
    fftUdp.beginMulticastPacket();
    fftUdp.write(reinterpret_cast<uint8_t *>(&transmitData), sizeof(transmitData));
    fftUdp.endPacket();
}