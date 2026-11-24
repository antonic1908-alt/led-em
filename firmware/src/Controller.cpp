// Controller.cpp — логіка режимів і скінченного автомата.

#include "Controller.h"  // оголошення класу Controller

// Перетворити стан на текст для логу.
const char* stateName(State st) {
  switch (st) {                        // switch по enum class — як у C#
    case State::Off:  return "OFF";    // вимкнено
    case State::On:   return "ON";     // горить, тригер активний
    case State::Hold: return "HOLD";   // горить, чекаємо закінчення holdSec
  }
  return "?";                          // сюди не дійдемо, але компілятор хоче бачити return
}

void Controller::update(uint32_t now, const Inputs& in) {
  // ---- Тригер «є людина» ----
  near_ = in.distAvailable && in.distValid && in.distCm < s_.distCm;  // «близько»: датчик є, бачить об'єкт і той ближче порогу
  switch (s_.trigger) {                                               // яку логіку обрано в меню
    case TRIG_PIR:          trigger_ = in.motion; break;              // лише рух
    case TRIG_DIST:         trigger_ = near_; break;                  // лише дистанція
    case TRIG_PIR_AND_DIST: trigger_ = in.motion && near_; break;     // рух І близько
    case TRIG_PIR_OR_DIST:                                            // рух АБО близько...
    default:                trigger_ = in.motion || near_; break;     // ...і він же для будь-якого незнайомого значення
  }                                                                   // break обов'язковий: у C++ case «провалюється» в наступний

  // ---- «Темно» з гістерезисом ----
  // Оцінюємо ЛИШЕ коли стрічка вимкнена: інакше її власне світло
  // потрапить на датчик, і вона вимкне сама себе.
  if (state_ == State::Off || !darkInit_) {                           // стрічка вимкнена або це перша оцінка
    if (!darkInit_) {                                                 // перша оцінка після старту
      dark_ = in.lux < s_.luxThreshold;                               // просто порівнюємо з порогом
      darkInit_ = true;                                               // далі — вже з гістерезисом
    } else if (in.lux < s_.luxThreshold) {                            // стало темніше за поріг...
      dark_ = true;                                                   // ...темно
    } else if (in.lux > (float)s_.luxThreshold + s_.luxHyst) {        // стало світліше за поріг + запас...
      dark_ = false;                                                  // ...світло
    }                                                                 // між порогом і порогом+запас — нічого не міняємо (гістерезис)
  }

  if (s_.mode == MODE_MANUAL) {                                       // у ручному режимі датчики ігноруються:
    state_ = manualOn_ ? State::On : State::Off;                      // стан визначає лише кнопка
    return;                                                           // далі автомат не потрібен
  }

  const bool lightOk = (s_.lightMode == LIGHTMODE_ALWAYS) || dark_;   // чи дозволяє освітленість вмикатися

  // ---- Скінченний автомат ----
  switch (state_) {
    case State::Off:                                                  // ВИМКНЕНО:
      if (trigger_ && lightOk) state_ = State::On;                    // є людина і темно → вмикаємо
      break;
    case State::On:                                                   // ГОРИТЬ:
      if (!trigger_) { state_ = State::Hold; holdStart_ = now; }      // людина зникла → починаємо відлік утримання
      break;
    case State::Hold:                                                 // УТРИМАННЯ:
      if (trigger_) state_ = State::On;                               // людина повернулась → знову On
      else if (now - holdStart_ >= (uint32_t)s_.holdSec * 1000UL) state_ = State::Off;  // час вийшов → вимикаємо
      break;                                                          // (holdSec * 1000 — секунди в мілісекунди)
  }
}

float Controller::targetPercent() const {
  return state_ == State::Off ? 0.0f : (float)s_.brightness;  // вимкнено → 0 %, інакше яскравість з налаштувань
}

void Controller::toggleMode(uint32_t now) {
  if (s_.mode == MODE_AUTO) {                     // було АВТО → стає РУЧНИЙ
    s_.mode = MODE_MANUAL;                        // міняємо режим у налаштуваннях
    manualOn_ = (state_ != State::Off);           // зберігаємо поточний стан світла — без різкого стрибка яскравості
  } else {                                        // було РУЧНИЙ → стає АВТО
    s_.mode = MODE_AUTO;
    if (manualOn_) { state_ = State::Hold; holdStart_ = now; }  // світить ще holdSec і гасне (якщо нікого немає)
    else           { state_ = State::Off; }                     // було вимкнено — лишається вимкнено
  }
  Serial.printf("[ctrl] mode -> %s\n", s_.mode == MODE_AUTO ? "AUTO" : "MANUAL");  // лог зміни режиму
}

void Controller::manualToggle(uint32_t now) {
  if (s_.mode == MODE_AUTO) {
    // В АВТО коротке MANUAL: переходимо в РУЧНИЙ і інвертуємо поточний стан
    const bool wasOn = (state_ != State::Off);    // чи горіло до натискання
    toggleMode(now);                              // перейти в ручний режим
    manualOn_ = !wasOn;                           // інвертувати: горіло → вимкнути, не горіло → увімкнути
  } else {                                        // уже в ручному режимі
    manualOn_ = !manualOn_;                       // просто перемкнути увімк/вимк
  }
  state_ = manualOn_ ? State::On : State::Off;    // одразу оновити стан (не чекаючи наступного update)
  Serial.printf("[ctrl] manual %s\n", manualOn_ ? "ON" : "OFF");  // лог
}

uint32_t Controller::holdLeftSec(uint32_t now) const {
  if (state_ != State::Hold) return 0;                         // не в утриманні — лишилось 0
  const uint32_t total = (uint32_t)s_.holdSec * 1000UL;        // повний час утримання, мс
  const uint32_t passed = now - holdStart_;                    // скільки вже минуло, мс
  return passed >= total ? 0 : (total - passed + 999) / 1000;  // залишок у секундах, округлений вгору (+999)
}
