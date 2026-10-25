#include "ble-link-event-log.h"

#include <Arduino.h>

namespace {

// A PC that still holds a pairing this remote forgot (FN/BOOT held to clear bonds) fails
// encryption and drops the link, again and again, before it is ever encrypted.
constexpr uint32_t kStaleBondDrops = 3;

}  // namespace

void BleLinkEventLog::print() {
  // Disconnects first: if the link dropped and came back since the last call, the last
  // line printed then shows the current state.
  const uint32_t disconnects = disconnects_.load();
  if (disconnects != shownDisconnects_) {
    Serial.printf("[ble] disconnected (x%u), reason=%u\n", static_cast<unsigned>(disconnects - shownDisconnects_),
                  static_cast<unsigned>(lastReason_.load()));
    shownDisconnects_ = disconnects;
  }
  const uint32_t connects = connects_.load();
  if (connects != shownConnects_) {
    Serial.printf("[ble] connected (x%u)\n", static_cast<unsigned>(connects - shownConnects_));
    shownConnects_ = connects;
  }
  const uint32_t failures = pairingFailures_.load();
  if (failures != shownPairingFailures_) {
    Serial.println("[ble] pairing failed, dropping link");
    shownPairingFailures_ = failures;
  }
  const uint32_t encryptions = encryptions_.load();
  if (encryptions != shownEncryptions_) {
    Serial.println("[ble] paired / encrypted");
    shownEncryptions_ = encryptions;
    staleBondHintShown_ = false;
  }
  const uint32_t updates = linkUpdates_.load();
  if (updates != shownLinkUpdates_) {
    const uint32_t it = linkIntervalTimeout_.load();
    Serial.printf("[ble] link: interval %.2f ms, latency %u, timeout %u ms\n", (it >> 16) * 1.25f,
                  static_cast<unsigned>(linkLatency_.load()), static_cast<unsigned>((it & 0xFFFF) * 10));
    shownLinkUpdates_ = updates;
  }
  if (!staleBondHintShown_ && dropsBeforeEncryption_.load() >= kStaleBondDrops) {
    Serial.println("[ble] the PC keeps dropping the link before encryption: it probably still holds an old"
                   " pairing. Remove \"AirPresenter S3\" from its Bluetooth list and pair again.");
    staleBondHintShown_ = true;
  }
}
