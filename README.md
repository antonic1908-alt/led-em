# Мікроконтролери з нуля: ESP32-C3 і STM32 → розумна LED-стрічка

**🌐 Посібник онлайн: https://antonic1908-alt.github.io/led-em/**

Навчальний комплект для людини, яка вміє програмувати на C# (Unity) на рівні junior і має базові знання електроніки, але ще не працювала з мікроконтролерами.

**Кінцева мета:** LED-стрічка 5V вмикається за рухом, освітленістю та дистанцією до датчика. Є ручний режим, а налаштування змінюються на OLED-дисплеї кнопками.

## З чого почати
1. Відкрийте посібник **онлайн** (посилання вище) або локально — **`site/index.html`** подвійним кліком. Це основна, ілюстрована версія посібника.
2. Ідіть розділами по черзі: план → ШІ-інструменти → ПЗ → симулятори → залізо → проєкт → STM32.
3. Уроки виконуйте в `lessons/`, фінальний проєкт — у `firmware/`.

## Карта файлів
| Що | Markdown | HTML (онлайн) |
|---|---|---|
| План навчання | [docs/01_learning_plan.md](docs/01_learning_plan.md) | [01-learning-plan](https://antonic1908-alt.github.io/led-em/01-learning-plan.html) |
| ШІ-інструменти і вайбкодинг | [docs/02_ai_tools_vibecoding.md](docs/02_ai_tools_vibecoding.md) | [02-ai-tools](https://antonic1908-alt.github.io/led-em/02-ai-tools.html) |
| Програмне забезпечення | [docs/03_software_setup.md](docs/03_software_setup.md) | [03-software](https://antonic1908-alt.github.io/led-em/03-software.html) |
| Симулятори та онлайн-інструменти | [docs/04_simulators_online.md](docs/04_simulators_online.md) | [04-simulators](https://antonic1908-alt.github.io/led-em/04-simulators.html) |
| Компоненти, закупівля, схема | [docs/05_hardware_bom_wiring.md](docs/05_hardware_bom_wiring.md) | [05-hardware](https://antonic1908-alt.github.io/led-em/05-hardware.html) |
| ТЗ і архітектура проєкту | [docs/06_project_spec.md](docs/06_project_spec.md) | [06-project](https://antonic1908-alt.github.io/led-em/06-project.html) |
| Перенесення на STM32 | [docs/07_stm32_port.md](docs/07_stm32_port.md) | [07-stm32](https://antonic1908-alt.github.io/led-em/07-stm32.html) |
| Уроки 01–08 | [lessons/](lessons/) | [lessons](https://antonic1908-alt.github.io/led-em/lessons.html) |
| Словник термінів | — | [glossary](https://antonic1908-alt.github.io/led-em/glossary.html) |
| Контекст для ШІ-агента | [CLAUDE.md](CLAUDE.md) | — |

## Швидкий старт (коли ПЗ вже встановлено)
```
cd firmware
pio run -e esp32c3 -t upload
pio device monitor -b 115200
```

## Як вносяться зміни
1. Кожна зміна робиться в окремій гілці й пропонується через Pull Request.
2. GitHub Actions автоматично збирає прошивку і всі уроки та перевіряє посилання на сайті.
3. Після перегляду і злиття PR сайт сам оновлюється на GitHub Pages за 1–2 хвилини.

> Посібник складено в листопаді 2026 року. Ціни, версії та рейтинги ШІ-інструментів швидко змінюються, тому перевіряйте їх на сайтах виробників.
