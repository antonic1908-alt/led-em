// Buttons.h — кнопка з антибрязкотом і подіями Short / Long / Repeat.
// Аналогія з Unity: Input.GetKeyDown + «утримання», але ми самі опитуємо пін у loop().
// «Брязкіт» (bounce): механічний контакт при натисканні кілька мілісекунд замикається-розмикається,
// і без фільтра одне натискання виглядало б як 5–10 натискань.

#pragma once          // підключати лише один раз
#include <Arduino.h>  // pinMode, digitalRead, millis, типи

// enum class — «строгий» enum (як звичайний enum у C#: треба писати BtnEvent::Short).
// : uint8_t — займає 1 байт.
enum class BtnEvent : uint8_t {
  None,    // нічого не сталося
  Short,   // коротке натискання (подія при відпусканні)
  Long,    // утримання >= LONG_MS (одноразово)
  Repeat   // після Long — повтор кожні REPEAT_MS, поки тримаємо (для швидкої зміни чисел)
};

class Button {
 public:  // публічна частина класу (у C++ пишеться секцією, а не перед кожним членом)
  static constexpr uint32_t DEBOUNCE_MS = 30;   // скільки мс сигнал має бути стабільним, щоб йому повірити
  static constexpr uint32_t LONG_MS     = 700;  // з якого часу утримання це «довге натискання»
  static constexpr uint32_t REPEAT_MS   = 150;  // період автоповтору при утриманні

  void begin(uint32_t pin);        // налаштувати пін (викликати один раз у setup(), як Awake/Start)
  BtnEvent update(uint32_t now);   // викликати кожен цикл loop() (як Update); повертає подію або None
  bool isDown() const { return stable_; }  // чи натиснута зараз; const = метод не змінює об'єкт

 private:  // приватні поля; підкреслення в кінці імені — домовленість «це поле класу»
  uint32_t pin_ = 0;          // номер піна
  bool raw_ = false;          // останнє «сире» значення (може брязкати)
  bool stable_ = false;       // значення після антибрязкоту (true = натиснута)
  bool longSent_ = false;     // чи вже відправили подію Long для поточного натискання
  uint32_t changedAt_ = 0;    // коли (millis) востаннє змінилось сире значення
  uint32_t pressedAt_ = 0;    // коли кнопку стабільно натиснули
  uint32_t lastRepeat_ = 0;   // коли востаннє видали Long/Repeat
};
