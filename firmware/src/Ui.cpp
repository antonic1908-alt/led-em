// Ui.cpp — малювання екранів і логіка меню.
// Система координат OLED: (0,0) — лівий верхній кут, x вправо до 127, y вниз до 63.
// Для тексту y — це БАЗОВА лінія (низ літер), а не верх.

#include "Ui.h"        // оголошення класу Ui
#include "config.h"    // піни I2C, адреса OLED, MENU_TIMEOUT_MS
#include <Wire.h>      // шина I2C
#include <U8g2lib.h>   // бібліотека графіки для монохромних дисплеїв (olikraus/U8g2)

namespace {  // усе нижче видно лише в цьому файлі
  // Повний буфер кадру (1 КБ RAM): малюємо все в пам'яті, потім sendBuffer().
  // Піни передаємо, щоб на ESP32 U8g2 не перевизначив I2C на піни за замовчуванням.
  // Назва класу розшифровується так: SSD1306 — контролер, 128X64 — розмір, NONAME — типовий модуль,
  // F — full buffer (повний буфер), HW_I2C — апаратний I2C.
  // Аргументи: U8G2_R0 — без повороту, U8X8_PIN_NONE — пін reset не підключений, далі SCL і SDA.
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA);

  // Шрифт з кирилицею (є і, ї, є; немає ґ). 6x12 — символ 6 px завширшки, рядок 12 px.
  const uint8_t* const FONT = u8g2_font_6x12_t_cyrillic;  // const після * — сам вказівник теж незмінний

  // Тип пункту меню: число, вибір зі списку, «скинути все», «вихід».
  enum Kind : uint8_t { K_NUM, K_ENUM, K_RESET, K_EXIT };

  // Тексти для пунктів-переліків. Індекс у масиві = значення в Settings.
  const char* const OPT_LIGHT[]   = {"BH1750", "Аналог"};                            // LightSource
  const char* const OPT_LMODE[]   = {"Лише темно", "Завжди"};                         // LightMode
  const char* const OPT_TRIGGER[] = {"Рух", "Дистанція", "Рух/Дист", "Рух+Дист"};     // TriggerMode

  // Опис одного пункту меню (таблиця даних замість купи if-ів — як масив ScriptableObject-ів).
  struct MenuItem {
    const char* label;           // назва пункту на екрані
    uint16_t Settings::*field;   // вказівник на поле структури (як reflection «на мінімалках»):
                                 // s_.*field дає доступ до потрібного поля конкретного об'єкта
    uint16_t min, max;           // допустимі межі значення
    uint16_t step;               // крок зміни; 0 = адаптивний крок
    Kind kind;                   // тип пункту
    const char* const* opts;     // масив текстів для K_ENUM (інакше nullptr)
    const char* unit;            // одиниця виміру після числа
  };

  // Таблиця пунктів меню. Порядок рядків = порядок на екрані.
  const MenuItem ITEMS[] = {
    // назва          поле                       min  max   крок тип     варіанти     одиниця
    {"Поріг світла", &Settings::luxThreshold, 1, 2000, 0,   K_NUM,  nullptr,     "lx"},
    {"Гістерезис",   &Settings::luxHyst,      0, 200,  0,   K_NUM,  nullptr,     "lx"},
    {"Датчик світла",&Settings::lightSource,  0, 1,    1,   K_ENUM, OPT_LIGHT,   ""},
    {"Умова",        &Settings::lightMode,    0, 1,    1,   K_ENUM, OPT_LMODE,   ""},
    {"Тригер",       &Settings::trigger,      0, 3,    1,   K_ENUM, OPT_TRIGGER, ""},
    {"Фільтр руху",  &Settings::pirFilterMs,  0, 2000, 100, K_NUM,  nullptr,     "мс"},
    {"Утримання",    &Settings::holdSec,      5, 600,  0,   K_NUM,  nullptr,     "с"},
    {"Поріг дист.",  &Settings::distCm,       5, 400,  5,   K_NUM,  nullptr,     "см"},
    {"Яскравість",   &Settings::brightness,   5, 100,  5,   K_NUM,  nullptr,     "%"},
    {"Плавність",    &Settings::fadeMs,       0, 5000, 100, K_NUM,  nullptr,     "мс"},
    {"Скинути все",  nullptr,                 0, 0,    0,   K_RESET,nullptr,     ""},
    {"Вихід",        nullptr,                 0, 0,    0,   K_EXIT, nullptr,     ""},
  };
  constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);  // кількість пунктів (розмір масиву / розмір елемента)
  constexpr uint8_t VISIBLE = 4;           // рядків меню під жовтою смугою
  constexpr uint8_t HEADER_H = 16;         // висота жовтої зони, px
  constexpr uint8_t LINE_H = 12;           // висота рядка тексту, px

  // Крок для числових пунктів з step = 0: що більше значення, то більший крок.
  uint16_t adaptiveStep(uint16_t v, bool up) {
    // Біля нуля — дрібний крок, далі — більший (як логарифмічний слайдер)
    uint16_t x = up ? v : (v > 0 ? v - 1 : 0);  // при русі вниз дивимось на v-1, щоб 20 → 19 ішло кроком 1
    if (x < 20)  return 1;                      // 0..19 — крок 1
    if (x < 100) return 5;                      // 20..99 — крок 5
    if (x < 500) return 25;                     // 100..499 — крок 25
    return 100;                                 // далі — крок 100
  }
}  // кінець анонімного namespace

void Ui::begin() {
  oled.setI2CAddress(I2C_ADDR_OLED << 1);  // U8g2 очікує 8-бітну адресу: 0x3C << 1 = 0x78
  oled.begin();                            // ініціалізувати контролер дисплея
  oled.setBusClock(400000);                // I2C на 400 кГц (fast mode) — кадр передається швидше
  oled.enableUTF8Print();                  // дозволити друк UTF-8 (кирилиця) через print()
  oled.setFont(FONT);                      // обрати шрифт
}

void Ui::openMenu(uint32_t now) {
  inMenu_ = true;      // меню відкрите
  editing_ = false;    // починаємо з навігації, не з редагування
  sel_ = 0;            // виділено перший пункт
  top_ = 0;            // список прокручено на початок
  lastInput_ = now;    // скидаємо таймер автовиходу
}

void Ui::showToast(const char* text, uint32_t now) {
  toast_ = text;              // запам'ятати текст (рядок має жити довго — передаємо лише літерали)
  toastUntil_ = now + 1200;   // показувати 1.2 с
}

void Ui::changeValue(uint8_t idx, int dir) {
  const MenuItem& it = ITEMS[idx];     // опис пункту (посилання, без копіювання)
  uint16_t& v = s_.*(it.field);        // посилання на відповідне поле налаштувань — змінюючи v, змінюємо s_
  if (it.kind == K_ENUM) {             // перелік — по колу
    if (dir > 0) v = (v >= it.max) ? it.min : v + 1;   // вгору: після останнього — перший
    else         v = (v <= it.min) ? it.max : v - 1;   // вниз: перед першим — останній
    return;                                            // для переліку все
  }
  const uint16_t step = it.step ? it.step : adaptiveStep(v, dir > 0);  // фіксований або адаптивний крок
  int32_t nv = (int32_t)v + dir * (int32_t)step;   // нове значення в знаковому типі (може стати < 0)
  if (it.step) nv = (nv / step) * step;   // вирівнюємо на крок (напр. 103 → 100 при кроці 5)
  if (nv < it.min) nv = it.min;           // не нижче мінімуму
  if (nv > it.max) nv = it.max;           // не вище максимуму
  v = (uint16_t)nv;                       // записуємо назад у налаштування
}

UiAction Ui::menuInput(BtnEvent up, BtnEvent down, BtnEvent ok, BtnEvent man, uint32_t now) {
  const bool anyInput = up != BtnEvent::None || down != BtnEvent::None ||   // чи була хоч одна подія
                        ok != BtnEvent::None || man != BtnEvent::None;
  if (anyInput) lastInput_ = now;                // було натискання — скидаємо таймер автовиходу

  if (now - lastInput_ > MENU_TIMEOUT_MS) {      // автовихід: 30 с ніхто нічого не натискав
    inMenu_ = editing_ = false;                  // закрити меню і режим редагування
    return UiAction::Exit;                       // main.cpp збереже налаштування
  }

  // Для UP/DOWN усі три події (Short, Long, Repeat) означають «крок»
  const int dir = (up != BtnEvent::None) ? +1 : (down != BtnEvent::None) ? -1 : 0;  // +1 вгору, -1 вниз, 0 — ні
  const MenuItem& it = ITEMS[sel_];              // поточний пункт

  if (editing_) {                                // --- режим редагування значення ---
    if (dir) { changeValue(sel_, dir); return UiAction::Changed; }    // UP/DOWN — змінити значення
    if (ok == BtnEvent::Short) { editing_ = false; return UiAction::Changed; }  // OK — підтвердити
    if (man == BtnEvent::Short) {             // MANUAL — скасувати
      s_.*(it.field) = backup_;               // повернути значення, яке було до редагування
      editing_ = false;                       // вийти з редагування
      return UiAction::Changed;
    }
    return UiAction::None;                    // інших дій у редагуванні немає
  }

  if (dir) {                                  // --- навігація по списку ---
    // У меню UP = вгору по списку
    if (dir > 0) sel_ = (sel_ == 0) ? ITEM_COUNT - 1 : sel_ - 1;  // вгору; з першого — на останній
    else         sel_ = (sel_ + 1) % ITEM_COUNT;                   // вниз; % — по колу на перший
    if (sel_ < top_) top_ = sel_;                                  // виділення вище видимої частини — прокрутити вгору
    if (sel_ >= top_ + VISIBLE) top_ = sel_ - VISIBLE + 1;         // нижче — прокрутити вниз
    return UiAction::None;
  }

  if (ok == BtnEvent::Short) {                // OK на пункті
    switch (it.kind) {
      case K_NUM:                             // число...
      case K_ENUM:                            // ...або перелік (спільна гілка: case без break «провалюється»)
        backup_ = s_.*(it.field);             // запам'ятати старе значення для скасування
        editing_ = true;                      // почати редагування
        return UiAction::None;
      case K_RESET: {                         // «Скинути все» ({} — блок, бо всередині оголошуємо змінну)
        const uint16_t mode = s_.mode;        // режим АВТО/РУЧНИЙ не скидаємо
        s_ = settings::defaults();            // заводські налаштування
        s_.mode = mode;                       // повертаємо режим
        showToast("Скинуто", now);            // повідомлення на екрані
        return UiAction::Changed;
      }
      case K_EXIT:                            // «Вихід»
        inMenu_ = false;                      // закрити меню
        return UiAction::Exit;
    }
  }

  if (man == BtnEvent::Short) {               // MANUAL у навігації — теж вихід з меню
    inMenu_ = false;
    return UiAction::Exit;
  }
  return UiAction::None;                      // нічого не змінилось
}

void Ui::formatValue(uint8_t idx, char* buf, size_t len) const {
  const MenuItem& it = ITEMS[idx];   // опис пункту
  buf[0] = '\0';                     // порожній рядок за замовчуванням ('\0' — кінець рядка в C)
  // snprintf — як string.Format, але пише в готовий буфер і не виходить за len байтів.
  if (it.kind == K_NUM)       snprintf(buf, len, "%u%s", (unsigned)(s_.*(it.field)), it.unit);  // число + одиниця
  else if (it.kind == K_ENUM) snprintf(buf, len, "%s", it.opts[s_.*(it.field)]);                // текст варіанта
}                                    // для K_RESET / K_EXIT значення немає — рядок лишається порожнім

void Ui::draw(uint32_t now, const UiStatus& st) {
  oled.clearBuffer();          // очистити буфер кадру в RAM (дисплей ще не змінюється)
  if (inMenu_) drawMenu();     // меню...
  else         drawMain(st);   // ...або головний екран

  if (toast_ && (int32_t)(toastUntil_ - now) > 0) {   // спливаюче повідомлення поверх екрана, поки не вийшов час
    const int w = oled.getUTF8Width(toast_) + 12;     // ширина рамки = ширина тексту + відступи
    const int x = (128 - w) / 2;                      // по центру екрана
    oled.setDrawColor(0); oled.drawBox(x, 26, w, 20); // колір 0 (чорний): стерти прямокутник під повідомленням
    oled.setDrawColor(1); oled.drawFrame(x, 26, w, 20); // колір 1 (світиться): рамка
    oled.setCursor(x + 6, 40);                        // позиція тексту (базова лінія y = 40)
    oled.print(toast_);                               // текст повідомлення
  } else {
    toast_ = nullptr;                                 // час вийшов — забуваємо повідомлення
  }
  oled.sendBuffer();           // відправити весь буфер на дисплей по I2C (тепер картинка змінилась)
}

void Ui::drawMain(const UiStatus& st) {
  char buf[32];                                       // тимчасовий буфер для рядків (32 байти на стеку)
  // --- жовта смуга: режим і яскравість ---
  oled.setCursor(0, 12);                              // лівий край, базова лінія 12
  oled.print(st.mode == MODE_AUTO ? "АВТО" : "РУЧНИЙ");  // режим
  if (st.ledLevel > 0.5f) snprintf(buf, sizeof(buf), "%d%%", (int)(st.ledLevel + 0.5f));  // яскравість з округленням; %% — символ %
  else                    snprintf(buf, sizeof(buf), "ВИМК");                             // стрічка вимкнена
  oled.setCursor(128 - oled.getUTF8Width(buf), 12);   // вирівняти по правому краю
  oled.print(buf);
  oled.drawHLine(0, HEADER_H - 1, 128);               // горизонтальна лінія під жовтою зоною

  // --- синя зона ---
  snprintf(buf, sizeof(buf), "Світло %.1f lx", st.lux);   // %.1f — дробове число з 1 знаком після коми
  oled.setCursor(0, HEADER_H + LINE_H);                   // перший рядок синьої зони
  oled.print(buf);
  oled.setCursor(128 - oled.getUTF8Width(st.lightName), HEADER_H + LINE_H);  // джерело світла праворуч
  oled.print(st.lightName);

  oled.setCursor(0, HEADER_H + 2 * LINE_H);               // другий рядок
  oled.print(st.motion ? "Рух: так" : "Рух: ні");         // стан PIR
  if (!st.distAvailable)  snprintf(buf, sizeof(buf), "Дист --");   // датчика немає
  else if (!st.distValid) snprintf(buf, sizeof(buf), "Дист >");    // нічого не видно в межах вимірювання
  else                    snprintf(buf, sizeof(buf), "%sДист %u", st.near ? "*" : "", (unsigned)st.distCm);  // «*» = близько
  oled.setCursor(128 - oled.getUTF8Width(buf), HEADER_H + 2 * LINE_H);  // праворуч
  oled.print(buf);

  oled.setCursor(0, HEADER_H + 3 * LINE_H + 2);           // третій рядок (трохи нижче)
  oled.print(st.stateText);                               // стан: ГОРИТЬ / УТРИМ / ВИМК…
}

void Ui::drawMenu() {
  char buf[24];                                       // буфер для тексту значення
  // --- жовта смуга ---
  oled.setCursor(0, 12);
  oled.print(editing_ ? "Зміна значення" : "Меню");   // заголовок залежить від режиму
  snprintf(buf, sizeof(buf), "%u/%u", (unsigned)(sel_ + 1), (unsigned)ITEM_COUNT);  // «3/12» — номер пункту
  oled.setCursor(128 - oled.getUTF8Width(buf), 12);   // праворуч
  oled.print(buf);
  oled.drawHLine(0, HEADER_H - 1, 128);               // лінія під заголовком

  // --- список ---
  for (uint8_t row = 0; row < VISIBLE; ++row) {       // для кожного з 4 видимих рядків
    const uint8_t idx = top_ + row;                   // який пункт меню показати в цьому рядку
    if (idx >= ITEM_COUNT) break;                     // пункти закінчились
    const int yTop = HEADER_H + row * LINE_H;         // верх рядка, px
    const int base = yTop + LINE_H - 2;               // базова лінія тексту
    formatValue(idx, buf, sizeof(buf));               // текст значення
    const int vw = oled.getUTF8Width(buf);            // ширина значення, px
    const bool selected = (idx == sel_);              // цей рядок виділений?

    if (selected && !editing_) {          // виділений рядок — інверсія
      oled.setDrawColor(1); oled.drawBox(0, yTop, 128, LINE_H);  // залити рядок
      oled.setDrawColor(0);                                      // текст малювати «чорним» по заливці
    }
    oled.setCursor(2, base);              // назва пункту ліворуч
    oled.print(ITEMS[idx].label);
    if (selected && editing_) {           // редаговане значення — у рамці-інверсії
      oled.setDrawColor(1); oled.drawBox(124 - vw - 3, yTop, vw + 5, LINE_H);  // залити лише зону значення
      oled.setDrawColor(0);                                                    // значення — «чорним»
    }
    oled.setCursor(126 - vw, base);       // значення праворуч
    oled.print(buf);
    oled.setDrawColor(1);                 // повернути звичайний колір для наступного рядка
  }
}
