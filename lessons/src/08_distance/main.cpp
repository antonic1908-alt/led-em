// Урок 08 — датчик дистанції.
// Варіант A (рекомендовано, I2C): VL53L0X — VIN -> 3V3, GND -> GND, SDA -> GPIO6, SCL -> GPIO7.
// Варіант B (ультразвук): HC-SR04P / RCWL-1601 (3.3 В) — TRIG -> GPIO2, ECHO -> GPIO20.
//   Звичайний HC-SR04 (5 В) — ECHO лише через дільник 1 кОм / 2 кОм!
// Оберіть варіант нижче.

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01
#include <Wire.h>     // шина I2C
#include <VL53L0X.h>  // бібліотека VL53L0X (pololu/VL53L0X)

#define USE_ULTRASONIC 0     // 0 = VL53L0X, 1 = HC-SR04 (перемикач на етапі компіляції)

constexpr int PIN_SDA = 6, PIN_SCL = 7;     // I2C для VL53L0X
constexpr int PIN_TRIG = 2, PIN_ECHO = 20;  // піни ультразвукового датчика

VL53L0X tof;          // об'єкт лазерного датчика (ToF = time of flight, «час польоту» світла)
bool ok = false;      // датчик готовий

void setup() {
  Serial.begin(115200);              // лог
  delay(500);                        // у уроці можна: дати USB-порту піднятися
#if USE_ULTRASONIC                   // --- HC-SR04 ---
  pinMode(PIN_TRIG, OUTPUT);         // TRIG — вихід (подаємо імпульс)
  pinMode(PIN_ECHO, INPUT);          // ECHO — вхід (датчик відповідає імпульсом)
  ok = true;                         // перевірити наявність HC-SR04 неможливо — вважаємо, що він є
#else                                // --- VL53L0X ---
  Wire.begin(PIN_SDA, PIN_SCL);      // запустити I2C
  tof.setTimeout(200);               // чекати відповіді не довше 200 мс
  ok = tof.init();                   // ініціалізувати; false — не відповів
  if (ok) tof.startContinuous(50);   // безперервні вимірювання кожні 50 мс
#endif
  Serial.printf("[dist] sensor %s\n", ok ? "ready" : "NOT found");
}

void loop() {
  static uint32_t last = 0;                   // час останнього вимірювання
  if (!ok || millis() - last < 150) return;   // датчика немає або ще не минуло 150 мс
  last = millis();
#if USE_ULTRASONIC                            // --- HC-SR04 ---
  digitalWrite(PIN_TRIG, HIGH);               // імпульс запуску...
  delayMicroseconds(10);                      // ...тривалістю 10 мкс...
  digitalWrite(PIN_TRIG, LOW);                // ...закінчити
  const unsigned long us = pulseIn(PIN_ECHO, HIGH, 25000UL);  // тривалість відлуння, мкс; звук: ~58 мкс на 1 см туди-назад
  if (us == 0) Serial.println("[dist] out of range");         // 0 = таймаут 25 мс: нічого немає
  else         Serial.printf("[dist] %lu cm\n", us / 58UL);   // мкс → см
#else                                         // --- VL53L0X ---
  const uint16_t mm = tof.readRangeContinuousMillimeters();   // остання відстань, мм
  if (tof.timeoutOccurred() || mm >= 2000) Serial.println("[dist] out of range");  // таймаут або > 2 м (8190 = «нічого»)
  else                                     Serial.printf("[dist] %u mm\n", mm);    // %u — беззнакове ціле
#endif
}

// ЗМІНИ САМ: увімкніть вбудований LED (GPIO8), коли об'єкт ближче 50 см.
// Подумайте: як зробити, щоб на межі 50 см LED не мерехтів? (Підказка: гістерезис.)
