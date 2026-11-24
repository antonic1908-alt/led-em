// Sensors.cpp — реалізація датчиків світла, руху та дистанції.

#include "Sensors.h"   // оголошення наших класів
#include "config.h"    // піни, адреси, константи
#include <Wire.h>      // бібліотека шини I2C (Wire = «два дроти»)
#include <BH1750.h>    // бібліотека датчика світла BH1750 (claws/BH1750)
#include <VL53L0X.h>   // бібліотека датчика дистанції VL53L0X (pololu/VL53L0X)
#include <math.h>      // математика: powf (степінь)

// Об'єкти бібліотек — по одному на всю програму, видимі лише в цьому файлі.
namespace {
  BH1750 bh(I2C_ADDR_BH1750);  // драйвер BH1750 з адресою 0x23 (створюється ще до setup())
  VL53L0X vl53;                // драйвер VL53L0X (адреса за замовчуванням 0x29)
}

// ============================================================================
// LightSensor
// ============================================================================
void LightSensor::begin() {
#if defined(ARDUINO_ARCH_STM32)       // на STM32...
  analogReadResolution(12);           // ...АЦП читати з 12-бітною точністю (0..4095); за замовчуванням там 10 біт
#endif
  pinMode(PIN_LIGHT_ADC, INPUT);      // пін аналогового датчика — вхід
  // Запустити BH1750 у режимі безперервного вимірювання з високою точністю (1 lx).
  // &Wire — передаємо адресу об'єкта шини I2C (вказівник), а не копію.
  bhOk_ = bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, I2C_ADDR_BH1750, &Wire);
  // printf: %s підставляє рядок; умова ? a : b — як у C#.
  Serial.printf("[light] BH1750 %s\n", bhOk_ ? "found" : "NOT found -> analog fallback");
}

float LightSensor::readAnalogLux() {
  // Усереднюємо 8 вимірювань — АЦП «шумить».
  uint32_t mvSum = 0;                                      // сума мілівольтів
  for (int i = 0; i < 8; ++i) {                            // 8 разів поспіль
#if defined(ARDUINO_ARCH_ESP32)                            // ESP32 вміє одразу віддавати...
    mvSum += analogReadMilliVolts(PIN_LIGHT_ADC);          // ...калібровані мілівольти
#else                                                      // STM32 повертає «сирі» 0..4095...
    mvSum += (uint32_t)analogRead(PIN_LIGHT_ADC) * 3300UL / 4095UL;  // ...переводимо в мВ: код * 3300 / 4095
                                                           // UL = unsigned long, щоб множення не переповнилось
#endif
  }
  float v = (mvSum / 8.0f) / 1000.0f;                      // середнє, перераховане у вольти
  const float vcc = ADC_VREF_V;                            // напруга живлення дільника (3.3 В)
  if (v < 0.005f) v = 0.005f;                              // захист від ділення на нуль у формулах нижче
  if (v > vcc - 0.005f) v = vcc - 0.005f;                  // і з іншого боку (vcc - v не може бути 0)

#if ANALOG_LIGHT_KIND == 2                                 // --- варіант: TEMT6000 ---
  // TEMT6000: струм фототранзистора I = U / 10k; приблизно 2 lx на 1 мкА.
  return (v / 10000.0f) * 1e6f * 2.0f;                     // струм в амперах → мікроампери (×1e6) → lx (×2)
#else                                                      // --- варіант: фоторезистор (LDR) ---
  // Опір фоторезистора з дільника напруги
  #if ANALOG_LIGHT_KIND == 0
  float rLdr = LDR_FIXED_OHM * v / (vcc - v);        // LDR внизу (між виходом і GND)
  #else
  float rLdr = LDR_FIXED_OHM * (vcc - v) / v;        // LDR вгорі (між 3V3 і виходом)
  #endif
  // Модель фоторезистора: R = RL10 * (lux/10)^(-gamma)  =>  lux = 10 * (RL10/R)^(1/gamma)
  return 10.0f * powf(LDR_RL10_OHM / rLdr, 1.0f / LDR_GAMMA);  // powf(a, b) = a у степені b (як Mathf.Pow)
#endif
}

void LightSensor::update(uint32_t now, uint16_t source) {
  if (!first_ && now - last_ < LIGHT_PERIOD_MS) return;  // ще не час (мінімум 200 мс між вимірюваннями) — виходимо
  last_ = now;                                           // запам'ятовуємо час цього вимірювання

  usingBh_ = (source == LIGHT_BH1750) && bhOk_;          // BH1750 використовуємо, якщо його обрано в меню І він знайдений
  float v;                                               // нове «сире» значення, lx
  if (usingBh_) {                                        // цифровий датчик
    v = bh.readLightLevel();          // прочитати люкси по I2C; від'ємне значення = помилка
    if (v < 0) return;                // помилка — пропускаємо цей раз, старе значення лишається
  } else {                                               // аналоговий датчик
    v = readAnalogLux();                                 // прочитати і перерахувати напругу
  }
  // Експоненційне згладжування (як Mathf.Lerp кожен кадр): рухаємось до нового значення на 30 %.
  lux_ = first_ ? v : lux_ + 0.3f * (v - lux_);         // перший раз просто беремо v
  first_ = false;                                        // далі вже не перший раз
}

// ============================================================================
// MotionSensor
// ============================================================================
void MotionSensor::begin() {
  pinMode(PIN_PIR, INPUT);   // звичайний вхід без підтяжки: HC-SR501 сам видає 0 / 3.3 В
}

void MotionSensor::update(uint32_t now, uint16_t filterMs) {
  const bool r = digitalRead(PIN_PIR) == HIGH;        // прочитати стан виходу PIR
  if (r && !raw_) highSince_ = now;                   // фронт (було LOW, стало HIGH): сигнал щойно з'явився
  raw_ = r;                                           // запам'ятовуємо стан
  motion_ = raw_ && (now - highSince_ >= filterMs);   // рух = HIGH триває вже не менше filterMs мс
}

// ============================================================================
// DistanceSensor
// ============================================================================
const char* DistanceSensor::name() const {
#if SIM_MODE              // у симуляції Wokwi...
  return "HC-SR04";       // ...ультразвуковий датчик
#else                     // на реальній платі...
  return "VL53L0X";       // ...лазерний ToF-датчик
#endif
}

void DistanceSensor::begin() {
#if SIM_MODE                              // --- HC-SR04 ---
  pinMode(PIN_US_TRIG, OUTPUT);           // TRIG — вихід (ми подаємо імпульс)
  digitalWrite(PIN_US_TRIG, LOW);         // у спокої TRIG = 0
  pinMode(PIN_US_ECHO, INPUT);            // ECHO — вхід (датчик відповідає імпульсом)
  ok_ = true;                             // HC-SR04 не вміє «відповідати», вважаємо, що він є
#else                                     // --- VL53L0X ---
  vl53.setBus(&Wire);                     // працювати через нашу шину I2C
  vl53.setTimeout(100);                   // чекати відповіді датчика не довше 100 мс
  ok_ = vl53.init();                      // ініціалізація; false — датчик не відповів
  if (ok_) vl53.startContinuous(50);      // запустити безперервні вимірювання: нове кожні 50 мс
#endif
  Serial.printf("[dist] %s %s\n", name(), ok_ ? "ready" : "NOT found (distance disabled)");  // лог результату
}

void DistanceSensor::update(uint32_t now) {
  if (!ok_ || now - last_ < DIST_PERIOD_MS) return;  // датчика немає або ще не час — виходимо
  last_ = now;                                       // час цього вимірювання
#if SIM_MODE                                         // --- HC-SR04 ---
  // HC-SR04: імпульс 10 мкс на TRIG, вимірюємо тривалість ECHO.
  // pulseIn блокує до 25 мс — для навчального проєкту прийнятно.
  digitalWrite(PIN_US_TRIG, HIGH);                   // почати імпульс запуску
  delayMicroseconds(10);                             // тримати 10 мкс (це дуже коротко, тож тут блокування допустиме)
  digitalWrite(PIN_US_TRIG, LOW);                    // закінчити імпульс
  unsigned long us = pulseIn(PIN_US_ECHO, HIGH, 25000UL);  // виміряти тривалість HIGH на ECHO, мкс; 0 = таймаут 25 мс
  valid_ = us > 0;                                   // 0 — відлуння не прийшло (нічого немає в межах ~4 м)
  cm_ = valid_ ? (uint16_t)(us / 58UL) : 0;          // звук проходить 1 см туди й назад приблизно за 58 мкс
#else                                                // --- VL53L0X ---
  uint16_t mm = vl53.readRangeContinuousMillimeters();  // взяти останнє вимірювання в мм
  valid_ = !vl53.timeoutOccurred() && mm < 2000;     // немає таймауту і менше 2 м; 8190/8191 = «нічого немає»
  cm_ = valid_ ? mm / 10 : 0;                        // мм → см (ціле ділення відкидає дробову частину)
#endif
}
