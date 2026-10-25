// Host tests for NoseUpPointingAxisDetector: pointing the nose at the ceiling and pressing
// AIR must reveal the nose axis (with sign) for any GY-521 mounting, and no other pose may
// be saved as the nose.
// Run: pio test -e native -f test_nose_up_pointing_axis
#include <unity.h>

#include <cmath>

#include "nose-up-pointing-axis-detector.h"

void setUp(void) {}
void tearDown(void) {}

namespace {

using Result = NoseUpPointingAxisDetector::Result;
constexpr float kDt = 0.0075f;  // motion tick
constexpr float kDegToRad = 3.14159265f / 180.0f;

NoseUpPointingAxisDetector startedFrom(float x, float y, float z) {
  NoseUpPointingAxisDetector d;
  const float up[3] = {x, y, z};
  d.start(up);
  return d;
}

// One tick with AIR just pressed while gravity points along (x, y, z).
Result confirmAt(NoseUpPointingAxisDetector& d, float x, float y, float z) {
  const float up[3] = {x, y, z};
  return d.update(up, true, kDt);
}

}  // namespace

void test_chip_up_nose_is_plus_x(void) {
  NoseUpPointingAxisDetector d = startedFrom(0, 0, 1);  // held normally, chip up
  TEST_ASSERT_TRUE(d.active());
  TEST_ASSERT_EQUAL(Result::Found, confirmAt(d, 1, 0, 0));
  TEST_ASSERT_EQUAL_UINT8(0, d.axis());
  TEST_ASSERT_EQUAL_INT8(1, d.sign());
  TEST_ASSERT_FALSE(d.active());
}

void test_chip_down_nose_is_minus_y(void) {
  NoseUpPointingAxisDetector d = startedFrom(0, 0, -1);  // module upside down
  TEST_ASSERT_EQUAL(Result::Found, confirmAt(d, 0, -1, 0));
  TEST_ASSERT_EQUAL_UINT8(1, d.axis());
  TEST_ASSERT_EQUAL_INT8(-1, d.sign());
}

void test_nothing_is_saved_without_the_press_and_it_times_out(void) {
  NoseUpPointingAxisDetector d = startedFrom(0, 0, 1);
  const float noseUp[3] = {1, 0, 0};
  Result r = Result::Waiting;
  float t = 0.0f;
  for (; t < nose_setup::kTimeoutSec + 1.0f && r == Result::Waiting; t += kDt) r = d.update(noseUp, false, kDt);
  TEST_ASSERT_EQUAL(Result::TimedOut, r);
  TEST_ASSERT_TRUE(t > nose_setup::kTimeoutSec);
  TEST_ASSERT_FALSE(d.active());
}

void test_pressing_air_in_normal_use_cancels_instead_of_saving(void) {
  NoseUpPointingAxisDetector d = startedFrom(0, 0, 1);
  TEST_ASSERT_EQUAL(Result::Rejected, confirmAt(d, 0.1f, 0, 0.99f));  // top face up
  TEST_ASSERT_FALSE(d.active());
}

void test_top_face_axis_is_rejected_with_either_sign(void) {
  NoseUpPointingAxisDetector d = startedFrom(0, 0, 1);
  TEST_ASSERT_EQUAL(Result::Rejected, confirmAt(d, 0, 0, -1));  // remote lying on its back
}

void test_setup_started_with_the_nose_tilted_up_still_works(void) {
  const float a = 40.0f * kDegToRad;  // FN released while looking at the LED
  NoseUpPointingAxisDetector d = startedFrom(std::sin(a), 0, std::cos(a));
  TEST_ASSERT_EQUAL(Result::Found, confirmAt(d, 1, 0, 0));
  TEST_ASSERT_EQUAL_UINT8(0, d.axis());
}

void test_snaps_a_rough_pose_but_not_an_ambiguous_one(void) {
  const float a = 25.0f * kDegToRad;
  NoseUpPointingAxisDetector d = startedFrom(0, 0, 1);
  TEST_ASSERT_EQUAL(Result::Found, confirmAt(d, std::cos(a), 0, std::sin(a)));
  TEST_ASSERT_EQUAL_UINT8(0, d.axis());

  NoseUpPointingAxisDetector e = startedFrom(0, 0, 1);
  const float h = std::sqrt(0.5f);
  TEST_ASSERT_EQUAL(Result::Rejected, confirmAt(e, h, -h, 0));  // halfway between +X and -Y
}

void test_missing_sensor_data_is_rejected(void) {
  NoseUpPointingAxisDetector d = startedFrom(0, 0, 1);
  TEST_ASSERT_EQUAL(Result::Rejected, confirmAt(d, 0, 0, 0));
}

void test_idle_detector_ignores_presses(void) {
  NoseUpPointingAxisDetector d;
  TEST_ASSERT_FALSE(d.active());
  TEST_ASSERT_EQUAL(Result::Waiting, confirmAt(d, 1, 0, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_chip_up_nose_is_plus_x);
  RUN_TEST(test_chip_down_nose_is_minus_y);
  RUN_TEST(test_nothing_is_saved_without_the_press_and_it_times_out);
  RUN_TEST(test_pressing_air_in_normal_use_cancels_instead_of_saving);
  RUN_TEST(test_top_face_axis_is_rejected_with_either_sign);
  RUN_TEST(test_setup_started_with_the_nose_tilted_up_still_works);
  RUN_TEST(test_snaps_a_rough_pose_but_not_an_ambiguous_one);
  RUN_TEST(test_missing_sensor_data_is_rejected);
  RUN_TEST(test_idle_detector_ignores_presses);
  return UNITY_END();
}
