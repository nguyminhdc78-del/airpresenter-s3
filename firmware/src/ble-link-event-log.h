// BLE connection events, recorded by the NimBLE host task (core 0) and printed later by
// loop() (core 1). Printing inside the BLE callbacks ran printf on the host task's small
// stack and could block it on a full USB serial buffer (nobody reading the port) while a
// PC with a stale pairing reconnected several times per second.
#pragma once

#include <stdint.h>

#include <atomic>

class BleLinkEventLog {
 public:
  static constexpr uint32_t kQuickDropMs = 5000;  // unencrypted link dropped this soon: suspicious

  // ---- core 0 (NimBLE callbacks): counters only, never blocks ----
  void connected() { connects_.fetch_add(1); }
  // `quickDropBeforeEncryption`: the link died within seconds, never encrypted (what a PC
  // holding a forgotten pairing does), as opposed to our own 35 s timeout or a slow cancel.
  void disconnected(int reason, bool quickDropBeforeEncryption) {
    lastReason_.store(static_cast<uint32_t>(reason));
    disconnects_.fetch_add(1);
    if (quickDropBeforeEncryption) dropsBeforeEncryption_.fetch_add(1);
  }
  void encrypted() {
    dropsBeforeEncryption_.store(0);
    encryptions_.fetch_add(1);
  }
  void pairingFailed() { pairingFailures_.fetch_add(1); }
  // Raw NimBLE units: interval x 1.25 ms, timeout x 10 ms.
  void linkParams(uint16_t interval, uint16_t latency, uint16_t timeout) {
    linkIntervalTimeout_.store((static_cast<uint32_t>(interval) << 16) | timeout);
    linkLatency_.store(latency);
    linkUpdates_.fetch_add(1);
  }

  // ---- core 1 (loop): prints what happened since the previous call ----
  void print();

 private:
  std::atomic<uint32_t> connects_{0};
  std::atomic<uint32_t> disconnects_{0};
  std::atomic<uint32_t> lastReason_{0};
  std::atomic<uint32_t> dropsBeforeEncryption_{0};  // in a row; reset once encrypted
  std::atomic<uint32_t> encryptions_{0};
  std::atomic<uint32_t> pairingFailures_{0};
  std::atomic<uint32_t> linkIntervalTimeout_{0};  // interval << 16 | timeout
  std::atomic<uint32_t> linkLatency_{0};
  std::atomic<uint32_t> linkUpdates_{0};

  uint32_t shownConnects_ = 0;
  uint32_t shownDisconnects_ = 0;
  uint32_t shownEncryptions_ = 0;
  uint32_t shownPairingFailures_ = 0;
  uint32_t shownLinkUpdates_ = 0;
  bool staleBondHintShown_ = false;
};
