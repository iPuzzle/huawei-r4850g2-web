# Huawei R4850G2 × ESP32

[Українська](#українська) · [English](#english)

---

## Українська

Прошивка для ESP32, що керує випрямлячами Huawei R4850G2 (до `MAX_UNITS` модулів паралельно) через CAN. Містить веб-панель керування, інтеграцію з Home Assistant через MQTT (автовиявлення), автоматику зарядки LiFePO4 та опційні OLED-дисплей і кнопки.

### Можливості

- Керування напругою, струмом і станом виходу (вкл/викл) усіх модулів одночасно (broadcast по CAN).
- Струм задається **сумарний** і ділиться порівну між модулями, що наразі онлайн.
- Керування вентилятором (авто / максимум / мінімальний рівень).
- Моніторинг по кожному модулю: вхідна/вихідна напруга, струм, потужність, ККД, температури, оберти вентилятора, частота мережі, втрата AC, перегрів, аварійні біти.
- Автозарядка LiFePO4: BULK (CC) → ABSORB (CV) → завершення за «хвостовим» струмом → вимкнення виходу або FLOAT; захист за температурою та за часом.
- Веб-інтерфейс на `http://<hostname>.local` (оновлення через WebSocket `/ws`).
- MQTT + Home Assistant Discovery: сенсори, числа, перемикачі, бінарні сенсори аварій.
- Опційно: OLED SSD1306 128×64 (SPI) і три кнопки (вкл/викл, струм +/−).
- Діагностика CAN (bus-off, лічильники помилок) з автовідновленням шини.

### Апаратна частина

| Компонент | Підключення |
|---|---|
| CAN-трансивер SN65HVD230 (3.3 В) | TX → GPIO5, RX ← GPIO4, VCC = 3V3, GND |
| CAN-шина | 125 кбіт/с, 29-бітні ID; CANH/CANL до роз'єму випрямляча, не забудьте термінатор |
| OLED SSD1306 (SPI), за бажанням | SCK → GPIO18, MOSI → GPIO23, DC → GPIO16, RES → GPIO17, CS → GPIO21 |
| Кнопки (між GPIO і GND), за бажанням | вкл/викл → GPIO25, струм + → GPIO26, струм − → GPIO27 |

Усі піни змінюються в `src/huawei_r4850g2_esp32/config.h`.

### Збірка та прошивка

1. Встановіть у Arduino IDE (або PlatformIO) підтримку плат ESP32.
2. Встановіть бібліотеки: **ESPAsyncWebServer** і **AsyncTCP** (me-no-dev), **PubSubClient**, **ArduinoJson** (v7); для OLED додатково **Adafruit SSD1306** і **Adafruit GFX Library**.
3. Відкрийте `src/huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino`.
4. У `config.h` вкажіть свої Wi-Fi та MQTT дані (SSID, пароль, адресу брокера, логін/пароль), піни та ліміти.
5. Виберіть свою плату ESP32, зберіть і прошийте. Монітор порту: 115200 бод.

### Налаштування (`config.h`)

- `MAX_UNITS` — максимум модулів на шині (адреси 1..N).
- `RATED_MAX_CURRENT` — номінальний струм модуля (62.5 А для R4850G2), потрібен для масштабування ліміту струму.
- `VOLT_MIN/MAX`, `CURR_MIN/MAX` — межі, які приймаються з веб/MQTT.
- `CHG_*` — параметри автозарядки (напруга, струм, tail, float, таймаут, макс. температура). Значення за замовчуванням — приклад для 16S LiFePO4, **змініть під свою батарею**.
- `OLED_ENABLED`, `BUTTONS_ENABLED`, `MQTT_ENABLED` — вмикання окремих функцій.
- `WRITE_OFFLINE_DEFAULTS` — чи записувати енергонезалежні значення (зношує пам'ять модуля, за замовчуванням вимкнено).

### MQTT

Базовий топік — `MQTT_BASE` (за замовчуванням `huawei_r4850`). Стан публікується (retained) у `<base>/state`, команди приймаються в `<base>/set/<name>`: `voltage`, `current`, `output`, `fanmax`, `fanduty`, `charge`, `chg_v`, `chg_a`, `chg_tail`. Сутності в Home Assistant з'являються автоматично.

### Увага

Ви працюєте з потужним джерелом живлення (до 3 кВт на модуль) та мережевою напругою. Перед підключенням до батареї перевірте напругу й струм заряду, межі в `config.h` і параметри автозарядки. Автор не несе відповідальності за пошкодження обладнання чи батарей.

### Джерела протоколу

- [patagonaa/huawei-r48xx](https://github.com/patagonaa/huawei-r48xx)
- [craigpeacock/Huawei_R4850G2_CAN](https://github.com/craigpeacock/Huawei_R4850G2_CAN)

---

## English

ESP32 firmware that controls Huawei R4850G2 rectifiers (up to `MAX_UNITS` modules in parallel) over CAN. It provides a web control panel, Home Assistant integration over MQTT (auto-discovery), an LiFePO4 charge controller, and an optional OLED display and buttons.

### Features

- Set voltage, current and output state (on/off) on all modules at once (CAN broadcast).
- Current is set as a **total** and split evenly among the modules currently online.
- Fan control (auto / max / minimum duty).
- Per-module monitoring: input/output voltage, current, power, efficiency, temperatures, fan RPM, mains frequency, AC loss, over-temperature, alarm bits.
- LiFePO4 auto-charge: BULK (CC) → ABSORB (CV) → finish on tail current → output off or FLOAT; temperature and time safety cut-offs.
- Web UI at `http://<hostname>.local` (live updates over WebSocket `/ws`).
- MQTT + Home Assistant Discovery: sensors, numbers, switches, and alarm binary sensors.
- Optional: SSD1306 128×64 SPI OLED and three buttons (on/off, current +/−).
- CAN diagnostics (bus-off, error counters) with automatic bus recovery.

### Hardware

| Component | Connection |
|---|---|
| SN65HVD230 CAN transceiver (3.3 V) | TX → GPIO5, RX ← GPIO4, VCC = 3V3, GND |
| CAN bus | 125 kbit/s, 29-bit IDs; CANH/CANL to the rectifier connector, remember bus termination |
| SSD1306 OLED (SPI), optional | SCK → GPIO18, MOSI → GPIO23, DC → GPIO16, RES → GPIO17, CS → GPIO21 |
| Buttons (GPIO to GND), optional | on/off → GPIO25, current + → GPIO26, current − → GPIO27 |

All pins can be changed in `src/huawei_r4850g2_esp32/config.h`.

### Build and flash

1. Install ESP32 board support in the Arduino IDE (or PlatformIO).
2. Install the libraries: **ESPAsyncWebServer** and **AsyncTCP** (me-no-dev), **PubSubClient**, **ArduinoJson** (v7); for the OLED also **Adafruit SSD1306** and **Adafruit GFX Library**.
3. Open `src/huawei_r4850g2_esp32/huawei_r4850g2_esp32.ino`.
4. In `config.h`, set your own Wi-Fi and MQTT details (SSID, password, broker address, user/password), pins and limits.
5. Select your ESP32 board, build and upload. Serial monitor: 115200 baud.

### Configuration (`config.h`)

- `MAX_UNITS` — maximum number of modules on the bus (addresses 1..N).
- `RATED_MAX_CURRENT` — module rated current (62.5 A for the R4850G2), used to scale the current limit register.
- `VOLT_MIN/MAX`, `CURR_MIN/MAX` — bounds accepted from web/MQTT.
- `CHG_*` — auto-charge parameters (voltage, current, tail, float, timeout, max temperature). Defaults are an example for 16S LiFePO4; **adjust them for your battery**.
- `OLED_ENABLED`, `BUTTONS_ENABLED`, `MQTT_ENABLED` — feature toggles.
- `WRITE_OFFLINE_DEFAULTS` — also write the non-volatile values (wears the module's memory; off by default).

### MQTT

The base topic is `MQTT_BASE` (default `huawei_r4850`). State is published (retained) to `<base>/state`; commands are accepted on `<base>/set/<name>`: `voltage`, `current`, `output`, `fanmax`, `fanduty`, `charge`, `chg_v`, `chg_a`, `chg_tail`. Home Assistant entities appear automatically.

### Warning

You are working with a high-power supply (up to 3 kW per module) and mains voltage. Before connecting a battery, check the charge voltage and current, the limits in `config.h` and the auto-charge parameters. The author is not responsible for damage to equipment or batteries.

### Protocol references

- [patagonaa/huawei-r48xx](https://github.com/patagonaa/huawei-r48xx)
- [craigpeacock/Huawei_R4850G2_CAN](https://github.com/craigpeacock/Huawei_R4850G2_CAN)
