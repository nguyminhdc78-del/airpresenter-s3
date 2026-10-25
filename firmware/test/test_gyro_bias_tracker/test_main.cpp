// Host tests for GyroBiasTracker: the offset must follow real still moments (table, steady
// hand) and must never learn a steady slow turn or a tilt as offset (that made the cursor
// drift on the next AIR press).
// Run: pio test -e native -f test_gyro_bias_tracker
#include <unity.h>

#include <cmath>

#include "gyro-bias-tracker.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

using namespace gyro_bias;
constexpr uint32_t kTickMs = 5;
const int16_t kLevel[3] = {0, 0, 16384};  // lying flat, 1 g up

// Feeds `seconds` of samples: gyro = offset + rate (deg/s) + tremor (deg/s, alternating sign),
// accel tilting by `tiltPerSec` g per second on X. Returns the time reached.
uint32_t feed(GyroBiasTracker& t, uint32_t startMs, float seconds, const float offsetLsb[3],
              float rateDps, float tremorDps, float tiltPerSec = 0.0f) {
  uint32_t now = startMs;
  const int n = static_cast<int>(seconds * 1000 / kTickMs);
  for (int i = 0; i < n; ++i, now += kTickMs) {
    const float wobble = (i % 2 ? 1.0f : -1.0f) * tremorDps;
    int16_t gyro[3];
    for (int a = 0; a < 3; ++a) gyro[a] = static_cast<int16_t>(lroundf(offsetLsb[a] + (rateDps + wobble) * kLsbPerDps * (a == 2)));
    int16_t accel[3] = {kLevel[0], kLevel[1], kLevel[2]};
    accel[0] = static_cast<int16_t>(lroundf(tiltPerSec * 16384.0f * (now - startMs) / 1000.0f));
    t.add(gyro, accel, now);
  }
  return now;
}

const float kOffset[3] = {-30.0f, 72.0f, -12.0f};  // a real chip's offsets, raw LSB

GyroBiasTracker trustedAt(const float bias[3]) {
  GyroBiasTracker t;
  t.set(bias, true);
  t.restart(0);
  return t;
}

}  // namespace

void test_table_still_window_takes_the_mean(void) {
  GyroBiasTracker t;  // untrusted, zero
  t.restart(0);
  feed(t, 0, 1.0f, kOffset, 0.0f, 0.0f);
  TEST_ASSERT_TRUE(t.trusted());
  for (int a = 0; a < 3; ++a) TEST_ASSERT_FLOAT_WITHIN(1.0f, kOffset[a], t.bias()[a]);
}

void test_hand_held_window_moves_a_trusted_offset_in_small_steps(void) {
  const float start[3] = {kOffset[0], kOffset[1], kOffset[2] + 50.0f};  // 0.76 deg/s off on Z
  GyroBiasTracker t = trustedAt(start);
  feed(t, 0, 0.8f, kOffset, 0.0f, 2.0f);  // one hand-held window with tremor
  TEST_ASSERT_FLOAT_WITHIN(0.6f, start[2] - kMaxStepLsb, t.bias()[2]);  // one 0.5 deg/s step
  feed(t, 800, 3.0f, kOffset, 0.0f, 2.0f);
  TEST_ASSERT_FLOAT_WITHIN(3.0f, kOffset[2], t.bias()[2]);  // healed over a few windows
}

void test_steady_slow_turn_is_not_learned(void) {
  GyroBiasTracker t = trustedAt(kOffset);
  feed(t, 0, 6.0f, kOffset, 3.0f, 1.5f);  // 3 deg/s steady yaw with tremor, gravity still
  TEST_ASSERT_FLOAT_WITHIN(1.0f, kOffset[2], t.bias()[2]);
  feed(t, 6000, 6.0f, kOffset, 20.0f, 0.0f);  // ultra-smooth 20 deg/s (turning chair)
  TEST_ASSERT_FLOAT_WITHIN(1.0f, kOffset[2], t.bias()[2]);
}

void test_tilting_while_still_is_not_learned(void) {
  GyroBiasTracker t;  // untrusted: would otherwise jump to whatever it sees
  t.restart(0);
  feed(t, 0, 2.0f, kOffset, 2.0f, 0.0f, 0.2f);  // slowly lowering the arm: gravity moves
  TEST_ASSERT_FALSE(t.trusted());
}

void test_window_needs_time_and_samples(void) {
  GyroBiasTracker t;
  t.restart(0);
  feed(t, 0, kWindowMs / 1000.0f - 0.05f, kOffset, 0.0f, 0.0f);
  TEST_ASSERT_FALSE(t.trusted());  // not yet a full window
  GyroBiasTracker sparse;
  sparse.restart(0);
  int16_t gyro[3] = {-30, 72, -12};
  for (uint32_t ms = 0; ms <= 2000; ms += 100) sparse.add(gyro, kLevel, ms);  // 21 samples in 2 s
  TEST_ASSERT_FALSE(sparse.trusted());
}

void test_untrusted_offset_is_replaced_by_a_hand_held_mean(void) {
  GyroBiasTracker t;  // failed calibration left a bad guess
  const float bad[3] = {0.0f, 0.0f, 300.0f};
  t.set(bad, false);
  t.restart(0);
  feed(t, 0, 0.8f, kOffset, 0.0f, 2.0f);
  TEST_ASSERT_TRUE(t.trusted());
  TEST_ASSERT_FLOAT_WITHIN(2.0f, kOffset[2], t.bias()[2]);
}

void test_restart_drops_the_window(void) {
  GyroBiasTracker t;
  t.restart(0);
  feed(t, 0, 0.5f, kOffset, 0.0f, 0.0f);
  t.restart(500);  // cursor used: half a window is forgotten
  feed(t, 500, 0.5f, kOffset, 0.0f, 0.0f);
  TEST_ASSERT_FALSE(t.trusted());
}

void test_wrong_trusted_offset_heals_after_agreeing_windows(void) {
  // Calibrated during a smooth 6 deg/s turn: trusted but wrong. Lying still on the table
  // for a while must still fix it (agreeing windows win over the trusted value).
  const float wrong[3] = {kOffset[0], kOffset[1], kOffset[2] + 6.0f * kLsbPerDps};
  GyroBiasTracker t = trustedAt(wrong);
  feed(t, 0, 4.0f, kOffset, 0.0f, 0.0f);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, wrong[2], t.bias()[2]);  // not yet: could be a slow turn
  feed(t, 4000, 6.0f, kOffset, 0.0f, 0.0f);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, kOffset[2], t.bias()[2]);  // healed after ~7.5 s
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_table_still_window_takes_the_mean);
  RUN_TEST(test_hand_held_window_moves_a_trusted_offset_in_small_steps);
  RUN_TEST(test_steady_slow_turn_is_not_learned);
  RUN_TEST(test_tilting_while_still_is_not_learned);
  RUN_TEST(test_window_needs_time_and_samples);
  RUN_TEST(test_untrusted_offset_is_replaced_by_a_hand_held_mean);
  RUN_TEST(test_restart_drops_the_window);
  RUN_TEST(test_wrong_trusted_offset_heals_after_agreeing_windows);
  return UNITY_END();
}
