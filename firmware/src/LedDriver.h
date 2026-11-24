// LedDriver.h — ШІМ на затвор AO3400, гамма-корекція і плавний перехід.
// Аналогія з Unity: твін (DOTween) значення яскравості до цілі.
// ШІМ (PWM) — пін дуже швидко (20 000 разів на секунду) вмикається і вимикається;
// чим більша частка часу «ввімкнено» (duty), тим яскравіше світить стрічка.

#pragma once          // підключати лише один раз
#include <Arduino.h>  // типи, UINT32_MAX

class LedDriver {
 public:
  void begin(uint32_t pin);                       // налаштувати ШІМ на піні (один раз у setup)
  void setTarget(float percent);                  // задати цільову яскравість 0..100 %
  void setFadeMs(uint16_t ms) { fadeMs_ = ms; }   // задати час плавного переходу 0→100 %
  void update(uint32_t now);                      // викликати кожен цикл: крок до цілі + запис у ШІМ
  float level() const { return level_; }          // поточна яскравість, %
  float target() const { return target_; }        // цільова яскравість, %

 private:
  void writeDuty(uint32_t duty);    // записати заповнення ШІМ (0..1023) у периферію
  uint32_t pin_ = 0;                // пін ШІМ
  float level_ = 0;                 // поточна яскравість, %
  float target_ = 0;                // цільова яскравість, %
  uint16_t fadeMs_ = 800;           // час переходу 0→100 %, мс
  uint32_t lastUpdate_ = 0;         // час попереднього update — для обчислення dt (як Time.deltaTime)
  uint32_t lastDuty_ = UINT32_MAX;  // останнє записане duty; UINT32_MAX = «ще нічого не писали»
};
