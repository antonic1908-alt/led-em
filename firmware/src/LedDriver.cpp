// LedDriver.cpp — реалізація ШІМ, плавного переходу і гамма-корекції.

#include "LedDriver.h"  // оголошення класу
#include "config.h"     // PWM_FREQ_HZ, PWM_BITS, LED_GAMMA
#include <math.h>       // powf, lroundf

void LedDriver::begin(uint32_t pin) {
  pin_ = pin;                                   // запам'ятовуємо пін
#if defined(ARDUINO_ARCH_ESP32)                 // --- ESP32 ---
  // LEDC — апаратний ШІМ-модуль ESP32. API відрізняється в Arduino-ESP32 2.x і 3.x.
  #if ESP_ARDUINO_VERSION_MAJOR >= 3            // нове API (3.x): прив'язка прямо до піна
  ledcAttach(pin_, PWM_FREQ_HZ, PWM_BITS);      // пін, частота 20 кГц, роздільність 10 біт
  #else                                         // старе API (2.x): через «канал» LEDC
  ledcSetup(0, PWM_FREQ_HZ, PWM_BITS);          // налаштувати канал 0
  ledcAttachPin(pin_, 0);                       // підключити пін до каналу 0
  #endif
#elif defined(ARDUINO_ARCH_STM32)               // --- STM32 ---
  // STM32duino сам налаштує таймер (TIM1 для PA8)
  analogWriteResolution(PWM_BITS);              // analogWrite приймає 0..1023
  analogWriteFrequency(PWM_FREQ_HZ);            // частота ШІМ 20 кГц
#endif
  writeDuty(0);                                 // на старті стрічка вимкнена
  lastUpdate_ = millis();                       // точка відліку для dt
}

void LedDriver::setTarget(float percent) {
  target_ = constrain(percent, 0.0f, 100.0f);   // constrain — Arduino-аналог Mathf.Clamp
}

void LedDriver::writeDuty(uint32_t duty) {
  if (duty == lastDuty_) return;   // те саме значення вже записане — не смикаємо периферію без потреби
  lastDuty_ = duty;                // запам'ятовуємо нове значення
#if defined(ARDUINO_ARCH_ESP32)    // --- ESP32 ---
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin_, duty);           // 3.x: пишемо за номером піна
  #else
  ledcWrite(0, duty);              // 2.x: пишемо за номером каналу
  #endif
#else                              // --- STM32 ---
  analogWrite(pin_, duty);         // у STM32duino analogWrite = ШІМ
#endif
}

void LedDriver::update(uint32_t now) {
  const uint32_t dt = now - lastUpdate_;  // скільки мс минуло з попереднього виклику
  lastUpdate_ = now;                      // запам'ятовуємо для наступного разу

  if (level_ != target_) {                // ще не дійшли до цілі
    if (fadeMs_ == 0) {                   // fade вимкнено...
      level_ = target_;                   // ...стрибаємо одразу
    } else {
      const float step = 100.0f * dt / fadeMs_;   // % за цей кадр (як speed * Time.deltaTime)
      if (level_ < target_) level_ = min(level_ + step, target_);  // ростемо, але не перескакуємо ціль
      else                  level_ = max(level_ - step, target_);  // зменшуємось, але не нижче цілі
    }
  }

  // Гамма-корекція: 50 % «на око» — це лише ~22 % часу ввімкнення
  const uint32_t maxDuty = (1UL << PWM_BITS) - 1;    // 1 << 10 = 1024; мінус 1 = 1023 — максимум ШІМ
  const float g = powf(level_ / 100.0f, LED_GAMMA);  // 0..1, піднесене до степеня 2.2
  uint32_t duty = (uint32_t)lroundf(g * maxDuty);    // перевести в 0..1023 з округленням
  if (level_ > 0 && duty == 0) duty = 1;             // якщо яскравість > 0, хоч трохи світимо (не 0 через округлення)
  writeDuty(duty);                                   // записати в периферію
}
