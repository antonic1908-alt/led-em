// Урок 05 — PIR-датчик руху HC-SR501.
// Схема: VCC -> 5V (!), GND -> GND, OUT -> GPIO3. Вихід модуля 3.3 В — безпечно для ESP32-C3.
// Налаштування на модулі: Tx (час) — на мінімум (проти годинникової), перемичка — H (повторний тригер),
// Sx (дальність) — за смаком. Після ввімкнення модулю потрібно ~30–60 с, щоб «прогрітися».

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01

constexpr int PIN_PIR = 3;   // вихід OUT датчика руху
constexpr int PIN_LED = 8;   // вбудований LED (active-low)

bool prev = false;           // стан датчика в попередньому loop() — щоб помітити момент зміни
uint32_t riseAt = 0;         // коли сигнал став HIGH (початок руху)

void setup() {
  Serial.begin(115200);                        // лог
  pinMode(PIN_PIR, INPUT);                     // звичайний вхід: датчик сам видає 0 або 3.3 В
  pinMode(PIN_LED, OUTPUT);                    // LED — вихід
  Serial.println("[pir] warming up ~30 s..."); // нагадування: перші 30–60 с датчик може «хибити»
}

void loop() {
  const uint32_t now = millis();                // поточний час
  const bool m = digitalRead(PIN_PIR) == HIGH;  // HIGH = датчик бачить рух
  digitalWrite(PIN_LED, m ? LOW : HIGH);   // вбудований LED дублює датчик (LOW = світить)

  if (m && !prev) {                        // фронт: було LOW, стало HIGH — рух почався
    riseAt = now;                          // запам'ятати момент
    Serial.printf("[pir] motion=1 at %lus\n", (unsigned long)(now / 1000));   // мс / 1000 = секунди
  }
  if (!m && prev) {                        // спад: було HIGH, стало LOW — сигнал закінчився
    Serial.printf("[pir] motion=0, HIGH lasted %lu ms\n", (unsigned long)(now - riseAt));  // скільки тривав
  }
  prev = m;                                // запам'ятати стан для наступного loop()
}

// ЗМІНИ САМ: виміряйте мінімальну тривалість HIGH при Tx на мінімумі.
// Додайте «програмну чутливість»: зараховувати рух, лише якщо HIGH тримається > N мс.
