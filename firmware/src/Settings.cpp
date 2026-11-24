// Settings.cpp — реалізація збереження/завантаження налаштувань.
// ESP32: бібліотека Preferences (NVS — «ключ-значення» у flash, як PlayerPrefs).
// STM32: бібліотека EEPROM (емуляція EEPROM в одній зі сторінок flash).

#include "Settings.h"  // наш власний заголовок (у лапках — шукати поруч із цим файлом)

#if defined(ARDUINO_ARCH_ESP32)     // збірка для ESP32...
#include <Preferences.h>            // ...підключаємо Preferences (NVS)
#elif defined(ARDUINO_ARCH_STM32)   // збірка для STM32...
#include <EEPROM.h>                 // ...підключаємо емуляцію EEPROM
#endif                              // кінець вибору платформи

// Анонімний namespace = «private static» на рівні файлу: ці імена не видно з інших .cpp.
namespace {
  constexpr uint32_t MAGIC   = 0x4C454453;  // «підпис» 'LEDS' у кодах ASCII — ознака, що у flash саме наші дані
  constexpr uint16_t VERSION = 1;           // версія формату; збільшуйте, якщо змінюєте структуру Settings

  // Blob — те, що реально пишемо у flash: підпис + версія + самі налаштування + контрольна сума.
  struct Blob {
    uint32_t magic;     // має дорівнювати MAGIC
    uint16_t version;   // має дорівнювати VERSION
    Settings s;         // корисні дані
    uint16_t checksum;  // контрольна сума поля s — щоб помітити пошкоджені дані
  };

  // Проста контрольна сума: проходимо по всіх байтах структури і «перемішуємо» їх.
  uint16_t checksum(const Settings& s) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&s);  // дивимось на структуру як на масив байтів
                                                              // (&s — адреса структури; reinterpret_cast — «вважай цю адресу іншим типом»)
    uint16_t sum = 0;                                         // накопичувач суми
    for (size_t i = 0; i < sizeof(Settings); ++i)             // sizeof — розмір структури в байтах
      sum = (uint16_t)((sum << 1) | (sum >> 15)) ^ p[i];      // циклічний зсув суми на 1 біт вліво і XOR з черговим байтом
    return sum;                                               // готова сума
  }

  // Обмежити значення v діапазоном [lo, hi] (як Mathf.Clamp у Unity, але для цілих).
  uint16_t clampU16(uint16_t v, uint16_t lo, uint16_t hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // кінець анонімного namespace

namespace settings {  // далі — реалізація функцій, оголошених у Settings.h

// Значення за замовчуванням («заводські налаштування»).
Settings defaults() {
  Settings s{};                       // {} — усі поля обнулені (у C++ локальні змінні інакше містять «сміття»!)
  s.luxThreshold = 30;                // темно, якщо менше 30 lx (сутінки в кімнаті)
  s.luxHyst      = 10;                // гістерезис 10 lx
  s.lightSource  = LIGHT_BH1750;      // основний датчик — цифровий BH1750
  s.lightMode    = LIGHTMODE_DARK_ONLY; // вмикатися лише в темряві
  s.trigger      = TRIG_PIR_OR_DIST;  // рух АБО хтось близько
  s.pirFilterMs  = 0;                 // без додаткового фільтра PIR
  s.holdSec      = 30;                // світити 30 с після останнього руху
  s.distCm       = 80;                // «близько» — ближче за 80 см
  s.brightness   = 80;                // 80 % яскравості
  s.fadeMs       = 800;               // плавне вмикання за 0.8 с
  s.mode         = MODE_AUTO;         // стартуємо в автоматичному режимі
  return s;                           // повертаємо КОПІЮ структури (для маленьких struct це нормально)
}

// Захист від неправильних значень (наприклад, якщо у flash були дані старішої версії).
void clamp(Settings& s) {
  s.luxThreshold = clampU16(s.luxThreshold, 1, 2000);  // поріг світла: 1..2000 lx
  s.luxHyst      = clampU16(s.luxHyst, 0, 200);        // гістерезис: 0..200 lx
  s.lightSource  = clampU16(s.lightSource, 0, 1);      // лише два джерела світла
  s.lightMode    = clampU16(s.lightMode, 0, 1);        // лише два режими світла
  s.trigger      = clampU16(s.trigger, 0, 3);          // чотири варіанти логіки датчиків
  s.pirFilterMs  = clampU16(s.pirFilterMs, 0, 2000);   // фільтр PIR: 0..2 с
  s.holdSec      = clampU16(s.holdSec, 5, 600);        // утримання: 5 с .. 10 хв
  s.distCm       = clampU16(s.distCm, 5, 400);         // дистанція: 5..400 см
  s.brightness   = clampU16(s.brightness, 5, 100);     // яскравість: 5..100 %
  s.fadeMs       = clampU16(s.fadeMs, 0, 5000);        // fade: 0..5 с
  s.mode         = clampU16(s.mode, 0, 1);             // AUTO або MANUAL
}

// Прочитати налаштування з flash. Повертає true, якщо дані знайдено і вони цілі.
bool load(Settings& s) {
  Blob b{};                                                   // буфер для прочитаних даних, обнулений
  bool ok = false;                                            // чи вдалося щось прочитати
#if defined(ARDUINO_ARCH_ESP32)                               // --- ESP32 ---
  Preferences prefs;                                          // об'єкт доступу до NVS
  prefs.begin("ledcfg", true);                                // відкрити «простір імен» ledcfg; true = лише читання
  ok = prefs.getBytes("cfg", &b, sizeof(b)) == sizeof(b);     // прочитати байти за ключем "cfg" у b; успіх, якщо прочитано рівно sizeof(b)
  prefs.end();                                                // закрити NVS (як Dispose у C#)
#elif defined(ARDUINO_ARCH_STM32)                             // --- STM32 ---
  EEPROM.get(0, b);                                           // прочитати b з адреси 0 емульованої EEPROM
  ok = true;                                                  // EEPROM завжди щось повертає; перевірку зробить MAGIC нижче
#endif                                                        // кінець вибору платформи
  // Дані приймаємо, лише якщо збігся підпис, версія і контрольна сума.
  if (ok && b.magic == MAGIC && b.version == VERSION && b.checksum == checksum(b.s)) {
    s = b.s;                                                  // копіюємо налаштування у змінну викликача
    clamp(s);                                                 // і про всяк випадок обмежуємо діапазони
    return true;                                              // успіх
  }
  s = defaults();                                             // інакше — заводські значення
  return false;                                               // повідомляємо, що збережених даних не було
}

// Записати налаштування у flash.
void save(const Settings& s) {
  Blob b{};                                   // готуємо блок для запису
  b.magic = MAGIC;                            // підпис
  b.version = VERSION;                        // версія формату
  b.s = s;                                    // самі налаштування
  b.checksum = checksum(s);                   // контрольна сума
#if defined(ARDUINO_ARCH_ESP32)               // --- ESP32 ---
  Preferences prefs;                          // об'єкт доступу до NVS
  prefs.begin("ledcfg", false);               // відкрити ledcfg; false = читання і запис
  prefs.putBytes("cfg", &b, sizeof(b));       // записати весь блок за ключем "cfg"
  prefs.end();                                // закрити (дані фіксуються у flash)
#elif defined(ARDUINO_ARCH_STM32)             // --- STM32 ---
  EEPROM.put(0, b);                           // записати b з адреси 0; STM32duino сам запише у flash лише змінені байти
#endif                                        // кінець вибору платформи
  Serial.println(F("[cfg] saved"));           // лог; F("...") тримає рядок у flash, а не в RAM (економія пам'яті)
}

}  // namespace settings
