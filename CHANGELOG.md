# Changelog

Усі помітні зміни проєкту. Формат — [Keep a Changelog](https://keepachangelog.com/uk/1.1.0/),
версії за [SemVer](https://semver.org/lang/uk/). English summary below.

## [Unreleased]

## [0.2.0] — 2026-11-11

### Додано
- **Вхід по паролю** (HTTP Basic Auth) для вебінтерфейсу та WebSocket у режимі роботи; у режимі точки доступу (первинне налаштування) пароль не вимагається. Логін/пароль за замовчуванням `admin`/`admin`, пароль зберігається в NVS і змінюється з дашборду (`/api/password`, мін. 6 символів).
- **Оновлення прошивки**: через браузер (`/update`, завантаження `firmware.bin`) і мережеве ArduinoOTA (`pio run -e esp32dev_ota -t upload`).
- **Збереження MQTT без перезавантаження** — окрема кнопка в секції «Налаштування» (`/api/mqtt`, застосовується одразу, не вимагає Wi-Fi).
- **Налаштування Wi-Fi прямо в дашборді** (раніше — лише окрема сторінка): скан, збереження, забути мережу всередині згортуваної секції «Налаштування».
- **Дефолтні значення модулів**: запис напруги/струму в offline-регістри модулів для standalone-старту без контролера (команди `def_v`, `def_a`, `def_write` у WebSocket/MQTT; числові сутності в Home Assistant).
- **Збереження уставок у NVS** — остання напруга/струм/стан виходу відновлюються після повного зникнення живлення.
- **Індикація натискання** веб- та апаратних кнопок.
- **Браузерне превʼю** вебморди без прошивки (`tools/gen_preview.py` → `preview/index.html`, `preview/wifi.html`; запускається перед кожним `pio run`).
- Окрема картка **«Статус підключень»** (MQTT, CAN) унизу дашборду.

### Змінено
- **Повний редизайн** вебінтерфейсу у стилі jkbms-web (темні токени, картки, плитки, згортувані секції).
- Розділи налаштувань (Wi-Fi/MQTT, дефолтні значення, оновлення, пароль) зведено в **одну згортувану секцію «Налаштування»**.
- Проєкт переведено на **PlatformIO** (`platformio.ini`, `src_dir → huawei_r4850g2_esp32`, `env esp32dev`/`esp32dev_ota`, авто-підтягування бібліотек).
- Таблиця розділів — **`min_spiffs.csv`** (два app-слоти для OTA); першу прошивку робити через USB.
- OLED спрощено: показує кількість модулів, напругу і струм.
- Структуру переміщено: прошивка тепер у `huawei_r4850g2_esp32/` у корені (без префікса `src/`).

### Прибрано
- **Автоматична зарядка LiFePO4** (CV/CC з таперингом, стан-машина заряду та відповідні команди, MQTT-топіки й сутності Home Assistant). Контролер керує напругою/струмом/виходом; фінальне відсічення лишається за BMS.

## [0.1.0] — 2026-11-10

### Додано
- Керування Huawei R4850G2 через CAN (TWAI, 125 кбіт/с, 29-бітні ID): напруга, сумарний ліміт струму, вихід вкл/викл (broadcast).
- Підтримка кількох модулів паралельно (до `MAX_UNITS`), поділ сумарного струму між онлайн-модулями з перерахунком при зміні кількості.
- Моніторинг по кожному модулю/фазі: вхід/вихід напруга, струм, потужність, ККД, температури, оберти вентилятора, частота мережі, слово аварій, втрата AC, перегрів, деталь `0x018F`.
- Керування вентилятором (авто / ручний рівень / максимум).
- Веб-дашборд (UK/EN) з живим оновленням по WebSocket.
- Інтеграція з Home Assistant через MQTT з автовиявленням (сенсори, числа, перемикачі, бінарні сенсори аварій, per-module).
- Опційний OLED SSD1306 (SPI) та апаратні кнопки (вкл/викл, струм +/−).
- Діагностика CAN з автовідновленням шини (bus-off).
- Captive-портал для первинного налаштування Wi-Fi/MQTT.

---

## English summary

### [0.2.0] — 2026-11-11
- **Added:** password login (Basic Auth; open in AP mode; `admin`/`admin`, changeable from the dashboard); firmware update over browser (`/update`) and network (ArduinoOTA); MQTT live-save without reboot (`/api/mqtt`); inline Wi-Fi setup in the dashboard; module default values written to offline registers (`def_v`/`def_a`/`def_write`); setpoint persistence in NVS (survives full power loss); button-press feedback; browser preview tooling (`tools/gen_preview.py`); a separate "Connection status" card.
- **Changed:** full jkbms-web-style redesign; all settings merged into one collapsible section; moved to PlatformIO (`esp32dev`/`esp32dev_ota`); `min_spiffs.csv` partitions for OTA (first flash over USB); simplified OLED; sources moved to `huawei_r4850g2_esp32/` (no `src/` prefix).
- **Removed:** LiFePO4 auto-charge (state machine, commands, MQTT topics, HA entities) — the BMS remains the final cut-off.

### [0.1.0] — 2026-11-10
- Initial release: CAN control, multi-unit parallel operation, per-module/per-phase monitoring, fan control, UK/EN web dashboard, Home Assistant MQTT discovery, optional OLED + buttons, CAN diagnostics with bus recovery, captive-portal setup.
