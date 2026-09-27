#include "wled_sync.h"

#ifdef USE_NETWORK

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/network/ip_address.h"

#include <lwip/igmp.h>
#include <lwip/ip4_addr.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace esphome {
namespace wled_sync {

static const char *const TAG = "wled_sync";

static const char *const HEADER_V2 = "00002";
static const char *const HEADER_V1 = "00001";
// A bit of headroom over the larger (v1, 83 byte) packet.
static const size_t MAX_PACKET_SIZE = 96;

static bool has_header(const uint8_t *buf, const char *header) { return memcmp(buf, header, 6) == 0; }

void WLEDSyncComponent::setup() {
  this->socket_ = socket::socket_ip(SOCK_DGRAM, IPPROTO_IP);
  if (this->socket_ == nullptr) {
    ESP_LOGE(TAG, "Could not create socket");
    this->mark_failed();
    return;
  }

  int enable = 1;
  this->socket_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  if (this->socket_->setblocking(false) != 0) {
    ESP_LOGE(TAG, "Unable to set socket non-blocking");
    this->mark_failed();
    return;
  }

  struct sockaddr_storage server;
  socklen_t sl = socket::set_sockaddr_any((struct sockaddr *) &server, sizeof(server), this->port_);
  if (sl == 0) {
    ESP_LOGE(TAG, "Unable to build sockaddr");
    this->mark_failed();
    return;
  }

  if (this->socket_->bind((struct sockaddr *) &server, sl) != 0) {
    ESP_LOGE(TAG, "Unable to bind to port %u", this->port_);
    this->mark_failed();
    return;
  }

  if (!this->join_multicast_group_()) {
    ESP_LOGW(TAG, "Failed to join multicast group %s; sync packets may not arrive", this->multicast_group_.c_str());
  }

  this->set_interval("publish", this->update_interval_, [this]() { this->publish_(); });
}

bool WLEDSyncComponent::join_multicast_group_() {
  ip4_addr_t group = network::IPAddress(this->multicast_group_);
  err_t err;
  {
    LwIPLock lock;
    err = igmp_joingroup(IP4_ADDR_ANY4, &group);
  }
  return err == ERR_OK;
}

void WLEDSyncComponent::loop() {
  uint8_t buf[MAX_PACKET_SIZE];
  ssize_t len;

  // Drain every queued packet each pass: WLED broadcasts at 20-50Hz, well
  // above the loop() rate under load, so leaving packets queued causes the
  // decoded state to lag further and further behind.
  while ((len = this->socket_->read(buf, sizeof(buf))) > 0) {
    if (len == sizeof(AudioSyncPacket) && has_header(buf, HEADER_V2)) {
      this->decode_v2_(buf);
    } else if (len == sizeof(AudioSyncPacketV1) && has_header(buf, HEADER_V1)) {
      this->decode_v1_(buf);
    } else {
      ESP_LOGV(TAG, "Ignoring packet of size %d with unrecognised header", (int) len);
    }
  }

  this->auto_reset_peak_();
}

void WLEDSyncComponent::decode_v2_(const uint8_t *buf) {
  AudioSyncPacket packet;
  memcpy(&packet, buf, sizeof(packet));

  this->volume_smth_ = std::fmax(packet.sample_smth, 0.0f);
  this->volume_raw_ = std::fmax(packet.sample_raw, 0.0f);
  this->fft_magnitude_ = std::fmax(packet.fft_magnitude, 0.0f);
  this->fft_major_peak_ = clamp(packet.fft_major_peak, 1.0f, 11025.0f);

  // Only set the flag if it isn't already pending publish/auto-reset.
  if (!this->sample_peak_ && packet.sample_peak > 0) {
    this->sample_peak_ = true;
    this->time_of_peak_ = millis();
  }
}

void WLEDSyncComponent::decode_v1_(const uint8_t *buf) {
  AudioSyncPacketV1 packet;
  memcpy(&packet, buf, sizeof(packet));

  this->volume_smth_ = std::fmax((float) packet.sample_agc, 0.0f);
  this->volume_raw_ = this->volume_smth_;  // v1 has no separate raw AGC sample.
  this->fft_magnitude_ = std::fmax((float) packet.fft_magnitude, 0.0f);
  this->fft_major_peak_ = clamp((float) packet.fft_major_peak, 1.0f, 11025.0f);

  if (!this->sample_peak_ && packet.sample_peak > 0) {
    this->sample_peak_ = true;
    this->time_of_peak_ = millis();
  }
}

void WLEDSyncComponent::auto_reset_peak_() {
  // Matches the reference decoder's fixed 50ms auto-reset window.
  static const uint32_t PEAK_RESET_MS = 50;
  if (this->sample_peak_ && millis() - this->time_of_peak_ > PEAK_RESET_MS) {
    this->sample_peak_ = false;
  }
}

void WLEDSyncComponent::publish_() {
  if (this->volume_smth_sensor_ != nullptr)
    this->volume_smth_sensor_->publish_state(this->volume_smth_);
  if (this->volume_raw_sensor_ != nullptr)
    this->volume_raw_sensor_->publish_state(this->volume_raw_);
  if (this->fft_magnitude_sensor_ != nullptr)
    this->fft_magnitude_sensor_->publish_state(this->fft_magnitude_);
  if (this->fft_major_peak_sensor_ != nullptr)
    this->fft_major_peak_sensor_->publish_state(this->fft_major_peak_);
  if (this->beat_binary_sensor_ != nullptr)
    this->beat_binary_sensor_->publish_state(this->sample_peak_);
}

void WLEDSyncComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "WLED Sync:");
  ESP_LOGCONFIG(TAG, "  Multicast group: %s", this->multicast_group_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Update interval: %ums", (unsigned) this->update_interval_);
  LOG_SENSOR("  ", "Volume (smoothed)", this->volume_smth_sensor_);
  LOG_SENSOR("  ", "Volume (raw)", this->volume_raw_sensor_);
  LOG_SENSOR("  ", "FFT Magnitude", this->fft_magnitude_sensor_);
  LOG_SENSOR("  ", "FFT Major Peak", this->fft_major_peak_sensor_);
  LOG_BINARY_SENSOR("  ", "Beat", this->beat_binary_sensor_);
}

}  // namespace wled_sync
}  // namespace esphome

#endif  // USE_NETWORK
