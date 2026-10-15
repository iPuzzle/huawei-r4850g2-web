#pragma once
// ============================================================================
//  config.h — усі налаштування в одному місці
// ============================================================================

// ---- Мова / Language ----
// Мова повідомлень прошивки (Serial-лог, помилки на дашборді та в HA).
// true = English, false = українська. Мову самого дашборду перемикає кнопка UK/EN.
#define FW_LANG_EN   false
#if FW_LANG_EN
  #define TR(uk, en) en
#else
  #define TR(uk, en) uk
#endif

// ---- Wi-Fi ----
// Параметри мережі в прошивці не зашиті: їх задають на сторінці /wifi
// (точка доступу AP_SSID при першому запуску) і зберігають в NVS.
#define HOSTNAME       "huawei-r4850"      // mDNS -> http://huawei-r4850.local

// Точка доступу для налаштування (піднімається, якщо Wi-Fi не задано або не
// вдалось підключитись). Портал: http://192.168.4.1
#define AP_SSID               "Huawei-R4850-Setup"
#define AP_PASSWORD           ""        // "" = відкрита; інакше 8..63 символи
#define WIFI_CONNECT_TIMEOUT_S  10      // скільки чекати підключення при старті
#define WIFI_RESET_HOLD_MS      3000    // утримання кнопки on/off при старті = скинути Wi-Fi й піднятись AP

// ---- MQTT / Home Assistant ----
#define MQTT_ENABLED         true               // головний перемикач на етапі збірки
// Хост/порт — лише значення за замовчуванням. Налаштування зі сторінки /wifi
// (хост, порт, логін, пароль) зберігаються в NVS і мають пріоритет. Порожній хост
// на сторінці = MQTT вимкнено; порожній логін = анонімне підключення.
// Логін/пароль у прошивці не зашиті.
#define MQTT_HOST            "192.168.0.198"
#define MQTT_PORT            1883
#define MQTT_BASE            "huawei_r4850"     // базовий топік + object_id у HA
#define HA_DISCOVERY_PREFIX  "homeassistant"    // стандартний префікс автовиявлення

// ---- Піни CAN (до трансивера SN65HVD230) ----
#define CAN_TX_PIN   GPIO_NUM_5
#define CAN_RX_PIN   GPIO_NUM_4

// ---- Параметри ректифікатора ----
// Номінальний макс. струм для масштабування регістра ліміту струму.
// R4850G2 = 3000 Вт / 48 В ≈ 62.5 А. Змініть, якщо ваш модуль інший.
#define RATED_MAX_CURRENT   62.5f

// ---- Кілька модулів паралельно ----
// Максимум модулів на шині, які прошивка відстежує (адреси 1..MAX_UNITS).
// Команди шлються broadcast -> діють на всі; струм ("Set current" / "Charge
// current") задається СУМАРНИЙ і ділиться порівну між присутніми модулями.
#define MAX_UNITS   6

// ---- Моніторинг стану самих модулів ----
#define AC_FAIL_V        50.0f   // вх. напруга нижче цього (модуль живий по CAN) = втрата AC/фази
#define PSU_OVERTEMP_C   95.0f   // темп. виходу вище цього = перегрів модуля
#define ALARM_DETAIL_BIT 2       // біт у слові аварій 0x83, що вимагає читання 0x018F

// Межі setpoint-ів (захист від помилкового введення з веб/MQTT)
#define VOLT_MIN   41.0f
#define VOLT_MAX   58.0f
#define CURR_MIN    0.0f
#define CURR_MAX   62.5f

// Значення за замовчуванням при старті
#define DEFAULT_VOLTAGE   53.5f
#define DEFAULT_CURRENT   10.0f
#define DEFAULT_OUTPUT_ON true

// Записувати також "offline" (енергонезалежні) значення при кожній зміні?
// false = безпечніше для ресурсу пам'яті; online-значення все одно
// перезастосовуються періодично. true = виживають після повного зникнення живлення.
#define WRITE_OFFLINE_DEFAULTS  false

// ---- Вентилятор ----
// Режим за замовчуванням: 0 = auto, 1 = max. Ручний рівень (%) у режимі auto
// працює як нижня межа обертів (0 = не примусово).
#define FAN_MODE_DEFAULT   0
#define FAN_DUTY_DEFAULT   0.0f      // %, 0 = auto

// ---- Автоматика зарядки LiFePO4 (CV/CC + tapering) ----
#define CHG_ENABLED_ON_BOOT   false
// Приклади нижче — для 16S LiFePO4 (~3.5 В/комірку). Змініть під свій банк!
#define CHG_VOLTAGE          56.0f   // напруга absorption (CV), В
#define CHG_CURRENT          30.0f   // струм bulk (CC), А
#define CHG_TAIL_CURRENT      3.0f   // струм завершення (tail), А  (~0.05C)
#define CHG_ABSORB_HOLD_S     120    // сек нижче tail, перш ніж вважати заряд завершеним
#define CHG_FLOAT_VOLTAGE     0.0f   // 0 = вимкнути вихід після заряду; інакше float, В (напр. 54.0)
#define CHG_RECHARGE_VOLTAGE 51.0f   // у float-режимі: якщо напруга просіла нижче — новий цикл
#define CHG_MAX_TIME_S       28800   // запобіжник: макс. тривалість заряду (8 год)
#define CHG_MAX_TEMP         70.0f   // °C: перевищення темп. виходу -> FAULT (вихід off)

// ---- Опційний OLED-дисплей (SSD1306 128x64, SPI) ----
// Бібліотеки (Library Manager): "Adafruit SSD1306" + "Adafruit GFX Library"
// 7-піновий модуль: GND VCC  D0=SCK  D1=MOSI  RES  DC  CS
#define OLED_ENABLED   true
#define OLED_WIDTH     128
#define OLED_HEIGHT    64
#define OLED_SCK       GPIO_NUM_18   // D0 / CLK  (апаратний VSPI)
#define OLED_MOSI      GPIO_NUM_23   // D1 / DATA (апаратний VSPI)
#define OLED_DC        GPIO_NUM_16   // DC
#define OLED_RST       GPIO_NUM_17   // RES
#define OLED_CS        GPIO_NUM_21   // CS

// ---- Опційні кнопки (active-low: кнопка між GPIO та GND, внутрішній pull-up) ----
#define BUTTONS_ENABLED   true
#define BTN_ONOFF_PIN     GPIO_NUM_25   // вкл/викл виходу
#define BTN_CURR_UP_PIN   GPIO_NUM_26   // струм +
#define BTN_CURR_DN_PIN   GPIO_NUM_27   // струм −
#define CURR_STEP         5.0f          // крок зміни струму (сумарного), А
#define BTN_DEBOUNCE_MS   40
