// ============================================================================
//  Huawei R4850G2  ×  ESP32  —  CAN control + Web UI + Home Assistant (MQTT)
// ----------------------------------------------------------------------------
//  Плата:      будь-яка ESP32 (використовується вбудований TWAI/CAN контролер)
//  Трансивер:  SN65HVD230 (3.3 В).  TX->D(GPIO5), RX<-R(GPIO4), VCC=3V3, GND
//  CAN:        125 kbps, 29-бітні розширені ID
//
//  Бібліотеки (Arduino IDE / PlatformIO):
//    - ESPAsyncWebServer   (me-no-dev)
//    - AsyncTCP            (me-no-dev)
//    - PubSubClient        (knolleary)
//    - ArduinoJson         (bblanchon)
//  driver/twai.h — вже в ESP32 core, окремо ставити не треба.
//
//  Протокол перевірено за документацією:
//    github.com/patagonaa/huawei-r48xx  та  github.com/craigpeacock/Huawei_R4850G2_CAN
// ============================================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "driver/twai.h"
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "index_html.h"
#include "wifi_html.h"

// ---- Протокол R48xx --------------------------------------------------------
// Команди шлемо broadcast (addr 0) -> діють на всі модулі одночасно.
// Відповіді читаємо per-unit за полем адреси в CAN ID (біти 22..16).
static const uint32_t ID_REQUEST = 0x108040FE;   // data-запит до всіх
static const uint32_t ID_SET     = 0x108080FE;   // запис регістра на всі
// (per-unit було б 0x1081..FE для addr1, 0x1082..FE для addr2, тощо)
static inline uint8_t idProto(uint32_t id){ return (id >> 23) & 0x3F; }  // =0x21
static inline uint8_t idAddr (uint32_t id){ return (id >> 16) & 0x7F; }  // 1..N
static inline uint8_t idCmd  (uint32_t id){ return (id >> 8)  & 0xFF; }  // 0x40/0x82

// Регістри статусу (data[1]); значення = int32 з data[4..7], big-endian
enum {
  R_INPUT_POWER = 0x70, R_INPUT_FREQ = 0x71, R_INPUT_CURR = 0x72,
  R_OUTPUT_POWER= 0x73, R_EFFICIENCY = 0x74, R_OUTPUT_VOLT= 0x75,
  R_MAX_CURR    = 0x76, R_INPUT_VOLT = 0x78, R_TEMP_OUT   = 0x7F,
  R_TEMP_IN     = 0x80, R_OUT_CURR1  = 0x81, R_OUT_CURR2  = 0x82,
  R_ALARM       = 0x83
};
// Регістри запису (data[0]=0x01, data[1]=reg)
static const uint8_t SET_VOLT_ONLINE  = 0x00;  // V*1024, скидається при втраті CAN
static const uint8_t SET_VOLT_OFFLINE = 0x01;  // V*1024, енергонезалежний дефолт
static const uint8_t SET_CURR_ONLINE  = 0x03;  // (A/max)*1250
static const uint8_t SET_CURR_OFFLINE = 0x04;
static const uint8_t SET_STANDBY      = 0x32;  // data[3]: 0=увімк, 1=standby
static const uint8_t SET_FAN_DUTY     = 0x14;  // data[2..3] BE = (%/100)*25600
static const uint8_t SET_FAN_MODE     = 0x34;  // data[3]: 0=auto, 1=max, 2=max(persist)

// Читання регістрів через "get" (команда 0x82) — теж broadcast
static const uint32_t ID_GET_REQ   = 0x108082FE;
static const uint8_t  R_FAN_STATUS = 0x87;      // 0x0187: min duty / duty target / RPM
static const uint8_t  R_DETAIL     = 0x8F;      // 0x018F: детальна аварійна інфо (коли bit2 у 0x83)

// ---- Стан ------------------------------------------------------------------
// Дані одного модуля
struct Unit {
  float outV=NAN, outA=NAN, outW=NAN, inV=NAN, inFreq=NAN, inW=NAN, eff=NAN;
  float tOut=NAN, tIn=NAN, maxA=NAN;
  float fanRpm=NAN, fanDutyTarget=NAN, fanMinDuty=NAN;
  uint32_t alarm=0;                 // сире слово аварій/статусу (рег. 0x83)
  bool acFail=false, overTemp=false, alarmBit2=false;
  char reg8F[16]="";                // сирі байти 0x018F (hex), коли bit2 активний
  unsigned long lastRx=0;
};
Unit u[MAX_UNITS + 1];        // індекс 1..MAX_UNITS (0 не використовується)

// Агрегат по всіх присутніх модулях (перераховується щоцикл)
struct Agg {
  int   count=0;
  float outV=NAN, outA=0, outW=0, inV=NAN, inW=0, eff=NAN;
  float tOut=NAN, tIn=NAN, maxA=0, fanRpm=NAN, fanDuty=NAN, fanMinDuty=NAN;
} ag;

// Тип кнопки з методом-обробником. pressed() — саме метод (а не вільна
// функція pressed(Button&)), щоб автопрототипи Arduino IDE не спотикались об тип.
struct Button {
  uint8_t pin; bool last; unsigned long tChange;
  bool pressed() {
    bool raw = (digitalRead(pin) == LOW);            // натиснуто = LOW
    unsigned long now = millis();
    if (raw != last && (now - tChange) > BTN_DEBOUNCE_MS) {
      tChange = now; last = raw;
      return raw;                                     // true лише в момент натискання
    }
    return false;
  }
};

float spVoltage = DEFAULT_VOLTAGE;
float spCurrent = DEFAULT_CURRENT;
bool  spOutput  = DEFAULT_OUTPUT_ON;

float   fanDutySp = FAN_DUTY_DEFAULT;
uint8_t fanMode   = FAN_MODE_DEFAULT;   // 0=auto, 1=max

// --- автоматика зарядки ---
enum ChargeState { CHG_OFF, CHG_BULK, CHG_ABSORB, CHG_DONE, CHG_FLOAT, CHG_FAULT };
ChargeState chgState = CHG_OFF;
bool  chgEnabled = CHG_ENABLED_ON_BOOT;
float chgVoltage = CHG_VOLTAGE, chgCurrent = CHG_CURRENT, chgTail = CHG_TAIL_CURRENT;
float chgFloat = CHG_FLOAT_VOLTAGE, chgRecharge = CHG_RECHARGE_VOLTAGE;
uint32_t chgAbsorbHold = CHG_ABSORB_HOLD_S, chgMaxTime = CHG_MAX_TIME_S;
float chgMaxTemp = CHG_MAX_TEMP;
unsigned long chgStart = 0, tailStart = 0;

const char* chargeStateName() {
  switch (chgState) {
    case CHG_BULK:   return "bulk";
    case CHG_ABSORB: return "absorb";
    case CHG_DONE:   return "done";
    case CHG_FLOAT:  return "float";
    case CHG_FAULT:  return "fault";
    default:         return "off";
  }
}

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiClient wifiClient;
bool apMode = false;               // true = піднята точка доступу для налаштування Wi-Fi
DNSServer dnsServer;               // captive portal (усі імена -> IP точки доступу)
String mqttHost = MQTT_HOST, mqttUser, mqttPass;   // налаштування MQTT (NVS, задаються на /wifi)
uint16_t mqttPort = MQTT_PORT;
inline bool mqttOn() { return MQTT_ENABLED && mqttHost.length() > 0; }
unsigned long restartAt = 0;       // відкладене перезавантаження після збереження Wi-Fi
PubSubClient mqtt(wifiClient);
uint32_t mqttReconnects = 0, mqttPubCount = 0, mqttCmdCount = 0;

// ---- Діагностика / вивід помилок -------------------------------------------
const char* canStateStr = "?";
uint32_t canTxFail = 0, canBusOff = 0;
int canTxErrCnt = 0, canRxErrCnt = 0;
char lastErr[48] = "";
unsigned long lastErrMs = 0;

void logError(const char* msg) {
  strncpy(lastErr, msg, sizeof(lastErr) - 1);
  lastErr[sizeof(lastErr) - 1] = 0;
  lastErrMs = millis();
  Serial.printf("[ERR] %s\n", msg);
}

// ---- CAN -------------------------------------------------------------------
void twaiInit() {
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t  t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t  f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) == ESP_OK && twai_start() == ESP_OK)
    Serial.println(TR("[CAN] запущено 125 kbps", "[CAN] started at 125 kbps"));
  else
    logError(TR("CAN: помилка ініціалізації", "CAN: init failed"));
}

bool canSend(uint32_t id, const uint8_t d[8]) {
  twai_message_t m = {};
  m.identifier = id; m.extd = 1; m.data_length_code = 8;
  memcpy(m.data, d, 8);
  if (twai_transmit(&m, pdMS_TO_TICKS(50)) == ESP_OK) return true;
  canTxFail++;
  return false;
}

void canRequestData() {
  uint8_t d[8] = {0};
  canSend(ID_REQUEST, d);
}

// Запис регістра, де значення лежить у data[4..7] (big-endian)
void setValueReg(uint8_t reg, uint32_t value) {
  uint8_t d[8] = { 0x01, reg, 0x00, 0x00,
                   (uint8_t)(value >> 24), (uint8_t)(value >> 16),
                   (uint8_t)(value >> 8),  (uint8_t)(value) };
  canSend(ID_SET, d);
}

void applyVoltage(float v) {
  v = constrain(v, VOLT_MIN, VOLT_MAX);
  uint32_t raw = (uint32_t)lroundf(v * 1024.0f);
  setValueReg(SET_VOLT_ONLINE, raw);
  if (WRITE_OFFLINE_DEFAULTS) setValueReg(SET_VOLT_OFFLINE, raw);
  Serial.printf(TR("[SET] U = %.1f В (raw 0x%04X)\n", "[SET] U = %.1f V (raw 0x%04X)\n"), v, raw);
}

void applyCurrent(float totalA) {
  totalA = constrain(totalA, CURR_MIN, CURR_MAX * MAX_UNITS);
  int n = max(unitsOnline(), 1);
  float perUnit = constrain(totalA / n, 0.0f, CURR_MAX);   // ліміт кожного модуля
  uint32_t raw = (uint32_t)lroundf(perUnit / RATED_MAX_CURRENT * 1250.0f);
  setValueReg(SET_CURR_ONLINE, raw);
  if (WRITE_OFFLINE_DEFAULTS) setValueReg(SET_CURR_OFFLINE, raw);
  Serial.printf(TR("[SET] I = %.1f А сум / %d = %.1f А/модуль (raw %u)\n", "[SET] I = %.1f A total / %d = %.1f A/module (raw %u)\n"), totalA, n, perUnit, raw);
}

void applyOutput(bool on) {
  // standby: data[3] = 0 (увімк) / 1 (standby)
  uint8_t d[8] = { 0x01, SET_STANDBY, 0x00, (uint8_t)(on ? 0 : 1), 0, 0, 0, 0 };
  canSend(ID_SET, d);
  Serial.printf(TR("[SET] вихід = %s\n", "[SET] output = %s\n"), on ? TR("УВІМК", "ON") : TR("ВИМК", "OFF"));
}

void applyFanDuty(float pct) {
  pct = constrain(pct, 0.0f, 100.0f);
  // нижче min duty PSU повертає помилку і скидає в 0 — підтягуємо до min
  if (pct > 0 && !isnan(ag.fanMinDuty) && pct < ag.fanMinDuty) pct = ag.fanMinDuty;
  uint16_t raw = (uint16_t)lroundf(pct / 100.0f * 25600.0f);
  uint8_t d[8] = { 0x01, SET_FAN_DUTY, (uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF), 0, 0, 0, 0 };
  canSend(ID_SET, d);
}

void applyFanMode(uint8_t mode) {
  uint8_t d[8] = { 0x01, SET_FAN_MODE, 0x00, mode, 0, 0, 0, 0 };
  canSend(ID_SET, d);
}

void applyFan() {              // режим + рівень (як нижня межа)
  applyFanMode(fanMode);
  if (fanMode == 0) applyFanDuty(fanDutySp);
}

void canRequestFanStatus() {   // get 0x0187
  uint8_t d[8] = { 0x01, R_FAN_STATUS, 0, 0, 0, 0, 0, 0 };
  canSend(ID_GET_REQ, d);
}

void canRequestDetail() {      // get 0x018F (детальна аварійна інфо)
  uint8_t d[8] = { 0x01, R_DETAIL, 0, 0, 0, 0, 0, 0 };
  canSend(ID_GET_REQ, d);
}

void resendSetpoints() {   // тримаємо online-значення "живими"
  applyVoltage(spVoltage);
  applyCurrent(spCurrent);
  applyOutput(spOutput);
  applyFan();
}

void canPoll() {
  twai_message_t m;
  while (twai_receive(&m, 0) == ESP_OK) {
    if (!m.extd || m.data_length_code != 8) continue;
    if (idProto(m.identifier) != 0x21) continue;
    uint8_t cmd = idCmd(m.identifier);
    if (cmd != 0x40 && cmd != 0x82) continue;         // лише data/get відповіді
    uint8_t addr = idAddr(m.identifier);
    if (addr < 1 || addr > MAX_UNITS || m.data[0] != 0x01) continue;
    Unit &un = u[addr];
    un.lastRx = millis();
    if (m.data[1] == R_FAN_STATUS) {   // 0x0187: min / target / RPM (пари 16-біт BE)
      un.fanMinDuty    = ((m.data[2] << 8) | m.data[3]) / 25600.0f * 100.0f;
      un.fanDutyTarget = ((m.data[4] << 8) | m.data[5]) / 25600.0f * 100.0f;
      un.fanRpm        =  (m.data[6] << 8) | m.data[7];
      continue;
    }
    if (m.data[1] == R_DETAIL) {        // 0x018F: layout невідомий — зберігаємо сирі байти 2..7
      snprintf(un.reg8F, sizeof(un.reg8F), "%02X%02X%02X%02X%02X%02X",
               m.data[2], m.data[3], m.data[4], m.data[5], m.data[6], m.data[7]);
      continue;
    }
    int32_t v = ((int32_t)m.data[4] << 24) | ((int32_t)m.data[5] << 16) |
                ((int32_t)m.data[6] << 8)  |  (int32_t)m.data[7];
    switch (m.data[1]) {
      case R_INPUT_POWER:  un.inW  = v / 1024.0f; break;
      case R_OUTPUT_POWER: un.outW = v / 1024.0f; break;
      case R_EFFICIENCY:   un.eff  = v / 1024.0f * 100.0f; break;
      case R_OUTPUT_VOLT:  un.outV = v / 1024.0f; break;
      case R_MAX_CURR:     un.maxA = v / 1250.0f * RATED_MAX_CURRENT; break;
      case R_INPUT_FREQ:   un.inFreq = v / 1024.0f; break;
      case R_INPUT_VOLT:   un.inV  = v / 1024.0f; break;
      case R_TEMP_OUT:     un.tOut = v / 1024.0f; break;
      case R_TEMP_IN:      un.tIn  = v / 1024.0f; break;
      case R_OUT_CURR2:    un.outA = v / 1024.0f; break;  // відфільтроване значення
      case R_ALARM:        un.alarm = (uint32_t)v; break; // сире слово аварій/статусу
    }
  }
}

bool unitPresent(int i) { return u[i].lastRx != 0 && (millis() - u[i].lastRx) < 4000; }
int  unitsOnline() { int n = 0; for (int i = 1; i <= MAX_UNITS; i++) if (unitPresent(i)) n++; return n; }
bool anyDetailNeeded() { for (int i = 1; i <= MAX_UNITS; i++) if (unitPresent(i) && u[i].alarmBit2) return true; return false; }
bool canAlive() { return unitsOnline() > 0; }

// Перерахувати агрегат по всіх присутніх модулях
void computeAgg() {
  Agg a;
  float vsum = 0, insum = 0, esum = 0; int vn = 0, inn = 0;
  for (int i = 1; i <= MAX_UNITS; i++) {
    if (!unitPresent(i)) continue;
    a.count++;
    if (!isnan(u[i].outA)) a.outA += u[i].outA;
    if (!isnan(u[i].outW)) a.outW += u[i].outW;
    if (!isnan(u[i].inW))  a.inW  += u[i].inW;
    if (!isnan(u[i].maxA)) a.maxA += u[i].maxA;
    if (!isnan(u[i].outV)) { vsum  += u[i].outV; vn++; }
    if (!isnan(u[i].inV))  { insum += u[i].inV; inn++; }
    if (!isnan(u[i].eff))    esum += u[i].eff;
    if (!isnan(u[i].tOut)) a.tOut = isnan(a.tOut) ? u[i].tOut : max(a.tOut, u[i].tOut);
    if (!isnan(u[i].tIn))  a.tIn  = isnan(a.tIn)  ? u[i].tIn  : max(a.tIn,  u[i].tIn);
    if (!isnan(u[i].fanRpm))      a.fanRpm      = isnan(a.fanRpm)      ? u[i].fanRpm      : max(a.fanRpm, u[i].fanRpm);
    if (!isnan(u[i].fanDutyTarget)) a.fanDuty   = isnan(a.fanDuty)     ? u[i].fanDutyTarget : max(a.fanDuty, u[i].fanDutyTarget);
    if (!isnan(u[i].fanMinDuty)) a.fanMinDuty  = isnan(a.fanMinDuty)  ? u[i].fanMinDuty  : max(a.fanMinDuty, u[i].fanMinDuty);
  }
  if (vn)  a.outV = vsum / vn;
  if (inn) a.inV  = insum / inn;
  if (a.count) a.eff = esum / a.count;
  ag = a;
}

// Стан самих модулів: втрата AC (живий по CAN, але вх. напруга ~0) і перегрів
void psuCheck() {
  for (int i = 1; i <= MAX_UNITS; i++) {
    if (!unitPresent(i)) { u[i].acFail = false; u[i].overTemp = false; continue; }
    bool ac = !isnan(u[i].inV)  && u[i].inV  < AC_FAIL_V;
    bool ot = !isnan(u[i].tOut) && u[i].tOut > PSU_OVERTEMP_C;
    bool b2 = (u[i].alarm >> ALARM_DETAIL_BIT) & 1u;
    char m[48];
    if (ac && !u[i].acFail)  { snprintf(m, sizeof(m), TR("Модуль %d: втрата AC/фази", "Module %d: AC/phase loss"), i); logError(m); }
    if (ot && !u[i].overTemp){ snprintf(m, sizeof(m), TR("Модуль %d: перегрів виходу", "Module %d: output overtemperature"), i); logError(m); }
    if (b2 && !u[i].alarmBit2){ snprintf(m, sizeof(m), TR("Модуль %d: аварія bit2 - читаю 0x018F", "Module %d: alarm bit2 - reading 0x018F"), i); logError(m); }
    if (!b2) u[i].reg8F[0] = 0;                 // біт знявся - скидаємо старе значення
    u[i].acFail = ac; u[i].overTemp = ot; u[i].alarmBit2 = b2;
  }
}

// Діагностика CAN/WiFi/модулів + авто-відновлення шини після bus-off
void diagTick() {
  twai_status_info_t s;
  if (twai_get_status_info(&s) == ESP_OK) {
    canTxErrCnt = s.tx_error_counter;
    canRxErrCnt = s.rx_error_counter;
    static twai_state_t prev = TWAI_STATE_RUNNING;
    switch (s.state) {
      case TWAI_STATE_RUNNING:    canStateStr = "running"; break;
      case TWAI_STATE_RECOVERING: canStateStr = "recovering"; break;
      case TWAI_STATE_STOPPED:
        canStateStr = "stopped";
        if (twai_start() == ESP_OK && prev == TWAI_STATE_RECOVERING) logError(TR("CAN відновлено", "CAN recovered"));
        break;
      case TWAI_STATE_BUS_OFF:
        canStateStr = "bus-off";
        if (prev != TWAI_STATE_BUS_OFF) { canBusOff++; logError(TR("CAN bus-off - CAN_H/L / термінатор?", "CAN bus-off - check CAN_H/L / termination")); }
        twai_initiate_recovery();
        break;
    }
    prev = s.state;
  }
  // зникнення відповідей від модулів
  static bool wasAlive = false;
  bool alive = canAlive();
  if (!alive && wasAlive) logError(TR("Немає відповіді від модулів", "No response from modules"));
  wasAlive = alive;
  // WiFi
  static bool wifiWas = true;
  bool wifiUp = (WiFi.status() == WL_CONNECTED);
  if (!wifiUp && wifiWas) logError(TR("WiFi відключено", "WiFi disconnected"));
  wifiWas = wifiUp;

  psuCheck();
}

// Оновити setpoint-и й застосувати лише змінене (щоб не спамити шину)
void setSp(float v, float a, bool out) {
  if (fabsf(v - spVoltage) > 0.05f) { spVoltage = v; applyVoltage(v); }
  if (fabsf(a - spCurrent) > 0.05f) { spCurrent = a; applyCurrent(a); }
  if (out != spOutput)              { spOutput  = out; applyOutput(out); }
}

// Стан-машина зарядки LiFePO4:
//   BULK (CC) -> ABSORB (CV, струм спадає сам) -> завершення за tail-струмом
//   -> вимкнення виходу або FLOAT. Викликати періодично.
void chargeTick() {
  if (!chgEnabled) { chgState = CHG_OFF; return; }
  unsigned long now = millis();

  // запобіжник по температурі
  if (canAlive() && !isnan(ag.tOut) && ag.tOut > chgMaxTemp) {
    if (chgState != CHG_FAULT) logError(TR("Зарядка: перегрів виходу", "Charge: output overtemperature"));
    chgState = CHG_FAULT; setSp(spVoltage, spCurrent, false); return;
  }

  switch (chgState) {
    case CHG_OFF:                       // щойно увімкнули автозарядку
      chgStart = now; tailStart = 0;
      setSp(chgVoltage, chgCurrent, true);
      chgState = CHG_BULK;
      break;

    case CHG_BULK:                      // модулі у CC, тримають струм, напруга росте
      setSp(chgVoltage, chgCurrent, true);
      if (!isnan(ag.outV) && ag.outV >= chgVoltage - 0.15f) { chgState = CHG_ABSORB; tailStart = 0; }
      if (now - chgStart > chgMaxTime * 1000UL) { logError(TR("Зарядка: таймаут", "Charge: timeout")); chgState = CHG_FAULT; }
      break;

    case CHG_ABSORB:                    // модулі у CV, сумарний струм спадає; чекаємо tail
      setSp(chgVoltage, chgCurrent, true);
      if (!isnan(ag.outA) && ag.outA <= chgTail) {
        if (tailStart == 0) tailStart = now;
        else if (now - tailStart >= chgAbsorbHold * 1000UL) chgState = CHG_DONE;
      } else tailStart = 0;
      if (now - chgStart > chgMaxTime * 1000UL) { logError(TR("Зарядка: таймаут", "Charge: timeout")); chgState = CHG_FAULT; }
      break;

    case CHG_DONE:
      if (chgFloat > 0) chgState = CHG_FLOAT;
      else setSp(spVoltage, spCurrent, false);      // вихід off, лишаємось у DONE
      break;

    case CHG_FLOAT:
      setSp(chgFloat, chgCurrent, true);
      if (!isnan(ag.outV) && ag.outV < chgRecharge) { chgStart = now; tailStart = 0; chgState = CHG_BULK; }
      break;

    case CHG_FAULT:                      // латч: скидається лише перезапуском зарядки
      setSp(spVoltage, spCurrent, false);
      break;
  }
}

void startCharge(bool on) {
  chgEnabled = on;
  chgState = CHG_OFF;   // гілка OFF сама переведе в BULK, якщо on
  if (on) chargeTick();
}

// ---- JSON статусу ----------------------------------------------------------
void buildStatusJson(char* buf, size_t n) {
  JsonDocument doc;
  doc["outV"]=ag.outV; doc["outA"]=ag.outA; doc["outW"]=ag.outW;
  doc["inV"]=ag.inV;   doc["inW"]=ag.inW;   doc["eff"]=ag.eff;
  doc["tOut"]=ag.tOut; doc["tIn"]=ag.tIn;   doc["maxA"]=ag.maxA;
  doc["units"]=ag.count;
  doc["spV"]=spVoltage; doc["spA"]=spCurrent;
  doc["output"]=spOutput; doc["can"]=canAlive();
  doc["fanRpm"]=ag.fanRpm; doc["fanDuty"]=ag.fanDuty; doc["fanSp"]=fanDutySp; doc["fanMode"]=fanMode;
  doc["chg"]=chgEnabled; doc["chgState"]=chargeStateName();
  doc["chgV"]=chgVoltage; doc["chgA"]=chgCurrent; doc["chgTail"]=chgTail;
  JsonArray arr = doc["u"].to<JsonArray>();
  for (int i = 1; i <= MAX_UNITS; i++) {
    if (!unitPresent(i)) continue;
    JsonObject o = arr.add<JsonObject>();
    o["i"]=i; o["v"]=u[i].outV; o["a"]=u[i].outA; o["t"]=u[i].tOut; o["rpm"]=u[i].fanRpm;
    o["vin"]=u[i].inV; o["hz"]=u[i].inFreq;
    o["acf"]=u[i].acFail; o["ot"]=u[i].overTemp; o["alm"]=u[i].alarm;
    o["b2"]=u[i].alarmBit2;
    if (u[i].reg8F[0]) o["r8f"]=u[i].reg8F;
  }
  bool psuAlarm = false;
  for (int i = 1; i <= MAX_UNITS; i++)
    if (unitPresent(i) && (u[i].acFail || u[i].overTemp)) psuAlarm = true;
  doc["psuAlarm"] = psuAlarm;
  JsonObject mq = doc["mqtt"].to<JsonObject>();
  mq["on"]=mqttOn(); mq["conn"]=mqtt.connected();
  mq["st"]=mqtt.state();
  mq["rc"]=mqttReconnects; mq["pub"]=mqttPubCount; mq["cmd"]=mqttCmdCount;
  JsonObject dg = doc["diag"].to<JsonObject>();
  dg["can"]=canStateStr; dg["txe"]=canTxErrCnt; dg["rxe"]=canRxErrCnt;
  dg["txf"]=canTxFail; dg["boff"]=canBusOff;
  if (lastErr[0]) { dg["err"]=lastErr; dg["errAge"]=(millis()-lastErrMs)/1000UL; }
  serializeJson(doc, buf, n);
}

// ---- WebSocket -------------------------------------------------------------
void handleCommand(const char* json) {
  JsonDocument d;
  if (deserializeJson(d, json)) return;
  const char* cmd = d["cmd"];
  if (!cmd) return;
  float val = d["value"] | 0.0f;
  if (!strcmp(cmd, "voltage"))      { chgEnabled=false; spVoltage = constrain(val, VOLT_MIN, VOLT_MAX); applyVoltage(spVoltage); }
  else if (!strcmp(cmd, "current")) { chgEnabled=false; spCurrent = constrain(val, CURR_MIN, CURR_MAX); applyCurrent(spCurrent); }
  else if (!strcmp(cmd, "output"))  { chgEnabled=false; spOutput  = (val != 0); applyOutput(spOutput); }
  else if (!strcmp(cmd, "fanmode")) { fanMode = (uint8_t)val; applyFan(); }
  else if (!strcmp(cmd, "fanduty")) { fanMode = 0; fanDutySp = constrain(val, 0.0f, 100.0f); applyFan(); }
  else if (!strcmp(cmd, "charge"))  { startCharge(val != 0); }
  else if (!strcmp(cmd, "chg_v"))   { chgVoltage = constrain(val, VOLT_MIN, VOLT_MAX); }
  else if (!strcmp(cmd, "chg_a"))   { chgCurrent = constrain(val, CURR_MIN, CURR_MAX); }
  else if (!strcmp(cmd, "chg_tail")){ chgTail    = constrain(val, 0.0f, CURR_MAX); }
  mqttPublishState();
}

void onWs(AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type, void*, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) { data[len] = 0; handleCommand((char*)data); }
}

void pushStatus() {
  if (ws.count() == 0) return;
  char buf[1536];
  buildStatusJson(buf, sizeof(buf));
  ws.textAll(buf);
}

// ---- MQTT / Home Assistant Discovery --------------------------------------
String baseTopic  = String(MQTT_BASE);
String stateTopic = baseTopic + "/state";
bool discPublished[MAX_UNITS + 1] = { false };   // чи опубліковано per-unit сенсори

void haSensor(const char* id, const char* name, const char* unit,
              const char* devclass, const char* jsonKey) {
  JsonDocument c;
  c["name"] = name;
  c["uniq_id"] = baseTopic + "_" + id;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = String("{{ value_json.") + jsonKey + " }}";
  if (unit && *unit)       c["unit_of_meas"] = unit;
  if (devclass && *devclass) c["dev_cla"] = devclass;
  c["stat_cla"] = "measurement";
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  dev["mf"] = "Huawei"; dev["mdl"] = "R4850G2";
  char topic[128], payload[512];
  snprintf(topic, sizeof(topic), "%s/sensor/%s/%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, id);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

void haNumber(const char* id, const char* name, const char* unit,
              const char* jsonKey, const char* cmdSuffix, float mn, float mx, float step) {
  JsonDocument c;
  c["name"] = name;
  c["uniq_id"] = baseTopic + "_" + id;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = String("{{ value_json.") + jsonKey + " }}";
  c["cmd_t"] = baseTopic + "/set/" + cmdSuffix;
  c["min"] = mn; c["max"] = mx; c["step"] = step;
  if (unit && *unit) c["unit_of_meas"] = unit;
  c["mode"] = "box";
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[128], payload[512];
  snprintf(topic, sizeof(topic), "%s/number/%s/%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, id);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

void haSwitch() {
  JsonDocument c;
  c["name"] = "Output";
  c["uniq_id"] = baseTopic + "_output";
  c["stat_t"] = stateTopic;
  c["val_tpl"] = "{{ 'ON' if value_json.output else 'OFF' }}";
  c["cmd_t"] = baseTopic + "/set/output";
  c["pl_on"] = "ON"; c["pl_off"] = "OFF";
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[128], payload[512];
  snprintf(topic, sizeof(topic), "%s/switch/%s/output/config", HA_DISCOVERY_PREFIX, MQTT_BASE);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

void haSwitchG(const char* id, const char* name, const char* valTpl, const char* cmdSuffix) {
  JsonDocument c;
  c["name"] = name;
  c["uniq_id"] = baseTopic + "_" + id;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = valTpl;
  c["cmd_t"] = baseTopic + "/set/" + cmdSuffix;
  c["pl_on"] = "ON"; c["pl_off"] = "OFF";
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[128], payload[512];
  snprintf(topic, sizeof(topic), "%s/switch/%s/%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, id);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

void haTextSensor(const char* id, const char* name, const char* jsonKey) {
  JsonDocument c;
  c["name"] = name;
  c["uniq_id"] = baseTopic + "_" + id;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = String("{{ value_json.") + jsonKey + " }}";
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[128], payload[512];
  snprintf(topic, sizeof(topic), "%s/sensor/%s/%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, id);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

// Сенсор одного модуля; значення тягнеться з масиву value_json.u за полем i
void haUnitSensor(int i, const char* key, const char* name, const char* unit,
                  const char* devclass, const char* attr) {
  JsonDocument c;
  char nm[48], uid[48], tpl[176];
  snprintf(nm,  sizeof(nm),  "Module %d %s", i, name);
  snprintf(uid, sizeof(uid), "%s_u%d_%s", MQTT_BASE, i, key);
  snprintf(tpl, sizeof(tpl),
    "{%% set d = value_json.u | selectattr('i','equalto',%d) | list %%}{{ d[0].%s if d else none }}",
    i, attr);
  c["name"] = nm;
  c["uniq_id"] = uid;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = tpl;
  if (unit && *unit) c["unit_of_meas"] = unit;
  if (devclass && *devclass) c["dev_cla"] = devclass;
  c["stat_cla"] = "measurement";
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[160], payload[512];
  snprintf(topic, sizeof(topic), "%s/sensor/%s/u%d_%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, i, key);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

// Глобальний binary_sensor (ON/OFF за шаблоном)
void haBinary(const char* id, const char* name, const char* devclass, const char* valTpl) {
  JsonDocument c;
  c["name"] = name;
  c["uniq_id"] = baseTopic + "_" + id;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = valTpl;
  c["pl_on"] = "ON"; c["pl_off"] = "OFF";
  if (devclass && *devclass) c["dev_cla"] = devclass;
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[128], payload[512];
  snprintf(topic, sizeof(topic), "%s/binary_sensor/%s/%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, id);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

// Per-unit binary_sensor: ON коли d[0].<attr> істинне
void haUnitBinary(int i, const char* key, const char* name, const char* devclass, const char* attr) {
  JsonDocument c;
  char nm[48], uid[48], tpl[176];
  snprintf(nm,  sizeof(nm),  "Module %d %s", i, name);
  snprintf(uid, sizeof(uid), "%s_u%d_%s", MQTT_BASE, i, key);
  snprintf(tpl, sizeof(tpl),
    "{%% set d = value_json.u | selectattr('i','equalto',%d) | list %%}{{ 'ON' if d and d[0].%s else 'OFF' }}",
    i, attr);
  c["name"] = nm;
  c["uniq_id"] = uid;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = tpl;
  c["pl_on"] = "ON"; c["pl_off"] = "OFF";
  if (devclass && *devclass) c["dev_cla"] = devclass;
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[160], payload[512];
  snprintf(topic, sizeof(topic), "%s/binary_sensor/%s/u%d_%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, i, key);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

// Per-unit text_sensor
void haUnitText(int i, const char* key, const char* name, const char* attr) {
  JsonDocument c;
  char nm[48], uid[48], tpl[176];
  snprintf(nm,  sizeof(nm),  "Module %d %s", i, name);
  snprintf(uid, sizeof(uid), "%s_u%d_%s", MQTT_BASE, i, key);
  snprintf(tpl, sizeof(tpl),
    "{%% set d = value_json.u | selectattr('i','equalto',%d) | list %%}{{ d[0].%s if d else none }}",
    i, attr);
  c["name"] = nm;
  c["uniq_id"] = uid;
  c["stat_t"] = stateTopic;
  c["val_tpl"] = tpl;
  JsonObject dev = c["dev"].to<JsonObject>();
  dev["ids"] = baseTopic; dev["name"] = "Huawei R4850G2";
  char topic[160], payload[512];
  snprintf(topic, sizeof(topic), "%s/sensor/%s/u%d_%s/config", HA_DISCOVERY_PREFIX, MQTT_BASE, i, key);
  serializeJson(c, payload, sizeof(payload));
  mqtt.publish(topic, payload, true);
}

void publishUnitDiscovery(int i) {
  haUnitSensor(i, "vin",  "input voltage",   "V",  "voltage",   "vin");
  haUnitSensor(i, "hz",   "input frequency", "Hz", "frequency", "hz");
  haUnitSensor(i, "iout", "output current",  "A",  "current",   "a");
  haUnitBinary(i, "acfail", "AC fail", "problem", "acf");
  haUnitBinary(i, "alarmbit2", "Alarm bit2", "problem", "b2");
  haUnitText(i, "reg8f", "0x018F", "r8f");
  discPublished[i] = true;
}

void mqttPublishDiscovery() {
  haSensor("out_v", "Output voltage", "V", "voltage", "outV");
  haSensor("out_a", "Output current", "A", "current", "outA");
  haSensor("out_w", "Output power",   "W", "power",   "outW");
  haSensor("in_v",  "Input voltage",  "V", "voltage", "inV");
  haSensor("in_w",  "Input power",     "W", "power",   "inW");
  haSensor("eff",   "Efficiency",      "%", "power_factor", "eff");
  haSensor("t_out", "Output temperature", "°C", "temperature", "tOut");
  haSensor("t_in",  "Input temperature",  "°C", "temperature", "tIn");
  haSensor("units", "Units online",         "",  "",            "units");
  haNumber("set_v", "Set voltage", "V", "spV", "voltage", VOLT_MIN, VOLT_MAX, 0.1);
  haNumber("set_a", "Set current", "A", "spA", "current", CURR_MIN, CURR_MAX, 0.5);
  haSwitch();
  // вентилятор
  haSensor("fan_rpm", "Fan RPM", "rpm", "", "fanRpm");
  haNumber("fan_duty", "Fan duty", "%", "fanSp", "fanduty", 0, 100, 5);
  haSwitchG("fan_max", "Fan max", "{{ 'ON' if value_json.fanMode == 1 else 'OFF' }}", "fanmax");
  // автозарядка
  haSwitchG("charge", "Auto charge", "{{ 'ON' if value_json.chg else 'OFF' }}", "charge");
  haTextSensor("chg_state", "Charge state", "chgState");
  haTextSensor("can_state", "CAN state", "diag.can");
  haTextSensor("last_error", "Last error", "diag.err");
  haBinary("psu_alarm", "PSU alarm", "problem", "{{ 'ON' if value_json.psuAlarm else 'OFF' }}");
  haNumber("chg_v", "Charge voltage", "V", "chgV", "chg_v", VOLT_MIN, VOLT_MAX, 0.1);
  haNumber("chg_a", "Charge current", "A", "chgA", "chg_a", CURR_MIN, CURR_MAX, 0.5);
  haNumber("chg_tail", "Tail current", "A", "chgTail", "chg_tail", 0, CURR_MAX, 0.5);
}

void mqttPublishState() {
  if (!mqtt.connected()) return;
  char buf[1536];
  buildStatusJson(buf, sizeof(buf));
  mqtt.publish(stateTopic.c_str(), buf, true);
  mqttPubCount++;
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String t(topic); String msg; msg.reserve(len);
  for (unsigned i = 0; i < len; i++) msg += (char)payload[i];
  mqttCmdCount++;
  if (t.endsWith("/set/voltage")) { chgEnabled=false; spVoltage = constrain(msg.toFloat(), VOLT_MIN, VOLT_MAX); applyVoltage(spVoltage); }
  else if (t.endsWith("/set/current")) { chgEnabled=false; spCurrent = constrain(msg.toFloat(), CURR_MIN, CURR_MAX); applyCurrent(spCurrent); }
  else if (t.endsWith("/set/output"))  { chgEnabled=false; spOutput  = (msg == "ON" || msg == "1"); applyOutput(spOutput); }
  else if (t.endsWith("/set/fanmax"))  { fanMode = (msg == "ON" || msg == "1") ? 1 : 0; applyFan(); }
  else if (t.endsWith("/set/fanduty")) { fanMode = 0; fanDutySp = constrain(msg.toFloat(), 0.0f, 100.0f); applyFan(); }
  else if (t.endsWith("/set/charge"))  { startCharge(msg == "ON" || msg == "1"); }
  else if (t.endsWith("/set/chg_v"))   { chgVoltage = constrain(msg.toFloat(), VOLT_MIN, VOLT_MAX); }
  else if (t.endsWith("/set/chg_a"))   { chgCurrent = constrain(msg.toFloat(), CURR_MIN, CURR_MAX); }
  else if (t.endsWith("/set/chg_tail")){ chgTail    = constrain(msg.toFloat(), 0.0f, CURR_MAX); }
  mqttPublishState();
}

const char* mqttErr(int s) {
  switch (s) {
    case -4: return "timeout";
    case -3: return TR("з'єднання втрачено", "connection lost");
    case -2: return TR("TCP не вдався (IP/порт/фаєрвол)", "TCP failed (IP/port/firewall)");
    case -1: return TR("відключено", "disconnected");
    case  1: return TR("поганий протокол", "bad protocol");
    case  2: return TR("поганий client id", "bad client id");
    case  3: return TR("брокер недоступний", "broker unavailable");
    case  4: return TR("невірні логін/пароль", "bad username/password");
    case  5: return TR("не авторизовано (ACL)", "not authorized (ACL)");
    default: return "?";
  }
}

void mqttReconnect() {
  static unsigned long last = 0;
  if (WiFi.status() != WL_CONNECTED) return;            // без мережі не лізем конектитись
  if (mqtt.connected() || millis() - last < 5000) return;
  last = millis();
  Serial.printf(TR("[MQTT] конект до %s:%d ...\n", "[MQTT] connecting to %s:%d ...\n"), mqttHost.c_str(), mqttPort);
  bool ok = (mqttUser.length() == 0)                    // порожній логін = анонімно
              ? mqtt.connect(HOSTNAME)
              : mqtt.connect(HOSTNAME, mqttUser.c_str(), mqttPass.c_str());
  if (ok) {
    mqttReconnects++;
    Serial.println(TR("[MQTT] під'єднано", "[MQTT] connected"));
    mqtt.subscribe((baseTopic + "/set/#").c_str());
    mqttPublishDiscovery();
    for (int i = 1; i <= MAX_UNITS; i++) discPublished[i] = false;  // перевидати per-unit
    mqttPublishState();
  } else {
    Serial.printf(TR("[MQTT] помилка: state=%d (%s)\n", "[MQTT] error: state=%d (%s)\n"), mqtt.state(), mqttErr(mqtt.state()));
  }
}

// ---- Опційний OLED + кнопки ------------------------------------------------
#if OLED_ENABLED
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);
bool oledOk = false;

void oledInit() {
  SPI.begin(OLED_SCK, -1, OLED_MOSI);           // MISO не потрібен для дисплея
  oledOk = oled.begin(SSD1306_SWITCHCAPVCC);    // для SPI адреса не потрібна
  if (oledOk) { oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE); oled.display(); }
  Serial.printf("[OLED] %s\n", oledOk ? TR("ок", "ok") : TR("не знайдено", "not found"));
}

void oledDraw() {
  if (!oledOk) return;
  oled.clearDisplay();
  // шапка: кількість модулів, стан виходу, наявність CAN
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("R4850 x%d  %s", ag.count, spOutput ? "ON" : "OFF");
  const char* tag = canAlive() ? "CAN"
                  : (strcmp(canStateStr, "bus-off") == 0)    ? "BOFF"
                  : (strcmp(canStateStr, "recovering") == 0) ? "REC"
                  : (strcmp(canStateStr, "running") == 0)    ? "n/u"
                  : "---";
  oled.setCursor(104, 0);
  oled.print(tag);
  // великі значення напруги та струму (сумарні)
  oled.setTextSize(2);
  oled.setCursor(0, 16);
  if (isnan(ag.outV)) oled.print("--.- V"); else oled.printf("%.1f V", ag.outV);
  oled.setCursor(0, 36);
  if (isnan(ag.outA)) oled.print("--.- A"); else oled.printf("%.1f A", ag.outA);
  // підвал: setpoint + стан зарядки
  // підвал: аварія модуля (латиницею) або setpoint + стан зарядки
  oled.setTextSize(1);
  oled.setCursor(0, 56);
  int fu = 0; const char* ft = nullptr;
  for (int i = 1; i <= MAX_UNITS; i++) {
    if (unitPresent(i) && u[i].acFail)   { fu = i; ft = "AC-FAIL";  break; }
    if (unitPresent(i) && u[i].overTemp) { fu = i; ft = "OVERTEMP"; break; }
  }
  if (ft)          oled.printf("! U%d %s", fu, ft);
  else if (apMode) oled.print("WiFi AP 192.168.4.1");
  else    oled.printf("set %.0f/%.0f  %s", spVoltage, spCurrent, chargeStateName());
  oled.display();
}
#else
void oledInit() {}
void oledDraw() {}
#endif

#if BUTTONS_ENABLED
Button btnOn{ BTN_ONOFF_PIN, false, 0 }, btnUp{ BTN_CURR_UP_PIN, false, 0 }, btnDn{ BTN_CURR_DN_PIN, false, 0 };

void buttonsInit() {
  pinMode(BTN_ONOFF_PIN,   INPUT_PULLUP);
  pinMode(BTN_CURR_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_CURR_DN_PIN, INPUT_PULLUP);
}

void buttonsPoll() {
  if (btnOn.pressed()) {                              // toggle виходу (виходить з автозарядки)
    chgEnabled = false; spOutput = !spOutput; applyOutput(spOutput); mqttPublishState();
  }
  if (btnUp.pressed()) {                              // струм + (сумарний)
    chgEnabled = false;
    spCurrent = constrain(spCurrent + CURR_STEP, CURR_MIN, CURR_MAX * MAX_UNITS);
    applyCurrent(spCurrent); mqttPublishState();
  }
  if (btnDn.pressed()) {                              // струм −
    chgEnabled = false;
    spCurrent = constrain(spCurrent - CURR_STEP, CURR_MIN, CURR_MAX * MAX_UNITS);
    applyCurrent(spCurrent); mqttPublishState();
  }
}
#else
void buttonsInit() {}
void buttonsPoll() {}
#endif

// ---- Wi-Fi: збережені параметри, підключення, точка доступу ----------------
// Параметри беруться лише з NVS (задано через /wifi); немає -> точка доступу.
bool wifiStored() {
  Preferences p; p.begin("wifi", true);
  bool has = p.getString("ssid", "").length() > 0;
  p.end();
  return has;
}

bool wifiLoad(String& ssid, String& pass) {
  Preferences p; p.begin("wifi", true);
  ssid = p.getString("ssid", ""); pass = p.getString("pass", "");
  p.end();
  return ssid.length() > 0;
}

void wifiSave(const String& ssid, const String& pass) {
  Preferences p; p.begin("wifi", false);
  p.putString("ssid", ssid); p.putString("pass", pass);
  p.end();
}

void wifiForget() {                               // MQTT-логін у NVS лишається
  Preferences p; p.begin("wifi", false);
  p.remove("ssid"); p.remove("pass");
  p.end();
}

void mqttConfigLoad() {                           // ключі NVS: mh=хост, mt=порт, mu=логін, mp=пароль
  Preferences p; p.begin("wifi", true);
  if (p.isKey("mh")) mqttHost = p.getString("mh", "");   // збережений порожній хост = MQTT вимкнено
  mqttPort = p.getUShort("mt", MQTT_PORT);
  mqttUser = p.getString("mu", ""); mqttPass = p.getString("mp", "");
  p.end();
}

bool wifiConnect(const String& ssid, const String& pass) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print(TR("[WiFi] підключення", "[WiFi] connecting"));
  for (int i = 0; i < WIFI_CONNECT_TIMEOUT_S * 4 && WiFi.status() != WL_CONNECTED; i++) { delay(250); Serial.print("."); }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void wifiOnConnected() {
  Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
  if (MDNS.begin(HOSTNAME)) Serial.printf("[mDNS] http://%s.local\n", HOSTNAME);
}

void wifiStartAp() {
  WiFi.disconnect(false);                         // перестати шукати збережену мережу
  WiFi.mode(WIFI_AP_STA);                         // STA лишається для сканування мереж
  bool pw = strlen(AP_PASSWORD) >= 8;
  WiFi.softAP(AP_SSID, pw ? AP_PASSWORD : nullptr);
  dnsServer.start(53, "*", WiFi.softAPIP());
  apMode = true;
  Serial.printf(TR("[WiFi] точка доступу '%s' -> http://%s\n", "[WiFi] access point '%s' -> http://%s\n"),
                AP_SSID, WiFi.softAPIP().toString().c_str());
}

void wifiStopAp() {
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apMode = false;
}

// Кнопка on/off, утримувана при старті, скидає збережений Wi-Fi.
bool wifiResetHeld() {
#if BUTTONS_ENABLED
  delay(20);
  unsigned long t0 = millis();
  while (digitalRead(BTN_ONOFF_PIN) == LOW) {
    if (millis() - t0 >= WIFI_RESET_HOLD_MS) return true;
    delay(10);
  }
#endif
  return false;
}

// Викликати з loop(): відкладений рестарт, DNS порталу і періодична повторна
// спроба підключення до роутера (лише поки до точки ніхто не підключений).
void wifiTick() {
  unsigned long now = millis();
  if (restartAt && now >= restartAt) ESP.restart();
  if (!apMode) return;
  dnsServer.processNextRequest();

  static unsigned long tTry = 0, tStart = 0;
  static bool trying = false;
  if (trying) {
    if (WiFi.status() == WL_CONNECTED) {
      trying = false; wifiStopAp(); wifiOnConnected();
    } else if (now - tStart > WIFI_CONNECT_TIMEOUT_S * 1000UL) {
      trying = false; WiFi.disconnect(false); tTry = now;
    }
    return;
  }
  String s, p;
  if (now - tTry >= 60000 && WiFi.softAPgetStationNum() == 0 && wifiLoad(s, p)) {
    tTry = now; tStart = now; trying = true;
    WiFi.begin(s.c_str(), p.c_str());
  }
}

void wifiRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send_P(200, "text/html", apMode ? WIFI_HTML : INDEX_HTML);
  });
  server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest* r) { r->send_P(200, "text/html", INDEX_HTML); });
  server.on("/wifi",      HTTP_GET, [](AsyncWebServerRequest* r) { r->send_P(200, "text/html", WIFI_HTML); });

  server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* r) {
    JsonDocument d;
    d["ap"] = apMode;
    d["stored"] = wifiStored();
    d["mh"] = mqttHost; d["mt"] = mqttPort; d["mu"] = mqttUser;
    d["cur"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String("");
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) {
      d["scanning"] = true;
    } else if (n < 0) {                           // ще не сканували / помилка -> запустити
      WiFi.scanNetworks(true);
      d["scanning"] = true;
    } else {
      d["scanning"] = false;
      JsonArray a = d["nets"].to<JsonArray>();
      for (int i = 0; i < n && a.size() < 20; i++) {
        String s = WiFi.SSID(i);
        if (s.length() == 0) continue;
        bool dup = false;
        for (JsonObject o : a) if (s == o["ssid"].as<const char*>()) { dup = true; break; }
        if (dup) continue;
        JsonObject o = a.add<JsonObject>();
        o["ssid"] = s; o["rssi"] = WiFi.RSSI(i); o["enc"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
      }
      WiFi.scanDelete();
    }
    String out; serializeJson(d, out);
    r->send(200, "application/json", out);
  });

  server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest* r) {
    String ssid = r->hasParam("ssid", true) ? r->getParam("ssid", true)->value() : "";
    String pass = r->hasParam("pass", true) ? r->getParam("pass", true)->value() : "";
    if (ssid.length() < 1 || ssid.length() > 32) { r->send(400, "application/json", "{\"ok\":false,\"err\":\"ssid\"}"); return; }
    if (pass.length() != 0 && (pass.length() < 8 || pass.length() > 63)) { r->send(400, "application/json", "{\"ok\":false,\"err\":\"pass\"}"); return; }
    bool hasMqtt = r->hasParam("mh", true);
    String mh = hasMqtt ? r->getParam("mh", true)->value() : "";
    String mu = r->hasParam("mu", true) ? r->getParam("mu", true)->value() : "";
    String mp = r->hasParam("mp", true) ? r->getParam("mp", true)->value() : "";
    long mt = r->hasParam("mt", true) ? r->getParam("mt", true)->value().toInt() : MQTT_PORT;
    mh.trim();
    if (mh.length() > 64 || mu.length() > 64 || mp.length() > 64) { r->send(400, "application/json", "{\"ok\":false,\"err\":\"mqtt\"}"); return; }
    if (mh.length() > 0 && (mt < 1 || mt > 65535)) { r->send(400, "application/json", "{\"ok\":false,\"err\":\"port\"}"); return; }
    wifiSave(ssid, pass);
    if (hasMqtt) {                                  // порожній логін = анонімно; порожній пароль = лишити старий
      Preferences p; p.begin("wifi", false);
      p.putString("mh", mh);
      if (mh.length() > 0) p.putUShort("mt", (uint16_t)mt);
      if (mu.length() == 0) { p.remove("mu"); p.remove("mp"); }
      else { p.putString("mu", mu); if (mp.length()) p.putString("mp", mp); }
      p.end();
    }
    Serial.printf(TR("[WiFi] збережено '%s', перезавантаження\n", "[WiFi] saved '%s', rebooting\n"), ssid.c_str());
    r->send(200, "application/json", "{\"ok\":true}");
    restartAt = millis() + 1500;
  });

  server.on("/wifi/forget", HTTP_POST, [](AsyncWebServerRequest* r) {
    wifiForget();
    Serial.println(TR("[WiFi] збережені параметри видалено, перезавантаження", "[WiFi] saved credentials cleared, rebooting"));
    r->send(200, "application/json", "{\"ok\":true}");
    restartAt = millis() + 1500;
  });

  // captive portal: будь-який невідомий запит у режимі AP -> сторінка налаштування
  server.onNotFound([](AsyncWebServerRequest* r) {
    if (apMode) r->redirect("http://" + WiFi.softAPIP().toString() + "/");
    else r->send(404, "text/plain", "Not found");
  });
}

// ---- setup / loop ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  oledInit();
  buttonsInit();

  twaiInit();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  if (wifiResetHeld()) {                              // кнопка утримана -> скинути Wi-Fi
    wifiForget();
    Serial.println(TR("[WiFi] скинуто кнопкою", "[WiFi] reset by button"));
    wifiStartAp();
  } else {
    String ssid, pass;
    if (wifiLoad(ssid, pass) && wifiConnect(ssid, pass)) wifiOnConnected();
    else wifiStartAp();                               // немає параметрів або не підключились
  }

  ws.onEvent(onWs);
  server.addHandler(&ws);
  wifiRoutes();
  server.begin();

  mqttConfigLoad();
  if (mqttOn()) {
    mqtt.setServer(mqttHost.c_str(), mqttPort);
    mqtt.setBufferSize(2048);
    mqtt.setSocketTimeout(2);            // не зависати надовго на недоступному брокері
    mqtt.setCallback(mqttCallback);
  }

  delay(500);
  resendSetpoints();   // початкові значення
}

void loop() {
  canPoll();
  computeAgg();

  // При зміні кількості онлайн-модулів перерахувати ліміт струму на модуль
  static int prevUnits = -1;
  if (ag.count != prevUnits) { prevUnits = ag.count; if (ag.count > 0) applyCurrent(spCurrent); }

  // Публікуємо per-unit (фазні) сенсори для щойно виявлених модулів
  if (mqttOn() && mqtt.connected())
    for (int i = 1; i <= MAX_UNITS; i++)
      if (unitPresent(i) && !discPublished[i]) publishUnitDiscovery(i);

  buttonsPoll();
  wifiTick();

  unsigned long now = millis();
  static unsigned long tReq = 0, tPush = 0, tResend = 0, tChg = 0, tOled = 0, tDiag = 0;

  if (now - tReq    >= 2000) { tReq    = now; canRequestData(); canRequestFanStatus(); if (anyDetailNeeded()) canRequestDetail(); }
  if (now - tPush   >= 1500) { tPush   = now; pushStatus(); mqttPublishState(); }
  if (now - tResend >= 10000){ tResend = now; resendSetpoints(); }
  if (now - tChg    >= 2000) { tChg    = now; chargeTick(); }
  if (now - tOled   >= 250)  { tOled   = now; oledDraw(); }
  if (now - tDiag   >= 1000) { tDiag   = now; diagTick(); }

  if (mqttOn()) { mqttReconnect(); mqtt.loop(); }
  ws.cleanupClients();
}
