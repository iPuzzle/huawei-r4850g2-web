# Huawei R4850G2 × ESP32

[Українська](#українська) · [English](#english)

---

## Українська

Прошивка для ESP32, що керує випрямлячами Huawei R4850G2 (до `MAX_UNITS` модулів паралельно) через CAN. Містить веб-панель керування у стилі jkbms-web, налаштування Wi-Fi/MQTT прямо з браузера, вхід по паролю, оновлення прошивки через браузер (OTA), інтеграцію з Home Assistant через MQTT (автовиявлення) та опційні OLED-дисплей і кнопки.

> Сумісність: протокол той самий для R4850G2, R4850G5 і R4875G5 — модулі різних поколінь можна змішувати на одній шині. Виходи зʼєднуються паралельно; кожен модуль може живитися від окремої фази (вхід ізольований від виходу).

### Можливості

- Керування напругою, сумарним лімітом струму і станом виходу (вкл/викл) усіх модулів одночасно (broadcast по CAN).
- Струм задається **сумарний** і ділиться порівну між модулями, що наразі онлайн (перераховується, коли кількість модулів змінюється).
- Останні задані напруга/струм/стан виходу зберігаються в NVS і **відновлюються після повного зникнення живлення**.
- **Дефолтні значення модулів**: запис напруги/струму в енергонезалежні (offline) регістри самих модулів — діють, коли модуль стартує **без** контролера (standalone).
- Керування вентилятором (авто / ручний рівень як нижня межа / максимум).
- Моніторинг по кожному модулю (і по кожній фазі): вхідна/вихідна напруга, струм, потужність, ККД, температури, оберти вентилятора, частота мережі, слово аварій, втрата AC/фази, перегрів, детальна аварійна інформація (`0x018F`).
- Веб-інтерфейс на `http://huawei-r4850.local` (`HOSTNAME`) з живим оновленням через WebSocket `/ws`, перемикачем мови UK/EN і згортуваною секцією «Налаштування».
- **Вхід по паролю** (HTTP Basic Auth) у режимі роботи; у режимі первинного налаштування (точка доступу) пароль не вимагається.
- Налаштування Wi-Fi і MQTT без перепрошивки — прямо в секції «Налаштування» дашборду, а також captive-портал при першому запуску. MQTT можна зберегти **окремо, без перезавантаження**.
- **Оновлення прошивки**: через браузер (завантаження `firmware.bin`) і мережеве (ArduinoOTA / `pio ... -e esp32dev_ota`).
- MQTT + Home Assistant Discovery: сенсори, числа, перемикачі, бінарні сенсори аварій, окремі сенсори для кожного модуля.
- Опційно: OLED SSD1306 128×64 (SPI, показує кількість модулів, напругу, струм) і три кнопки (вкл/викл, струм +/−) з індикацією натискання.
- Діагностика CAN (bus-off, лічильники помилок) з **автовідновленням шини**; остання помилка видна на дашборді та в HA.
- Браузерне **превʼю** вебморди без прошивки (`tools/gen_preview.py`).
- Двомовні повідомлення прошивки (Serial, помилки): `FW_LANG_EN` у `config.h`.

### Апаратна частина

| Компонент | Підключення |
|---|---|
| CAN-трансивер SN65HVD230 (3.3 В) | TX → GPIO5, RX ← GPIO4, VCC = 3V3, GND |
| CAN-шина | 125 кбіт/с, 29-бітні ID; CANH/CANL до роз'єму випрямляча; термінатор 120 Ω на обох фізичних кінцях шини |
| OLED SSD1306 (SPI), за бажанням | SCK → GPIO18, MOSI → GPIO23, DC → GPIO16, RES → GPIO17, CS → GPIO21 |
| Кнопки (між GPIO і GND), за бажанням | вкл/викл → GPIO25, струм + → GPIO26, струм − → GPIO27 |

Усі піни змінюються в `huawei_r4850g2_esp32/config.h`. Рекомендована плата — ESP32 WROOM-32 (бажано з роз'ємом під зовнішню антену, якщо контролер стоїть у металевій шафі). Живлення контролера — від DC-шини батареї через понижувач 48–58 В → 5 В (вхід ≥ 65 В), щоб працювати і під час відсутності мережі; спільна земля з `DC−`.

> **Увага.** Щоб модуль видавав напругу, обидва піни slot-detect мають бути замкнені на `DC−`. Паралельне підключення до зарядженої батареї дає іскри — застосовуйте precharge. Кожну фазу заводьте як «фаза + нуль» (не міжфазно). Фінальне відсічення заряду завжди лишається за BMS.

### Структура проєкту

```
huawei-r4850g2-web/
├─ platformio.ini                      конфіг PlatformIO (src_dir → huawei_r4850g2_esp32)
├─ huawei_r4850g2_esp32/
│  ├─ huawei_r4850g2_esp32.ino         уся логіка прошивки (CAN, веб, MQTT, OLED, кнопки, Wi-Fi, OTA, auth)
│  ├─ config.h                         усі налаштування в одному місці
│  ├─ index_html.h                     веб-дашборд ( / та /dashboard )
│  └─ wifi_html.h                      сторінка налаштування для режиму точки доступу
├─ tools/gen_preview.py                генератор браузерного превʼю
├─ preview/{index,wifi}.html           згенероване превʼю (відкривається без прошивки)
├─ doc/                                даташити, фото
├─ README.md · CHANGELOG.md · LICENSE
```

### Збірка та прошивка (PlatformIO)

```sh
pio run                              # зібрати (env esp32dev)
pio run -t upload                    # прошити через USB
pio device monitor                   # Serial 115200 (з декодером винятків)
pio run -e esp32dev_ota -t upload    # мережеве оновлення (ArduinoOTA; пароль = пароль вебморди)
```

- Бібліотеки тягне PlatformIO автоматично (`lib_deps`): `ESP32Async/ESPAsyncWebServer` + `AsyncTCP`, `PubSubClient`, `ArduinoJson` (v7), `Adafruit SSD1306` + `Adafruit GFX`. `driver/twai.h`, `Preferences`, `ESPmDNS`, `DNSServer`, `Update`, `ArduinoOTA` входять до ESP32 core.
- Таблиця розділів — `min_spiffs.csv` (два app-слоти для OTA). **Першу прошивку робіть через USB** (змінюється таблиця розділів), подальші — через браузер або `esp32dev_ota`.
- Дані Wi-Fi/MQTT і пароль у прошивці **не зашиваються** — задаються після першого запуску.

> Arduino IDE також підходить (відкрийте `huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino`), але встановіть саме форк **ESP32Async** для ESPAsyncWebServer/AsyncTCP — він має `AsyncAuthenticationMiddleware`.

### Перший запуск: Wi-Fi, MQTT, пароль

1. Якщо Wi-Fi ще не збережено (або підключитись не вдалось за `WIFI_CONNECT_TIMEOUT_S` секунд), ESP32 піднімає точку доступу `Huawei-R4850-Setup` (`AP_SSID`; пароль `AP_PASSWORD`, порожній = без пароля). Це captive-портал; сторінка також доступна на `http://192.168.4.1`. **У режимі точки доступу пароль на вхід не вимагається.**
2. Оберіть мережу, введіть пароль і, за потреби, дані MQTT-брокера (хост, порт, логін, пароль). Порожній хост = MQTT вимкнено, порожній логін = анонімно, порожній пароль = лишити попередній.
3. Після збереження пристрій перезавантажується і підключається до роутера; панель — на `http://huawei-r4850.local` або за IP з Serial-логу.
4. **Вхід:** логін `admin`, пароль `admin` за замовчуванням (`WEB_USER` / `WEB_DEFAULT_PASS`). **Змініть його** в «Налаштування → Пароль доступу» (мін. 6 символів; пристрій перезавантажиться).
5. Налаштування Wi-Fi/MQTT пізніше — у секції «Налаштування» на дашборді. **MQTT** можна зберегти окремою кнопкою — застосовується одразу, без перезавантаження; **Wi-Fi** зберігається з перезавантаженням.
6. **Скидання Wi-Fi:** утримуйте кнопку вкл/викл під час старту ≥ `WIFI_RESET_HOLD_MS` (3 с) — збережена мережа видаляється й піднімається точка доступу (MQTT-налаштування лишаються).

### Вебінтерфейс

Дашборд складається з карток: **Статус** (плитки показників), **Модулі** (per-unit/per-phase), **Напруга**, **Ліміт струму**, **Вихід**, **Вентилятор**, згортувана **Налаштування** (Wi-Fi та MQTT, дефолтні значення модулів, оновлення прошивки, пароль доступу) і окрема **Статус підключень** (MQTT, CAN) унизу.

HTTP-маршрути: `/` (дашборд або, в AP, сторінка налаштування), `/dashboard`, `/wifi` + `/wifi/scan|save|forget`, `/api/mqtt` (зберегти MQTT без ребуту), `/api/password`, `/update` (OTA), WebSocket `/ws`. У режимі STA усе під Basic Auth.

### Налаштування (`config.h`)

- `FW_LANG_EN` — мова повідомлень прошивки (`false` = українська). Мову дашборду перемикає кнопка UK/EN.
- `HOSTNAME`, `AP_SSID`, `AP_PASSWORD`, `WIFI_CONNECT_TIMEOUT_S`, `WIFI_RESET_HOLD_MS` — мережа та точка доступу.
- `WEB_USER`, `WEB_DEFAULT_PASS` — логін і початковий пароль вебінтерфейсу (пароль зберігається в NVS, змінюється з дашборду).
- `MQTT_ENABLED` — головний перемикач MQTT на етапі збірки; `MQTT_HOST`/`MQTT_PORT` — лише значення за замовчуванням (збережені на дашборді мають пріоритет); `MQTT_BASE` — базовий топік; `HA_DISCOVERY_PREFIX` — префікс автовиявлення HA.
- `CAN_TX_PIN`, `CAN_RX_PIN` — піни CAN.
- `MAX_UNITS` — максимум модулів на шині (адреси 1..N, за замовчуванням 6).
- `RATED_MAX_CURRENT` — номінальний струм модуля (62.5 А для R4850G2), для масштабування регістра струму.
- `AC_FAIL_V`, `PSU_OVERTEMP_C`, `ALARM_DETAIL_BIT` — пороги моніторингу модулів (втрата AC, перегрів, аварійний біт для читання `0x018F`).
- `VOLT_MIN/MAX`, `CURR_MIN/MAX` — межі, які приймаються з веб/MQTT.
- `DEFAULT_VOLTAGE`, `DEFAULT_CURRENT`, `DEFAULT_OUTPUT_ON` — значення при старті, якщо в NVS ще нічого немає.
- `PSU_DEFAULT_VOLTAGE`, `PSU_DEFAULT_CURRENT` — початкові «дефолтні значення модулів» (offline-регістри) для UI.
- `FAN_MODE_DEFAULT`, `FAN_DUTY_DEFAULT` — режим і рівень вентилятора при старті.
- `OLED_ENABLED`, `BUTTONS_ENABLED` — вмикання OLED і кнопок; `OLED_*`, `BTN_*` — піни, `CURR_STEP` — крок струму кнопками (за замовчуванням 5 А), `BTN_DEBOUNCE_MS`.
- `WRITE_OFFLINE_DEFAULTS` — чи дублювати запис offline-значень при кожній зміні online (зношує пам'ять модуля, за замовчуванням вимкнено).

### Керування

Усі способи керування працюють з одним станом; зміни напруги/струму/виходу зберігаються в NVS.

- **Веб/WebSocket `/ws`** — JSON `{"cmd": "...", "value": ...}`: `voltage`, `current` (сумарний А), `output`, `fanmode` (0=авто,1=макс), `fanduty`, `def_v`, `def_a`, `def_write` (записати дефолти в модулі).
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
| `def_v`, `def_a` | дефолтні напруга/струм модулів |
| `def_write` | записати дефолтні значення в модулі |

Сутності в Home Assistant з'являються автоматично (пристрій «Huawei R4850G2»):

- сенсори: вихідна напруга/струм/потужність, вхідна напруга/потужність, ККД, температури входу/виходу, кількість модулів онлайн, оберти вентилятора;
- числа: напруга, струм, рівень вентилятора, дефолтна напруга, дефолтний струм;
- перемикачі: вихід, макс. вентилятор;
- текстові сенсори: стан CAN, остання помилка; бінарний сенсор «PSU alarm»;
- для кожного виявленого модуля: вхідна напруга, частота мережі, вихідний струм, «AC fail», «Alarm bit2», вміст `0x018F`.

### Превʼю вебморди

`tools/gen_preview.py` витягує HTML з `index_html.h`/`wifi_html.h` і додає мок-дані (фейкові WebSocket/fetch), генеруючи `preview/index.html` та `preview/wifi.html` — їх можна відкрити у браузері без прошивки. Запускається автоматично перед кожним `pio run` (pre-скрипт) або вручну: `python tools/gen_preview.py`.

### Увага

Ви працюєте з потужним джерелом живлення (до 3 кВт на модуль) та мережевою напругою. Перед підключенням до батареї перевірте напругу й струм, межі в `config.h` і стан BMS. Автор не несе відповідальності за пошкодження обладнання чи батарей.

### Джерела протоколу

- [patagonaa/huawei-r48xx](https://github.com/patagonaa/huawei-r48xx)
- [craigpeacock/Huawei_R4850G2_CAN](https://github.com/craigpeacock/Huawei_R4850G2_CAN)

---

## English

ESP32 firmware that controls Huawei R4850G2 rectifiers (up to `MAX_UNITS` modules in parallel) over CAN. It provides a jkbms-web-styled web control panel, Wi-Fi/MQTT setup from the browser, password login, firmware update over the browser (OTA), Home Assistant integration over MQTT (auto-discovery), and an optional OLED display and buttons.

> Compatibility: the protocol is the same for R4850G2, R4850G5 and R4875G5 — modules of different generations can be mixed on one bus. Outputs are paralleled; each module may run off a separate phase (input is isolated from output).

### Features

- Set voltage, total current limit and output state (on/off) on all modules at once (CAN broadcast).
- Current is a **total**, split evenly among the modules currently online (recalculated whenever the module count changes).
- The last voltage/current/output state is stored in NVS and **restored after a full power loss**.
- **Module default values**: write voltage/current into the modules' non-volatile (offline) registers — they apply when a module starts **without** the controller (standalone).
- Fan control (auto / manual level as a lower bound / max).
- Per-module (and per-phase) monitoring: input/output voltage, current, power, efficiency, temperatures, fan RPM, mains frequency, alarm word, AC/phase loss, over-temperature, detailed alarm info (`0x018F`).
- Web UI at `http://huawei-r4850.local` (`HOSTNAME`) with live WebSocket `/ws` updates, a UK/EN toggle and a collapsible Settings section.
- **Password login** (HTTP Basic Auth) while running; the first-time setup (access-point) mode requires no password.
- Wi-Fi and MQTT setup without reflashing — right in the dashboard Settings section, plus a captive portal on first boot. MQTT can be saved **separately, without a reboot**.
- **Firmware update**: over the browser (upload `firmware.bin`) and over the network (ArduinoOTA / `pio ... -e esp32dev_ota`).
- MQTT + Home Assistant Discovery: sensors, numbers, switches, alarm binary sensors, and separate per-module sensors.
- Optional: SSD1306 128×64 SPI OLED (shows module count, voltage, current) and three buttons (on/off, current +/−) with press feedback.
- CAN diagnostics (bus-off, error counters) with **automatic bus recovery**; the last error is shown on the dashboard and in HA.
- Browser **preview** of the web UI without flashing (`tools/gen_preview.py`).
- Bilingual firmware messages (Serial, errors): `FW_LANG_EN` in `config.h`.

### Hardware

| Component | Connection |
|---|---|
| SN65HVD230 CAN transceiver (3.3 V) | TX → GPIO5, RX ← GPIO4, VCC = 3V3, GND |
| CAN bus | 125 kbit/s, 29-bit IDs; CANH/CANL to the rectifier connector; 120 Ω terminator at both physical ends |
| SSD1306 OLED (SPI), optional | SCK → GPIO18, MOSI → GPIO23, DC → GPIO16, RES → GPIO17, CS → GPIO21 |
| Buttons (GPIO to GND), optional | on/off → GPIO25, current + → GPIO26, current − → GPIO27 |

All pins can be changed in `huawei_r4850g2_esp32/config.h`. Recommended board: ESP32 WROOM-32 (ideally with an external-antenna connector if the controller sits in a metal cabinet). Power the controller from the battery DC bus via a 48–58 V → 5 V buck (≥ 65 V input) so it stays alive during a mains outage; share ground with `DC−`.

> **Note.** Both slot-detect pins must be tied to `DC−` for a module to output voltage. Paralleling onto a charged battery causes sparks — use pre-charge. Wire each phase as "line + neutral" (never line-to-line). The BMS remains the final charge cut-off.

### Project layout

```
huawei-r4850g2-web/
├─ platformio.ini                      PlatformIO config (src_dir → huawei_r4850g2_esp32)
├─ huawei_r4850g2_esp32/
│  ├─ huawei_r4850g2_esp32.ino         all firmware logic (CAN, web, MQTT, OLED, buttons, Wi-Fi, OTA, auth)
│  ├─ config.h                         every setting in one place
│  ├─ index_html.h                     the web dashboard ( / and /dashboard )
│  └─ wifi_html.h                      setup page for access-point mode
├─ tools/gen_preview.py                browser-preview generator
├─ preview/{index,wifi}.html           generated preview (opens without flashing)
├─ doc/                                datasheets, photos
├─ README.md · CHANGELOG.md · LICENSE
```

### Build and flash (PlatformIO)

```sh
pio run                              # build (env esp32dev)
pio run -t upload                    # flash over USB
pio device monitor                   # Serial 115200 (with exception decoder)
pio run -e esp32dev_ota -t upload    # network update (ArduinoOTA; password = web password)
```

- Libraries are pulled automatically (`lib_deps`): `ESP32Async/ESPAsyncWebServer` + `AsyncTCP`, `PubSubClient`, `ArduinoJson` (v7), `Adafruit SSD1306` + `Adafruit GFX`. `driver/twai.h`, `Preferences`, `ESPmDNS`, `DNSServer`, `Update`, `ArduinoOTA` ship with the ESP32 core.
- Partition table is `min_spiffs.csv` (two app slots for OTA). **Flash the first time over USB** (the partition table changes); subsequent updates can go over the browser or `esp32dev_ota`.
- Wi-Fi/MQTT credentials and the password are **not** hard-coded — set them after the first boot.

> The Arduino IDE also works (open `huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino`), but install the **ESP32Async** fork of ESPAsyncWebServer/AsyncTCP — it provides `AsyncAuthenticationMiddleware`.

### First run: Wi-Fi, MQTT, password

1. If no Wi-Fi is stored (or the connection fails within `WIFI_CONNECT_TIMEOUT_S` seconds), the ESP32 starts the access point `Huawei-R4850-Setup` (`AP_SSID`; password `AP_PASSWORD`, empty = open). It is a captive portal; the page is also at `http://192.168.4.1`. **No login password is required in access-point mode.**
2. Pick a network, enter its password and, optionally, the MQTT broker details (host, port, user, password). Empty host = MQTT off, empty user = anonymous, empty password = keep the previous one.
3. After saving, the device reboots and joins your router; the dashboard is at `http://huawei-r4850.local` or at the IP in the Serial log.
4. **Login:** username `admin`, password `admin` by default (`WEB_USER` / `WEB_DEFAULT_PASS`). **Change it** under Settings → Access password (min 6 chars; the device reboots).
5. Change Wi-Fi/MQTT later in the dashboard Settings section. **MQTT** has its own save button — applied immediately, no reboot; **Wi-Fi** is saved with a reboot.
6. **Wi-Fi reset:** hold the on/off button at boot for ≥ `WIFI_RESET_HOLD_MS` (3 s) — the stored network is deleted and the access point starts (MQTT settings are kept).

### Web UI

The dashboard is a set of cards: **Status** (tiles), **Modules** (per-unit/per-phase), **Voltage**, **Current limit**, **Output**, **Fan**, a collapsible **Settings** (Wi-Fi & MQTT, module default values, firmware update, access password) and a separate **Connection status** (MQTT, CAN) at the bottom.

HTTP routes: `/` (dashboard, or the setup page in AP), `/dashboard`, `/wifi` + `/wifi/scan|save|forget`, `/api/mqtt` (save MQTT without reboot), `/api/password`, `/update` (OTA), WebSocket `/ws`. In STA mode everything is behind Basic Auth.

### Configuration (`config.h`)

- `FW_LANG_EN` — firmware message language (`false` = Ukrainian). The dashboard language is toggled with the UK/EN button.
- `HOSTNAME`, `AP_SSID`, `AP_PASSWORD`, `WIFI_CONNECT_TIMEOUT_S`, `WIFI_RESET_HOLD_MS` — network and access point.
- `WEB_USER`, `WEB_DEFAULT_PASS` — web username and initial password (the password is stored in NVS, changed from the dashboard).
- `MQTT_ENABLED` — build-time master switch; `MQTT_HOST`/`MQTT_PORT` are only defaults (saved values take precedence); `MQTT_BASE` — base topic; `HA_DISCOVERY_PREFIX` — HA discovery prefix.
- `CAN_TX_PIN`, `CAN_RX_PIN` — CAN pins.
- `MAX_UNITS` — maximum modules on the bus (addresses 1..N, default 6).
- `RATED_MAX_CURRENT` — module rated current (62.5 A for the R4850G2), used to scale the current register.
- `AC_FAIL_V`, `PSU_OVERTEMP_C`, `ALARM_DETAIL_BIT` — module monitoring thresholds (AC loss, over-temperature, alarm bit that triggers reading `0x018F`).
- `VOLT_MIN/MAX`, `CURR_MIN/MAX` — bounds accepted from web/MQTT.
- `DEFAULT_VOLTAGE`, `DEFAULT_CURRENT`, `DEFAULT_OUTPUT_ON` — startup values when NVS is empty.
- `PSU_DEFAULT_VOLTAGE`, `PSU_DEFAULT_CURRENT` — initial "module default values" (offline registers) for the UI.
- `FAN_MODE_DEFAULT`, `FAN_DUTY_DEFAULT` — fan mode and level at startup.
- `OLED_ENABLED`, `BUTTONS_ENABLED` — enable the OLED and buttons; `OLED_*`, `BTN_*` — pins, `CURR_STEP` — current step for the buttons (default 5 A), `BTN_DEBOUNCE_MS`.
- `WRITE_OFFLINE_DEFAULTS` — also mirror offline values on every online change (wears the module's memory; off by default).

### Control

All control surfaces share the same state; voltage/current/output changes are stored in NVS.

- **Web/WebSocket `/ws`** — JSON `{"cmd": "...", "value": ...}`: `voltage`, `current` (total A), `output`, `fanmode` (0=auto,1=max), `fanduty`, `def_v`, `def_a`, `def_write` (write defaults into the modules).
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
| `def_v`, `def_a` | module default voltage/current |
| `def_write` | write the default values into the modules |

Home Assistant entities appear automatically (device "Huawei R4850G2"):

- sensors: output voltage/current/power, input voltage/power, efficiency, input/output temperatures, units online, fan RPM;
- numbers: voltage, current, fan duty, default voltage, default current;
- switches: output, fan max;
- text sensors: CAN state, last error; binary sensor "PSU alarm";
- for every detected module: input voltage, mains frequency, output current, "AC fail", "Alarm bit2", the `0x018F` contents.

### Web UI preview

`tools/gen_preview.py` extracts the HTML from `index_html.h`/`wifi_html.h` and injects mock data (fake WebSocket/fetch), producing `preview/index.html` and `preview/wifi.html` — openable in a browser without flashing. It runs automatically before every `pio run` (pre-script) or manually: `python tools/gen_preview.py`.

### Warning

You are working with a high-power supply (up to 3 kW per module) and mains voltage. Before connecting a battery, check the voltage and current, the limits in `config.h` and the BMS state. The author is not responsible for damage to equipment or batteries.

### Protocol references

- [patagonaa/huawei-r48xx](https://github.com/patagonaa/huawei-r48xx)
- [craigpeacock/Huawei_R4850G2_CAN](https://github.com/craigpeacock/Huawei_R4850G2_CAN)
