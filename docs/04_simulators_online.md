# 04. Симулятори та онлайн-інструменти

## Wokwi — головний симулятор
- **Що це:** браузерний симулятор мікроконтролерів (https://wokwi.com). Є розширення для VS Code.
- **Що вміє для нас:**
  - плати: **ESP32-C3** (DevKitM-1), STM32F103 Blue Pill, Nucleo C031/L031;
  - компоненти: SSD1306 OLED, PIR-датчик руху, фоторезисторний модуль, HC-SR04, кнопки, світлодіоди, резистори.
- **Чого немає:** BH1750, VL53L0X, STM32F411 Black Pill, MOSFET-и. Тому:
  - світло в симуляції — **фоторезисторний модуль** (аналоговий вхід);
  - дистанція в симуляції — **HC-SR04** (env `esp32c3_sim`);
  - стрічка в симуляції — звичайний світлодіод на піні ШІМ, яскравість якого видно;
  - STM32-логіку можна перевірити на Blue Pill (F103), але основну симуляцію робимо на ESP32-C3.
- **Ціна:** онлайн безкоштовний для особистого використання. Wokwi for VS Code: 30-денна пробна ліцензія, безкоштовно для open-source, для решти — платні плани (Hobby+). Актуальні ціни: https://wokwi.com/pricing

### Як запустити симуляцію проєкту
**Варіант A — VS Code**
1. `cd firmware && pio run -e esp32c3_sim`
2. Відкрити `firmware/diagram.json` → F1 → «Wokwi: Start Simulator».

**Варіант B — браузер**
1. Новий проєкт ESP32-C3 на wokwi.com.
2. Вставити вміст `firmware/diagram.json` у вкладку diagram.json.
3. Скопіювати файли з `firmware/src/` (кілька вкладок).
4. Додати бібліотеки U8g2, BH1750, VL53L0X через Library Manager.
5. На початку `config.h` додати `#define SIM_MODE 1`.

**Під час симуляції:**
- клацніть на PIR — імітація руху;
- клацніть на фоторезисторі — повзунок lux;
- клацніть на HC-SR04 — повзунок відстані;
- кнопки натискаються мишею.

## Інші симулятори
| Інструмент | Для чого | Посилання |
|---|---|---|
| **Falstad CircuitJS** | Аналогові схеми: побачити струм через MOSFET, напругу на дільнику з фоторезистором, ефект pull-up. Чудово для розуміння електроніки | https://www.falstad.com/circuit/ |
| **SimulIDE** | Офлайн-симулятор (AVR, частково інші) з осцилографом | https://simulide.com |
| **Tinkercad Circuits** | Найпростіший старт, але лише Arduino Uno. Підходить для перших вправ з кнопками і світлодіодами | https://www.tinkercad.com/circuits |
| **Renode** | Емулятор для STM32 (просунутий рівень, без UI-компонентів) | https://renode.io |

## Інструменти для схем і документації
| Інструмент | Для чого |
|---|---|
| **EasyEDA** (онлайн) | Схеми та друковані плати, велика бібліотека компонентів, замовлення плат на JLCPCB |
| **KiCad** (офлайн, безкоштовний) | Професійні схеми і PCB |
| **Fritzing** | «Макетні» малюнки схем |
| **Cirkit Designer** | Онлайн-схеми з бібліотекою модулів, зокрема ESP32-C3 Super Mini |

## Довідники та калькулятори
- Розпіновка ESP32-C3 Super Mini: https://lastminuteengineers.com/esp32-c3-super-mini-pinout-reference/ та https://mischianti.org/esp32-c3-super-mini-high-resolution-pinout-datasheet-and-specs/
- Розпіновка Black Pill F411: https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0
- Документація Wokwi щодо деталей: https://docs.wokwi.com/getting-started/supported-hardware
- Калькулятор дільника напруги / закону Ома: будь-який онлайн «voltage divider calculator»
- Datasheet-и: AO3400 (Alpha & Omega), BH1750FVI (Rohm), SSD1306 (Solomon Systech), VL53L0X (ST), HC-SR501 (опис модуля)
