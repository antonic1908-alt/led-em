// Урок 06 — шина I2C: сканер адрес + датчик освітленості BH1750 (GY-302).
// Схема: VCC -> 3V3, GND -> GND, SDA -> GPIO6, SCL -> GPIO7, ADDR не підключати (адреса 0x23).
// На шину можна вішати багато пристроїв паралельно — кожен має свою адресу.

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++); чому «Arduino», а не C++ — див. довідку в уроці 01
#include <Wire.h>     // бібліотека шини I2C
#include <BH1750.h>   // бібліотека датчика BH1750 (claws/BH1750, підключена в platformio.ini через lib_deps)

constexpr int PIN_SDA = 6;   // лінія даних I2C
constexpr int PIN_SCL = 7;   // тактова лінія I2C

BH1750 bh;                   // об'єкт датчика (адреса за замовчуванням 0x23)
bool bhOk = false;           // чи відповів датчик

// Перебрати всі можливі адреси і надрукувати ті, де хтось відповів.
void scanI2C() {
  Serial.println("[i2c] scanning...");
  int found = 0;                                 // лічильник знайдених пристроїв
  for (uint8_t addr = 1; addr < 127; ++addr) {   // 7-бітні адреси: 1..126
    Wire.beginTransmission(addr);                // почати «розмову» з адресою addr
    if (Wire.endTransmission() == 0) {   // завершити; 0 = пристрій відповів ACK («я тут»)
      Serial.printf("[i2c] found 0x%02X", addr);                          // %02X — шістнадцяткове, 2 цифри
      if (addr == 0x23 || addr == 0x5C) Serial.print("  (BH1750)");       // підказки, що це може бути
      if (addr == 0x3C || addr == 0x3D) Serial.print("  (SSD1306 OLED)");
      if (addr == 0x29) Serial.print("  (VL53L0X)");
      Serial.println();                          // кінець рядка
      found++;                                   // +1 знайдений
    }
  }
  Serial.printf("[i2c] done, %d device(s)\n", found);  // підсумок
}

void setup() {
  Serial.begin(115200);              // лог
  delay(500);                        // у уроці можна: дати USB-порту піднятися, щоб не пропустити результат сканування
  Wire.begin(PIN_SDA, PIN_SCL);      // запустити I2C на наших пінах
  scanI2C();                         // один раз просканувати шину
  bhOk = bh.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);  // запустити BH1750: безперервно, точність 1 lx
  Serial.printf("[light] BH1750 %s\n", bhOk ? "ok" : "NOT found — перевірте дроти");
}

void loop() {
  static uint32_t last = 0;                      // час останнього читання
  if (!bhOk || millis() - last < 500) return;    // датчика немає або ще не минуло 500 мс — виходимо
  last = millis();                               // запам'ятати час
  const float lux = bh.readLightLevel();         // прочитати освітленість у люксах (від'ємне = помилка)
  Serial.printf("[light] lux=%.1f\n", lux);      // надрукувати з 1 знаком після коми
}

// ЗМІНИ САМ: підключіть одночасно BH1750 і OLED — сканер має знайти 0x23 і 0x3C.
// Поміняйте SDA і SCL місцями — що покаже сканер? (Нічого не згорить.)
