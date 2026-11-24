// Settings.h — налаштування, що зберігаються у flash (переживають вимкнення живлення).
// Аналогія з Unity: ScriptableObject з полями + PlayerPrefs для збереження.
// .h-файл — це «оголошення» (що існує), а .cpp — «реалізація» (як працює).
// У C# це було б в одному файлі; у C++ розділяють, щоб інші файли бачили лише інтерфейс.

#pragma once          // підключати цей файл лише один раз
#include <Arduino.h>  // типи uint16_t тощо

// enum з явним базовим типом uint16_t (як enum LightSource : ushort у C#).
// Значення пишемо явно (= 0, = 1), бо вони зберігаються у flash і не повинні «з'їхати».
enum LightSource : uint16_t { LIGHT_BH1750 = 0, LIGHT_ANALOG = 1 };                  // звідки брати освітленість
enum LightMode   : uint16_t { LIGHTMODE_DARK_ONLY = 0, LIGHTMODE_ALWAYS = 1 };       // вмикати лише в темряві чи завжди
enum TriggerMode : uint16_t { TRIG_PIR = 0, TRIG_DIST = 1, TRIG_PIR_OR_DIST = 2, TRIG_PIR_AND_DIST = 3 };  // що вважати «є людина»
enum RunMode     : uint16_t { MODE_AUTO = 0, MODE_MANUAL = 1 };                      // автоматичний або ручний режим

// Усі поля uint16_t (беззнакове 16-бітне ціле, як ushort у C#) — так меню може працювати
// з ними однаково (через вказівник uint16_t*).
// struct у C++ — це майже те саме, що class, але поля за замовчуванням public.
struct Settings {
  uint16_t luxThreshold;  // lx: «темно», якщо освітленість нижче цього порогу
  uint16_t luxHyst;       // lx: гістерезис — запас, щоб стрічка не «блимала» біля порогу
  uint16_t lightSource;   // значення з enum LightSource
  uint16_t lightMode;     // значення з enum LightMode
  uint16_t trigger;       // значення з enum TriggerMode
  uint16_t pirFilterMs;   // мс: скільки PIR має триматися HIGH, щоб зарахувати рух (фільтр хибних спрацювань)
  uint16_t holdSec;       // с: скільки світити після останнього тригера
  uint16_t distCm;        // см: поріг «близько» для датчика дистанції
  uint16_t brightness;    // %: цільова яскравість стрічки
  uint16_t fadeMs;        // мс: час плавного переходу 0→100 %
  uint16_t mode;          // значення з enum RunMode (запам'ятовуємо останній режим)
};                        // у C++ після оголошення struct/class обов'язкова крапка з комою

// namespace — як namespace у C#: групує функції, щоб імена не конфліктували.
// Тут лише оголошення функцій (прототипи); тіла — у Settings.cpp.
namespace settings {
  Settings defaults();               // повернути налаштування «з коробки»
  void clamp(Settings& s);           // привести значення до допустимих меж; & = передача за посиланням (як ref у C#)
  bool load(Settings& s);            // прочитати з flash; false — якщо збережених даних немає (тоді defaults)
  void save(const Settings& s);      // записати у flash; const & = «за посиланням, але без права змінювати» (як in у C#)
}                                    // кінець namespace settings
