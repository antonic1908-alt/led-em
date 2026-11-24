// Buttons.cpp — реалізація кнопки з антибрязкотом.

#include "Buttons.h"  // оголошення класу Button

// Button:: — «цей метод належить класу Button» (у C# метод пишеться всередині класу, тут — окремо).
void Button::begin(uint32_t pin) {
  pin_ = pin;                                  // запам'ятовуємо номер піна
  pinMode(pin_, INPUT_PULLUP);                 // вхід з внутрішнім резистором до 3.3 В:
                                               // відпущена = HIGH, натиснута = LOW (замикає на GND)
  raw_ = stable_ = (digitalRead(pin_) == LOW); // початковий стан: LOW означає «натиснута»
  changedAt_ = millis();                       // millis() — скільки мс минуло від старту плати (як Time.time * 1000)
}

BtnEvent Button::update(uint32_t now) {
  const bool raw = (digitalRead(pin_) == LOW); // прочитати пін зараз: true = контакт замкнений
  if (raw != raw_) {          // значення змінилось — контакт «брязкає» або справді натиснули...
    raw_ = raw;               // ...запам'ятовуємо нове сире значення
    changedAt_ = now;         // ...і момент зміни
  }

  // Значення стабільне довше за DEBOUNCE_MS і відрізняється від прийнятого — приймаємо
  if (stable_ != raw_ && now - changedAt_ >= DEBOUNCE_MS) {
    // Примітка: now - changedAt_ правильно працює навіть при переповненні millis() (~49 днів),
    // бо віднімання беззнакових чисел «загортається».
    stable_ = raw_;           // приймаємо новий стабільний стан
    if (stable_) {            // щойно натиснули
      pressedAt_ = now;       // запам'ятовуємо момент натискання
      longSent_ = false;      // для нового натискання Long ще не відправляли
    } else if (!longSent_) {  // відпустили до LONG_MS — це коротке натискання
      return BtnEvent::Short; // повертаємо подію (вихід з функції)
    }
  }

  if (stable_) {                                         // поки кнопку тримають...
    if (!longSent_ && now - pressedAt_ >= LONG_MS) {     // ...тримають довше LONG_MS і Long ще не було
      longSent_ = true;                                  // позначаємо, що Long відправлено
      lastRepeat_ = now;                                 // відлік автоповтору починаємо звідси
      return BtnEvent::Long;                             // подія «довге натискання»
    }
    if (longSent_ && now - lastRepeat_ >= REPEAT_MS) {   // ...після Long минув ще один період повтору
      lastRepeat_ = now;                                 // оновлюємо момент повтору
      return BtnEvent::Repeat;                           // подія «повтор»
    }
  }
  return BtnEvent::None;                                 // нічого цікавого не сталося
}
