// Host unit tests for CursorMotionAccumulator (header-only, no Arduino deps).
// Run: pio test -e native -f test_cursor_motion
#include <unity.h>

#include <cmath>

#include "cursor-motion-accumulator.h"

void setUp(void) {}
void tearDown(void) {}

// ---- fractional carry ----

void test_fraction_carries_until_whole_count(void) {
  CursorMotionAccumulator acc;
  int8_t x = 9, y = 9;  // sentinels: take() must write both outputs on every call
  TEST_ASSERT_FALSE(acc.take(0.4f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(0, x);
  TEST_ASSERT_FALSE(acc.take(0.4f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(0, x);
  TEST_ASSERT_TRUE(acc.take(0.4f, 0.0f, x, y));  // 1.2 -> 1, keep 0.2
  TEST_ASSERT_EQUAL_INT8(1, x);
  TEST_ASSERT_EQUAL_INT8(0, y);
}

void test_axes_carry_independently(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_FALSE(acc.take(0.5f, 0.0f, x, y));  // x pending 0.5, y 0
  // A shared remainder would turn this y 0.5 into a whole count right here.
  TEST_ASSERT_FALSE(acc.take(0.0f, 0.5f, x, y));  // y pending 0.5, x still 0.5
  TEST_ASSERT_TRUE(acc.take(0.5f, 0.0f, x, y));   // x 1.0 -> 1, y stays pending
  TEST_ASSERT_EQUAL_INT8(1, x);
  TEST_ASSERT_EQUAL_INT8(0, y);
  TEST_ASSERT_TRUE(acc.take(0.0f, 0.5f, x, y));   // y 1.0 -> 1, x idle
  TEST_ASSERT_EQUAL_INT8(0, x);
  TEST_ASSERT_EQUAL_INT8(1, y);
}

void test_instances_keep_separate_remainders(void) {
  CursorMotionAccumulator a, b;
  int8_t x = 0, y = 0;
  TEST_ASSERT_FALSE(a.take(0.5f, 0.0f, x, y));
  TEST_ASSERT_FALSE(b.take(0.5f, 0.0f, x, y));  // would emit 1 if the remainder were shared
  TEST_ASSERT_TRUE(a.take(0.5f, 0.0f, x, y));   // a: 0.5 + 0.5 -> 1
  TEST_ASSERT_EQUAL_INT8(1, x);
}

// ---- truncation toward zero ----

void test_positive_input_truncates_toward_zero(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(2.7f, 0.0f, x, y));  // 2.7 -> 2, keep 0.7
  TEST_ASSERT_EQUAL_INT8(2, x);
  TEST_ASSERT_TRUE(acc.take(0.5f, 0.0f, x, y));  // 1.2 -> 1, keep 0.2
  TEST_ASSERT_EQUAL_INT8(1, x);
}

void test_negative_input_truncates_toward_zero(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(-2.7f, 0.0f, x, y));  // floor would give -3; trunc gives -2
  TEST_ASSERT_EQUAL_INT8(-2, x);
  TEST_ASSERT_FALSE(acc.take(0.0f, 0.0f, x, y));  // -0.7 still pending
  TEST_ASSERT_TRUE(acc.take(-0.5f, 0.0f, x, y));  // -1.2 -> -1, keep -0.2
  TEST_ASSERT_EQUAL_INT8(-1, x);
  TEST_ASSERT_EQUAL_INT8(0, y);
}

void test_negative_fraction_carries_toward_minus_one(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_FALSE(acc.take(-0.4f, 0.0f, x, y));
  TEST_ASSERT_FALSE(acc.take(-0.4f, 0.0f, x, y));
  TEST_ASSERT_TRUE(acc.take(-0.4f, 0.0f, x, y));  // -1.2 -> -1
  TEST_ASSERT_EQUAL_INT8(-1, x);
}

// ---- carry invariant ----

void test_emitted_total_tracks_requested_total(void) {
  // Telescoping sum: emitted = requested - remainder with |remainder| < 1, so no drift.
  const float inputs[] = {0.4f, -1.3f, 2.9f, 0.05f, -0.75f, 3.6f, -0.2f, 7.1f, -4.45f};
  const int count = static_cast<int>(sizeof(inputs) / sizeof(inputs[0]));
  CursorMotionAccumulator acc;
  double requested = 0.0;
  long emitted = 0;
  for (int round = 0; round < 20; ++round) {
    for (int i = 0; i < count; ++i) {
      int8_t x = 0, y = 0;
      acc.take(inputs[i], 0.0f, x, y);
      requested += inputs[i];
      emitted += x;
      TEST_ASSERT_TRUE(std::fabs(static_cast<double>(emitted) - requested) < 1.0 + 1e-3);
    }
  }
  TEST_ASSERT_TRUE(emitted > 100);  // non-vacuous: real movement happened
}

// ---- clamp to +-127, overflow dropped ----

void test_overflow_clamped_to_127_and_dropped(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(200.0f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(127, x);
  // The 73 counts past 127 are dropped, not queued: the next idle tick stays silent.
  TEST_ASSERT_FALSE(acc.take(0.0f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(0, x);
}

void test_exact_127_passes_and_128_clamps(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(127.0f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(127, x);
  TEST_ASSERT_TRUE(acc.take(128.0f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(127, x);
  TEST_ASSERT_FALSE(acc.take(0.0f, 0.0f, x, y));  // the 1 excess count is gone
}

void test_negative_overflow_clamped_to_minus_127_on_y(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(0.0f, -300.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(0, x);
  TEST_ASSERT_EQUAL_INT8(-127, y);  // symmetric range: -128 is never emitted
  TEST_ASSERT_FALSE(acc.take(0.0f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(0, y);
}

void test_clamp_keeps_fraction_but_drops_whole_overflow(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(200.5f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(127, x);
  // Only the 0.5 fraction survives the clamp: 0.5 + 0.5 emits exactly 1.
  TEST_ASSERT_TRUE(acc.take(0.5f, 0.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(1, x);
}

void test_fast_diagonal_keeps_its_direction_when_clamped(void) {
  CursorMotionAccumulator acc;
  int8_t x = 0, y = 0;
  TEST_ASSERT_TRUE(acc.take(250.0f, -100.0f, x, y));  // heading -21.8 deg
  TEST_ASSERT_EQUAL_INT8(127, x);
  TEST_ASSERT_EQUAL_INT8(-50, y);  // not -100: that would bend it to -38 deg
  TEST_ASSERT_TRUE(acc.take(-90.0f, 300.0f, x, y));
  TEST_ASSERT_EQUAL_INT8(-38, x);
  TEST_ASSERT_EQUAL_INT8(127, y);
}

// ---- return value ----

void test_return_true_when_either_axis_nonzero(void) {
  CursorMotionAccumulator acc;
  int8_t x = 9, y = 9;
  TEST_ASSERT_FALSE(acc.take(0.0f, 0.0f, x, y));
  TEST_ASSERT_FALSE(acc.take(0.1f, -0.2f, x, y));  // fractions only
  TEST_ASSERT_TRUE(acc.take(0.0f, -1.0f, x, y));   // Y alone is enough
  TEST_ASSERT_EQUAL_INT8(0, x);
  TEST_ASSERT_EQUAL_INT8(-1, y);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_fraction_carries_until_whole_count);
  RUN_TEST(test_axes_carry_independently);
  RUN_TEST(test_instances_keep_separate_remainders);
  RUN_TEST(test_positive_input_truncates_toward_zero);
  RUN_TEST(test_negative_input_truncates_toward_zero);
  RUN_TEST(test_negative_fraction_carries_toward_minus_one);
  RUN_TEST(test_emitted_total_tracks_requested_total);
  RUN_TEST(test_overflow_clamped_to_127_and_dropped);
  RUN_TEST(test_exact_127_passes_and_128_clamps);
  RUN_TEST(test_negative_overflow_clamped_to_minus_127_on_y);
  RUN_TEST(test_clamp_keeps_fraction_but_drops_whole_overflow);
  RUN_TEST(test_fast_diagonal_keeps_its_direction_when_clamped);
  RUN_TEST(test_return_true_when_either_axis_nonzero);
  return UNITY_END();
}
