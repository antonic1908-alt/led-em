// Урок 04 — АЦП і фоторезистор (дільник напруги).
// Рекомендована схема для ESP32-C3:  3V3 ─[LDR GL5516]─┬─[10 кОм]─ GND
//                                                      └─> GPIO0
// Більше світла -> менший опір LDR -> вища напруга на GPIO0.
// (АЦП ESP32-C3 точний приблизно до 2.5 В, тому в темряві напруга має бути МАЛОЮ — так і є.)
// Для TEMT6000: VCC->3V3, GND->GND, SIG->GPIO0.

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01
#include <math.h>     // powf

constexpr int PIN_ADC = 0;          // аналоговий вхід (на ESP32-C3 АЦП є лише на GPIO0–4)
constexpr float VCC = 3.3f;         // напруга живлення дільника, В
constexpr float R_FIXED = 10000.0f; // постійний резистор дільника, Ом
constexpr float RL10 = 8000.0f;    // опір LDR при 10 lux (з datasheet, приблизно)
constexpr float GAMMA = 0.5f;      // «нахил» характеристики LDR (з datasheet, приблизно)

float filtered = -1;               // згладжене значення lux; -1 = «ще немає жодного вимірювання»

void setup() {
  Serial.begin(115200);            // лог
  pinMode(PIN_ADC, INPUT);         // пін — вхід
}

void loop() {
  static uint32_t last = 0;         // час останнього вимірювання (static — зберігається між викликами)
  const uint32_t now = millis();    // поточний час
  if (now - last < 200) return;     // 5 разів на секунду: якщо ще не час — виходимо з loop()
  last = now;                       // запам'ятати час вимірювання

  const int raw = analogRead(PIN_ADC);                 // «сирий» код АЦП 0..4095 (12 біт)
  const uint32_t mv = analogReadMilliVolts(PIN_ADC);   // калібровані мВ (ESP32 враховує похибку свого АЦП)
  float v = mv / 1000.0f;                              // мВ → В
  v = constrain(v, 0.005f, VCC - 0.005f);              // не даємо v = 0 або v = VCC, щоб не ділити на нуль

  const float rLdr = R_FIXED * (VCC - v) / v;          // закон Ома для дільника: опір LDR зі знайденої напруги
  const float lux = 10.0f * powf(RL10 / rLdr, 1.0f / GAMMA);  // модель LDR: опір → люкси

  // Експоненційний фільтр: прибирає «шум» АЦП (кожен раз проходимо 20 % шляху до нового значення)
  filtered = (filtered < 0) ? lux : filtered + 0.2f * (lux - filtered);

  // Виводимо всі проміжні величини: так видно, на якому кроці щось не так.
  Serial.printf("[adc] raw=%d mV=%lu R_ldr=%.0f lux~%.1f filtered=%.1f\n",
                raw, (unsigned long)mv, rLdr, lux, filtered);
}

// ЗМІНИ САМ: закрийте датчик пальцем, посвітіть ліхтариком телефону — запишіть значення.
// Порівняйте з BH1750 з уроку 06 і підберіть RL10/GAMMA так, щоб показники зблизилися (калібрування).
