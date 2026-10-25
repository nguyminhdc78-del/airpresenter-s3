// Host unit tests for GyroAirMouseMapper dynamics: speed law, acceleration curve, smoothing
// (light low-pass + soft tiered smoothing) and its independence from the tick length.
// Part of the test_air_mouse_mapper suite; RUN_TEST calls are issued from runDynamicsTests().
#include <unity.h>

#include <cmath>

#include "air-mouse-mapper-test-support.h"

using namespace mapper_test;

namespace {

constexpr float kPi = 3.14159265f;

void test_output_matches_speed_law_after_filter_settles(void) {
  // Slow (fully smoothed tier), middle (blended) and fast (direct) rates, both signs.
  const float rateSet[] = {3.0f, 8.0f, 10.0f, 45.0f, 120.0f, 500.0f, -25.0f};  // 500 = full scale
  for (float r : rateSet) {
    const float expected = speedFor(r) * kDt;
    TEST_ASSERT_FLOAT_WITHIN(tolFor(expected), expected, settledDownStep(r));
  }
}

void test_acceleration_is_superlinear(void) {
  // speed ~ m + m^2 / knee: doubling the rate must more than double the counts per tick.
  const float rateSet[] = {10.0f, 20.0f, 50.0f, 100.0f};
  for (float r : rateSet) {
    const float once = settledDownStep(r);
    const float twice = settledDownStep(2.0f * r);
    TEST_ASSERT_TRUE(std::fabs(twice) > 2.0f * std::fabs(once));
  }
}

void test_output_rises_monotonically_to_the_speed_law(void) {
  const float inputs[] = {20.0f, 6.0f};  // fast and slow-tier turns
  for (float input : inputs) {
    GyroAirMouseMapper m;
    float prev = 0.0f;
    for (int n = 1; n <= 800; ++n) {
      float dx = 0.0f, dy = 0.0f;  // per-tick increment only
      m.update(0.0f, input, true, kDt, dx, dy);
      TEST_ASSERT_TRUE(dy >= prev - 1e-6f);  // never dips while the rate is held: no wobble
      prev = dy;
    }
    TEST_ASSERT_FLOAT_WITHIN(tolFor(speedFor(input) * kDt), speedFor(input) * kDt, prev);
  }
}

void test_tick_length_does_not_change_the_motion(void) {
  // Same turn, ticks of 7.5 ms, 3.75 ms and alternating 6.5 / 8.5 ms (late + catch-up).
  const float inputs[] = {25.0f, 6.0f};
  for (float input : inputs) {
    float x = 0.0f, ref = 0.0f, half = 0.0f;
    totalsOver(0.0f, input, 0.6f, kDt, x, ref);
    totalsOver(0.0f, input, 0.6f, kDt / 2.0f, x, half);
    GyroAirMouseMapper m;
    float jx = 0.0f, jittered = 0.0f;
    for (int i = 0; i < 80; ++i) m.update(0.0f, input, true, (i % 2) ? 0.0085f : 0.0065f, jx, jittered);
    TEST_ASSERT_FLOAT_WITHIN(0.02f * ref, ref, half);
    TEST_ASSERT_FLOAT_WITHIN(0.02f * ref, ref, jittered);
  }
}

void test_flick_then_stop_does_not_coast(void) {
  // 80 dps flick for 0.3 s, then the hand stops: the heavy filter must not keep the cursor going.
  GyroAirMouseMapper m;
  float dx = 0.0f, during = 0.0f, after = 0.0f;
  runConstant(m, 0.0f, 80.0f, true, 40, dx, during);
  runConstant(m, 0.0f, 0.0f, true, 80, dx, after);
  TEST_ASSERT_TRUE(after > 0.0f);  // the light filter still eases out
  TEST_ASSERT_TRUE(after < 0.06f * during);
}

void test_slow_aiming_tremor_is_steadied(void) {
  // Slow 6 dps aim with an 8 Hz +-3 dps hand tremor: the cursor speed ripple must be well
  // below what the unsmoothed speed law would give for the same input. The old fixed 21 ms
  // filter passed 0.69 of it; the speed-adaptive one passes about 0.38.
  GyroAirMouseMapper m;
  float lo = 1e9f, hi = -1e9f, rawLo = 1e9f, rawHi = -1e9f;
  for (int n = 0; n < 400; ++n) {
    const float t = n * kDt;
    const float rate = 6.0f + 3.0f * std::sin(2.0f * kPi * 8.0f * t);
    float dx = 0.0f, dy = 0.0f;
    m.update(0.0f, rate, true, kDt, dx, dy);
    if (n < 200) continue;  // skip the start-up
    lo = std::fmin(lo, dy);
    hi = std::fmax(hi, dy);
    rawLo = std::fmin(rawLo, speedFor(rate) * kDt);
    rawHi = std::fmax(rawHi, speedFor(rate) * kDt);
  }
  TEST_ASSERT_TRUE(hi - lo < 0.45f * (rawHi - rawLo));
}

void test_cursor_stops_after_motion_ends(void) {
  GyroAirMouseMapper m;
  float dx = 0.0f, dy = 0.0f;
  runConstant(m, 0.0f, 30.0f, true, 400, dx, dy);  // filters settle on 30 dps
  // Hand stopped: output decays with the filters until it sits inside the dead-zone (exactly 0).
  constexpr int kTicks = 300;
  float stepY[kTicks] = {};
  for (int n = 0; n < kTicks; ++n) {
    float sx = 0.0f;
    m.update(0.0f, 0.0f, true, kDt, sx, stepY[n]);
  }
  TEST_ASSERT_TRUE(stepY[0] > 0.0f);  // still easing out right after the stop
  int stop = 0;
  while (stop < kTicks && stepY[stop] != 0.0f) ++stop;
  TEST_ASSERT_TRUE(stop < kTicks);  // movement does end
  for (int n = stop; n < kTicks; ++n) TEST_ASSERT_EQUAL_FLOAT(0.0f, stepY[n]);  // and never restarts
}

}  // namespace

void runDynamicsTests(void) {
  RUN_TEST(test_output_matches_speed_law_after_filter_settles);
  RUN_TEST(test_acceleration_is_superlinear);
  RUN_TEST(test_output_rises_monotonically_to_the_speed_law);
  RUN_TEST(test_tick_length_does_not_change_the_motion);
  RUN_TEST(test_flick_then_stop_does_not_coast);
  RUN_TEST(test_slow_aiming_tremor_is_steadied);
  RUN_TEST(test_cursor_stops_after_motion_ends);
}
