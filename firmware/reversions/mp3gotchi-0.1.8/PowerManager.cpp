#include "PowerManager.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include "esp_sleep.h"
#include "esp_bt.h"
#include <WiFi.h>

RTC_DATA_ATTR uint32_t mp3GotchiSleepMagic = 0;
static constexpr uint32_t SLEEP_MAGIC = 0x4D503347; // MP3G

void PowerManager::begin() {
  pinMode(Pins::ENCODER_SW, INPUT);
}

void PowerManager::markNormalBoot() {
  mp3GotchiSleepMagic = 0;
}

void PowerManager::enterDeepSleep() {
  mp3GotchiSleepMagic = SLEEP_MAGIC;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  while (digitalRead(Pins::ENCODER_SW) == LOW) {
    delay(10);
  }
  delay(120);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 0);
  Serial.flush();
  esp_deep_sleep_start();
}
