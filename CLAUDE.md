# CLAUDE.md — контекст для ШІ-агента

Це навчальний проєкт: «розумна» одноколірна LED-стрічка 5V на ESP32-C3 Super Mini (основна плата) і STM32F411CEU6 Black Pill (друга плата).
Користувач — початківець в embedded (досвід C#/Unity). **Пояснюй кожну зміну простими словами і порівнюй з C#/Unity, де можливо.**

## Залізо (не вигадуй інших компонентів і пінів)
| Вузол | Компонент | Інтерфейс |
|---|---|---|
| Світло (основний) | GY-302 BH1750 | I2C 0x23 |
| Світло (аналоговий) | GL5516 / GM5528 у дільнику з 10k до 3V3, або TEMT6000 | ADC |
| Рух | HC-SR501 (живлення 5V, вихід 3.3V) | цифровий вхід |
| Дистанція | VL53L0X (I2C 0x29) — треба докупити; у симуляції HC-SR04 | I2C / TRIG+ECHO |
| Дисплей | SSD1306 0.96" 128x64, жовто-синій (верхні 16 px жовті) | I2C 0x3C |
| Кнопки | 4 шт.: UP, DOWN, OK, MANUAL → GND, INPUT_PULLUP | цифрові входи |
| Ключ стрічки | AO3400 N-MOSFET, low-side, 100 Ω у затворі, 100k затвор→GND | ШІМ |

## Піни (джерело правди: `firmware/src/config.h`)
| Сигнал | ESP32-C3 Super Mini | STM32F411 Black Pill |
|---|---|---|
| I2C SDA / SCL | GPIO6 / GPIO7 | PB7 / PB6 |
| ШІМ стрічки | GPIO10 | PA8 (TIM1_CH1) |
| PIR | GPIO3 | PB0 |
| Аналоговий датчик світла | GPIO0 (ADC1_CH0) | PA1 |
| Кнопки UP / DOWN / OK / MANUAL | GPIO4 / GPIO5 / GPIO1 / GPIO8 | PB12 / PB13 / PB14 / PB15 |
| HC-SR04 TRIG / ECHO | GPIO2 / GPIO20 | PB8 / PB9 |

Обмеження:
- **Логіка 3.3V.** Не подавай 5V на GPIO ESP32-C3.
- **Strapping-піни ESP32-C3:** GPIO2, GPIO8, GPIO9. GPIO9 — кнопка BOOT. GPIO8 — вбудований LED (active-low).
- PA0 на Black Pill зайнятий кнопкою KEY, PC13 — вбудований LED.

## Структура
- `docs/` — Markdown-посібник (коротка версія).
- `site/` — HTML-посібник: відкривати `site/index.html`.
- `lessons/` — PlatformIO-проєкт з уроками; кожен урок — окремий env.
- `firmware/` — PlatformIO-проєкт основної прошивки. Env-и: `esp32c3`, `esp32c3_sim`, `blackpill`.

## Команди
```
cd firmware
pio run -e esp32c3                 # зібрати для реальної ESP32-C3
pio run -e esp32c3 -t upload       # прошити
pio device monitor -b 115200       # лог
pio run -e esp32c3_sim             # збірка для Wokwi (HC-SR04 замість VL53L0X)
pio run -e blackpill               # STM32F411
cd ../lessons && pio run -e l01_blink
```

## Правила для агента
1. Один маленький крок за раз. Після кожного кроку — `pio run`, і тільки потім наступний.
2. Не використовувати `delay()` в основній прошивці, лише неблокуючий код на `millis()`.
3. Не змінювати піни без оновлення `config.h`, `CLAUDE.md`, `docs/05_hardware_bom_wiring.md` і `site/05-hardware.html`.
4. Перед роботою з новою мікросхемою дивитися datasheet у `docs/datasheets/`, якщо він є.
5. Логи друкувати у форматі `[модуль] повідомлення`, щоб користувач міг скопіювати їх агенту.
6. **Git: ніколи не комітити і не пушити напряму в `main`.** Кожна зміна йде окремою гілкою з Pull Request. Зливає PR лише користувач, після перегляду різниці і зеленого статусу перевірок (`.github/workflows/checks.yml`). Після злиття сайт сам публікується на GitHub Pages (`.github/workflows/pages.yml`).
