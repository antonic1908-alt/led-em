// Урок 01 — Blink без delay() + Serial.
// Мета: зрозуміти setup()/loop(), millis(), вивід у Serial Monitor.

// ============================================================================
// ДОВІДКА: «Arduino» чи «C++»?
// ============================================================================
// Це не вибір «або-або». Весь код уроків і проєкту написаний на C++,
// а Arduino — це бібліотека (фреймворк), написана на C++, якою ми користуємось.
//
// Аналогія з Unity:
//   Мова C#                                     ->  мова C++
//   Рушій Unity з API UnityEngine               ->  фреймворк Arduino з API Arduino.h
//   using UnityEngine;                          ->  #include <Arduino.h>
//   Debug.Log / Time.time / Input.GetKey        ->  Serial.println / millis() / digitalRead()
//   Start() / Update() викликає рушій           ->  setup() / loop() викликає ядро Arduino
// Ви ж не кажете «пишу на Unity, а не на C#» — пишете на C#, використовуючи Unity. Тут так само.
//
// Що робить #include <Arduino.h>:
//   буквально вставляє вміст файлу Arduino.h у цей файл перед компіляцією. Після цього компілятор
//   знає pinMode, digitalWrite, digitalRead, analogRead, millis, micros, delay, об'єкт Serial,
//   типи uint8_t/uint32_t і константи HIGH, LOW, INPUT_PULLUP.
//   Без цього рядка pinMode(...) дасть помилку «not declared» — як у C# без потрібного using.
//
// Де ж main()? У звичайній програмі на C++ усе починається з main(). Вона є і тут,
// просто захована всередині фреймворку. Спрощено:
//   int main() {
//     init();              // налаштувати чип: тактування, таймери, USB
//     setup();             // ВАША функція — один раз
//     for (;;) loop();     // ВАША функція — знову і знову, вічно
//   }
// Як у Unity: ігровий цикл належить рушію, а ви пишете лише Start і Update.
// (На ESP32 замість main() є app_main(), який запускає цей цикл окремою задачею FreeRTOS.)
//
// Як збирається прошивка (що таке PlatformIO — див. довідку в lessons/platformio.ini):
// PlatformIO бере ваші .cpp + вихідний код ядра Arduino, і все це
// компілює звичайний компілятор C++ (GCC): для ESP32-C3 — riscv32-esp-elf-g++,
// для STM32 — arm-none-eabi-g++. Тому доступні класи, enum class, constexpr, посилання, шаблони.
//
// Навіщо Arduino, якщо можна писати напряму через бібліотеки виробника чипа?
// Увімкнути пін 10:
//   Arduino (однаково на ESP32 і STM32):  pinMode(10, OUTPUT); digitalWrite(10, HIGH);
//   ESP-IDF (лише ESP32):                 gpio_reset_pin(GPIO_NUM_10);
//                                         gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT);
//                                         gpio_set_level(GPIO_NUM_10, 1);
//   STM32 HAL (лише STM32, пін PA8):      __HAL_RCC_GPIOA_CLK_ENABLE();
//                                         GPIO_InitTypeDef g = {0};
//                                         g.Pin = GPIO_PIN_8; g.Mode = GPIO_MODE_OUTPUT_PP;
//                                         HAL_GPIO_Init(GPIOA, &g);
//                                         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
// Плюси Arduino: один API для різних чипів (наш проєкт збирається і на ESP32-C3, і на STM32F411),
// тисячі готових бібліотек (BH1750, VL53L0X, U8g2), менше коду на старті.
// Мінуси: частина деталей схована — менше контролю, трохи більша прошивка.
// Тому після проєкту в плані навчання (docs/07) є крок: переписати те саме на STM32 HAL / ESP-IDF.
//
// А .ino-файли? В Arduino IDE файли мають розширення .ino, і #include <Arduino.h> там можна
// не писати — IDE додає його сама. Звідси враження, що «Arduino — окрема мова».
// У PlatformIO ми пишемо звичайні .cpp, тому підключаємо Arduino.h явно, як будь-яку бібліотеку.
// ============================================================================

#include <Arduino.h>  // фреймворк Arduino (бібліотека на C++): pinMode, digitalWrite, millis, Serial (як using UnityEngine;)

constexpr int PIN_LED = 8;            // вбудований синій LED Super Mini (active-low: LOW = світить)
constexpr uint32_t INTERVAL_MS = 500; // перемикати LED кожні 500 мс; uint32_t — беззнакове 32-бітне ціле (як uint у C#)

// Глобальні змінні — зберігають значення між викликами loop() (як поля MonoBehaviour).
uint32_t lastToggle = 0;              // коли (millis) востаннє перемикали LED
bool ledOn = false;                   // LED зараз світить?
uint32_t counter = 0;                 // скільки разів перемкнули

void setup() {                        // ≈ Start(): виконується один раз після ввімкнення або RESET
  Serial.begin(115200);               // запустити послідовний порт (USB) на 115200 біт/с — для логів
  pinMode(PIN_LED, OUTPUT);           // пін LED — вихід (ми керуємо напругою на ньому)
}

void loop() {                         // ≈ Update(), але тисячі разів на секунду; викликається знову і знову
  const uint32_t now = millis();      // скільки мс минуло від старту плати
  // Чому "now - lastToggle", а не "now > lastToggle + INTERVAL"?
  // Бо через ~49 днів millis() переповниться, а віднімання беззнакових чисел усе одно дасть правильну різницю.
  if (now - lastToggle >= INTERVAL_MS) {        // минуло 500 мс з останнього перемикання?
    lastToggle = now;                           // запам'ятати момент перемикання
    ledOn = !ledOn;                             // інвертувати стан: було true — стане false і навпаки
    digitalWrite(PIN_LED, ledOn ? LOW : HIGH);  // active-low: щоб світив — подаємо LOW (0 В)
    // printf: %d — ціле, %lu — unsigned long; ++counter спершу збільшує лічильник, потім підставляє.
    Serial.printf("[blink] led=%d count=%lu uptime=%lums\n", ledOn, (unsigned long)++counter, (unsigned long)now);
  }
  // Тут можна робити інші справи — програма не «застигла» в delay().
}

// ЗМІНИ САМ: зробіть «серцебиття» — два короткі спалахи, потім пауза 1 с.
