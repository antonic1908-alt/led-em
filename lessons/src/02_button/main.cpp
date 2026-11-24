// Урок 02 — Кнопки: INPUT_PULLUP, брязкіт контактів, коротке/довге натискання.
// Схема: кнопка між GPIO4 і GND. Зовнішній резистор не потрібен — є внутрішній pull-up (~45 кОм).

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01

constexpr int PIN_BTN = 4;            // UP у фінальному проєкті
constexpr int PIN_LED = 8;            // вбудований LED (active-low)
constexpr uint32_t DEBOUNCE_MS = 30;  // сигнал має бути стабільним 30 мс, щоб вважати його справжнім
constexpr uint32_t LONG_MS = 700;     // утримання від 700 мс — «довге натискання»

bool rawPrev = false, stable = false, longSent = false;  // попереднє сире значення / стабільне значення / чи вже був LONG
uint32_t changedAt = 0, pressedAt = 0;                   // коли змінилось сире значення / коли стабільно натиснули
uint32_t rawChanges = 0;   // скільки разів «сирий» сигнал змінився — побачите брязкіт!

void setup() {
  Serial.begin(115200);             // лог у Serial Monitor
  pinMode(PIN_BTN, INPUT_PULLUP);   // вхід з внутрішньою підтяжкою до 3.3 В: відпущена = HIGH, натиснута = LOW
  pinMode(PIN_LED, OUTPUT);         // LED — вихід
  digitalWrite(PIN_LED, HIGH);      // вимкнути LED (active-low: HIGH = не світить)
}

void loop() {
  const uint32_t now = millis();                 // поточний час
  const bool raw = digitalRead(PIN_BTN) == LOW;  // прочитати кнопку: true = натиснута

  // Сире значення змінилось: запам'ятати його, час зміни і порахувати зміну.
  if (raw != rawPrev) { rawPrev = raw; changedAt = now; rawChanges++; }

  // Сире значення відрізняється від стабільного і не змінювалось уже DEBOUNCE_MS — приймаємо.
  if (stable != raw && now - changedAt >= DEBOUNCE_MS) {
    stable = raw;                                // новий стабільний стан
    if (stable) {                                // щойно натиснули:
      pressedAt = now; longSent = false;         // запам'ятати час, LONG для цього натискання ще не було
      Serial.printf("[btn] down (raw changes so far: %lu)\n", (unsigned long)rawChanges);
    } else if (!longSent) {                      // відпустили раніше, ніж спрацював LONG:
      Serial.println("[btn] SHORT");             // це коротке натискання
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));   // перемкнути LED (прочитати поточний стан виходу й інвертувати)
    }
  }
  // Тримаємо довше LONG_MS і LONG ще не відправляли — відправляємо один раз.
  if (stable && !longSent && now - pressedAt >= LONG_MS) {
    longSent = true;                             // більше не повторювати для цього натискання
    Serial.println("[btn] LONG");                // довге натискання
  }
}

// ЗМІНИ САМ: поставте DEBOUNCE_MS = 0 і поклацайте — скільки «SHORT» на одне натискання?
// Потім оформіть логіку в клас Button (порівняйте з ../../firmware/src/Buttons.cpp).
