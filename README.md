# Huawei R4850G2 × ESP32

[Українська](#українська) · [English](#english)

---

## Українська

Прошивка для ESP32, що керує випрямлячами Huawei R4850G2 (до `MAX_UNITS` модулів паралельно) через CAN. Містить веб-панель керування, налаштування Wi-Fi/MQTT через браузер, інтеграцію з Home Assistant через MQTT (автовиявлення), автоматику зарядки LiFePO4 та опційні OLED-дисплей і кнопки.

### Можливості

- Керування напругою, струмом і станом виходу (вкл/викл) усіх модулів одночасно (broadcast по CAN).
- Струм задається **сумарний** і ділиться порівну між модулями, що наразі онлайн (перераховується, коли кількість модулів змінюється).
- Керування вентилятором (авто / ручний рівень як нижня межа / максимум).
- Моніторинг по кожному модулю: вхідна/вихідна напруга, струм, потужність, ККД, температури, оберти вентилятора, частота мережі, слово аварій, втрата AC/фази, перегрів, детальна аварійна інформація (`0x018F`).
- Автозарядка LiFePO4: BULK (CC) → ABSORB (CV) → завершення за «хвостовим» струмом → вимкнення виходу або FLOAT (з новим циклом, якщо напруга просіла); захист за температурою та за часом (FAULT із фіксацією до перезапуску зарядки).
- Веб-інтерфейс на `http://<hostname>.local` (за замовчуванням `http://huawei-r4850.local`) з живим оновленням через WebSocket `/ws` і перемикачем мови UK/EN.
- Налаштування Wi-Fi та MQTT без перепрошивки: точка доступу з captive-порталом і сторінка `/wifi`; дані зберігаються в NVS.
- MQTT + Home Assistant Discovery: сенсори, числа, перемикачі, бінарні сенсори аварій, окремі сенсори для кожного модуля.
- Опційно: OLED SSD1306 128×64 (SPI) і три кнопки (вкл/викл, струм +/−).
- Діагностика CAN (bus-off, лічильники помилок) з автовідновленням шини; остання помилка видна на дашборді та в HA.
- Двомовні повідомлення прошивки (Serial, помилки): `FW_LANG_EN` у `config.h`.

### Апаратна частина

| Компонент | Підключення |
|---|---|
| CAN-трансивер SN65HVD230 (3.3 В) | TX → GPIO5, RX ← GPIO4, VCC = 3V3, GND |
| CAN-шина | 125 кбіт/с, 29-бітні ID; CANH/CANL до роз'єму випрямляча, не забудьте термінатор |
| OLED SSD1306 (SPI), за бажанням | SCK → GPIO18, MOSI → GPIO23, DC → GPIO16, RES → GPIO17, CS → GPIO21 |
| Кнопки (між GPIO і GND), за бажанням | вкл/викл → GPIO25, струм + → GPIO26, струм − → GPIO27 |

Усі піни змінюються в `src/huawei_r4850g2_esp32/config.h`.

### Структура проєкту

- `src/huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino` — уся логіка прошивки (CAN, веб, MQTT, зарядка, OLED, кнопки, Wi-Fi).
- `src/huawei_r4850g2_esp32/config.h` — усі налаштування в одному місці.
- `src/huawei_r4850g2_esp32/index_html.h` — веб-панель керування (віддається на `/` і `/dashboard`).
- `src/huawei_r4850g2_esp32/wifi_html.h` — сторінка налаштування Wi-Fi/MQTT (`/wifi`, а в режимі точки доступу — також `/`).

### Збірка та прошивка

1. Встановіть у Arduino IDE (або PlatformIO) підтримку плат ESP32.
2. Встановіть бібліотеки: **ESPAsyncWebServer** і **AsyncTCP** (me-no-dev), **PubSubClient**, **ArduinoJson** (v7); для OLED додатково **Adafruit SSD1306** і **Adafruit GFX Library**. `driver/twai.h`, `Preferences`, `ESPmDNS`, `DNSServer` входять до складу ESP32 core.
3. Відкрийте `src/huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino`.
4. У `config.h` перевірте піни, ліміти та параметри зарядки. Дані Wi-Fi та MQTT у прошивці **не зашиваються** — їх задають після першого запуску (див. нижче).
5. Виберіть свою плату ESP32, зберіть і прошийте. Монітор порту: 115200 бод; рядки лога мають префікси `[CAN]`, `[SET]`, `[MQTT]`, `[WiFi]`, `[ERR]`.

### Перший запуск: Wi-Fi та MQTT

1. Якщо Wi-Fi ще не збережено (або підключитись не вдалось за `WIFI_CONNECT_TIMEOUT_S` секунд), ESP32 піднімає відкриту точку доступу `Huawei-R4850-Setup` (`AP_SSID`; пароль — `AP_PASSWORD`, порожній = без пароля). Вона працює як captive-портал; налаштування також доступні за адресою `http://192.168.4.1`.
2. На сторінці оберіть мережу зі списку, введіть пароль і, за потреби, дані MQTT-брокера (хост, порт, логін, пароль). Порожній хост = MQTT вимкнено, порожній логін = анонімне підключення, порожній пароль = лишити попередній.
3. Після збереження пристрій перезавантажується і підключається до роутера; панель доступна на `http://huawei-r4850.local` (`HOSTNAME`) або за IP з Serial-логу.
4. Поки до точки доступу ніхто не підключений, прошивка раз на 60 с пробує знову підключитись до збереженої мережі.
5. Змінити налаштування пізніше можна на `http://<адреса>/wifi` (посилання є внизу дашборду).
6. **Скидання Wi-Fi:** утримуйте кнопку вкл/викл під час старту ≥ `WIFI_RESET_HOLD_MS` (3 с) — збережена мережа видаляється й піднімається точка доступу (налаштування MQTT лишаються).

### Налаштування (`config.h`)

- `FW_LANG_EN` — мова повідомлень прошивки (`false` = українська, `true` = English). Мову дашборду перемикає кнопка UK/EN.
- `HOSTNAME`, `AP_SSID`, `AP_PASSWORD`, `WIFI_CONNECT_TIMEOUT_S`, `WIFI_RESET_HOLD_MS` — мережа та точка доступу.
- `MQTT_ENABLED` — головний перемикач MQTT на етапі збірки; `MQTT_HOST`/`MQTT_PORT` — лише значення за замовчуванням (збережені на `/wifi` мають пріоритет); `MQTT_BASE` — базовий топік; `HA_DISCOVERY_PREFIX` — префікс автовиявлення Home Assistant.
- `CAN_TX_PIN`, `CAN_RX_PIN` — піни CAN.
- `MAX_UNITS` — максимум модулів на шині (адреси 1..N, за замовчуванням 6).
- `RATED_MAX_CURRENT` — номінальний струм модуля (62.5 А для R4850G2), потрібен для масштабування ліміту струму.
- `AC_FAIL_V`, `PSU_OVERTEMP_C`, `ALARM_DETAIL_BIT` — пороги моніторингу модулів (втрата AC, перегрів, аварійний біт для читання `0x018F`).
- `VOLT_MIN/MAX`, `CURR_MIN/MAX` — межі, які приймаються з веб/MQTT. Через веб/MQTT сумарний струм обмежується `CURR_MAX` (один модуль), а кнопками — до `CURR_MAX * MAX_UNITS`.
- `DEFAULT_VOLTAGE`, `DEFAULT_CURRENT`, `DEFAULT_OUTPUT_ON` — значення при старті.
- `FAN_MODE_DEFAULT`, `FAN_DUTY_DEFAULT` — режим і рівень вентилятора при старті.
- `CHG_*` — параметри автозарядки (напруга, струм, tail, `CHG_ABSORB_HOLD_S`, float і напруга повторного заряду, таймаут, макс. температура, `CHG_ENABLED_ON_BOOT`). Значення за замовчуванням — приклад для 16S LiFePO4, **змініть під свою батарею**. `CHG_FLOAT_VOLTAGE = 0` — вимкнути вихід після заряду.
- `OLED_ENABLED`, `BUTTONS_ENABLED` — вмикання OLED і кнопок; `OLED_*`, `BTN_*` — піни, `CURR_STEP` — крок струму кнопками (за замовчуванням 5 А), `BTN_DEBOUNCE_MS`.
- `WRITE_OFFLINE_DEFAULTS` — чи записувати енергонезалежні значення (зношує пам'ять модуля, за замовчуванням вимкнено). Online-значення в будь-якому разі перезастосовуються кожні 10 с.

### Керування

Усі три способи керування працюють з одним станом. Ручна зміна напруги, струму чи виходу (веб, MQTT, кнопки) **виходить з автозарядки**.

- **Веб/WebSocket `/ws`** — JSON `{"cmd": "...", "value": ...}`: `voltage`, `current`, `output`, `fanmode`, `fanduty`, `charge`, `chg_v`, `chg_a`, `chg_tail`.
- **MQTT** — див. нижче.
- **Кнопки** — вкл/викл виходу, струм ± `CURR_STEP`.

### MQTT

Базовий топік — `MQTT_BASE` (за замовчуванням `huawei_r4850`). Стан публікується (retained) у `<base>/state` (той самий JSON, що йде на дашборд), команди приймаються в `<base>/set/<name>`:

| Топік `set/…` | Значення |
|---|---|
| `voltage`, `current` | число (В / сумарні А) |
| `output` | `ON`/`1` або `OFF`/`0` |
| `fanmax` | `ON`/`1` = максимальні оберти, інакше авто |
| `fanduty` | рівень вентилятора, % (переводить у режим авто) |
| `charge` | `ON`/`1` — запустити автозарядку, інакше зупинити |
| `chg_v`, `chg_a`, `chg_tail` | параметри автозарядки |

Сутності в Home Assistant з'являються автоматично (пристрій «Huawei R4850G2»):

- сенсори: вихідна напруга/струм/потужність, вхідна напруга/потужність, ККД, температури входу/виходу, кількість модулів онлайн, оберти вентилятора;
- числа: напруга, струм, рівень вентилятора, параметри зарядки (напруга, струм, tail);
- перемикачі: вихід, макс. вентилятор, автозарядка;
- текстові сенсори: стан зарядки, стан CAN, остання помилка; бінарний сенсор «PSU alarm»;
- для кожного виявленого модуля: вхідна напруга, частота мережі, вихідний струм, «AC fail», «Alarm bit2», вміст `0x018F`.

### Автозарядка LiFePO4

`OFF → BULK (CC) → ABSORB (CV) → DONE → FLOAT` (або вихід вимкнено, якщо `CHG_FLOAT_VOLTAGE = 0`).

- BULK → ABSORB, коли напруга досягає `chgV − 0.15 В`.
- ABSORB → DONE, коли сумарний струм ≤ tail протягом `CHG_ABSORB_HOLD_S` секунд.
- У FLOAT, якщо напруга впала нижче `CHG_RECHARGE_VOLTAGE`, починається новий цикл.
- Перевищення `CHG_MAX_TEMP` або `CHG_MAX_TIME_S` → **FAULT**: вихід вимикається, стан фіксується до перезапуску зарядки.

### Увага

Ви працюєте з потужним джерелом живлення (до 3 кВт на модуль) та мережевою напругою. Перед підключенням до батареї перевірте напругу й струм заряду, межі в `config.h` і параметри автозарядки. Автор не несе відповідальності за пошкодження обладнання чи батарей.

### Джерела протоколу

- [patagonaa/huawei-r48xx](https://github.com/patagonaa/huawei-r48xx)
- [craigpeacock/Huawei_R4850G2_CAN](https://github.com/craigpeacock/Huawei_R4850G2_CAN)

---

## English

ESP32 firmware that controls Huawei R4850G2 rectifiers (up to `MAX_UNITS` modules in parallel) over CAN. It provides a web control panel, Wi-Fi/MQTT setup from the browser, Home Assistant integration over MQTT (auto-discovery), an LiFePO4 charge controller, and an optional OLED display and buttons.

### Features

- Set voltage, current and output state (on/off) on all modules at once (CAN broadcast).
- Current is set as a **total** and split evenly among the modules currently online (recalculated whenever the module count changes).
- Fan control (auto / manual level used as a lower bound / max).
- Per-module monitoring: input/output voltage, current, power, efficiency, temperatures, fan RPM, mains frequency, alarm word, AC/phase loss, over-temperature, detailed alarm info (`0x018F`).
- LiFePO4 auto-charge: BULK (CC) → ABSORB (CV) → finish on tail current → output off or FLOAT (with a new cycle if the voltage sags); temperature and time safety cut-offs (latched FAULT until charging is restarted).
- Web UI at `http://<hostname>.local` (default `http://huawei-r4850.local`) with live updates over WebSocket `/ws` and a UK/EN language toggle.
- Wi-Fi and MQTT setup without reflashing: access point with a captive portal and the `/wifi` page; settings are stored in NVS.
- MQTT + Home Assistant Discovery: sensors, numbers, switches, alarm binary sensors, and separate per-module sensors.
- Optional: SSD1306 128×64 SPI OLED and three buttons (on/off, current +/−).
- CAN diagnostics (bus-off, error counters) with automatic bus recovery; the last error is shown on the dashboard and in HA.
- Bilingual firmware messages (Serial, errors): `FW_LANG_EN` in `config.h`.

### Hardware

| Component | Connection |
|---|---|
| SN65HVD230 CAN transceiver (3.3 V) | TX → GPIO5, RX ← GPIO4, VCC = 3V3, GND |
| CAN bus | 125 kbit/s, 29-bit IDs; CANH/CANL to the rectifier connector, remember bus termination |
| SSD1306 OLED (SPI), optional | SCK → GPIO18, MOSI → GPIO23, DC → GPIO16, RES → GPIO17, CS → GPIO21 |
| Buttons (GPIO to GND), optional | on/off → GPIO25, current + → GPIO26, current − → GPIO27 |

All pins can be changed in `src/huawei_r4850g2_esp32/config.h`.

### Project layout

- `src/huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino` — all firmware logic (CAN, web, MQTT, charging, OLED, buttons, Wi-Fi).
- `src/huawei_r4850g2_esp32/config.h` — every setting in one place.
- `src/huawei_r4850g2_esp32/index_html.h` — the web dashboard (served at `/` and `/dashboard`).
- `src/huawei_r4850g2_esp32/wifi_html.h` — the Wi-Fi/MQTT setup page (`/wifi`, and `/` while in access-point mode).

### Build and flash

1. Install ESP32 board support in the Arduino IDE (or PlatformIO).
2. Install the libraries: **ESPAsyncWebServer** and **AsyncTCP** (me-no-dev), **PubSubClient**, **ArduinoJson** (v7); for the OLED also **Adafruit SSD1306** and **Adafruit GFX Library**. `driver/twai.h`, `Preferences`, `ESPmDNS` and `DNSServer` ship with the ESP32 core.
3. Open `src/huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino`.
4. In `config.h`, check the pins, limits and charge parameters. Wi-Fi and MQTT credentials are **not** hard-coded — you set them after the first boot (see below).
5. Select your ESP32 board, build and upload. Serial monitor: 115200 baud; log lines are prefixed `[CAN]`, `[SET]`, `[MQTT]`, `[WiFi]`, `[ERR]`.

### First run: Wi-Fi and MQTT

1. If no Wi-Fi is stored (or the connection fails within `WIFI_CONNECT_TIMEOUT_S` seconds), the ESP32 starts an open access point `Huawei-R4850-Setup` (`AP_SSID`; password is `AP_PASSWORD`, empty = open). It works as a captive portal; the setup page is also at `http://192.168.4.1`.
2. On the page, pick a network from the list, enter its password and, optionally, the MQTT broker details (host, port, user, password). Empty host = MQTT off, empty user = anonymous connection, empty password = keep the previous one.
3. After saving, the device reboots and joins your router; the dashboard is at `http://huawei-r4850.local` (`HOSTNAME`) or at the IP shown in the Serial log.
4. While nobody is connected to the access point, the firmware retries the stored network every 60 s.
5. You can change the settings later at `http://<address>/wifi` (there is a link at the bottom of the dashboard).
6. **Wi-Fi reset:** hold the on/off button at boot for ≥ `WIFI_RESET_HOLD_MS` (3 s) — the stored network is deleted and the access point starts (MQTT settings are kept).

### Configuration (`config.h`)

- `FW_LANG_EN` — firmware message language (`false` = Ukrainian, `true` = English). The dashboard language is switched with the UK/EN button.
- `HOSTNAME`, `AP_SSID`, `AP_PASSWORD`, `WIFI_CONNECT_TIMEOUT_S`, `WIFI_RESET_HOLD_MS` — network and access point.
- `MQTT_ENABLED` — build-time master switch for MQTT; `MQTT_HOST`/`MQTT_PORT` are only defaults (values saved on `/wifi` take precedence); `MQTT_BASE` — base topic; `HA_DISCOVERY_PREFIX` — Home Assistant discovery prefix.
- `CAN_TX_PIN`, `CAN_RX_PIN` — CAN pins.
- `MAX_UNITS` — maximum number of modules on the bus (addresses 1..N, default 6).
- `RATED_MAX_CURRENT` — module rated current (62.5 A for the R4850G2), used to scale the current limit register.
- `AC_FAIL_V`, `PSU_OVERTEMP_C`, `ALARM_DETAIL_BIT` — module monitoring thresholds (AC loss, over-temperature, alarm bit that triggers reading `0x018F`).
- `VOLT_MIN/MAX`, `CURR_MIN/MAX` — bounds accepted from web/MQTT. Over web/MQTT the total current is clamped to `CURR_MAX` (one module), while the buttons allow up to `CURR_MAX * MAX_UNITS`.
- `DEFAULT_VOLTAGE`, `DEFAULT_CURRENT`, `DEFAULT_OUTPUT_ON` — startup values.
- `FAN_MODE_DEFAULT`, `FAN_DUTY_DEFAULT` — fan mode and level at startup.
- `CHG_*` — auto-charge parameters (voltage, current, tail, `CHG_ABSORB_HOLD_S`, float and recharge voltage, timeout, max temperature, `CHG_ENABLED_ON_BOOT`). Defaults are an example for 16S LiFePO4; **adjust them for your battery**. `CHG_FLOAT_VOLTAGE = 0` turns the output off after charging.
- `OLED_ENABLED`, `BUTTONS_ENABLED` — enable the OLED and buttons; `OLED_*`, `BTN_*` — pins, `CURR_STEP` — current step for the buttons (default 5 A), `BTN_DEBOUNCE_MS`.
- `WRITE_OFFLINE_DEFAULTS` — also write the non-volatile values (wears the module's memory; off by default). Online values are re-applied every 10 s regardless.

### Control

All three control surfaces share the same state. Manually changing voltage, current or output (web, MQTT, buttons) **leaves auto-charge**.

- **Web/WebSocket `/ws`** — JSON `{"cmd": "...", "value": ...}`: `voltage`, `current`, `output`, `fanmode`, `fanduty`, `charge`, `chg_v`, `chg_a`, `chg_tail`.
- **MQTT** — see below.
- **Buttons** — output on/off, current ± `CURR_STEP`.

### MQTT

The base topic is `MQTT_BASE` (default `huawei_r4850`). State is published (retained) to `<base>/state` (the same JSON the dashboard receives); commands are accepted on `<base>/set/<name>`:

| `set/…` topic | Value |
|---|---|
| `voltage`, `current` | number (V / total A) |
| `output` | `ON`/`1` or `OFF`/`0` |
| `fanmax` | `ON`/`1` = max fan speed, otherwise auto |
| `fanduty` | fan level, % (switches to auto mode) |
| `charge` | `ON`/`1` starts auto-charge, otherwise stops it |
| `chg_v`, `chg_a`, `chg_tail` | auto-charge parameters |

Home Assistant entities appear automatically (device "Huawei R4850G2"):

- sensors: output voltage/current/power, input voltage/power, efficiency, input/output temperatures, units online, fan RPM;
- numbers: voltage, current, fan duty, charge parameters (voltage, current, tail);
- switches: output, fan max, auto charge;
- text sensors: charge state, CAN state, last error; binary sensor "PSU alarm";
- for every detected module: input voltage, mains frequency, output current, "AC fail", "Alarm bit2", the `0x018F` contents.

### LiFePO4 auto-charge

`OFF → BULK (CC) → ABSORB (CV) → DONE → FLOAT` (or output off if `CHG_FLOAT_VOLTAGE = 0`).

- BULK → ABSORB when the voltage reaches `chgV − 0.15 V`.
- ABSORB → DONE when the total current stays ≤ tail for `CHG_ABSORB_HOLD_S` seconds.
- In FLOAT, if the voltage drops below `CHG_RECHARGE_VOLTAGE`, a new cycle begins.
- Exceeding `CHG_MAX_TEMP` or `CHG_MAX_TIME_S` → **FAULT**: the output is switched off and the state is latched until charging is restarted.

### Warning

You are working with a high-power supply (up to 3 kW per module) and mains voltage. Before connecting a battery, check the charge voltage and current, the limits in `config.h` and the auto-charge parameters. The author is not responsible for damage to equipment or batteries.

### Protocol references

- [patagonaa/huawei-r48xx](https://github.com/patagonaa/huawei-r48xx)
- [craigpeacock/Huawei_R4850G2_CAN](https://github.com/craigpeacock/Huawei_R4850G2_CAN)
