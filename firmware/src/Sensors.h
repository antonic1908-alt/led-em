// Sensors.h — датчики світла, руху та дистанції.
// Кожен клас має begin() (як Start у Unity) і update(now) (як Update).
// update нічого не чекає (майже — див. HC-SR04), тому loop() крутиться швидко.

#pragma once          // підключати лише один раз
#include <Arduino.h>  // базові типи і функції Arduino
#include "Settings.h" // enum LightSource (BH1750 / аналоговий)

// ---------------- Світло ----------------
class LightSensor {
 public:
  void begin();                                 // знайти BH1750 і налаштувати аналоговий вхід
  void update(uint32_t now, uint16_t source);   // оновити значення; source: LightSource з налаштувань
  float lux() const { return lux_; }            // поточна (згладжена) освітленість у люксах
  bool bhFound() const { return bhOk_; }        // чи відповів BH1750 при старті
  bool usingBh() const { return usingBh_; }     // чи зараз реально використовується BH1750
  const char* name() const { return usingBh_ ? "BH1750" : "ADC"; }  // назва джерела для дисплея/логу
                                                // const char* — вказівник на рядок у стилі C (аналог string, лише для читання)

 private:
  float readAnalogLux();   // прочитати аналоговий датчик і перерахувати напругу в люкси
  bool bhOk_ = false;      // BH1750 знайдено на шині I2C
  bool usingBh_ = false;   // останнє вимірювання зроблено через BH1750
  bool first_ = true;      // перше вимірювання (для нього не робимо згладжування і не чекаємо період)
  float lux_ = 0;          // згладжене значення, lx
  uint32_t last_ = 0;      // час (millis) останнього вимірювання
};

// ---------------- Рух (HC-SR501) ----------------
class MotionSensor {
 public:
  void begin();                                  // налаштувати пін PIR як вхід
  void update(uint32_t now, uint16_t filterMs);  // прочитати пін; filterMs — мінімальна тривалість сигналу
  bool motion() const { return motion_; }        // рух з урахуванням фільтра
  bool raw() const { return raw_; }              // «сирий» стан піна без фільтра

 private:
  bool raw_ = false;        // останній прочитаний стан піна (true = HIGH)
  bool motion_ = false;     // результат з урахуванням фільтра
  uint32_t highSince_ = 0;  // з якого моменту пін тримається HIGH
};

// ---------------- Дистанція (VL53L0X або HC-SR04) ----------------
class DistanceSensor {
 public:
  void begin();                                // ініціалізувати датчик (який саме — залежить від SIM_MODE)
  void update(uint32_t now);                   // раз на DIST_PERIOD_MS зробити вимірювання
  bool found() const { return ok_; }           // датчик відповів при старті
  bool valid() const { return valid_; }        // є об'єкт у межах вимірювання
  uint16_t cm() const { return cm_; }          // відстань у сантиметрах (0, якщо не valid)
  const char* name() const;                    // назва датчика (тіло — у Sensors.cpp)

 private:
  bool ok_ = false;        // датчик знайдено
  bool valid_ = false;     // останнє вимірювання коректне
  uint16_t cm_ = 0;        // остання відстань, см
  uint32_t last_ = 0;      // час останнього вимірювання
};
