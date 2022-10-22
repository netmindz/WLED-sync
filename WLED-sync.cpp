#include "WLED-sync.h"

bool isValidUdpSyncVersion(char header[6]) {
  return (header == UDP_SYNC_HEADER);
}
