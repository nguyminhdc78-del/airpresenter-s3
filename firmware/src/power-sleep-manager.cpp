#include "power-sleep-manager.h"

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_system.h>

#include "remote-user-config.h"

namespace {
// Only used when every wake button is held (remote squeezed in a bag): wake up
// later and try again instead of sleeping until the battery is pulled.
constexpr uint64_t kNoButtonWakeRetryUs = 5ULL * 60ULL * 1000000ULL;
}  // namespace

void PowerSleepManager::begin(const uint8_t* buttonPins, uint8_t count) {
  pins_ = buttonPins;
  count_ = count;
  lastActivityMs_ = millis();
  // ESP-IDF: pins used for ext1 wake-up come back routed to the RTC domain and
  // must be released with rtc_gpio_deinit() before digital use.
  if (wokeFromDeepSleep()) {
    for (uint8_t i = 0; i < count_; ++i) {
      const gpio_num_t gpio = static_cast<gpio_num_t>(pins_[i]);
      if (rtc_gpio_is_valid_gpio(gpio)) rtc_gpio_deinit(gpio);
    }
  }
}

bool PowerSleepManager::shouldSleep(uint32_t now, bool connected) {
  if (wasConnected_ && !connected) lastActivityMs_ = now;  // link just dropped: wait for the PC
  wasConnected_ = connected;
  const uint32_t timeout = connected ? cfg::kIdleSleepTimeoutMs : cfg::kUnconnectedSleepTimeoutMs;
  return now - lastActivityMs_ >= timeout;
}

bool PowerSleepManager::userCameBack() const { return anyButtonPressed() && waitForButtonsReleased(2000); }

bool PowerSleepManager::waitForButtonsReleased(uint32_t timeoutMs) const {
  const uint32_t start = millis();
  while (anyButtonPressed()) {
    if (millis() - start >= timeoutMs) return false;
    delay(10);
  }
  return true;
}

void PowerSleepManager::enterDeepSleep() {
  Serial.println("[power] idle -> deep sleep (press any button to wake)");
  Serial.flush();

  waitForButtonsReleased(2000);  // a held button would wake us immediately

  uint64_t wakeMask = 0;
  for (uint8_t i = 0; i < count_; ++i) {
    const uint8_t pin = pins_[i];
    const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
    // GPIO0 is a boot strapping pin: waking with it held low is asking for trouble.
    if (pin == 0 || !rtc_gpio_is_valid_gpio(gpio)) continue;
    if (digitalRead(pin) == LOW) continue;  // stuck/held button must not block sleep
    rtc_gpio_pullup_en(gpio);  // keep the button pulled high while asleep
    rtc_gpio_pulldown_dis(gpio);
    wakeMask |= 1ULL << pin;
  }

  if (wakeMask != 0) {
    // RTC peripherals must stay powered for the internal pull-ups to work.
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
  } else {
    esp_sleep_enable_timer_wakeup(kNoButtonWakeRetryUs);
  }
  esp_deep_sleep_start();
}

bool PowerSleepManager::wokeFromDeepSleep() {
  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  return cause == ESP_SLEEP_WAKEUP_EXT1 || cause == ESP_SLEEP_WAKEUP_TIMER;
}

void PowerSleepManager::logWakeReason() {
  // A crash, watchdog or supply dip last time explains a "random" reconnect: say so.
  const esp_reset_reason_t reset = esp_reset_reason();
  if (reset == ESP_RST_PANIC) Serial.println("[power] last reset: firmware crash (panic)");
  if (reset == ESP_RST_INT_WDT || reset == ESP_RST_TASK_WDT || reset == ESP_RST_WDT) {
    Serial.println("[power] last reset: watchdog (firmware stuck)");
  }
  if (reset == ESP_RST_BROWNOUT) Serial.println("[power] last reset: brownout - supply voltage dipped, check wiring");

  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) return;
  const uint64_t status = esp_sleep_get_ext1_wakeup_status();
  for (uint8_t pin = 0; pin < 64; ++pin) {
    if (status & (1ULL << pin)) Serial.printf("[power] woke up by GPIO%u\n", pin);
  }
}

bool PowerSleepManager::anyButtonPressed() const {
  for (uint8_t i = 0; i < count_; ++i) {
    if (digitalRead(pins_[i]) == LOW) return true;
  }
  return false;
}
