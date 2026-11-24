// Урок 07 — OLED SSD1306 128x64 (жовто-синій) через бібліотеку U8g2.
// Схема: VCC -> 3V3, GND -> GND, SDA -> GPIO6, SCL -> GPIO7.
// Жовта смуга — верхні 16 рядків пікселів (колір фізичний, змінити не можна) — використовуємо під заголовок.
// Координати: (0,0) — лівий верхній кут; x до 127 вправо, y до 63 вниз.

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01
#include <Wire.h>     // шина I2C
#include <U8g2lib.h>  // графічна бібліотека U8g2 (olikraus/U8g2)

constexpr int PIN_SDA = 6;   // I2C SDA
constexpr int PIN_SCL = 7;   // I2C SCL

// F = повний буфер кадру в RAM (1 КБ). Малюємо все, потім sendBuffer() — як рендер кадру в Unity.
// Аргументи: без повороту (R0), пін reset не підключений, піни SCL і SDA.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);

void setup() {
  Serial.begin(115200);                       // лог (тут не використовується, але звично мати)
  oled.begin();                               // ініціалізувати дисплей
  oled.enableUTF8Print();                     // для кирилиці
  oled.setFont(u8g2_font_6x12_t_cyrillic);    // шрифт 6x12 px з кирилицею
}

void loop() {
  const uint32_t now = millis();             // поточний час
  static uint32_t last = 0;                  // час останнього кадру
  if (now - last < 50) return;               // ~20 кадрів на секунду
  last = now;

  oled.clearBuffer();                        // очистити буфер у RAM
  // Жовта зона
  oled.setCursor(0, 12);                     // y = 12 — БАЗОВА лінія тексту (низ літер)
  oled.print("Привіт, OLED!");               // текст
  oled.drawHLine(0, 15, 128);                // горизонтальна лінія на межі жовтої зони
  // Синя зона
  oled.setCursor(0, 30);
  oled.printf("Час: %lu с", (unsigned long)(now / 1000));   // секунди від старту
  // Анімований «прогрес-бар»
  const int w = (now / 20) % 128;            // ширина росте на 1 px кожні 20 мс і повертається до 0
  oled.drawFrame(0, 40, 128, 10);            // рамка: x, y, ширина, висота
  oled.drawBox(0, 40, w, 10);                // заповнена частина
  // Кружечок, що бігає
  oled.drawDisc(w, 58, 4);                   // зафарбоване коло: центр (w, 58), радіус 4
  oled.sendBuffer();                         // відправити кадр на дисплей
}

// ЗМІНИ САМ: намалюйте іконку лампочки з drawCircle/drawLine і виведіть «ВИМК/УВІМК».
