#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================================================
#  Генератор превʼю веб-морди.
#  Витягує HTML зі скетчевого index_html.h і додає мок-дані (фейковий WebSocket),
#  щоб сторінку можна було відкрити у звичайному браузері БЕЗ прошивки ESP32.
#
#  Запуск вручну:   python tools/gen_preview.py
#  Автоматично:     PlatformIO викликає його як pre-скрипт (див. platformio.ini),
#                   тож preview/index.html оновлюється при КОЖНІЙ збірці —
#                   превʼю ніколи не розходиться з основною сторінкою.
#
#  Результат:  preview/index.html  (self-contained, відкривати подвійним кліком)
# ============================================================================
import os, re, sys, io

try:
    HERE = os.path.dirname(os.path.abspath(__file__))
except NameError:
    HERE = os.path.join(os.getcwd(), 'tools')

HEADER = os.path.normpath(os.path.join(HERE, '..', 'huawei_r4850g2_esp32', 'index_html.h'))
OUTDIR = os.path.normpath(os.path.join(HERE, '..', 'preview'))
OUT    = os.path.join(OUTDIR, 'index.html')

SHIM = '''<!-- ===== PREVIEW SHIM (згенеровано автоматично; джерело: index_html.h) ===== -->
<style>
#__pv{position:sticky;top:0;z-index:9999;background:#111;color:#eee;font:13px system-ui,sans-serif;padding:8px 12px;border-bottom:1px solid #333;display:flex;gap:8px;align-items:center;flex-wrap:wrap}
#__pv b{color:#f59e0b;margin-right:4px}
#__pv .s{opacity:.7;margin-right:4px}
#__pv button{background:#222;color:#eee;border:1px solid #444;border-radius:6px;padding:5px 10px;cursor:pointer;font:13px system-ui,sans-serif}
#__pv button.on{background:#3b82f6;border-color:#3b82f6}
</style>
<div id="__pv"><b>PREVIEW</b><span class="s">mock-дані · сценарій:</span></div>
<script>
(function(){
  var scenario='ok';
  var scn=[['ok','Норма'],['acfail','Втрата фази L2'],['busoff','CAN bus-off'],['mqtt','MQTT офлайн'],['overtemp','Перегрів модуля']];
  function rnd(a){ return (Math.random()*2-1)*a; }
  function base(){
    var mods=[1,2,3].map(function(i){
      return {i:i, vin:230+rnd(2), hz:50+rnd(0.05), v:55.9+rnd(0.05), a:9+Math.random()*1.2,
              t:42+Math.random()*3, rpm:4200+Math.random()*300, alm:0, acf:false, ot:false, b2:false, r8f:'',
              sn:['686875042029','70688530A1A6','2C682408351B'][i-1]||'', mdl:['R4850G2','R4850G5','R4830G2'][i-1]||'', slot:i};
    });
    return {
      outV:55.9+rnd(0.05), outA:28+rnd(1), outW:1560+rnd(40), inV:230+rnd(1.5), inW:1700+rnd(40), eff:96+Math.random(),
      tOut:44+Math.random()*2, tIn:30+Math.random()*2,
      spV:56.0, spA:30, output:true, can:true,
      fanRpm:4300+Math.random()*200, fanDuty:35, fanSp:0, fanMode:0,
      defV:53.5, defA:62.5,
      units:3, u:mods,
      mqtt:{on:true, conn:true, st:0, rc:1, pub:1240, cmd:5},
      diag:{can:'running', txe:0, rxe:0, txf:0, boff:0}
    };
  }
  function sample(){
    var s=base();
    if(scenario==='acfail'){ var u=s.u[1]; u.vin=0; u.hz=0; u.acf=true; u.a=0;
      s.diag.err='Модуль 2: втрата AC/фази'; s.diag.errAge=7; s.psuAlarm=true; }
    else if(scenario==='busoff'){ s.can=false; s.diag.can='bus-off'; s.diag.boff=2; s.diag.txe=128; s.diag.txf=5;
      s.diag.err='CAN bus-off - CAN_H/L / термінатор?'; s.diag.errAge=3; }
    else if(scenario==='mqtt'){ s.mqtt.conn=false; s.mqtt.st=-2; }
    else if(scenario==='overtemp'){ s.u[0].ot=true; s.u[0].t=97;
      s.diag.err='Модуль 1: перегрів виходу'; s.diag.errAge=4; }
    return s;
  }
  var listeners=[];
  function push(){ var d=JSON.stringify(sample());
    listeners.forEach(function(ws){ if(ws.onmessage) ws.onmessage({data:d}); }); }
  // --- мок CAN-trace для debug-секції ---
  var trSub=false, trFroz=true, trRoll=false;
  var trFrames=[
    {t:1203,x:0,dl:8,id:'10070100',d:'3001A1B2C3D40000'},
    {t:1251,x:0,dl:8,id:'10070200',d:'3002E5F607080000'},
    {t:1298,x:0,dl:8,id:'10078100',d:'3081000102030405'},
    {t:5010,x:1,dl:8,id:'108040FE',d:'0000000000000000'},
    {t:5013,x:0,dl:8,id:'1081407F',d:'017500000DF80000'},
    {t:5015,x:0,dl:8,id:'1082407F',d:'017500000DF80000'},
    {t:5018,x:0,dl:8,id:'1083407F',d:'0183000000000004'}
  ];
  function pushTrace(){ if(!trSub) return;
    var tr={froz:trFroz,roll:trRoll,len:trFrames.length,now:6000,quiet:0,f:trFrames};
    listeners.forEach(function(ws){ if(ws.onmessage) ws.onmessage({data:JSON.stringify({tr:tr})}); }); }
  function applyCmd(c){ if(!c) return;
    if(c.cmd==='trace'){ trSub=!!c.value; if(trSub) pushTrace(); }
    else if(c.cmd==='trace_clear'){ trFroz=false; pushTrace(); setTimeout(function(){trFroz=true;pushTrace();},400); }
    else if(c.cmd==='trace_roll'){ trRoll=!!c.value; pushTrace(); }
  }
  function MockWS(url){ this.url=url; this.readyState=0; var self=this;
    setTimeout(function(){ self.readyState=1; if(self.onopen) self.onopen(); listeners.push(self);
      if(self.onmessage) self.onmessage({data:JSON.stringify(sample())}); }, 120); }
  MockWS.prototype.send=function(raw){ try{ applyCmd(JSON.parse(raw)); }catch(e){} };
  MockWS.prototype.close=function(){ this.readyState=3; var i=listeners.indexOf(this); if(i>=0) listeners.splice(i,1); };
  MockWS.prototype.addEventListener=function(t,f){ this['on'+t]=f; };
  MockWS.CONNECTING=0; MockWS.OPEN=1; MockWS.CLOSING=2; MockWS.CLOSED=3;
  window.WebSocket=MockWS;
  var _of=window.fetch;
  window.fetch=function(u,o){u=String(u);
    function R(ob){return Promise.resolve({ok:true,status:200,json:function(){return Promise.resolve(ob);},text:function(){return Promise.resolve(JSON.stringify(ob));}});}
    if(u.indexOf('/wifi/scan')>=0)return R({ap:false,stored:true,cur:'MyHomeWiFi_5G',mh:'192.168.0.198',mt:1883,mu:'mqtt_user',scanning:false,nets:[{ssid:'MyHomeWiFi_5G',rssi:-47,enc:true},{ssid:'Office-Guest',rssi:-66,enc:false},{ssid:'Neighbor_2.4',rssi:-75,enc:true}]});
    if(u.indexOf('/wifi/save')>=0||u.indexOf('/wifi/forget')>=0||u.indexOf('/api/password')>=0||u.indexOf('/api/mqtt')>=0)return R({ok:true});
    return _of?_of.apply(this,arguments):R({});
  };
  setInterval(push, 1500);
  setInterval(pushTrace, 1000);
  function mk(){ var bar=document.getElementById('__pv'); if(!bar) return;
    scn.forEach(function(p){ var b=document.createElement('button'); b.textContent=p[1];
      if(p[0]===scenario) b.className='on';
      b.onclick=function(){ scenario=p[0];
        Array.prototype.forEach.call(bar.querySelectorAll('button'),function(x){x.className='';});
        b.className='on'; push(); };
      bar.appendChild(b); }); }
  if(document.readyState!=='loading') mk(); else document.addEventListener('DOMContentLoaded',mk);
})();
</script>
<!-- ===== /PREVIEW SHIM ===== -->
'''

def extract(header_text):
    m = re.search(r'R"([A-Za-z0-9_]*)\((.*?)\)\1"', header_text, re.DOTALL)
    return m.group(2) if m else None

def build():
    with io.open(HEADER, 'r', encoding='utf-8') as f:
        html = extract(f.read())
    if html is None:
        raise RuntimeError('не знайдено R"HTMLDOC(...)" у ' + HEADER)
    new, n = re.subn(r'<body[^>]*>', lambda mm: mm.group(0) + '\n' + SHIM, html, count=1, flags=re.IGNORECASE)
    if n == 0:
        new = SHIM + html
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    with io.open(OUT, 'w', encoding='utf-8') as f:
        f.write(new)
    return OUT

try:
    out = build()
    print('[gen_preview] OK -> ' + out)
except Exception as e:
    # превʼю ніколи не ламає збірку прошивки
    sys.stderr.write('[gen_preview] УВАГА: ' + str(e) + '\n')

# ---- Сторінка налаштувань Wi-Fi/MQTT (wifi_html.h) -------------------------
WIFI_HEADER = os.path.normpath(os.path.join(HERE, '..', 'huawei_r4850g2_esp32', 'wifi_html.h'))
WIFI_OUT    = os.path.join(OUTDIR, 'wifi.html')
SHIM_WIFI = '''<!-- ===== PREVIEW SHIM wifi (fetch mock; джерело: wifi_html.h) ===== -->
<style>
#__pv{position:sticky;top:0;z-index:9999;background:#111;color:#eee;font:13px system-ui,sans-serif;padding:8px 12px;border-bottom:1px solid #333;display:flex;gap:8px;align-items:center;flex-wrap:wrap}
#__pv b{color:#f59e0b;margin-right:4px}#__pv .s{opacity:.7;margin-right:4px}
#__pv button{background:#222;color:#eee;border:1px solid #444;border-radius:6px;padding:5px 10px;cursor:pointer;font:13px system-ui,sans-serif}
#__pv button.on{background:#3b82f6;border-color:#3b82f6}
</style>
<div id="__pv"><b>PREVIEW</b><span class="s">mock-дані (сканування фейкове) · режим:</span></div>
<script>
(function(){
  var scenario='sta';
  var scn=[['sta','Підключено (STA)'],['ap','Точка доступу (AP)']];
  function scanData(){
    var ap=(scenario==='ap');
    return { ap:ap, stored:!ap, scanning:false,
      cur: ap ? '' : 'MyHomeWiFi_5G',
      mh:'192.168.0.198', mt:1883, mu:'mqtt_user',
      nets:[ {ssid:'MyHomeWiFi_5G',rssi:-48,enc:true},
             {ssid:'MyHomeWiFi',rssi:-53,enc:true},
             {ssid:'Office-Guest',rssi:-67,enc:false},
             {ssid:'Neighbor_2.4',rssi:-74,enc:true},
             {ssid:'Starlink',rssi:-82,enc:true} ] };
  }
  function resp(obj){ return Promise.resolve({ ok:true, status:200,
    json:function(){return Promise.resolve(obj);},
    text:function(){return Promise.resolve(JSON.stringify(obj));} }); }
  var of=window.fetch;
  window.fetch=function(url,opts){ url=String(url);
    if(url.indexOf('/wifi/scan')>=0)   return resp(scanData());
    if(url.indexOf('/wifi/save')>=0)   return resp({ok:true});
    if(url.indexOf('/wifi/forget')>=0) return resp({ok:true});
    return of?of.apply(this,arguments):resp({}); };
  function mk(){ var bar=document.getElementById('__pv'); if(!bar) return;
    scn.forEach(function(p){ var b=document.createElement('button'); b.textContent=p[1];
      if(p[0]===scenario) b.className='on';
      b.onclick=function(){ scenario=p[0];
        Array.prototype.forEach.call(bar.querySelectorAll('button'),function(x){x.className='';});
        b.className='on'; try{ if(window.scan) window.scan(); }catch(e){} };
      bar.appendChild(b); }); }
  if(document.readyState!=='loading') mk(); else document.addEventListener('DOMContentLoaded',mk);
})();
</script>
<!-- ===== /PREVIEW SHIM wifi ===== -->
'''
def build_wifi():
    with io.open(WIFI_HEADER, 'r', encoding='utf-8') as f:
        html = extract(f.read())
    if html is None:
        raise RuntimeError('no raw string in wifi_html.h')
    new, n = re.subn(r'<body[^>]*>', lambda mm: mm.group(0) + '\n' + SHIM_WIFI, html, count=1, flags=re.IGNORECASE)
    if n == 0:
        new = SHIM_WIFI + html
    if not os.path.isdir(OUTDIR):
        os.makedirs(OUTDIR)
    with io.open(WIFI_OUT, 'w', encoding='utf-8') as f:
        f.write(new)
    return WIFI_OUT
try:
    print('[gen_preview] OK -> ' + build_wifi())
except Exception as e:
    sys.stderr.write('[gen_preview] УВАГА (wifi): ' + str(e) + '\n')
