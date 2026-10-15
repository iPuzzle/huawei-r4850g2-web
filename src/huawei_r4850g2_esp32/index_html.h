#pragma once
#include <pgmspace.h>

// Веб-дашборд (UK/EN, кнопка в шапці). Оновлення через WebSocket (ws://<host>/ws).
const char INDEX_HTML[] PROGMEM = R"HTMLDOC(
<!DOCTYPE html><html lang="uk"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Huawei R4850G2</title>
<style>
:root{--bg:#0f1115;--card:#181b21;--line:#272b33;--txt:#e6e8ec;--mut:#8b909a;--acc:#3b82f6;--ok:#22c55e;--err:#ef4444;--warn:#f59e0b}
*{box-sizing:border-box}body{margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--txt)}
.wrap{max-width:760px;margin:0 auto;padding:18px}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}
h1{font-size:18px;font-weight:600;margin:0}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;background:var(--err);margin-right:6px;vertical-align:middle}
.dot.on{background:var(--ok)}
.pill{font-size:12px;color:var(--mut)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin-bottom:18px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px 14px}
.card .lbl{font-size:12px;color:var(--mut);margin-bottom:4px}
.card .val{font-size:22px;font-weight:600}
.card .u{font-size:13px;color:var(--mut);font-weight:400;margin-left:3px}
.ctl{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:12px}
.ctl h2{font-size:14px;margin:0 0 12px;font-weight:600}
.row{display:flex;align-items:center;gap:12px;margin-bottom:12px}
input[type=range]{flex:1;accent-color:var(--acc)}
input[type=number]{width:88px;background:var(--bg);border:1px solid var(--line);color:var(--txt);border-radius:8px;padding:8px;font-size:15px}
button{border:none;border-radius:9px;padding:9px 16px;font-size:14px;font-weight:600;cursor:pointer}
.apply{background:var(--acc);color:#fff}
.toggle{width:100%;padding:14px;font-size:15px}
.toggle.on{background:var(--ok);color:#06210f}.toggle.off{background:var(--err);color:#2a0808}
.foot{font-size:11px;color:var(--mut);text-align:center;margin-top:8px}
.seg{display:flex;gap:8px;margin-bottom:12px}
.seg button{flex:1;background:var(--bg);border:1px solid var(--line);color:var(--txt)}
.seg button.act{background:var(--acc);color:#fff;border-color:var(--acc)}
.badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:12px;font-weight:600;background:var(--line);color:var(--txt)}
.badge.bulk{background:#1d4ed8;color:#fff}.badge.absorb{background:#f59e0b;color:#241a03}
.badge.done{background:var(--ok);color:#06210f}.badge.float{background:#0891b2;color:#03191d}
.badge.fault{background:var(--err);color:#2a0808}
.mini{font-size:12px;color:var(--mut)}
.lang{background:var(--card);border:1px solid var(--line);color:var(--txt);padding:3px 9px;font-size:12px;border-radius:8px;margin-left:10px}
</style></head><body><div class="wrap">
<header>
  <h1>Huawei R4850G2</h1>
  <span class="pill"><span id="dot" class="dot"></span><span id="conn">офлайн</span><button class="lang" id="langBtn" onclick="toggleLang()">EN</button></span>
</header>

<div class="grid">
  <div class="card"><div class="lbl" data-i="l_outV">Вихідна напруга</div><div class="val"><span id="outV">–</span><span class="u" data-i="u_V">В</span></div></div>
  <div class="card"><div class="lbl" data-i="l_outA">Вихідний струм</div><div class="val"><span id="outA">–</span><span class="u" data-i="u_A">А</span></div></div>
  <div class="card"><div class="lbl" data-i="l_outW">Вихідна потужність</div><div class="val"><span id="outW">–</span><span class="u" data-i="u_W">Вт</span></div></div>
  <div class="card"><div class="lbl" data-i="l_inV">Вхідна напруга</div><div class="val"><span id="inV">–</span><span class="u" data-i="u_V">В</span></div></div>
  <div class="card"><div class="lbl" data-i="l_inW">Вхідна потужність</div><div class="val"><span id="inW">–</span><span class="u" data-i="u_W">Вт</span></div></div>
  <div class="card"><div class="lbl" data-i="l_eff">ККД</div><div class="val"><span id="eff">–</span><span class="u">%</span></div></div>
  <div class="card"><div class="lbl" data-i="l_tOut">Темп. виходу</div><div class="val"><span id="tOut">–</span><span class="u">°C</span></div></div>
  <div class="card"><div class="lbl" data-i="l_tIn">Темп. входу</div><div class="val"><span id="tIn">–</span><span class="u">°C</span></div></div>
  <div class="card"><div class="lbl" data-i="l_units">Модулів онлайн</div><div class="val"><span id="units">–</span></div></div>
</div>

<div id="uSec" class="ctl" style="display:none">
  <h2 data-i="h_units">Модулі (per-unit)</h2>
  <div id="uList" class="mini"></div>
</div>

<div class="ctl">
  <h2 data-i="h_volt">Напруга</h2>
  <div class="row">
    <input type="range" id="vR" min="41" max="58" step="0.1" value="53.5" oninput="vN.value=this.value">
    <input type="number" id="vN" min="41" max="58" step="0.1" value="53.5" oninput="vR.value=this.value">
    <button class="apply" data-i="set" onclick="send('voltage',+vN.value)">Задати</button>
  </div>
  <h2 data-i="h_curr">Ліміт струму (сумарний, на всі модулі)</h2>
  <div class="row">
    <input type="range" id="cR" min="0" max="62.5" step="0.5" value="10" oninput="cN.value=this.value">
    <input type="number" id="cN" min="0" max="62.5" step="0.5" value="10" oninput="cR.value=this.value">
    <button class="apply" data-i="set" onclick="send('current',+cN.value)">Задати</button>
  </div>
</div>

<div class="ctl">
  <h2 data-i="h_out">Вихід</h2>
  <button id="outBtn" class="toggle off" onclick="send('output', outState?0:1)">–</button>
</div>

<div class="ctl">
  <h2><span data-i="h_fan">Вентилятор</span> · <span id="fanRpm" class="mini">–</span></h2>
  <div class="seg">
    <button id="fA" data-i="f_auto" onclick="send('fanmode',0)">Авто</button>
    <button id="fM" data-i="f_man" onclick="fanManual()">Ручний</button>
    <button id="fX" data-i="f_max" onclick="send('fanmode',1)">Макс</button>
  </div>
  <div class="row">
    <input type="range" id="fR" min="0" max="100" step="5" value="0" oninput="fV.textContent=this.value+'%'">
    <span id="fV" class="mini" style="width:44px">0%</span>
    <button class="apply" data-i="set" onclick="fanManual()">Задати</button>
  </div>
  <div class="mini"><span data-i="f_cur">Поточний рівень</span>: <span id="fanDuty">–</span>%</div>
</div>

<div class="ctl">
  <h2><span data-i="h_chg">Автозарядка LiFePO4</span> · <span id="chgBadge" class="badge">off</span></h2>
  <div class="row">
    <label class="mini" style="width:64px" data-i="g_v">Напруга</label>
    <input type="number" id="gV" step="0.1" value="56.0" onchange="send('chg_v',+this.value)"><span class="mini" data-i="g_vu">В (CV)</span>
  </div>
  <div class="row">
    <label class="mini" style="width:64px" data-i="g_a">Струм</label>
    <input type="number" id="gA" step="0.5" value="30" onchange="send('chg_a',+this.value)"><span class="mini" data-i="g_au">А (bulk / CC)</span>
  </div>
  <div class="row">
    <label class="mini" style="width:64px">Tail</label>
    <input type="number" id="gT" step="0.5" value="3" onchange="send('chg_tail',+this.value)"><span class="mini" data-i="g_tu">А (завершення)</span>
  </div>
  <button id="chgBtn" class="toggle off" onclick="send('charge', chgOn?0:1)">–</button>
</div>

<div class="foot">Setpoint: <span id="spV">–</span> <span data-i="u_V">В</span> / <span id="spA">–</span> <span data-i="u_A">А</span> · <span data-i="reapply">перезастосовується автоматично</span></div>
<div class="foot" id="mqttLine">MQTT: –</div>
<div class="foot" id="diagLine">CAN: –</div>
<div class="foot"><a href="/wifi" data-i="wifi_link" style="color:var(--mut)">Налаштування Wi-Fi</a></div>
</div>

<script>
const I18N={
 uk:{online:'онлайн',offline:'офлайн',l_outV:'Вихідна напруга',l_outA:'Вихідний струм',l_outW:'Вихідна потужність',l_inV:'Вхідна напруга',l_inW:'Вхідна потужність',l_eff:'ККД',l_tOut:'Темп. виходу',l_tIn:'Темп. входу',l_units:'Модулів онлайн',
  u_V:'В',u_A:'А',u_W:'Вт',h_units:'Модулі (per-unit)',h_volt:'Напруга',h_curr:'Ліміт струму (сумарний, на всі модулі)',set:'Задати',h_out:'Вихід',h_fan:'Вентилятор',
  f_auto:'Авто',f_man:'Ручний',f_max:'Макс',f_cur:'Поточний рівень',h_chg:'Автозарядка LiFePO4',g_v:'Напруга',g_a:'Струм',g_vu:'В (CV)',g_au:'А (bulk / CC)',g_tu:'А (завершення)',
  reapply:'перезастосовується автоматично',wifi_link:'Налаштування Wi-Fi',rpm:'об/хв',hz:'Гц',vin:'вх',vout:'вих',
  outOn:'УВІМКНЕНО — натисни, щоб вимкнути',outOff:'ВИМКНЕНО — натисни, щоб увімкнути',chgOn:'АКТИВНА — натисни, щоб зупинити',chgOff:'ВИМКНЕНА — натисни, щоб запустити',
  mOff:'вимкнено',mOn:'онлайн',mBad:'офлайн',tries:'спроб',
  e_n4:'timeout',e_n3:'з\'єднання втрачено',e_n2:'TCP не вдався (IP/порт)',e_n1:'вимкнено',e_1:'протокол',e_2:'client id',e_3:'недоступний',e_4:'логін/пароль',e_5:'ACL'},
 en:{online:'online',offline:'offline',l_outV:'Output voltage',l_outA:'Output current',l_outW:'Output power',l_inV:'Input voltage',l_inW:'Input power',l_eff:'Efficiency',l_tOut:'Output temp.',l_tIn:'Input temp.',l_units:'Modules online',
  u_V:'V',u_A:'A',u_W:'W',h_units:'Modules (per-unit)',h_volt:'Voltage',h_curr:'Current limit (total, all modules)',set:'Apply',h_out:'Output',h_fan:'Fan',
  f_auto:'Auto',f_man:'Manual',f_max:'Max',f_cur:'Current level',h_chg:'LiFePO4 auto-charge',g_v:'Voltage',g_a:'Current',g_vu:'V (CV)',g_au:'A (bulk / CC)',g_tu:'A (termination)',
  reapply:'re-applied automatically',wifi_link:'Wi-Fi settings',rpm:'rpm',hz:'Hz',vin:'in',vout:'out',
  outOn:'ON — tap to turn off',outOff:'OFF — tap to turn on',chgOn:'ACTIVE — tap to stop',chgOff:'STOPPED — tap to start',
  mOff:'disabled',mOn:'online',mBad:'offline',tries:'tries',
  e_n4:'timeout',e_n3:'connection lost',e_n2:'TCP failed (IP/port)',e_n1:'disabled',e_1:'protocol',e_2:'client id',e_3:'unavailable',e_4:'login/password',e_5:'ACL'}
};
let lang='uk', last=null, connOn=false;
try{lang=localStorage.getItem('lang')||((navigator.language||'uk').toLowerCase().startsWith('uk')?'uk':'en')}catch(e){}
function t(k){return (I18N[lang]&&I18N[lang][k])||I18N.uk[k]||k}
function applyLang(){
  document.documentElement.lang=lang;
  document.querySelectorAll('[data-i]').forEach(e=>{e.textContent=t(e.dataset.i)});
  langBtn.textContent=lang=='uk'?'EN':'UK';
  setConn(connOn);
  if(last) update(last);
}
function toggleLang(){lang=lang=='uk'?'en':'uk';try{localStorage.setItem('lang',lang)}catch(e){};applyLang()}
let ws, outState=false, chgOn=false;
function fanManual(){send('fanduty',+document.getElementById('fR').value)}
function connect(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen =()=>setConn(true);
  ws.onclose=()=>{setConn(false);setTimeout(connect,1500)};
  ws.onmessage=e=>update(JSON.parse(e.data));
}
function setConn(o){connOn=o;dot.className='dot'+(o?' on':'');conn.textContent=o?t('online'):t('offline')}
function send(cmd,value){ if(ws&&ws.readyState==1) ws.send(JSON.stringify({cmd,value})) }
function f(x,d=1){return (x==null||isNaN(x))?'–':Number(x).toFixed(d)}
function update(s){
  last=s;
  outV.textContent=f(s.outV); outA.textContent=f(s.outA); outW.textContent=f(s.outW,0);
  inV.textContent=f(s.inV);   inW.textContent=f(s.inW,0);  eff.textContent=f(s.eff,0);
  tOut.textContent=f(s.tOut); tIn.textContent=f(s.tIn);
  spV.textContent=f(s.spV);   spA.textContent=f(s.spA);
  setConn(s.can); // online = CAN живий
  if(!update._init){update._init=1; gV.value=(+s.chgV).toFixed(1); gA.value=(+s.chgA).toFixed(1); gT.value=(+s.chgTail).toFixed(1);}

  // кілька модулів
  units.textContent = s.units!=null ? s.units : '–';
  const mx = Math.max(1, s.units||1) * 62.5;   // сумарний максимум струму
  cR.max=mx; cN.max=mx; gA.max=mx;
  if(s.u && s.u.length){
    uSec.style.display='';
    uList.innerHTML = s.u.map(x=>{
      const alm = x.alm ? ' · alm '+(x.alm>>>0).toString(16) : '';
      const fl = (x.acf?' ⚠AC':'')+(x.ot?' ⚠HOT':'')+(x.b2?' ⚠BIT2'+(x.r8f?' 0x18F:'+x.r8f:''):'');
      const base = '#'+x.i+':  '+t('vin')+' '+f(x.vin)+' '+t('u_V')+' '+f(x.hz)+' '+t('hz')+'  →  '+t('vout')+' '+f(x.v)+' '+t('u_V')+' / '+f(x.a)+' '+t('u_A')+' · '+f(x.t,0)+' °C · '+f(x.rpm,0)+' '+t('rpm')+alm;
      return base + (fl ? ' <b style="color:var(--err)">'+fl+'</b>' : '');
    }).join('<br>');
  } else uSec.style.display='none';
  outState = !!s.output;
  outBtn.textContent = t(outState ? 'outOn' : 'outOff');
  outBtn.className = 'toggle '+(outState?'on':'off');

  fanRpm.textContent  = f(s.fanRpm,0)+' '+t('rpm');
  fanDuty.textContent = f(s.fanDuty,0);
  fA.className = (s.fanMode==0 && s.fanSp==0) ? 'act':'';
  fM.className = (s.fanMode==0 && s.fanSp>0)  ? 'act':'';
  fX.className = (s.fanMode==1) ? 'act':'';

  chgOn = !!s.chg;
  const cs = s.chgState||'off';
  chgBadge.textContent = cs; chgBadge.className = 'badge '+cs;
  chgBtn.textContent = t(chgOn ? 'chgOn' : 'chgOff');
  chgBtn.className = 'toggle '+(chgOn?'on':'off');

  const m = s.mqtt || {};
  const ek=String(m.st).replace('-','n');
  mqttLine.textContent = !m.on ? 'MQTT: '+t('mOff')
    : m.conn ? ('MQTT: ✓ '+t('mOn')+' · reconnect '+m.rc+' · pub '+m.pub+' · cmd '+m.cmd)
    : ('MQTT: ✗ '+t('mBad')+' ('+(I18N[lang]['e_'+ek]||m.st)+') · '+t('tries')+' '+m.rc);

  const d = s.diag || {};
  let dl = 'CAN: '+(d.can||'?')+' · tx-err '+(d.txe||0)+' · rx-err '+(d.rxe||0);
  if(d.txf) dl += ' · tx-fail '+d.txf;
  if(d.boff) dl += ' · bus-off '+d.boff;
  if(d.err) dl += '  ⚠ '+d.err+' ('+d.errAge+'s)';
  diagLine.textContent = dl;
  diagLine.style.color = d.err ? 'var(--err)' : '';
}
applyLang();
connect();
</script></body></html>
)HTMLDOC";
