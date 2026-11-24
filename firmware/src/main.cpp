// main.cpp — точка входу. setup() ≈ Start(), loop() ≈ Update() в Unity.
// Тут немає логіки — лише «склеювання» модулів.
// Схема одного проходу loop(): прочитати входи → обробити кнопки → логіка → вихід (стрічка, дисплей, лог).

#include <Arduino.h>     // ядро Arduino: setup/loop, millis, Serial
#include <Wire.h>        // шина I2C
#include "config.h"      // піни і константи
#include "Settings.h"    // налаштування та їх збереження
#include "Buttons.h"     // кнопки
#include "Sensors.h"     // датчики світла, руху, дистанції
#include "LedDriver.h"   // ШІМ стрічки
#include "Controller.h"  // логіка режимів
#include "Ui.h"          // дисплей і меню

// Глобальні об'єкти — живуть увесь час роботи плати (як компоненти на одному GameObject).
// У C++ вони створюються до виклику setup(), без new.
Settings cfg;                               // поточні налаштування (одна копія на всю програму)
Button btnUp, btnDown, btnOk, btnManual;    // чотири кнопки
LightSensor light;                          // датчик світла
MotionSensor pir;                           // датчик руху
DistanceSensor dist;                        // датчик дистанції
LedDriver led;                              // драйвер стрічки
Controller ctrl(cfg);                       // контролер отримує посилання на cfg
Ui ui(cfg);                                 // UI теж працює з тим самим cfg

uint32_t lastUi = 0, lastLog = 0;  // коли востаннє малювали дисплей і друкували лог
bool dirty = false;          // налаштування змінено, але ще не збережено

// static у функції на рівні файлу = «видно лише в цьому .cpp» (як private).
static void initI2C() {
#if defined(ARDUINO_ARCH_ESP32)          // ESP32: піни I2C передаються прямо в begin
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
#else                                    // STM32: спершу обираємо піни, потім begin
  Wire.setSDA(PIN_I2C_SDA);
  Wire.setSCL(PIN_I2C_SCL);
  Wire.begin();
#endif
  Wire.setClock(400000);                 // швидкість шини 400 кГц (усі наші пристрої її підтримують)
}

// Скласти текст стану для нижнього рядка дисплея. buf — куди писати, len — розмір буфера.
static void makeStateText(char* buf, size_t len, uint32_t now) {
  if (ctrl.mode() == MODE_MANUAL) {                     // ручний режим: лише увімк/вимк
    snprintf(buf, len, "%s", ctrl.state() == State::Off ? "Ручний: вимк" : "Ручний: увімк");
    return;
  }
  switch (ctrl.state()) {                               // автоматичний режим — залежно від стану
    case State::On:   snprintf(buf, len, "ГОРИТЬ"); break;                                                // горить
    case State::Hold: snprintf(buf, len, "Утримання %lus", (unsigned long)ctrl.holdLeftSec(now)); break;  // відлік; %lu — unsigned long
    case State::Off:                                                                                     // вимкнено — пояснюємо чому
      if (cfg.lightMode == LIGHTMODE_DARK_ONLY && !ctrl.dark()) snprintf(buf, len, "Чекаю: світло");   // занадто світло
      else snprintf(buf, len, "Чекаю на рух");                                                          // темно, але нікого немає
      break;
  }
}

void setup() {
  Serial.begin(115200);       // запустити послідовний порт (USB) на швидкості 115200 біт/с
  delay(300);   // дати USB-CDC піднятися (єдиний delay — і лише при старті)
  Serial.println(F("\n[main] boot"));   // перший рядок логу

  const bool loaded = settings::load(cfg);                                 // прочитати налаштування з flash у cfg
  Serial.printf("[cfg] %s\n", loaded ? "loaded from flash" : "defaults");  // звідки взялися налаштування

  led.begin(PIN_LED_PWM);   // першим: тримаємо стрічку вимкненою
  initI2C();                // шина I2C потрібна дисплею і датчикам — запускаємо до них

  btnUp.begin(PIN_BTN_UP);          // кнопки: INPUT_PULLUP
  btnDown.begin(PIN_BTN_DOWN);
  btnOk.begin(PIN_BTN_OK);
  btnManual.begin(PIN_BTN_MANUAL);

  ui.begin();       // дисплей
  light.begin();    // датчик світла (шукає BH1750)
  pir.begin();      // датчик руху
  dist.begin();     // датчик дистанції

  // Підсумок старту: які датчики знайдено.
  Serial.printf("[main] ready, light=%s dist=%s\n",
                light.bhFound() ? "BH1750" : "ADC", dist.found() ? dist.name() : "none");
}

void loop() {
  const uint32_t now = millis();   // один «час кадру» для всіх модулів (як Time.time на початку Update)

  // 1. Вхідні дані
  const BtnEvent eUp = btnUp.update(now);        // подія кнопки UP (або None)
  const BtnEvent eDown = btnDown.update(now);    // подія кнопки DOWN
  const BtnEvent eOk = btnOk.update(now);        // подія кнопки OK
  const BtnEvent eMan = btnManual.update(now);   // подія кнопки MANUAL

  light.update(now, cfg.lightSource);   // датчик світла (сам вирішує, чи настав час читати)
  pir.update(now, cfg.pirFilterMs);     // датчик руху з фільтром з налаштувань
  dist.update(now);                     // датчик дистанції

  // 2. Кнопки: меню або головний екран
  if (ui.inMenu()) {                                         // --- відкрите меню ---
    const UiAction a = ui.menuInput(eUp, eDown, eOk, eMan, now);  // усі кнопки обробляє меню
    if (a == UiAction::Changed) dirty = true;                // щось змінили — треба буде зберегти
    if (a == UiAction::Exit && dirty) {                      // вийшли з меню і є незбережені зміни
      settings::save(cfg);                                   // записати у flash
      dirty = false;                                         // тепер усе збережено
      ui.showToast("Збережено", now);                        // показати повідомлення
    }
  } else {                                                   // --- головний екран ---
    if (eOk == BtnEvent::Short) ui.openMenu(now);            // OK — відкрити меню
    if (eMan == BtnEvent::Short) { ctrl.manualToggle(now); dirty = true; }  // коротке MANUAL — увімк/вимк вручну
    if (eMan == BtnEvent::Long)  {                           // довге MANUAL — змінити режим
      ctrl.toggleMode(now);                                  // АВТО <-> РУЧНИЙ
      dirty = true;                                          // режим теж зберігаємо
      ui.showToast(ctrl.mode() == MODE_AUTO ? "Режим АВТО" : "Режим РУЧНИЙ", now);  // повідомлення
    }
    // UP/DOWN на головному екрані — яскравість ±5 %
    if (eUp != BtnEvent::None && cfg.brightness < 100) { cfg.brightness += 5; dirty = true; }  // яскравіше (до 100 %)
    if (eDown != BtnEvent::None && cfg.brightness > 5) { cfg.brightness -= 5; dirty = true; }  // тьмяніше (до 5 %)
    // Зберігаємо зміни з головного екрана, коли кнопки відпущені
    // (щоб не писати у flash на кожен крок автоповтору — flash має обмежену кількість циклів запису)
    if (dirty && !btnUp.isDown() && !btnDown.isDown() && !btnManual.isDown()) {
      settings::save(cfg);
      dirty = false;
    }
  }

  // 3. Логіка
  // Збираємо знімок датчиків (агрегатна ініціалізація: поля по порядку оголошення в struct Inputs).
  Inputs in{light.lux(), pir.motion(), dist.valid(), dist.cm(), dist.found()};
  ctrl.update(now, in);   // контролер вирішує, чи світити

  // 4. Вихід
  led.setFadeMs(cfg.fadeMs);              // час плавного переходу з налаштувань
  led.setTarget(ctrl.targetPercent());    // цільова яскравість від контролера
  led.update(now);                        // крок fade і запис у ШІМ

  if (now - lastUi >= UI_PERIOD_MS) {     // дисплей оновлюємо лише 10 разів на секунду (I2C не миттєвий)
    lastUi = now;                         // запам'ятати час
    char stateText[32];                   // буфер для тексту стану
    makeStateText(stateText, sizeof(stateText), now);  // заповнити його
    // Заповнюємо UiStatus по порядку полів: режим, текст, lux, джерело, рух, дистанція…, яскравість.
    UiStatus st{ctrl.mode(), stateText, light.lux(), light.usingBh() ? "BH" : "ADC",
                pir.motion(), dist.found(), dist.valid(), dist.cm(), ctrl.isNear(), led.level()};
    ui.draw(now, st);                     // намалювати і відправити на OLED
  }

  if (now - lastLog >= LOG_PERIOD_MS) {   // раз на секунду — рядок логу в Serial Monitor
    lastLog = now;
    // Один рядок з усім станом: його зручно скопіювати ШІ-агенту для діагностики.
    Serial.printf("[ctrl] mode=%s state=%s(%lus) lux=%.1f dark=%d motion=%d dist=%ucm near=%d out=%d%%\n",
                  ctrl.mode() == MODE_AUTO ? "AUTO" : "MANUAL", stateName(ctrl.state()),
                  (unsigned long)ctrl.holdLeftSec(now), light.lux(), ctrl.dark(), pir.motion(),
                  (unsigned)dist.cm(), ctrl.isNear(), (int)(led.level() + 0.5f));
  }
}  // loop() завершився — ядро Arduino одразу викличе його знову
