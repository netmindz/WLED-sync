#include <ESP8266WiFi.h>
#include <ESPAsyncE131.h>

#include <ESPDMX.h>

#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync


#define UNIVERSE 1        // First DMX Universe to listen for
#define UNIVERSE_COUNT 1  // Total number of Universes to listen for, starting at UNIVERSE
#define CHANNELS 400

ESPAsyncE131 e131(UNIVERSE_COUNT);

WLEDSync sync;
uint8_t fftResult[NUM_GEQ_CHANNELS]= {0};


#include "wifi.h"
const char ssid[] = SECRET_SSID;
const char passphrase[] = SECRET_PSK;

DMXESPSerial dmx;

void setup() {
  Serial.begin(115200);

  // Make sure you're in station mode
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  if (passphrase != NULL)
    WiFi.begin(ssid, passphrase);
  else
    WiFi.begin(ssid);

  Serial.print("Waiting on wifi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("w");
  }
  Serial.println("\nDone");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(2000);
  // Choose one to begin listening for E1.31 data
  //if (e131.begin(E131_UNICAST)) {
  if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT)) {  // Listen via Multicast
    Serial.println(F("Listening for data..."));
  }
  else {
    Serial.println(F("*** e131.begin failed ***"));
  }
  dmx.init(512);

  sync.begin();

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

int led = 0;
void loop() {
  if (sync.read()) {
    dmx.write(400, 255);
    for (int b = 0; b < NUM_GEQ_CHANNELS; b++) {
      uint8_t val = sync.fftResult[b];
      dmx.write((CHANNELS + 1 + b), val);
    }
    // Serial.println("T");
  }
  else {
    // Serial.println("F");
    dmx.write(400, 0);    
  }
  if (!e131.isEmpty()) {
    led = !led;
    digitalWrite(LED_BUILTIN, led);

    e131_packet_t packet;
    e131.pull(&packet);     // Pull packet from ring buffer

//    EVERY_N_SECONDS( 2 ) {
      // Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
      //               htons(packet.universe),                 // The Universe for this packet
      //               htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
      //               e131.stats.num_packets,                 // Packet counter
      //               e131.stats.packet_errors,               // Packet error counter
      //               packet.property_values[1]);             // Dimmer data for Channel 1
    //}

    /* Parse a packet and update pixels */
    for (int i = 1; i < CHANNELS; i++) {
      int v = packet.property_values[i];
      dmx.write(i, v);
    }
  }

  dmx.update();

}
