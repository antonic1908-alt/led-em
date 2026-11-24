# 07. Перенесення на STM32F411 (Black Pill)

## Навіщо STM32 після ESP32
ESP32 — зручний і з Wi-Fi. STM32 — індустріальний стандарт: його використовують у побутовій техніці, автомобілях, промисловості. Перенесення того самого проєкту показує, що саме залежить від заліза, а що ні.

## Крок 1. Той самий код, інша плата (PlatformIO + STM32duino)
У `firmware/platformio.ini` вже є env `blackpill`:
```
pio run -e blackpill               # зібрати
pio run -e blackpill -t upload     # прошити (за замовчуванням через DFU)
```

**Що змінилося в коді** (див. `#if defined(ARDUINO_ARCH_STM32)`):
| Що | ESP32-C3 | STM32F411 |
|---|---|---|
| Піни | GPIO-номери | PB7, PA8… (`config.h`) |
| I2C | `Wire.begin(SDA, SCL)` | `Wire.setSDA(PB7); Wire.setSCL(PB6); Wire.begin();` |
| ШІМ | LEDC: `ledcAttach` / `ledcWrite` | `analogWriteFrequency()` + `analogWrite()` (таймер TIM1) |
| Збереження | `Preferences` (NVS) | `EEPROM` (емуляція у flash-секторі) |
| АЦП | `analogReadMilliVolts()` (калібрований) | `analogRead()` 12 біт, 3.3 В = 4095 |

**Підключення:**
- див. таблицю в [05](05_hardware_bom_wiring.md);
- живлення модулів — від піна **3V3** Black Pill;
- HC-SR501 і стрічка — від 5V.

**Способи прошивки:**
1. **USB DFU:** затиснути BOOT0 → натиснути NRST → відпустити BOOT0 → `pio run -e blackpill -t upload`. На Windows може знадобитися драйвер WinUSB через Zadig або встановлений STM32CubeProgrammer.
2. **ST-Link V2:** у `platformio.ini` поставити `upload_protocol = stlink`. З'єднання: SWDIO↔DIO, SWCLK↔CLK, GND↔GND, 3.3V↔3V3.
3. **Serial-лог:** через USB CDC (у нас увімкнено прапорами `-DUSBCON -DPIO_FRAMEWORK_ARDUINO_ENABLE_CDC`).

## Крок 2. «По-справжньому»: STM32CubeIDE + HAL
Мета — зрозуміти, що роблять Arduino-функції «під капотом».
1. **Новий проєкт:** File → New → STM32 Project → обрати **STM32F411CEUx**.
2. **CubeMX (файл .ioc):**
   - RCC: HSE = Crystal (25 МГц на Black Pill); Clock Configuration → HCLK 100 МГц;
   - SYS: Debug = Serial Wire (інакше ST-Link «загубить» плату);
   - **TIM1 CH1 = PWM Generation** на PA8. Prescaler = 0, Counter Period (ARR) = 4999 → 100 МГц / 5000 = **20 кГц**;
   - **I2C1** на PB6/PB7, Fast Mode 400 кГц;
   - **ADC1 IN1** на PA1;
   - GPIO Input з Pull-up: PB12–PB15 (кнопки); GPIO Input: PB0 (PIR);
   - USB_OTG_FS Device + Middleware USB_DEVICE CDC — для логу (або USART1 на PA9/PA10 + USB-UART адаптер).
3. **Згенерувати код.** Свій код пишіть **лише між коментарями** `/* USER CODE BEGIN */ … /* USER CODE END */`, інакше CubeMX його зітре. Скажіть про це ШІ-агенту явно.
4. **Відповідники:**
   - `analogWrite(PA8, d)` → `__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, d)` після `HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1)`;
   - `digitalRead(PB12)` → `HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12)`;
   - `millis()` → `HAL_GetTick()`;
   - `Wire` → `HAL_I2C_Master_Transmit / Receive`. BH1750 — це всього 2 команди: `0x01` (power on) і `0x10` (continuous H-res), а потім читання 2 байтів, lux = raw / 1.2.
5. **Налагодження:** кнопка Debug → breakpoints, покрокове виконання, вікно Live Expressions. Саме тут ST-Link окупається.

## Кроки 3+ (за бажанням)
- Переписати `Buttons` на переривання EXTI.
- Перевести BH1750 на I2C через DMA.
- Спробувати FreeRTOS (CubeMX → Middleware → FREERTOS): окремі задачі для датчиків і UI.
- Порівняти споживання струму ESP32-C3 і STM32 у сні.
