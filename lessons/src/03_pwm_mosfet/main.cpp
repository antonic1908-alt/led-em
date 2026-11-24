// Урок 03 — ШІМ + MOSFET AO3400: плавне «дихання» LED-стрічки.
// Схема (low-side ключ):
//   +5V БЖ -> стрічка(+); стрічка(−) -> Drain AO3400; Source -> GND;
//   GPIO10 -> 100 Ом -> Gate; Gate -> 100 кОм -> GND. ЗЕМЛІ БЖ І ПЛАТИ З'ЄДНАТИ!
// Без стрічки можна перевірити на звичайному світлодіоді з резистором 220 Ом на GPIO10.

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01
#include <math.h>     // cosf, powf

constexpr int PIN_PWM = 10;                      // пін ШІМ → затвор MOSFET
constexpr uint32_t FREQ = 20000;   // 20 кГц: нечутно і без мерехтіння
constexpr uint8_t BITS = 10;       // роздільність 10 біт: заповнення 0..1023
constexpr uint32_t MAX_DUTY = (1u << BITS) - 1;  // 1 << 10 = 1024, мінус 1 = 1023 — «ввімкнено весь час»

// Налаштувати апаратний ШІМ (модуль LEDC в ESP32). У різних версіях Arduino-ESP32 API різне.
void pwmBegin() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3     // Arduino-ESP32 3.x (наша версія):
  ledcAttach(PIN_PWM, FREQ, BITS);     // прив'язати ШІМ прямо до піна
#else                                  // старий 2.x:
  ledcSetup(0, FREQ, BITS);            // налаштувати канал 0
  ledcAttachPin(PIN_PWM, 0);           // підключити пін до каналу 0
#endif
}

// Записати заповнення ШІМ (0 = вимкнено, MAX_DUTY = повністю ввімкнено).
void pwmWrite(uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_PWM, duty);            // 3.x: за номером піна
#else
  ledcWrite(0, duty);                  // 2.x: за номером каналу
#endif
}

void setup() {
  Serial.begin(115200);   // лог
  pwmBegin();             // увімкнути ШІМ на піні
}

void loop() {
  const uint32_t now = millis();                              // поточний час, мс
  // «Дихання»: синусоїда з періодом 4 с, від 0 до 1
  const float phase = (now % 4000) / 4000.0f;                 // фаза 0..1 (% — остача від ділення: повтор кожні 4 с)
  const float level = 0.5f - 0.5f * cosf(phase * 2 * PI);     // плавно 0 → 1 → 0 за період (cosf — косинус для float)
  // Гамма-корекція: без неї стрічка «довго яскрава і різко гасне»
  const bool useGamma = true;                                 // перемикач для експерименту
  const float out = useGamma ? powf(level, 2.2f) : level;     // level у степені 2.2 — під сприйняття ока
  pwmWrite((uint32_t)(out * MAX_DUTY));                       // 0..1 → 0..1023 і в ШІМ

  static uint32_t lastLog = 0;      // static усередині функції: змінна зберігає значення між викликами
  if (now - lastLog >= 250) {       // лог 4 рази на секунду
    lastLog = now;
    Serial.printf("[pwm] level=%.2f duty=%lu\n", level, (unsigned long)(out * MAX_DUTY));  // %.2f — 2 знаки після коми
  }
}

// ЗМІНИ САМ: поставте useGamma = false і порівняйте на око.
// Спробуйте FREQ = 200 і подивіться на стрічку через камеру телефону (смуги!).
// Через 5 хв на 100 % торкніться AO3400 — він має бути ледь теплим.
