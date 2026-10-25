// Host tests for GravityReference: the same hand motion must move the cursor the same way
// whether the GY-521 sits chip up, chip down or the remote is rolled or tilted.
// Conventions: sensor-frame gyro rates in deg/s (right-hand rule); forward = sensor +X.
// Run: pio test -e native -f test_gravity_axes
#include <unity.h>

#include <cmath>

#include "gravity-referenced-screen-axes.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

const float kForwardX[3] = {1.0f, 0.0f, 0.0f};

// Gravity estimate settled on one accelerometer reading (the first sample sets it).
GravityReference settledOn(float ax, float ay, float az) {
  GravityReference g;
  const float a[3] = {ax, ay, az};
  g.addAccel(a, 0.05f);
  return g;
}

// Gyro rates for a turn of `dps` about `axis` (sensor frame, unit vector).
void spinAbout(const float axis[3], float dps, float out[3]) {
  for (int i = 0; i < 3; ++i) out[i] = axis[i] * dps;
}

// Turn left = counter-clockwise seen from above = positive turn about world "up".
// Nose up = negative turn about world "left" (= up x forward).
void checkTurnLeftAndNoseUp(const GravityReference& g, const float up[3], const float left[3]) {
  float r[3];
  spinAbout(up, 30.0f, r);
  ScreenRates s = g.toScreen(r, kForwardX);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, -30.0f, s.right);  // cursor goes left
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, s.down);
  spinAbout(left, -30.0f, r);
  s = g.toScreen(r, kForwardX);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, s.right);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, -30.0f, s.down);  // cursor goes up
}

}  // namespace

void test_chip_up(void) {
  const float up[3] = {0, 0, 1}, left[3] = {0, 1, 0};
  checkTurnLeftAndNoseUp(settledOn(0, 0, 1), up, left);
}

void test_chip_down_moves_the_cursor_the_same_way(void) {
  // Module flipped over its X axis: sensor Z and Y now point down and right.
  const float up[3] = {0, 0, -1}, left[3] = {0, -1, 0};
  checkTurnLeftAndNoseUp(settledOn(0, 0, -1), up, left);
}

void test_remote_rolled_onto_its_side(void) {
  // Rolled 90 degrees: sensor +Y points up, so left = up x forward = -Z.
  const float up[3] = {0, 1, 0}, left[3] = {0, 0, -1};
  checkTurnLeftAndNoseUp(settledOn(0, 1, 0), up, left);
}

void test_pointing_40_degrees_up_still_turns_about_vertical(void) {
  const float t = 40.0f * 3.14159265f / 180.0f;
  const float up[3] = {std::sin(t), 0, std::cos(t)}, left[3] = {0, 1, 0};
  checkTurnLeftAndNoseUp(settledOn(std::sin(t), 0, std::cos(t)), up, left);
}

void test_a_one_sample_shake_barely_moves_the_estimate(void) {
  GravityReference g;
  TEST_ASSERT_FALSE(g.ready());
  const float rest[3] = {0, 0, 1}, shake[3] = {1, 0, 1};
  g.addAccel(rest, 0.05f);
  TEST_ASSERT_TRUE(g.ready());
  g.addAccel(shake, 0.05f);  // 1 g sideways jolt for one tick
  const float up[3] = {0, 0, 1};
  float r[3];
  spinAbout(up, 30.0f, r);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, -30.0f, g.toScreen(r, kForwardX).right);
}

void test_pointing_straight_up_gives_no_pitch_and_finite_values(void) {
  GravityReference g = settledOn(1, 0, 0);  // nose straight up: pitch axis undefined
  const float r[3] = {10, 20, 30};
  const ScreenRates s = g.toScreen(r, kForwardX);
  TEST_ASSERT_TRUE(std::isfinite(s.right) && std::isfinite(s.down));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.down);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, -10.0f, s.right);  // yaw about vertical still works
}

void test_gyro_turns_up_with_the_remote(void) {
  // Roll +90 deg about the nose (X) in 7.5 ms steps: sensor +Y ends up pointing up.
  GravityReference g = settledOn(0, 0, 1);
  const float rates[3] = {90.0f, 0.0f, 0.0f};
  for (int i = 0; i < 133; ++i) g.rotate(rates, 0.0075f);  // ~1 s
  const float* u = g.up();
  const float n = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
  TEST_ASSERT_FLOAT_WITHIN(0.03f, 0.0f, u[0] / n);
  TEST_ASSERT_FLOAT_WITHIN(0.03f, 1.0f, u[1] / n);
  TEST_ASSERT_FLOAT_WITHIN(0.03f, 0.0f, u[2] / n);
}

void test_swinging_the_remote_does_not_tilt_up(void) {
  GravityReference g = settledOn(0, 0, 1);
  const float swing[3] = {0.6f, 0.0f, 1.0f};  // 1.17 g: the hand is accelerating it
  for (int i = 0; i < 50; ++i) g.addAccel(swing, 0.05f);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, g.up()[0]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_chip_up);
  RUN_TEST(test_chip_down_moves_the_cursor_the_same_way);
  RUN_TEST(test_remote_rolled_onto_its_side);
  RUN_TEST(test_pointing_40_degrees_up_still_turns_about_vertical);
  RUN_TEST(test_a_one_sample_shake_barely_moves_the_estimate);
  RUN_TEST(test_pointing_straight_up_gives_no_pitch_and_finite_values);
  RUN_TEST(test_gyro_turns_up_with_the_remote);
  RUN_TEST(test_swinging_the_remote_does_not_tilt_up);
  return UNITY_END();
}
