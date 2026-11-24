// Ui.h — екрани на OLED SSD1306 (128x64, верхні 16 px жовті) і меню налаштувань.
// Аналогія з Unity: це як UI Canvas + скрипт меню, тільки ми самі малюємо кожен піксель
// у буфер пам'яті і потім одним пакетом відправляємо його на дисплей.

#pragma once          // підключати лише один раз
#include <Arduino.h>  // базові типи
#include "Settings.h" // налаштування, які редагує меню
#include "Buttons.h"  // BtnEvent — події кнопок

// Що сталося в меню після обробки натискань (повертає menuInput):
enum class UiAction : uint8_t { None, Changed, Exit };  // нічого / змінено значення / вийшли з меню

struct UiStatus {            // що показати на головному екрані (заповнюється в main.cpp)
  RunMode mode;              // АВТО чи РУЧНИЙ
  const char* stateText;     // «ГОРИТЬ», «УТРИМ 12с» …
  float lux;                 // освітленість, lx
  const char* lightName;     // назва джерела світла ("BH1750" / "ADC")
  bool motion;               // є рух
  bool distAvailable;        // датчик дистанції є
  bool distValid;            // датчик дистанції бачить об'єкт
  uint16_t distCm;           // відстань, см
  bool near;                 // об'єкт ближче за поріг
  float ledLevel;            // поточна яскравість, %
};

class Ui {
 public:
  explicit Ui(Settings& s) : s_(s) {}  // конструктор: запам'ятати посилання на налаштування
  void begin();                        // ініціалізувати дисплей (у setup)

  bool inMenu() const { return inMenu_; }  // зараз відкрите меню?
  void openMenu(uint32_t now);             // відкрити меню з першого пункту
  // Обробити події чотирьох кнопок у меню; повертає, що сталося.
  UiAction menuInput(BtnEvent up, BtnEvent down, BtnEvent ok, BtnEvent man, uint32_t now);
  void showToast(const char* text, uint32_t now);   // коротке спливаюче повідомлення на 1.2 с

  void draw(uint32_t now, const UiStatus& st);      // намалювати поточний екран (головний або меню)

 private:
  void drawMain(const UiStatus& st);                           // головний екран зі станом датчиків
  void drawMenu();                                             // список пунктів меню
  void formatValue(uint8_t idx, char* buf, size_t len) const;  // записати значення пункту idx як текст у buf
  void changeValue(uint8_t idx, int dir);                      // змінити значення пункту idx на крок вгору (+1) / вниз (-1)

  Settings& s_;                  // посилання на спільні налаштування
  bool inMenu_ = false;          // меню відкрите
  bool editing_ = false;         // зараз редагуємо значення (а не ходимо по списку)
  uint8_t sel_ = 0;              // індекс виділеного пункту
  uint8_t top_ = 0;              // індекс першого видимого пункту (для прокрутки)
  uint16_t backup_ = 0;          // значення до редагування (для скасування)
  uint32_t lastInput_ = 0;       // час останнього натискання (для автовиходу)
  const char* toast_ = nullptr;  // текст спливаючого повідомлення; nullptr = немає (як null у C#)
  uint32_t toastUntil_ = 0;      // до якого моменту показувати повідомлення
};
