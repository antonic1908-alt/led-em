// Controller.h — «мозок»: режими АВТО/РУЧНИЙ і скінченний автомат OFF → ON → HOLD → OFF.
// Аналогія з Unity: Animator зі станами або GameManager зі switch(state).
// Контролер нічого не знає про піни: він отримує «знімок» датчиків (Inputs)
// і вирішує, якою має бути яскравість. Так його легко тестувати і переносити на іншу плату.

#pragma once          // підключати лише один раз
#include <Arduino.h>  // базові типи
#include "Settings.h" // налаштування (пороги, режим, логіка датчиків)

// Стани автомата: Off — вимкнено, On — є тригер, Hold — тригер зник, світимо ще holdSec секунд.
enum class State : uint8_t { Off, On, Hold };

struct Inputs {          // знімок датчиків на цей кадр (заповнюється в main.cpp)
  float lux;             // освітленість, lx
  bool motion;           // PIR бачить рух
  bool distValid;        // датчик дистанції бачить об'єкт
  uint16_t distCm;       // відстань до об'єкта, см
  bool distAvailable;    // чи взагалі є датчик дистанції
};

class Controller {
 public:
  // Конструктор. explicit — заборонити неявне перетворення Settings → Controller.
  // ": s_(s)" — список ініціалізації: поле-посилання s_ «прив'язується» до s (посилання можна задати лише тут).
  explicit Controller(Settings& s) : s_(s) {}

  void update(uint32_t now, const Inputs& in);  // один «тік» логіки: оновити тригер, «темно» і стан
  float targetPercent() const;     // яку яскравість віддати в LedDriver (0 або brightness)

  // Команди від кнопок
  void toggleMode(uint32_t now);   // довге MANUAL: АВТО <-> РУЧНИЙ
  void manualToggle(uint32_t now); // коротке MANUAL: увімкнути/вимкнути вручну

  RunMode mode() const { return (RunMode)s_.mode; }  // поточний режим; (RunMode) — приведення типу, як у C#
  State state() const { return state_; }             // поточний стан автомата
  bool dark() const { return dark_; }                // чи вважаємо, що зараз темно
  bool trigger() const { return trigger_; }          // чи є «людина» за обраною логікою датчиків
  bool isNear() const { return near_; }              // чи об'єкт ближче за поріг
  uint32_t holdLeftSec(uint32_t now) const;          // скільки секунд лишилось у стані Hold

 private:
  Settings& s_;                // ПОСИЛАННЯ на спільні налаштування (не копія: зміни в меню одразу видно тут)
  State state_ = State::Off;   // поточний стан автомата
  bool manualOn_ = false;      // у ручному режимі: стрічка ввімкнена?
  bool dark_ = false;          // результат оцінки «темно» (з гістерезисом)
  bool darkInit_ = false;      // чи вже робили першу оцінку «темно»
  bool trigger_ = false;       // результат логіки датчиків
  bool near_ = false;          // об'єкт ближче за поріг
  uint32_t holdStart_ = 0;     // коли почався стан Hold (millis)
};

const char* stateName(State st);  // назва стану для логу ("OFF"/"ON"/"HOLD"); вільна функція поза класом
