#pragma once
#include <pgmspace.h>

// Веб-дашборд (UK/EN, кнопка в шапці). Оновлення через WebSocket (ws://<host>/ws).
// Дизайн узгоджено зі стилем jkbms-web (токени кольорів, картки, tiles, pill).
const char INDEX_HTML[] PROGMEM = R"HTMLDOC(
<!DOCTYPE html><html lang="uk"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Huawei R4850G2</title>
<style>
:root{--bg:#0f1216;--card:#171b21;--line:#262c35;--txt:#e6e9ee;--mut:#8a93a1;--acc:#4ea1ff;--ok:#38c172;--warn:#f0b429;--bad:#ef5350}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--txt);font:15px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
header{display:flex;align-items:center;flex-wrap:wrap;gap:8px 12px;padding:12px 16px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--bg);z-index:5}
header h1{font-size:17px;margin:0;font-weight:600}header .sp{flex:1}
main{max-width:980px;margin:0 auto;padding:16px;display:grid;gap:14px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px}
.card h2{font-size:13px;text-transform:uppercase;letter-spacing:.06em;color:var(--mut);margin:0 0 12px;font-weight:600;display:flex;align-items:center;gap:8px;flex-wrap:wrap}
.pill{font-size:12px;padding:3px 10px;border-radius:99px;background:var(--line);color:var(--mut);white-space:nowrap;display:inline-flex;align-items:center;gap:6px;text-decoration:none}
.pill.ok{background:#173b27;color:var(--ok)}.pill.bad{background:#3e1c1c;color:var(--bad)}
.dot{width:8px;height:8px;border-radius:50%;background:var(--bad)}.pill.ok .dot{background:var(--ok)}
button,select,input{font:inherit;color:var(--txt);background:#20262e;border:1px solid var(--line);border-radius:7px;padding:7px 12px}
button{cursor:pointer}button:hover{border-color:#3a4350}button.pri{background:var(--acc);border-color:var(--acc);color:#04121f;font-weight:600}
button:disabled{opacity:.45;cursor:default}
input{min-width:0}.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-bottom:10px}.row:last-child{margin-bottom:0}
.mut{color:var(--mut)}.sm{font-size:13px}
.tiles{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
.tile{background:#11151a;border:1px solid var(--line);border-radius:8px;padding:10px 12px}
.tile .l{font-size:12px;color:var(--mut)}.tile .v{font-size:24px;font-weight:600;font-variant-numeric:tabular-nums}
.tile .v small{font-size:13px;color:var(--mut);font-weight:400;margin-left:3px}
input[type=range]{flex:1;accent-color:var(--acc);padding:0;background:none}
input[type=number]{width:94px;font-variant-numeric:tabular-nums}
.seg{display:flex;gap:8px;margin-bottom:10px}.seg button{flex:1}
.seg button.act{background:var(--acc);border-color:var(--acc);color:#04121f;font-weight:600}
.toggle{width:100%;padding:13px;font-size:15px;font-weight:600;border-radius:8px}
.toggle.on{background:var(--ok);border-color:var(--ok);color:#04121f}.toggle.off{background:#20262e;color:var(--mut)}
.foot{font-size:12px;color:var(--mut)}
.lang{padding:4px 10px;font-size:12px}
.bar{height:6px;background:#0b0e12;border-radius:3px;overflow:hidden;margin-top:10px}
.bar i{display:block;height:100%;width:0;background:var(--acc);transition:width .2s}
.u{display:block;margin-top:4px}
button.flash{box-shadow:0 0 0 3px rgba(78,161,255,.5)}
button{transition:transform .08s,box-shadow .18s}button:active{transform:scale(.97)}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%) translateY(16px);background:var(--card);border:1px solid var(--line);color:var(--txt);padding:9px 15px;border-radius:9px;font-size:13px;opacity:0;transition:opacity .2s,transform .2s;pointer-events:none;z-index:50}
#toast.show{opacity:1;transform:translateX(-50%) translateY(0)}#toast.ok{border-color:var(--ok)}#toast.bad{border-color:var(--bad)}
details.card summary{cursor:pointer;list-style:none;font-size:13px;text-transform:uppercase;letter-spacing:.06em;color:var(--mut);font-weight:600;display:flex;align-items:center;gap:8px}
details.card summary::-webkit-details-marker{display:none}
details.card summary::before{content:'▸';font-size:11px}
details.card[open] summary::before{content:'▾'}
details.card[open] summary{margin-bottom:6px}
.subh{font-size:12px;text-transform:uppercase;letter-spacing:.06em;color:var(--mut);font-weight:600;margin:16px 0 10px;padding-top:14px;border-top:1px solid var(--line)}
.subh.first{margin-top:10px;padding-top:0;border-top:none}
.dan{background:transparent;border-color:#5a2a2a;color:#ff8a80}
.net{display:flex;justify-content:space-between;align-items:center;padding:8px 4px;border-bottom:1px solid var(--line);cursor:pointer;font-size:14px}
.net:last-child{border-bottom:none}.net:hover{color:var(--acc)}
.trace{font:12px/1.5 ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;max-height:320px;overflow:auto;background:#0b0e12;border:1px solid var(--line);border-radius:8px;padding:8px 10px;white-space:pre;margin-top:10px}
.trace .rx{color:var(--acc)}.trace .tx{color:var(--warn)}.trace .neg{color:var(--ok)}
.trace .em{color:var(--mut)}
@media(max-width:640px){.tiles{grid-template-columns:repeat(2,1fr)}}
</style></head><body>
<header>
  <h1>Huawei R4850G2</h1>
  <span class="sp"></span>
  <span id="conn" class="pill"><span class="dot"></span><span id="connTxt">офлайн</span></span>
  <button class="lang" id="langBtn" onclick="toggleLang()">EN</button>
</header>
<main>

<div class="card">
  <h2 data-i="h_status">Статус</h2>
  <div class="tiles">
    <div class="tile"><div class="l" data-i="l_outV">Вихідна напруга</div><div class="v"><span id="outV">–</span><small data-i="u_V">В</small></div></div>
    <div class="tile"><div class="l" data-i="l_outA">Вихідний струм</div><div class="v"><span id="outA">–</span><small data-i="u_A">А</small></div></div>
    <div class="tile"><div class="l" data-i="l_outW">Вихідна потужність</div><div class="v"><span id="outW">–</span><small data-i="u_W">Вт</small></div></div>
    <div class="tile"><div class="l" data-i="l_inV">Вхідна напруга</div><div class="v"><span id="inV">–</span><small data-i="u_V">В</small></div></div>
    <div class="tile"><div class="l" data-i="l_inW">Вхідна потужність</div><div class="v"><span id="inW">–</span><small data-i="u_W">Вт</small></div></div>
    <div class="tile"><div class="l" data-i="l_eff">ККД</div><div class="v"><span id="eff">–</span><small>%</small></div></div>
    <div class="tile"><div class="l" data-i="l_tOut">Темп. виходу</div><div class="v"><span id="tOut">–</span><small>°C</small></div></div>
    <div class="tile"><div class="l" data-i="l_tIn">Темп. входу</div><div class="v"><span id="tIn">–</span><small>°C</small></div></div>
    <div class="tile"><div class="l" data-i="l_units">Модулів онлайн</div><div class="v"><span id="units">–</span></div></div>
  </div>
  <div class="foot" style="margin-top:12px">Setpoint: <span id="spV">–</span> <span data-i="u_V">В</span> / <span id="spA">–</span> <span data-i="u_A">А</span> · <span data-i="reapply">перезастосовується автоматично</span></div>
</div>

<div id="uSec" class="card" style="display:none">
  <h2 data-i="h_units">Модулі</h2>
  <div id="uList" class="sm mut"></div>
</div>

<div class="card">
  <h2 data-i="h_volt">Напруга</h2>
  <div class="row">
    <input type="range" id="vR" min="41" max="58" step="0.1" value="53.5" oninput="vN.value=this.value">
    <input type="number" id="vN" min="41" max="58" step="0.1" value="53.5" oninput="vR.value=this.value">
    <button class="pri" data-i="set" onclick="send('voltage',+vN.value)">Задати</button>
  </div>
  <h2 style="margin:16px 0 12px" data-i="h_curr">Ліміт струму (сумарний, на всі модулі)</h2>
  <div class="row">
    <input type="range" id="cR" min="0" max="62.5" step="0.5" value="10" oninput="cN.value=this.value">
    <input type="number" id="cN" min="0" max="62.5" step="0.5" value="10" oninput="cR.value=this.value">
    <button class="pri" data-i="set" onclick="send('current',+cN.value)">Задати</button>
  </div>
</div>

<div class="card">
  <h2 data-i="h_out">Вихід</h2>
  <button id="outBtn" class="toggle off" onclick="send('output', outState?0:1)">–</button>
</div>

<div class="card">
  <h2><span data-i="h_fan">Вентилятор</span> · <span id="fanRpm" class="mut sm">–</span></h2>
  <div class="seg">
    <button id="fA" data-i="f_auto" onclick="send('fanmode',0)">Авто</button>
    <button id="fM" data-i="f_man" onclick="fanManual()">Ручний</button>
    <button id="fX" data-i="f_max" onclick="send('fanmode',1)">Макс</button>
  </div>
  <div class="row">
    <input type="range" id="fR" min="0" max="100" step="5" value="0" oninput="fV.textContent=this.value+'%'">
    <span id="fV" class="mut sm" style="width:44px">0%</span>
    <button class="pri" data-i="set" onclick="fanManual()">Задати</button>
  </div>
  <div class="foot"><span data-i="f_cur">Поточний рівень</span>: <span id="fanDuty">–</span>%</div>
</div>

<details class="card" id="setCard">
  <summary data-i="h_set">Налаштування</summary>

  <div class="subh first" data-i="sub_wifi">Wi-Fi та MQTT</div>
  <div id="wcur" class="foot" style="margin-bottom:10px"></div>
  <div class="row">
    <button id="wscanBtn" data-i="w_scan" onclick="wscan()">Знайти мережі</button>
    <span id="wscanMsg" class="foot"></span>
  </div>
  <div id="wlist" class="sm" style="margin-bottom:10px"></div>
  <div class="row"><input id="ssid" maxlength="32" autocomplete="off" autocapitalize="off" data-ph="ph_ssid" style="flex:1 1 100%"></div>
  <div class="row"><input id="wpass" type="password" maxlength="63" autocomplete="off" data-ph="ph_wpass" style="flex:1 1 100%"></div>
  <div class="row"><input id="mh" maxlength="64" autocomplete="off" autocapitalize="off" data-ph="ph_mh" style="flex:1"><input id="mt" type="number" min="1" max="65535" value="1883" style="width:96px"></div>
  <div class="row"><input id="mu" maxlength="64" autocomplete="off" autocapitalize="off" data-ph="ph_mu" style="flex:1 1 100%"></div>
  <div class="row"><input id="mp" type="password" maxlength="64" autocomplete="off" data-ph="ph_mp" style="flex:1 1 100%"></div>
  <button style="width:100%" data-i="m_save" onclick="mqttSave()">Зберегти MQTT</button>
  <div class="foot" data-i="m_note" style="margin:6px 0 14px">Застосовується одразу, без перезавантаження.</div>
  <button class="pri" style="width:100%" data-i="w_save" onclick="wsave()">Зберегти Wi-Fi і перезавантажити</button>
  <button class="dan" id="wforgetBtn" style="width:100%;margin-top:8px;display:none" data-i="w_forget" onclick="wforget()">Забути мережу</button>
  <div id="wmsg" class="foot" style="margin-top:8px"></div>

  <div class="subh" data-i="h_def">Дефолтні значення модулів</div>
  <div class="foot" data-i="def_hint" style="margin-bottom:12px">Записуються в памʼять модулів; діють при старті без контролера.</div>
  <div class="row">
    <label class="mut sm" style="width:90px" data-i="d_v">Напруга</label>
    <input type="number" id="dV" min="48" max="58" step="0.1" value="53.5"><span class="mut sm" data-i="u_V">В</span>
  </div>
  <div class="row">
    <label class="mut sm" style="width:90px" data-i="d_a">Струм</label>
    <input type="number" id="dA" min="0" max="62.5" step="0.5" value="62.5"><span class="mut sm" data-i="d_au">А/модуль</span>
  </div>
  <button class="pri" style="width:100%" data-i="d_write" onclick="writeDefaults()">Записати в модулі</button>

  <div class="subh" data-i="h_fw">Оновлення прошивки</div>
  <div class="foot" data-i="fw_note" style="margin-bottom:12px">Виберіть firmware.bin і натисніть «Оновити». Після запису пристрій перезавантажиться.</div>
  <div class="row">
    <input type="file" id="fwFile" accept=".bin,application/octet-stream" style="flex:1">
    <button class="pri" id="fwBtn" data-i="fw_btn" onclick="doUpdate()">Оновити</button>
  </div>
  <div class="bar"><i id="fwBar"></i></div>

  <div class="subh" data-i="h_pw">Пароль доступу</div>
  <div class="foot" data-i="pw_note" style="margin-bottom:12px">Логін: admin. Пароль — мінімум 6 символів. Після зміни пристрій перезавантажиться.</div>
  <div class="row">
    <input type="password" id="np" maxlength="64" autocomplete="new-password" data-ph="pw_ph" style="flex:1">
    <button class="pri" data-i="pw_btn" onclick="changePass()">Змінити</button>
  </div>
</details>

<div class="card">
  <h2 data-i="h_conn">Статус підключень</h2>
  <div class="foot" id="mqttLine">MQTT: –</div>
  <div class="foot" id="diagLine" style="margin-top:6px">CAN: –</div>
</div>

<details class="card" id="dbgCard">
  <summary data-i="h_dbg">Діагностика CAN (trace)</summary>
  <div class="foot" data-i="dbg_note" style="margin:6px 0 10px">Кадри шини в реальному часі. За замовчуванням — знімок перших кадрів після старту (негоціація адрес модулів). «Записати знову» очищає й ловить новий знімок; «Рухомий» показує останні кадри безперервно.</div>
  <div class="row">
    <button id="trClrBtn" data-i="dbg_clear" onclick="traceClear()">Записати знову</button>
    <button id="trRollBtn" data-i="dbg_roll" onclick="traceRoll()">Рухомий</button>
    <span id="trStat" class="foot"></span>
  </div>
  <div id="trace" class="trace em" data-i="dbg_empty">—</div>
  <div class="foot" style="margin-top:8px"><span class="rx" style="font-family:monospace">rx</span> <span data-i="dbg_rx">— від модулів</span> · <span class="tx" style="font-family:monospace">tx</span> <span data-i="dbg_tx">— від контролера</span> · <span class="neg" style="font-family:monospace">neg</span> <span data-i="dbg_neg">— негоціація адрес</span>. <span data-i="dbg_cols">Колонки: Δмс, напрям, CAN ID, addr/cmd, дані.</span></div>
</details>

</main>
<div id="toast"></div>

<script>
const I18N={
 uk:{online:'онлайн',offline:'офлайн',h_status:'Статус',l_outV:'Вихідна напруга',l_outA:'Вихідний струм',l_outW:'Вихідна потужність',l_inV:'Вхідна напруга',l_inW:'Вхідна потужність',l_eff:'ККД',l_tOut:'Темп. виходу',l_tIn:'Темп. входу',l_units:'Модулів онлайн',
  u_V:'В',u_A:'А',u_W:'Вт',h_units:'Модулі',h_volt:'Напруга',h_curr:'Ліміт струму (сумарний, на всі модулі)',set:'Задати',h_out:'Вихід',h_fan:'Вентилятор',
  f_auto:'Авто',f_man:'Ручний',f_max:'Макс',f_cur:'Поточний рівень',h_def:'Дефолтні значення модулів',def_hint:'Записуються в памʼять модулів; діють при старті без контролера.',d_v:'Напруга',d_a:'Струм',d_au:'А/модуль',d_write:'Записати в модулі',sent:'Надіслано',nolink:'Немає звʼязку',
  h_fw:'Оновлення прошивки',fw_note:'Виберіть firmware.bin і натисніть «Оновити». Після запису пристрій перезавантажиться.',fw_btn:'Оновити',fw_prog:'Завантаження',fw_ok:'Прошивку записано, перезапуск…',fw_err:'Помилка оновлення',nofile:'Оберіть файл .bin',
  h_pw:'Пароль доступу',pw_note:'Логін: admin. Пароль — мінімум 6 символів. Після зміни пристрій перезавантажиться.',pw_ph:'Новий пароль',pw_btn:'Змінити',pw_ok:'Пароль змінено, перезапуск…',pw_err:'Помилка',pw_short:'Мінімум 6 символів',
  h_set:'Налаштування',h_conn:'Статус підключень',sub_wifi:'Wi-Fi та MQTT',w_scan:'Знайти мережі',w_save:'Зберегти Wi-Fi і перезавантажити',w_forget:'Забути мережу',w_cur:'Поточна',w_ap:'Режим точки доступу',w_none:'Мереж не знайдено',w_scanning:'Пошук…',w_saved:'Збережено. Перезавантаження…',w_confirm:'Забути збережену мережу й перезавантажитись?',w_err_ssid:'SSID: 1–32 символи',w_err_pass:'Пароль: 8–63 символи або порожній',w_err_mqtt:'MQTT: поля до 64 символів',w_err_port:'MQTT: порт 1–65535',w_err_net:'Помилка звʼязку',ph_ssid:'Назва мережі (SSID)',ph_wpass:'Пароль Wi-Fi (порожній = відкрита)',ph_mh:'MQTT хост (порожній = вимкнено)',ph_mu:'MQTT логін (порожній = анонімно)',ph_mp:'MQTT пароль (порожній = лишити)',m_save:'Зберегти MQTT',m_saved:'MQTT збережено',m_note:'Застосовується одразу, без перезавантаження.',
  reapply:'перезастосовується автоматично',wifi_link:'Налаштування Wi-Fi',rpm:'об/хв',hz:'Гц',vin:'вх',vout:'вих',
  outOn:'УВІМКНЕНО — натисни, щоб вимкнути',outOff:'ВИМКНЕНО — натисни, щоб увімкнути',
  mOff:'вимкнено',mOn:'онлайн',mBad:'офлайн',tries:'спроб',
  h_dbg:'Діагностика CAN (trace)',dbg_note:'Кадри шини в реальному часі. За замовчуванням — знімок перших кадрів після старту (негоціація адрес модулів). «Записати знову» очищає й ловить новий знімок; «Рухомий» показує останні кадри безперервно.',dbg_clear:'Записати знову',dbg_roll:'Рухомий',dbg_roll_on:'Рухомий ✓',dbg_empty:'—',dbg_cols:'Колонки: Δмс, напрям, CAN ID, addr/cmd, дані.',dbg_frozen:'знімок зафіксовано',dbg_rec:'запис…',dbg_rolling:'рухомий',dbg_quiet:'тихий старт',dbg_frames:'кадрів',dbg_wait:'очікування кадрів…',dbg_rx:'— від модулів',dbg_tx:'— від контролера',dbg_neg:'— негоціація адрес',m_slot:'слот',
  e_n4:'timeout',e_n3:'з\'єднання втрачено',e_n2:'TCP не вдався (IP/порт)',e_n1:'вимкнено',e_1:'протокол',e_2:'client id',e_3:'недоступний',e_4:'логін/пароль',e_5:'ACL'},
 en:{online:'online',offline:'offline',h_status:'Status',l_outV:'Output voltage',l_outA:'Output current',l_outW:'Output power',l_inV:'Input voltage',l_inW:'Input power',l_eff:'Efficiency',l_tOut:'Output temp.',l_tIn:'Input temp.',l_units:'Modules online',
  u_V:'V',u_A:'A',u_W:'W',h_units:'Modules',h_volt:'Voltage',h_curr:'Current limit (total, all modules)',set:'Apply',h_out:'Output',h_fan:'Fan',
  f_auto:'Auto',f_man:'Manual',f_max:'Max',f_cur:'Current level',h_def:'Module default values',def_hint:'Written into the modules; used when they start without the controller.',d_v:'Voltage',d_a:'Current',d_au:'A/module',d_write:'Write to modules',sent:'Sent',nolink:'No link',
  h_fw:'Firmware update',fw_note:'Pick firmware.bin and press Update. The device reboots after flashing.',fw_btn:'Update',fw_prog:'Uploading',fw_ok:'Firmware written, rebooting…',fw_err:'Update failed',nofile:'Choose a .bin file',
  h_pw:'Access password',pw_note:'Username: admin. Password — at least 6 characters. The device reboots after the change.',pw_ph:'New password',pw_btn:'Change',pw_ok:'Password changed, rebooting…',pw_err:'Error',pw_short:'At least 6 characters',
  h_set:'Settings',h_conn:'Connection status',sub_wifi:'Wi-Fi & MQTT',w_scan:'Scan networks',w_save:'Save Wi-Fi & reboot',w_forget:'Forget network',w_cur:'Current',w_ap:'Access point mode',w_none:'No networks found',w_scanning:'Scanning…',w_saved:'Saved. Rebooting…',w_confirm:'Forget the saved network and reboot?',w_err_ssid:'SSID: 1–32 characters',w_err_pass:'Password: 8–63 chars or empty',w_err_mqtt:'MQTT: fields up to 64 chars',w_err_port:'MQTT: port 1–65535',w_err_net:'Connection error',ph_ssid:'Network name (SSID)',ph_wpass:'Wi-Fi password (empty = open)',ph_mh:'MQTT host (empty = disabled)',ph_mu:'MQTT username (empty = anonymous)',ph_mp:'MQTT password (empty = keep)',m_save:'Save MQTT',m_saved:'MQTT saved',m_note:'Applied immediately, no reboot.',
  reapply:'re-applied automatically',wifi_link:'Wi-Fi settings',rpm:'rpm',hz:'Hz',vin:'in',vout:'out',
  outOn:'ON — tap to turn off',outOff:'OFF — tap to turn on',
  mOff:'disabled',mOn:'online',mBad:'offline',tries:'tries',
  h_dbg:'CAN diagnostics (trace)',dbg_note:'Live bus frames. By default a snapshot of the first frames after boot (module address negotiation). “Record again” clears and captures a fresh snapshot; “Rolling” shows the latest frames continuously.',dbg_clear:'Record again',dbg_roll:'Rolling',dbg_roll_on:'Rolling ✓',dbg_empty:'—',dbg_cols:'Columns: Δms, dir, CAN ID, addr/cmd, data.',dbg_frozen:'snapshot frozen',dbg_rec:'recording…',dbg_rolling:'rolling',dbg_quiet:'quiet start',dbg_frames:'frames',dbg_wait:'waiting for frames…',dbg_rx:'— from modules',dbg_tx:'— from controller',dbg_neg:'— address negotiation',m_slot:'slot',
  e_n4:'timeout',e_n3:'connection lost',e_n2:'TCP failed (IP/port)',e_n1:'disabled',e_1:'protocol',e_2:'client id',e_3:'unavailable',e_4:'login/password',e_5:'ACL'}
};
let lang='uk', last=null, connOn=false;
try{lang=localStorage.getItem('lang')||((navigator.language||'uk').toLowerCase().startsWith('uk')?'uk':'en')}catch(e){}
function t(k){return (I18N[lang]&&I18N[lang][k])||I18N.uk[k]||k}
function applyLang(){
  document.documentElement.lang=lang;
  document.querySelectorAll('[data-i]').forEach(e=>{e.textContent=t(e.dataset.i)});
  document.querySelectorAll('[data-ph]').forEach(e=>{e.placeholder=t(e.dataset.ph)});
  langBtn.textContent=lang=='uk'?'EN':'UK';
  setConn(connOn);
  if(last) update(last);
}
function toggleLang(){lang=lang=='uk'?'en':'uk';try{localStorage.setItem('lang',lang)}catch(e){};applyLang()}
let ws, outState=false;
function fanManual(){send('fanduty',+document.getElementById('fR').value)}
function writeDefaults(){ send('def_v',+dV.value); send('def_a',+dA.value); send('def_write',1); }
let toastT;
function toast(msg,ok){var el=document.getElementById('toast');if(!el)return;el.textContent=msg;el.className='show '+(ok?'ok':'bad');clearTimeout(toastT);toastT=setTimeout(function(){el.className=''},1600);}
function connect(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen =()=>{ setConn(true); var dc=document.getElementById('dbgCard'); if(dc&&dc.open) tsend('trace',1); };
  ws.onclose=()=>{setConn(false);setTimeout(connect,1500)};
  ws.onmessage=e=>{ const m=JSON.parse(e.data); if(m.tr) renderTrace(m.tr); else update(m); };
}
function setConn(o){connOn=o;var p=document.getElementById('conn');p.className='pill '+(o?'ok':'bad');document.getElementById('connTxt').textContent=o?t('online'):t('offline');}
function send(cmd,value){ if(ws&&ws.readyState==1){ ws.send(JSON.stringify({cmd,value})); toast(t('sent'),true); } else toast(t('nolink'),false); }
function f(x,d=1){return (x==null||isNaN(x))?'–':Number(x).toFixed(d)}
function doUpdate(){
  var fi=document.getElementById('fwFile');
  if(!fi.files.length){toast(t('nofile'),false);return;}
  var fd=new FormData(); fd.append('f', fi.files[0]);
  var x=new XMLHttpRequest(); x.open('POST','/update');
  x.upload.onprogress=function(e){ if(e.lengthComputable){ var pc=Math.round(e.loaded/e.total*100); document.getElementById('fwBar').style.width=pc+'%'; } };
  x.onload=function(){ toast(x.status==200?t('fw_ok'):t('fw_err'),x.status==200); };
  x.onerror=function(){ toast(t('fw_err'),false); document.getElementById('fwBtn').disabled=false; };
  x.send(fd);
  document.getElementById('fwBtn').disabled=true;
}
function changePass(){
  var p=document.getElementById('np').value;
  if(p.length<6){toast(t('pw_short'),false);return;}
  fetch('/api/password',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'p='+encodeURIComponent(p)})
    .then(r=>r.json()).then(j=>toast(j.ok?t('pw_ok'):t('pw_err'),!!j.ok)).catch(()=>toast(t('pw_err'),false));
}
function update(s){
  last=s;
  outV.textContent=f(s.outV); outA.textContent=f(s.outA); outW.textContent=f(s.outW,0);
  inV.textContent=f(s.inV);   inW.textContent=f(s.inW,0);  eff.textContent=f(s.eff,0);
  tOut.textContent=f(s.tOut); tIn.textContent=f(s.tIn);
  spV.textContent=f(s.spV);   spA.textContent=f(s.spA);
  setConn(s.can); // online = CAN живий
  if(!update._init && s.defV!=null){update._init=1; dV.value=(+s.defV).toFixed(1); dA.value=(+s.defA).toFixed(1);}

  units.textContent = s.units!=null ? s.units : '–';
  const mx = Math.max(1, s.units||1) * 62.5;   // сумарний максимум струму
  cR.max=mx; cN.max=mx;
  if(s.u && s.u.length){
    uSec.style.display='';
    uList.innerHTML = s.u.map(x=>{
      const alm = x.alm ? ' · alm '+(x.alm>>>0).toString(16) : '';
      const fl = (x.acf?' ⚠AC':'')+(x.ot?' ⚠HOT':'')+(x.b2?' ⚠BIT2'+(x.r8f?' 0x18F:'+x.r8f:''):'');
      const base = '#'+x.i+':  '+t('vin')+' '+f(x.vin)+' '+t('u_V')+' '+f(x.hz)+' '+t('hz')+'  →  '+t('vout')+' '+f(x.v)+' '+t('u_V')+' / '+f(x.a)+' '+t('u_A')+' · '+f(x.t,0)+' °C · '+f(x.rpm,0)+' '+t('rpm')+alm;
      const idp=[]; if(x.mdl)idp.push(x.mdl.trim()); if(x.sn)idp.push('SN '+x.sn); if(x.slot!=null)idp.push(t('m_slot')+' '+x.slot);
      const idl = idp.length ? '<div class="foot" style="margin:2px 0 0;font-family:monospace">'+idp.join(' · ')+'</div>' : '';
      return '<div class="u">'+base + (fl ? ' <b style="color:var(--bad)">'+fl+'</b>' : '')+idl+'</div>';
    }).join('');
  } else uSec.style.display='none';
  outState = !!s.output;
  outBtn.textContent = t(outState ? 'outOn' : 'outOff');
  outBtn.className = 'toggle '+(outState?'on':'off');

  fanRpm.textContent  = f(s.fanRpm,0)+' '+t('rpm');
  fanDuty.textContent = f(s.fanDuty,0);
  fA.className = (s.fanMode==0 && s.fanSp==0) ? 'act':'';
  fM.className = (s.fanMode==0 && s.fanSp>0)  ? 'act':'';
  fX.className = (s.fanMode==1) ? 'act':'';

  const m = s.mqtt || {};
  const ek=String(m.st).replace('-','n');
  mqttLine.textContent = !m.on ? 'MQTT: '+t('mOff')
    : m.conn ? ('MQTT: ✓ '+t('mOn')+' · reconnect '+m.rc+' · pub '+m.pub+' · cmd '+m.cmd)
    : ('MQTT: ✗ '+t('mBad')+' ('+(I18N[lang]['e_'+ek]||m.st)+') · '+t('tries')+' '+m.rc);

  const d = s.diag || {};
  let dl = 'CAN: '+(d.can||'?')+' · tx-err '+(d.txe||0)+' · rx-err '+(d.rxe||0);
  if(d.txf) dl += ' · tx-fail '+d.txf;
  if(d.boff) dl += ' · bus-off '+d.boff;
  if(d.quiet>0) dl += ' · '+t('dbg_quiet')+' '+d.quiet+'s';
  if(d.err) dl += '  ⚠ '+d.err+' ('+d.errAge+'s)';
  diagLine.textContent = dl;
  diagLine.style.color = d.err ? 'var(--bad)' : '';
}
// --- Wi-Fi/MQTT у секції «Налаштування» (ті самі ендпоінти /wifi/*) ---
let wnets=null, wFilled=false;
function esc(s){return s.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
async function wscan(){
  document.getElementById('wscanMsg').textContent=t('w_scanning');
  try{
    const r=await (await fetch('/wifi/scan')).json();
    if(!r.scanning) wnets=r.nets||[];
    document.getElementById('wcur').textContent = r.ap ? t('w_ap') : (r.cur ? t('w_cur')+': '+r.cur : '');
    document.getElementById('wforgetBtn').style.display = r.stored ? '' : 'none';
    if(!wFilled){ wFilled=true; mh.value=r.mh||''; mt.value=r.mt||1883; mu.value=r.mu||''; }
    if(wnets!==null){
      document.getElementById('wscanMsg').textContent='';
      document.getElementById('wlist').innerHTML = wnets.length ? wnets.map((n,i)=>'<div class="net" onclick="wpick('+i+')"><span>'+(n.enc?'🔒 ':'')+esc(n.ssid)+'</span><span class="foot">'+n.rssi+' dBm</span></div>').join('') : t('w_none');
    }
    if(r.scanning) setTimeout(wscan,1500);
  }catch(e){ document.getElementById('wscanMsg').textContent=t('w_err_net'); }
}
function wpick(i){ document.getElementById('ssid').value=wnets[i].ssid; document.getElementById('wpass').focus(); }
async function wsave(){
  var msg=document.getElementById('wmsg'); msg.style.color=''; msg.textContent='';
  var body='ssid='+encodeURIComponent(document.getElementById('ssid').value)+'&pass='+encodeURIComponent(document.getElementById('wpass').value)
    +'&mh='+encodeURIComponent(mh.value)+'&mt='+encodeURIComponent(mt.value)+'&mu='+encodeURIComponent(mu.value)+'&mp='+encodeURIComponent(mp.value);
  try{
    var r=await (await fetch('/wifi/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body})).json();
    msg.style.color=r.ok?'var(--ok)':'var(--bad)';
    msg.textContent=r.ok?t('w_saved'):t('w_err_'+r.err);
  }catch(e){ msg.style.color='var(--bad)'; msg.textContent=t('w_err_net'); }
}
async function wforget(){
  if(!confirm(t('w_confirm'))) return;
  var msg=document.getElementById('wmsg');
  try{ var r=await (await fetch('/wifi/forget',{method:'POST'})).json();
    msg.style.color=r.ok?'var(--ok)':'var(--bad)'; msg.textContent=r.ok?t('w_saved'):t('w_err_net'); }
  catch(e){ msg.style.color='var(--bad)'; msg.textContent=t('w_err_net'); }
}
function mqttSave(){
  var body='mh='+encodeURIComponent(mh.value)+'&mt='+encodeURIComponent(mt.value)+'&mu='+encodeURIComponent(mu.value)+'&mp='+encodeURIComponent(mp.value);
  fetch('/api/mqtt',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body})
    .then(r=>r.json()).then(j=>toast(j.ok?t('m_saved'):t('w_err_'+(j.err||'net')),!!j.ok)).catch(()=>toast(t('w_err_net'),false));
}
document.getElementById('setCard').addEventListener('toggle',function(){ if(this.open && !this._loaded){ this._loaded=1; wscan(); } });
// --- Debug / CAN trace ---
let traceRolling=false;
function tsend(cmd,value){ if(ws&&ws.readyState==1) ws.send(JSON.stringify({cmd,value})); }
function fmtFrame(fr,prev){
  const id=(parseInt(fr.id,16)>>>0);
  const proto=(id>>>23)&0x3f, addr=(id>>>16)&0x7f, cmd=(id>>>8)&0xff;
  const dt=(prev==null?0:(fr.t-prev));
  const neg=proto!==0x21;
  const dl=(fr.dl!=null?fr.dl:8);
  const data=fr.d.replace(/(..)/g,'$1 ').trim().split(' ').slice(0,dl).join(' ');
  const tag=neg?'neg':(fr.x?'tx':'rx');
  const ac=neg?'  neg  ':('a'+addr+' c'+cmd.toString(16).toUpperCase().padStart(2,'0'));
  const head=((dt>9999?'9999+':dt+'')+'ms').padStart(7)+' '+(fr.x?'tx':'rx');
  return '<span class="'+tag+'">'+head+'</span>  '+fr.id+'  '+ac+'  '+data;
}
function renderTrace(tr){
  traceRolling=!!tr.roll;
  const el=document.getElementById('trace'), st=document.getElementById('trStat');
  const rb=document.getElementById('trRollBtn');
  const parts=[tr.len+' '+t('dbg_frames'), tr.roll?t('dbg_rolling'):(tr.froz?t('dbg_frozen'):t('dbg_rec'))];
  if(tr.quiet>0) parts.push(t('dbg_quiet')+' '+tr.quiet+'s');
  st.textContent=parts.join(' · ');
  rb.textContent=tr.roll?t('dbg_roll_on'):t('dbg_roll'); rb.className=tr.roll?'pri':'';
  if(!tr.f||!tr.f.length){ el.className='trace em'; el.textContent=t('dbg_wait'); return; }
  el.className='trace';
  let prev=null; const rows=tr.f.map(fr=>{const r=fmtFrame(fr,prev);prev=fr.t;return r;});
  el.innerHTML=rows.join('\n');
  el.scrollTop=traceRolling?el.scrollHeight:0;
}
function traceClear(){ tsend('trace_clear',1); const el=document.getElementById('trace'); el.className='trace em'; el.textContent=t('dbg_wait'); }
function traceRoll(){ traceRolling=!traceRolling; tsend('trace_roll',traceRolling?1:0); }
document.getElementById('dbgCard').addEventListener('toggle',function(){ tsend('trace',this.open?1:0); if(this.open){ const el=document.getElementById('trace'); if(el.classList.contains('em')) el.textContent=t('dbg_wait'); } });
document.addEventListener('click',function(e){var b=e.target.closest('button');if(!b)return;b.classList.remove('flash');void b.offsetWidth;b.classList.add('flash');setTimeout(function(){b.classList.remove('flash')},350);});
applyLang();
connect();
</script></body></html>
)HTMLDOC";
