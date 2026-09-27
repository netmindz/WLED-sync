#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/components/socket/socket.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <memory>
#include <string>

namespace esphome {
namespace wled_sync {

static const uint8_t NUM_GEQ_CHANNELS = 16;

// WLED AudioReactive usermod "UDP Sound Sync" wire formats.
// See https://github.com/netmindz/WLED-sync for the reference decoder these
// structs and the validation/decode logic below are ported from.
struct __attribute__((packed)) AudioSyncPacket {
  char header[6];
  uint8_t gap1[2];
  float sample_raw;
  float sample_smth;
  uint8_t sample_peak;
  uint8_t frame_counter;
  uint8_t fft_result[NUM_GEQ_CHANNELS];
  uint8_t gap2[2];
  float fft_magnitude;
  float fft_major_peak;
};

struct AudioSyncPacketV1 {
  char header[6];
  uint8_t my_vals[32];
  int32_t sample_agc;
  int32_t sample_raw;
  float sample_avg;
  uint8_t sample_peak;
  uint8_t fft_result[NUM_GEQ_CHANNELS];
  double fft_magnitude;
  double fft_major_peak;
};

class WLEDSyncComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_multicast_group(const std::string &group) { this->multicast_group_ = group; }
  void set_port(uint16_t port) { this->port_ = port; }
  void set_update_interval(uint32_t update_interval) { this->update_interval_ = update_interval; }

  void set_volume_smth_sensor(sensor::Sensor *sensor) { this->volume_smth_sensor_ = sensor; }
  void set_volume_raw_sensor(sensor::Sensor *sensor) { this->volume_raw_sensor_ = sensor; }
  void set_fft_magnitude_sensor(sensor::Sensor *sensor) { this->fft_magnitude_sensor_ = sensor; }
  void set_fft_major_peak_sensor(sensor::Sensor *sensor) { this->fft_major_peak_sensor_ = sensor; }
  void set_beat_binary_sensor(binary_sensor::BinarySensor *sensor) { this->beat_binary_sensor_ = sensor; }

 protected:
  bool join_multicast_group_();
  void decode_v2_(const uint8_t *buf);
  void decode_v1_(const uint8_t *buf);
  void auto_reset_peak_();
  void publish_();

  std::string multicast_group_{"239.0.0.1"};
  uint16_t port_{11988};
  uint32_t update_interval_{1000};

  std::unique_ptr<socket::Socket> socket_;

  // Decoded, not-yet-published state (packets arrive at 20-50Hz; sensors are
  // published on a slower timer via publish_() instead of on every packet).
  float volume_smth_{0.0f};
  float volume_raw_{0.0f};
  float fft_magnitude_{0.0f};
  float fft_major_peak_{1.0f};
  bool sample_peak_{false};
  uint32_t time_of_peak_{0};

  sensor::Sensor *volume_smth_sensor_{nullptr};
  sensor::Sensor *volume_raw_sensor_{nullptr};
  sensor::Sensor *fft_magnitude_sensor_{nullptr};
  sensor::Sensor *fft_major_peak_sensor_{nullptr};
  binary_sensor::BinarySensor *beat_binary_sensor_{nullptr};
};

}  // namespace wled_sync
}  // namespace esphome
