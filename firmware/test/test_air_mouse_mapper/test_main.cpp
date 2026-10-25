// Host unit tests for GyroAirMouseMapper: clutch gating, dead-zone, right/down routing and
// the output contract. Dynamics (speed law, acceleration, smoothing) live in
// air-mouse-mapper-dynamics-tests.cpp, which this file runs via runDynamicsTests().
// Run: pio test -e native -f test_air_mouse_mapper
#include <unity.h>

#include <cmath>

#include "air-mouse-mapper-test-support.h"

using namespace mapper_test;

void setUp(void) {}
void tearDown(void) {}

void runDynamicsTests(void);  // defined in air-mouse-mapper-dynamics-tests.cpp

namespace {

// Movement from a fresh mapper after 60 ticks of constant rates.
void totalsAfter60(float right, float down, float& dx, float& dy) {
  GyroAirMouseMapper m;
  dx = 0.0f;
  dy = 0.0f;
  runConstant(m, right, down, true, 60, dx, dy);
}

}  // namespace

// ---- clutch gating ----

void test_clutch_released_no_movement_and_outputs_untouched(void) {
  GyroAirMouseMapper m;
  float dx = 2.5f, dy = -1.25f;
  runConstant(m, 80.0f, 80.0f, false, 50, dx, dy);
  TEST_ASSERT_EQUAL_FLOAT(2.5f, dx);
  TEST_ASSERT_EQUAL_FLOAT(-1.25f, dy);
}

void test_clutch_release_restarts_filter_on_press(void) {
  GyroAirMouseMapper used;
  float dx = 0.0f, dy = 0.0f;
  runConstant(used, 60.0f, -40.0f, true, 40, dx, dy);          // filter settles
  float ignoredX = 0.0f, ignoredY = 0.0f;
  used.update(60.0f, -40.0f, false, kDt, ignoredX, ignoredY);  // release resets the filter
  float usedX = 0.0f, usedY = 0.0f;
  used.update(60.0f, -40.0f, true, kDt, usedX, usedY);         // first tick after re-press
  GyroAirMouseMapper fresh;
  float freshX = 0.0f, freshY = 0.0f;
  fresh.update(60.0f, -40.0f, true, kDt, freshX, freshY);
  TEST_ASSERT_TRUE(freshX != 0.0f && freshY != 0.0f);  // non-vacuous: first tick moves
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, freshX, usedX);      // no carry-over from the old hold
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, freshY, usedY);
}

// ---- dead-zone (hand tremor) ----

void test_rates_inside_deadzone_give_zero(void) {
  // Combined turn speed <= kAirDeadzoneDps is hand tremor: no movement, the edge included.
  const float dz = cfg::kAirDeadzoneDps;
  const float cases[4][2] = {{dz, 0.0f}, {0.0f, -dz}, {0.6f * dz, -0.7f * dz}, {-0.5f * dz, 0.5f * dz}};
  for (int c = 0; c < 4; ++c) {
    GyroAirMouseMapper m;
    float dx = 0.0f, dy = 0.0f;
    runConstant(m, cases[c][0], cases[c][1], true, 100, dx, dy);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dy);
  }
}

// ---- routing: right -> dx, down -> dy, independently ----

void test_right_drives_dx_and_down_drives_dy(void) {
  float rx = 0.0f, ry = 0.0f;
  totalsAfter60(30.0f, 0.0f, rx, ry);
  TEST_ASSERT_TRUE(rx > 0.0f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, ry);

  float lx = 0.0f, ly = 0.0f;
  totalsAfter60(-30.0f, 0.0f, lx, ly);  // left is the mirror image
  TEST_ASSERT_FLOAT_WITHIN(tolFor(rx), -rx, lx);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, ly);

  float dx = 0.0f, dy = 0.0f;
  totalsAfter60(0.0f, 30.0f, dx, dy);  // vertical behaves exactly like horizontal
  TEST_ASSERT_FLOAT_WITHIN(tolFor(rx), rx, dy);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
}

// ---- diagonals keep their direction ----

void test_diagonal_direction_is_preserved(void) {
  // Fast (acceleration curve) and slow (near the dead-zone) diagonals, both quadrants.
  const float cases[4][2] = {{40.0f, 20.0f}, {-30.0f, 3.0f}, {2.0f, -1.0f}, {-90.0f, -45.0f}};
  for (int c = 0; c < 4; ++c) {
    float dx = 0.0f, dy = 0.0f;
    totalsAfter60(cases[c][0], cases[c][1], dx, dy);
    TEST_ASSERT_TRUE(dx != 0.0f && dy != 0.0f);  // the smaller axis is not swallowed
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, cases[c][1] / cases[c][0], dy / dx);
  }
}

void test_diagonal_speed_follows_the_combined_turn_rate(void) {
  // A 3-4-5 diagonal travels as far as a straight 50 dps turn, split 3:4.
  float dx = 0.0f, dy = 0.0f;
  totalsAfter60(30.0f, 40.0f, dx, dy);
  float straight = 0.0f, zero = 0.0f;
  totalsAfter60(50.0f, 0.0f, straight, zero);
  TEST_ASSERT_FLOAT_WITHIN(tolFor(straight), straight, std::sqrt(dx * dx + dy * dy));
  TEST_ASSERT_FLOAT_WITHIN(tolFor(straight), 0.6f * straight, dx);
}

// ---- output contract ----

void test_outputs_accumulate_and_zero_dt_adds_nothing(void) {
  GyroAirMouseMapper fresh;
  float baseX = 0.0f, baseY = 0.0f;
  fresh.update(0.0f, 25.0f, true, kDt, baseX, baseY);
  // Caller may already hold D-pad nudge motion: contributions must be added, not assigned.
  GyroAirMouseMapper preloaded;
  float dx = 4.0f, dy = -2.0f;
  preloaded.update(0.0f, 25.0f, true, kDt, dx, dy);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 4.0f, dx);           // no right rate, dx unchanged
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -2.0f + baseY, dy);  // down added on top

  GyroAirMouseMapper idle;
  float zeroX = 0.0f, zeroY = 0.0f;
  for (int i = 0; i < 10; ++i) idle.update(0.0f, 25.0f, true, 0.0f, zeroX, zeroY);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, zeroX);  // no elapsed time, no movement
  TEST_ASSERT_EQUAL_FLOAT(0.0f, zeroY);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_clutch_released_no_movement_and_outputs_untouched);
  RUN_TEST(test_clutch_release_restarts_filter_on_press);
  RUN_TEST(test_rates_inside_deadzone_give_zero);
  RUN_TEST(test_right_drives_dx_and_down_drives_dy);
  RUN_TEST(test_diagonal_direction_is_preserved);
  RUN_TEST(test_diagonal_speed_follows_the_combined_turn_rate);
  RUN_TEST(test_outputs_accumulate_and_zero_dt_adds_nothing);
  runDynamicsTests();
  return UNITY_END();
}
