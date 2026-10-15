#pragma once
#include <pgmspace.h>

// Сторінка налаштування Wi-Fi (/wifi; у режимі точки доступу — також "/").
// Список мереж: GET /wifi/scan, збереження: POST /wifi/save, скидання: POST /wifi/forget.
const char WIFI_HTML[] PROGMEM = R"WIFIDOC(
<!DOCTYPE html><html lang="uk"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wi-Fi · Huawei R4850G2</title>
<style>
:root{--bg:#0f1115;--card:#181b21;--line:#272b33;--txt:#e6e8ec;--mut:#8b909a;--acc:#3b82f6;--ok:#22c55e;--err:#ef4444}
*{box-sizing:border-box}body{margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--txt)}
.wrap{max-width:460px;margin:0 auto;padding:18px}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}
h1{font-size:18px;font-weight:600;margin:0}
.ctl{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:12px}
.ctl h2{font-size:14px;margin:0 0 12px;font-weight:600}
.mini{font-size:12px;color:var(--mut)}
input{width:100%;background:var(--bg);border:1px solid var(--line);color:var(--txt);border-radius:8px;padding:10px;font-size:15px;margin-bottom:10px}
button{border:none;border-radius:9px;padding:10px 16px;font-size:14px;font-weight:600;cursor:pointer}
.apply{background:var(--acc);color:#fff;width:100%}
.danger{background:transparent;color:var(--err);border:1px solid var(--err);width:100%;margin-top:8px}
.lang{background:var(--card);border:1px solid var(--line);color:var(--txt);padding:3px 9px;font-size:12px;border-radius:8px}
.net{display:flex;justify-content:space-between;padding:9px 4px;border-bottom:1px solid var(--line);cursor:pointer;font-size:14px}
.net:last-child{border-bottom:none}.net:hover{color:var(--acc)}
#msg{margin-top:10px;font-size:13px}
a{color:var(--mut)}
</style></head><body><div class="wrap">
<header><h1>Wi-Fi</h1><button class="lang" id="langBtn" onclick="toggleLang()">EN</button></header>

<div class="ctl">
  <h2 data-i="nets">Доступні мережі</h2>
  <div id="list" class="mini" data-i="scanning">Пошук мереж…</div>
</div>

<div class="ctl">
  <h2 data-i="conn">Підключення</h2>
  <div id="cur" class="mini" style="margin-bottom:10px"></div>
  <input id="ssid" maxlength="32" autocomplete="off" autocapitalize="off" data-ph="ssid">
  <input id="pass" type="password" maxlength="63" autocomplete="off" data-ph="pass">
  <button class="apply" id="saveBtn" data-i="save" onclick="save()">Зберегти й перезавантажити</button>
  <button class="danger" id="forgetBtn" data-i="forget" style="display:none" onclick="forget()">Забути збережену мережу</button>
  <div id="msg"></div>
</div>

<div class="ctl">
  <h2>MQTT</h2>
  <div style="display:flex;gap:8px">
    <input id="mh" maxlength="64" autocomplete="off" autocapitalize="off" data-ph="mh" style="flex:1">
    <input id="mt" type="number" min="1" max="65535" value="1883" style="width:90px">
  </div>
  <input id="mu" maxlength="64" autocomplete="off" autocapitalize="off" data-ph="mu">
  <input id="mp" type="password" maxlength="64" autocomplete="off" data-ph="mp">
  <div class="mini" data-i="mqtt_note">Зберігається кнопкою вище разом із Wi-Fi.</div>
</div>
<div class="mini" style="text-align:center"><a href="/dashboard" data-i="dash">Дашборд</a></div>
</div>
<script>
const I18N={
 uk:{nets:'Доступні мережі',scanning:'Пошук мереж…',none:'Мереж не знайдено',conn:'Підключення',save:'Зберегти й перезавантажити',forget:'Забути збережену мережу',dash:'Дашборд',
  ssid:'Назва мережі (SSID)',mh:'MQTT хост (порожній = вимкнено)',mu:'MQTT логін (порожній = анонімно)',mp:'MQTT пароль (порожній = лишити старий)',mqtt_note:'Зберігається кнопкою вище разом із Wi-Fi.',e_mqtt:'MQTT: поля до 64 символів',e_port:'MQTT: порт 1–65535',pass:'Пароль (порожній для відкритої мережі)',cur:'Поточна мережа',ap:'Режим точки доступу — підключення до роутера немає',
  saved:'Збережено. Пристрій перезавантажується — підключіться до своєї мережі й відкрийте http://huawei-r4850.local або IP з роутера.',
  e_ssid:'Назва мережі має бути 1–32 символи',e_pass:'Пароль має бути 8–63 символи (або порожній)',e_net:'Помилка зв\'язку',
  confirm:'Забути збережену мережу й перезавантажитись у режим налаштування?'},
 en:{nets:'Available networks',scanning:'Scanning…',none:'No networks found',conn:'Connection',save:'Save and reboot',forget:'Forget saved network',dash:'Dashboard',
  ssid:'Network name (SSID)',mh:'MQTT host (empty = disabled)',mu:'MQTT username (empty = anonymous)',mp:'MQTT password (empty = keep current)',mqtt_note:'Saved together with Wi-Fi by the button above.',e_mqtt:'MQTT: fields up to 64 characters',e_port:'MQTT: port 1–65535',pass:'Password (empty for open network)',cur:'Current network',ap:'Access point mode — not connected to a router',
  saved:'Saved. The device is rebooting — join your network and open http://huawei-r4850.local or the IP from your router.',
  e_ssid:'SSID must be 1–32 characters',e_pass:'Password must be 8–63 characters (or empty)',e_net:'Connection error',
  confirm:'Forget the saved network and reboot into setup mode?'}
};
let lang='uk',info={},nets=null;
try{lang=localStorage.getItem('lang')||((navigator.language||'uk').toLowerCase().startsWith('uk')?'uk':'en')}catch(e){}
function t(k){return (I18N[lang]&&I18N[lang][k])||I18N.uk[k]||k}
function esc(s){return s.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}
function render(){
  document.documentElement.lang=lang;
  document.querySelectorAll('[data-i]').forEach(e=>{ if(!(e.id=='list'&&nets)) e.textContent=t(e.dataset.i)});
  document.querySelectorAll('[data-ph]').forEach(e=>{e.placeholder=t(e.dataset.ph)});
  langBtn.textContent=lang=='uk'?'EN':'UK';
  cur.textContent = info.ap ? t('ap') : (info.cur ? t('cur')+': '+info.cur : '');
  forgetBtn.style.display = info.stored ? '' : 'none';
  if(nets){
    list.innerHTML = nets.length ? nets.map((n,i)=>'<div class="net" onclick="pick('+i+')"><span>'+(n.enc?'🔒 ':'')+esc(n.ssid)+'</span><span class="mini">'+n.rssi+' dBm</span></div>').join('') : t('none');
  }
}
function toggleLang(){lang=lang=='uk'?'en':'uk';try{localStorage.setItem('lang',lang)}catch(e){};render()}
function pick(i){ssid.value=nets[i].ssid;pass.focus()}
async function scan(){
  try{
    const r=await (await fetch('/wifi/scan')).json();
    info=r; if(!r.scanning){nets=r.nets||[];}
    if(!mh.dataset.set){mh.value=r.mh||'';mt.value=r.mt||1883;mu.value=r.mu||'';mh.dataset.set=1}
    render();
    if(r.scanning) setTimeout(scan,1500);
  }catch(e){setTimeout(scan,3000)}
}
async function post(url,body){
  try{ return await (await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body})).json() }
  catch(e){ return {ok:false,err:'net'} }
}
async function save(){
  msg.style.color='';msg.textContent='';
  const r=await post('/wifi/save','ssid='+encodeURIComponent(ssid.value)+'&pass='+encodeURIComponent(pass.value)+'&mh='+encodeURIComponent(mh.value)+'&mt='+encodeURIComponent(mt.value)+'&mu='+encodeURIComponent(mu.value)+'&mp='+encodeURIComponent(mp.value));
  msg.style.color=r.ok?'var(--ok)':'var(--err)';
  msg.textContent=r.ok?t('saved'):t('e_'+r.err);
  if(r.ok){saveBtn.disabled=true}
}
async function forget(){
  if(!confirm(t('confirm'))) return;
  const r=await post('/wifi/forget','');
  msg.style.color=r.ok?'var(--ok)':'var(--err)';
  msg.textContent=r.ok?t('saved'):t('e_net');
}
render();scan();
</script></body></html>
)WIFIDOC";
