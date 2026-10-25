// Host tests for ImuSampleSanityCheck: garbled I2C reads (stuck bus, bit errors) must be
// rejected, real readings - even violent swings - must pass.
// Run: pio test -e native -f test_imu_sample_sanity
#include <unity.h>

#include "imu-sample-sanity-check.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

// accel x,y,z (LSB, 16384 = 1 g), temperature (~25 C), gyro x,y,z (LSB, 65.5 = 1 deg/s)
const int16_t kRest[7] = {120, -340, 16500, 1200, -26, 42, -13};

}  // namespace

void test_real_reading_passes(void) {
  ImuSampleSanityCheck c;
  TEST_ASSERT_TRUE(c.plausible(kRest, 0));
  TEST_ASSERT_TRUE(c.plausible(kRest, 4));
}

void test_stuck_bus_patterns_are_rejected(void) {
  ImuSampleSanityCheck c;
  const int16_t zeros[7] = {0, 0, 0, 0, 0, 0, 0};
  const int16_t ones[7] = {-1, -1, -1, -1, -1, -1, -1};  // SDA stuck high: 0xFFFF
  TEST_ASSERT_FALSE(c.plausible(zeros, 0));
  TEST_ASSERT_FALSE(c.plausible(ones, 0));
}

void test_impossible_temperature_is_rejected(void) {
  ImuSampleSanityCheck c;
  int16_t v[7] = {120, -340, 16500, 32000, -26, 42, -13};
  TEST_ASSERT_FALSE(c.plausible(v, 0));
}

void test_bit_error_jump_is_rejected(void) {
  ImuSampleSanityCheck c;
  TEST_ASSERT_TRUE(c.plausible(kRest, 0));
  int16_t glitch[7] = {120, -340, 16500, 1200, -26, 42 + 0x4000, -13};  // one flipped high bit
  TEST_ASSERT_FALSE(c.plausible(glitch, 4));
  TEST_ASSERT_TRUE(c.plausible(kRest, 8));  // the next good read passes again
}

void test_violent_but_real_swing_passes(void) {
  ImuSampleSanityCheck c;
  TEST_ASSERT_TRUE(c.plausible(kRest, 0));
  // 4 ms later: 120 deg/s more turn rate and 1 g more on X (a hard flick).
  const int16_t flick[7] = {16500, -340, 16500, 1200, 7834, 42, -13};
  TEST_ASSERT_TRUE(c.plausible(flick, 4));
}

void test_jump_after_a_gap_is_accepted(void) {
  ImuSampleSanityCheck c;
  TEST_ASSERT_TRUE(c.plausible(kRest, 0));
  int16_t later[7] = {120, -340, 16500, 1200, 20000, 42, -13};  // 300 deg/s, after 0.5 s
  TEST_ASSERT_TRUE(c.plausible(later, 500));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_real_reading_passes);
  RUN_TEST(test_stuck_bus_patterns_are_rejected);
  RUN_TEST(test_impossible_temperature_is_rejected);
  RUN_TEST(test_bit_error_jump_is_rejected);
  RUN_TEST(test_violent_but_real_swing_passes);
  RUN_TEST(test_jump_after_a_gap_is_accepted);
  return UNITY_END();
}
