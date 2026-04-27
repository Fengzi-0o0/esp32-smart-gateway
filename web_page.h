#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

static const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-S3 Config</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh}
.hdr{background:linear-gradient(135deg,#4f46e5,#7c3aed);padding:18px 20px 40px;text-align:center;position:relative}
.hdr h1{font-size:18px;font-weight:700;letter-spacing:1px}
.hsub{display:flex;align-items:center;justify-content:center;gap:8px;margin-top:4px}
.hsub p{font-size:11px;opacity:.75}
.abtn{background:rgba(255,255,255,.15);border:1px solid rgba(255,255,255,.3);color:#fff;width:20px;height:20px;border-radius:50%;font-size:10px;cursor:pointer;display:inline-flex;align-items:center;justify-content:center;transition:.2s;line-height:1}
.abtn:hover{background:rgba(255,255,255,.35)}
.lbtn{background:rgba(255,255,255,.15);border:1px solid rgba(255,255,255,.3);color:#fff;padding:5px 12px;border-radius:20px;font-size:11px;cursor:pointer;transition:.2s;backdrop-filter:blur(4px)}
.lbtn:hover{background:rgba(255,255,255,.3)}
.nv{display:flex;background:#1e293b;border-bottom:1px solid #334155;overflow-x:auto}
.nv button{flex:1;min-width:60px;padding:12px 6px;background:none;border:none;color:#94a3b8;font-size:12px;cursor:pointer;border-bottom:2px solid transparent;transition:.2s;white-space:nowrap}
.nv button.active{color:#a78bfa;border-bottom-color:#7c3aed}
.nv button:hover{color:#e2e8f0}
.ct{max-width:560px;margin:0 auto;padding:16px}
.pn{display:none}
.pn.active{display:block}
.cd{background:#1e293b;border-radius:10px;padding:18px;margin-bottom:14px;border:1px solid #334155}
.cd h3{font-size:13px;color:#a78bfa;margin-bottom:14px;padding-bottom:8px;border-bottom:1px solid #334155}
label{display:block;font-size:11px;color:#94a3b8;margin-bottom:3px;margin-top:10px}
label:first-child{margin-top:0}
input,select{width:100%;padding:9px 11px;background:#0f172a;border:1px solid #475569;border-radius:6px;color:#e2e8f0;font-size:13px;outline:none;transition:.2s}
input:focus,select:focus{border-color:#7c3aed}
input::placeholder{color:#475569}
.rw{display:flex;gap:8px;align-items:end}
.rw input{flex:1}
.bt{display:inline-block;padding:9px 14px;border:none;border-radius:6px;font-size:12px;font-weight:600;cursor:pointer;transition:.2s}
.bp{background:#4f46e5;color:#fff}
.bp:hover{background:#6366f1}
.bd{background:#dc2626;color:#fff}
.bd:hover{background:#ef4444}
.bs{background:#059669;color:#fff}
.bs:hover{background:#10b981}
.bb{background:#475569;color:#fff}
.bb:hover{background:#64748b}
.sb{padding:16px 0}
.ti{display:flex;align-items:center;gap:8px;padding:8px 0;border-bottom:1px solid #1e293b}
.ti:last-child{border-bottom:none}
.ti .nf{flex:1;font-size:13px;overflow:hidden}
.ti .nf small{color:#64748b;font-size:10px}
.tt{position:fixed;top:16px;left:50%;transform:translateX(-50%);padding:10px 22px;border-radius:8px;font-size:13px;font-weight:600;z-index:999;opacity:0;transition:.3s;pointer-events:none}
.tt.show{opacity:1}
.tt.ok{background:#059669;color:#fff}
.tt.er{background:#dc2626;color:#fff}
.tr{display:flex;align-items:center;justify-content:space-between;margin-top:14px;padding:10px 0;border-top:1px solid #334155}
.tl{font-size:12px;color:#cbd5e1}
.td{font-size:10px;color:#64748b;margin-top:2px}
.tg{position:relative;width:44px;height:24px;flex-shrink:0;cursor:pointer}
.tg input{opacity:0;width:0;height:0}
.tg .sl{position:absolute;inset:0;background:#475569;border-radius:24px;transition:.3s}
.tg .sl:before{content:'';position:absolute;width:18px;height:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
.tg input:checked+.sl{background:#7c3aed}
.tg input:checked+.sl:before{transform:translateX(20px)}
.mqtt-switch{background:#1e293b;border-radius:10px;padding:16px 18px;margin-bottom:14px;border:1px solid #334155;display:flex;align-items:center;justify-content:space-between}
.mqtt-switch .ms-info .ms-label{font-size:13px;color:#cbd5e1}
.mqtt-switch .ms-info .ms-desc{font-size:10px;color:#64748b;margin-top:2px}
.abp{display:none;min-height:100vh;background:#0f172a}
.abp.active{display:block}
.ahd{background:linear-gradient(135deg,#4f46e5,#7c3aed);padding:18px 20px;display:flex;align-items:center;gap:12px}
.akb{background:rgba(255,255,255,.15);border:1px solid rgba(255,255,255,.3);color:#fff;width:32px;height:32px;border-radius:8px;font-size:16px;cursor:pointer;display:flex;align-items:center;justify-content:center;transition:.2s}
.st{text-align:center;padding:8px 0;font-size:11px;color:#64748b}
.akb:hover{background:rgba(255,255,255,.35)}
.aht h2{font-size:16px;font-weight:700;letter-spacing:1px}
.aht p{font-size:10px;opacity:.7;margin-top:2px}
.aby{max-width:560px;margin:0 auto;padding:16px}
.asec{margin-bottom:16px}
.asec:last-child{margin-bottom:0}
.asec h4{font-size:13px;color:#a78bfa;margin-bottom:6px;padding-left:10px;border-left:3px solid #7c3aed}
.asec p{font-size:12px;color:#cbd5e1;line-height:1.75;padding:0 0 0 10px;margin-bottom:2px}
.adiv{border:none;border-top:1px solid #334155;margin:14px 0}
.aft{text-align:center;font-size:10px;color:#475569;margin-top:20px;padding-bottom:30px}
.otl{display:block;text-align:center;padding:12px;background:#1e293b;border-radius:8px;color:#a78bfa;font-size:13px;text-decoration:none;border:1px solid #334155;margin-top:10px}
.otl:hover{border-color:#7c3aed}
</style>
</head>
<body>
<div id="mp">
<div class="hdr">
      <h1>ESP32-S3 Smart Gateway <span id="fwVer" style="font-size:10px;opacity:.4;font-weight:400;vertical-align:bottom"></span></h1>
    <div class="hsub">

    <p id="stl">Device Configuration Portal</p>
    <button class="abtn" onclick="showAbout()" title="About">i</button>
  </div>
  <div style="position:absolute;bottom:10px;right:16px;display:flex;gap:6px;align-items:center">
    <a href="/monitor" class="lbtn" style="text-decoration:none;min-width:46px;text-align:center" id="monBtn">Monitor</a>
    <a href="/builder" class="lbtn" style="text-decoration:none;min-width:46px;text-align:center" id="builderBtn">Builder</a>
    <button class="lbtn" style="min-width:28px;text-align:center" onclick="toggleLang()" id="lbtn">EN</button>
  </div>



</div>
<div class="nv" id="nb">
  <button class="active" onclick="showTab('wifi',this)" id="tb0">WiFi</button>
  <button onclick="showTab('mqtt',this)" id="tb1">MQTT</button>
  <button onclick="showTab('sub',this)" id="tb2">Sub</button>
  <button onclick="showTab('pub',this)" id="tb3">Pub</button>
</div>
<div class="ct">
<div id="wifi" class="pn active">
  <div class="cd">
    <h3 id="t1">STA WiFi</h3>
    <label id="l1">Device Name</label>
    <input id="dn" placeholder="Living Room Light">
    <div class="sb">
      <button class="bt bs" onclick="saveDeviceName()" style="width:100%" id="b2n">Save Device Name</button>
    </div>
    <label id="l2">SSID</label>
    <div class="rw">
      <input id="ss" placeholder="WiFi name">
      <button class="bt bb" onclick="scanWifi()" style="margin-bottom:0" id="b1">Scan</button>
    </div>
    <div id="sl" style="margin-top:6px"></div>
    <label id="l3">Password</label>
    <input id="sp" type="password" placeholder="WiFi password">
    <div class="tr">
      <div>
        <div class="tl" id="gh">Hide Device AP</div>
        <div class="td" id="gd">Device AP is hidden</div>
      </div>
      <label class="tg">
        <input type="checkbox" id="ah">
        <span class="sl"></span>
      </label>
    </div>
  </div>
  <div class="sb">
    <button class="bt bp" onclick="saveWifi()" style="width:100%" id="b2">Save &amp; Reconnect</button>
  </div>
  <a href="/ota" class="otl" id="otl">Firmware Update (OTA)</a>
</div>


<div id="mqtt" class="pn">
  <div class="cd">
    <h3 id="t2">MQTT Server</h3>
    <label id="l4">Server Domain</label>
    <input id="mh" placeholder="example.emqxsl.cn">
    <label id="l5">SSL Port</label>
    <input id="mpo" type="number" placeholder="8883">
    <label id="l6">Username</label>
    <input id="mu" placeholder="MQTT username">
    <label id="l7">Password</label>
    <input id="mx" type="password" placeholder="MQTT password">
    <label id="l8">Client ID</label>
    <input id="mc" placeholder="Unique client ID">
    <div style="margin-top:12px">
      <label style="display:inline-flex;align-items:center;gap:6px;cursor:pointer">
        <input type="checkbox" id="ms" checked>
        <span id="l9">Enable SSL/TLS</span>
      </label>
    </div>
  </div>
  <div class="mqtt-switch">
    <div class="ms-info">
      <div class="ms-label" id="msl">MQTT 连接</div>
      <div class="ms-desc" id="msd">关闭后设备不连接 MQTT 服务器</div>
    </div>
    <label class="tg">
      <input type="checkbox" id="me" checked onchange="toggleMqtt()">
      <span class="sl"></span>
    </label>
  </div>
  <div class="sb">
    <button class="bt bp" onclick="saveMqtt()" style="width:100%" id="b3">Save MQTT</button>
  </div>
</div>



<div id="sub" class="pn">
  <div class="cd">
    <h3 id="t3">Add Subscribe Topic</h3>
    <label>Topic</label>
    <input id="ns" placeholder="device/+/control">
    <label>QoS</label>
    <select id="nq">
      <option value="0">0 - At most once</option>
      <option value="1">1 - At least once</option>
      <option value="2">2 - Exactly once</option>
    </select>
    <div style="margin-top:12px">
      <button class="bt bs" onclick="addSub()" id="b4">+ Add Topic</button>
    </div>
  </div>
  <div class="cd">
    <h3 id="t4">Current Subscriptions</h3>
    <div id="sL"></div>
  </div>
  <div class="sb">
    <button class="bt bp" onclick="saveTopics()" style="width:100%" id="b5">Save All Topics</button>
  </div>
</div>
<div id="pub" class="pn">
  <div class="cd">
    <h3 id="t5">Add Publish Topic</h3>
    <label>Topic</label>
    <input id="np" placeholder="device/status">
    <label>QoS</label>
    <select id="pq">
      <option value="0">0 - At most once</option>
      <option value="1">1 - At least once</option>
      <option value="2">2 - Exactly once</option>
    </select>
    <div style="margin-top:12px">
      <button class="bt bs" onclick="addPub()" id="b6">+ Add Topic</button>
    </div>
  </div>
  <div class="cd">
    <h3 id="t6">Current Publish Topics</h3>
    <div id="pL"></div>
  </div>
  <div class="sb">
    <button class="bt bp" onclick="saveTopics()" style="width:100%" id="b7">Save All Topics</button>
  </div>
</div>

</div>
<div id="toast" class="tt"></div>
<div class="st" id="di"></div>
</div>
<div id="abp" class="abp">
<div class="ahd">
  <button class="akb" onclick="hideAbout()">&#8592;</button>
  <div class="aht">
    <h2 id="a1">Device Declaration</h2>
    <p id="a2">Firmware developer declaration</p>
  </div>
</div>
<div class="aby">
  <div class="asec">
    <h4 id="a3">Product Info</h4>
    <p id="p1">Product Name: ESP32-S3 Smart Gateway</p>
    <p id="p2">Firmware Version: v1.2.0</p>
    <p id="p3">Manufacturer: Your Company Name</p>
  </div>
  <hr class="adiv">
  <div class="asec">
    <h4 id="a4">Copyright</h4>
    <p id="p4">2025 Your Company Name. All rights reserved.</p>
  </div>
  <hr class="adiv">
  <div class="asec">
    <h4 id="a5">Disclaimer</h4>
    <p id="p5">This device is provided as is without any express or implied warranties.</p>
  </div>
  <hr class="adiv">
  <div class="asec">
    <h4 id="a6">Contact</h4>
    <p id="p6">Email: your_email@example.com</p>
  </div>
  <div class="aft" id="a7">This declaration is editable only via source code.</div>
</div>
</div>
<script>
var cfg={};
var subTopics=[];
var pubTopics=[];
var loaded=false;
var lang=localStorage.getItem('lang')||'zh';
var fwVersion='v1.3.0'; 

function t(zh,en){return lang==='zh'?zh:en;}

function showAbout(){
  document.getElementById('mp').style.display='none';
  document.getElementById('abp').classList.add('active');
}

function hideAbout(){
  document.getElementById('abp').classList.remove('active');
  document.getElementById('mp').style.display='block';
}

function toggleLang(){
  lang=(lang==='zh')?'en':'zh';
  localStorage.setItem('lang',lang);
  applyLang();
  renderSubs();
  renderPubs();
  loadDeviceInfo();
}

function applyLang(){
  document.getElementById('stl').textContent=t('设备配置门户','Device Configuration Portal');
  document.getElementById('lbtn').textContent=lang==='zh'?'EN':'中文';
  document.getElementById('tb0').textContent=t('WiFi','WiFi');
  document.getElementById('tb1').textContent=t('MQTT','MQTT');
  document.getElementById('tb2').textContent=t('订阅','Sub');
  document.getElementById('tb3').textContent=t('发布','Pub');

  document.getElementById('t1').textContent=t('STA WiFi 连接','STA WiFi Connection');
  document.getElementById('l1').textContent=t('设备名称','Device Name');
  document.getElementById('dn').placeholder=t('例如 客厅灯','e.g. Living Room Light');
  document.getElementById('b2n').textContent=t('保存设备名称','Save Device Name');
  document.getElementById('l2').textContent=t('WiFi 名称','SSID');
  document.getElementById('ss').placeholder=t('请输入 WiFi 名称','Enter WiFi name');
  document.getElementById('b1').textContent=t('扫描','Scan');
  document.getElementById('l3').textContent=t('WiFi 密码','Password');
  document.getElementById('sp').placeholder=t('请输入 WiFi 密码','Enter WiFi password');
  document.getElementById('b2').textContent=t('保存并重新连接','Save & Reconnect');
  document.getElementById('otl').textContent=t('固件升级 (OTA)','Firmware Update (OTA)');
  var c=document.getElementById('ah');
  if(c.checked){
    document.getElementById('gh').textContent=t('隐藏设备热点','Hide Device AP');
    document.getElementById('gd').textContent=t('隐藏后需手动搜索WiFi','Device AP hidden from list');
  }else{
    document.getElementById('gh').textContent=t('显示设备热点','Show Device AP');
    document.getElementById('gd').textContent=t('设备热点可见','Device AP visible');
  }
  document.getElementById('t2').textContent=t('MQTT 服务器','MQTT Server');
  document.getElementById('l4').textContent=t('服务器域名','Server Domain');
  document.getElementById('mh').placeholder=t('例如 example.emqxsl.cn','e.g. example.emqxsl.cn');
  document.getElementById('l5').textContent=t('SSL 端口','SSL Port');
  document.getElementById('mpo').placeholder=t('例如 8883','e.g. 8883');
  document.getElementById('l6').textContent=t('用户名','Username');
  document.getElementById('mu').placeholder=t('请输入 MQTT 用户名','Enter MQTT username');
  document.getElementById('l7').textContent=t('密码','Password');
  document.getElementById('mx').placeholder=t('请输入 MQTT 密码','Enter MQTT password');
  document.getElementById('l8').textContent=t('客户端 ID','Client ID');
  document.getElementById('mc').placeholder=t('请输入客户端ID','Enter client ID');
  document.getElementById('l9').textContent=t('启用 SSL/TLS 加密','Enable SSL/TLS Encryption');
  document.getElementById('b3').textContent=t('保存 MQTT 配置','Save MQTT Config');
  document.getElementById('msl').textContent=t('MQTT 连接','MQTT Connection');
  document.getElementById('msd').textContent=t('关闭后设备不连接 MQTT 服务器','When off, device will not connect to MQTT server');
  document.getElementById('t3').textContent=t('添加订阅主题','Add Subscribe Topic');
  document.getElementById('ns').placeholder=t('例如 device/+/control','e.g. device/+/control');
  document.getElementById('b4').textContent=t('+ 添加主题','+ Add Topic');
  document.getElementById('t4').textContent=t('当前订阅列表','Current Subscriptions');
  document.getElementById('b5').textContent=t('保存所有主题','Save All Topics');
  document.getElementById('t5').textContent=t('添加发布主题','Add Publish Topic');
  document.getElementById('np').placeholder=t('例如 device/status','e.g. device/status');
  document.getElementById('b6').textContent=t('+ 添加主题','+ Add Topic');
  document.getElementById('t6').textContent=t('当前发布列表','Current Publish Topics');
  document.getElementById('b7').textContent=t('保存所有主题','Save All Topics');
  document.getElementById('a1').textContent=t('设备声明','Device Declaration');
  document.getElementById('a2').textContent=t('固件开发者声明','Firmware developer declaration');
document.getElementById('a3').textContent=t('产品信息','Product Info');
document.getElementById('p1').textContent=t('产品名称：ESP32-S3 智能网关毕设','Product Name: ESP32-S3 Smart Gateway Graduation Project');
document.getElementById('p2').textContent=t('固件版本：'+fwVersion,'Firmware Version: '+fwVersion);
document.getElementById('p3').textContent=t('制造商：邱烽全','Developer: Qiu Fengquan');
document.getElementById('a4').textContent=t('版权声明','Copyright');
document.getElementById('p4').textContent=t('2025 个人 保留所有权利。','2025 Qiu Fengquan. All rights reserved.');
document.getElementById('a5').textContent=t('免责声明','Disclaimer');
document.getElementById('p5').textContent=t('本设备以个人开发为主，AI为辅的模式，如有雷同纯属巧合','This device was primarily developed by an individual with AI assistance. Any resemblance to other works is purely coincidental.');
document.getElementById('a6').textContent=t('联系方式','Contact');
document.getElementById('p6').textContent=t('邮箱：2402605915@qq.com','Email: 2402605915@qq.com');
document.getElementById('a7').textContent=t('本声明内容由固件开发者编写，仅可通过修改源代码更改。','This declaration is written by the firmware developer and can only be modified through source code.');
document.getElementById('monBtn').textContent=t('监控','Monitor');
var bb=document.getElementById('builderBtn');
if(bb) bb.textContent=t('编辑器','Builder');






}

function showTab(id,btn){
  var ps=document.querySelectorAll('.pn');
  for(var i=0;i<ps.length;i++)ps[i].classList.remove('active');
  var bs=document.querySelectorAll('#nb button');
  for(var j=0;j<bs.length;j++)bs[j].classList.remove('active');
  var tg=document.getElementById(id);
  if(tg)tg.classList.add('active');
  if(btn)btn.classList.add('active');
}

function toast(msg,ok){
  var te=document.getElementById('toast');
  te.textContent=msg;
  te.className='tt show '+(ok?'ok':'er');
  setTimeout(function(){te.className='tt';},2500);
}

function loadConfig(){
  fetch('/api/config').then(function(r){
    if(!r.ok)throw new Error('HTTP '+r.status);
    return r.text();
  }).then(function(txt){
    if(!txt||txt.length<2)throw new Error('Empty response');
    var d=JSON.parse(txt);
    cfg=d;
    subTopics=[];
    pubTopics=[];
    if(d.subTopics&&d.subTopics.length){
      for(var i=0;i<d.subTopics.length;i++){
        subTopics.push({topic:d.subTopics[i].topic||'',qos:d.subTopics[i].qos||0});
      }
    }
    if(d.pubTopics&&d.pubTopics.length){
      for(var j=0;j<d.pubTopics.length;j++){
        pubTopics.push({topic:d.pubTopics[j].topic||'',qos:d.pubTopics[j].qos||0});
      }
    }
    document.getElementById('ss').value=d.staSsid||'';
    document.getElementById('sp').value=d.staPass||'';
    document.getElementById('mh').value=d.mqttHost||'';
    document.getElementById('mpo').value=d.mqttPort||8883;
    document.getElementById('mu').value=d.mqttUser||'';
    document.getElementById('mx').value=d.mqttPass||'';
    document.getElementById('mc').value=d.mqttClientId||'';
    document.getElementById('ms').checked=(d.mqttSsl!==false);
    document.getElementById('me').checked=(d.mqttEnabled!==false);
    document.getElementById('ah').checked=(d.apHidden!==false);
    document.getElementById('dn').value=d.deviceName||'';
    loaded=true;
    applyLang();
    renderSubs();
    renderPubs();
  }).catch(function(e){
    toast(t('加载配置失败: ','Load failed: ')+(e.message||''),false);
  });
}

function loadDeviceInfo(){
  fetch('/api/info').then(function(r){return r.json();}).then(function(d){
    var c=t('已连接','Conn');
    var dc=t('未连接','Disconn');
    document.getElementById('di').textContent='AP: '+(d.apIP||'-')+' | STA: '+(d.staIP||'-')+' | MQTT: '+((d.mqttStatus==='Connected')?c:dc);
    if(d.firmware){
      fwVersion='v'+d.firmware;
      document.getElementById('p2').textContent=t('固件版本：'+fwVersion,'Firmware Version: '+fwVersion);
      document.getElementById('fwVer').textContent=fwVersion;
    }
  }).catch(function(){});
}


function scanWifi(){
  toast(t('正在扫描...','Scanning...'),true);
  fetch('/api/scan').then(function(r){return r.json();}).then(function(list){
    var div=document.getElementById('sl');
    if(!list.length){
      div.innerHTML='<small style="color:#64748b">'+t('未发现可用网络','No networks')+'</small>';
      return;
    }
    var h='';
    for(var i=0;i<list.length;i++){
      var n=list[i];
      var es=n.ssid.replace(/'/g,"\\'");
      h+='<div style="padding:5px 8px;margin:2px 0;background:#0f172a;border-radius:4px;cursor:pointer;font-size:12px;display:flex;justify-content:space-between" onclick="pickSsid(\''+es+'\')"><span>'+n.ssid+'</span><span style="color:#64748b">'+n.rssi+' dBm</span></div>';
    }
    div.innerHTML=h;
  }).catch(function(){toast(t('扫描失败','Scan failed'),false);});
}

function pickSsid(v){
  document.getElementById('ss').value=v;
  document.getElementById('sl').innerHTML='';
}

function saveDeviceName(){
  var name=document.getElementById('dn').value.trim();
  fetch('/api/device',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({deviceName:name})
  }).then(function(r){return r.json();}).then(function(d){
    if(d.ok)toast(t('设备名称已保存','Device name saved'),true);
    else toast(t('保存失败','Save failed'),false);
  }).catch(function(){toast(t('保存失败','Save failed'),false);});
}

function postCfg(){
  if(!loaded){
    toast(t('配置未加载，请刷新页面','Config not loaded, please refresh'),false);
    return Promise.reject(new Error('not loaded'));
  }
  var body={};
  var ssid=document.getElementById('ss').value.trim();
  if(ssid)body.staSsid=ssid;
  body.staPass=document.getElementById('sp').value;
  var host=document.getElementById('mh').value.trim();
  if(host)body.mqttHost=host;
  body.mqttPort=parseInt(document.getElementById('mpo').value)||8883;
  body.mqttUser=document.getElementById('mu').value.trim();
  body.mqttPass=document.getElementById('mx').value;
  body.mqttClientId=document.getElementById('mc').value.trim();
  body.mqttSsl=document.getElementById('ms').checked;
  body.mqttEnabled=document.getElementById('me').checked;
  body.deviceName=document.getElementById('dn').value.trim();
  body.apHidden=document.getElementById('ah').checked;
  body.subTopics=subTopics;
  body.pubTopics=pubTopics;
  return fetch('/api/config',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(body)
  }).then(function(r){return r.json();});
}

function saveWifi(){
  postCfg().then(function(d){
    if(d.ok)toast(t('已保存！正在重新连接...','Saved! Reconnecting...'),true);
    else toast(t('保存失败','Save failed'),false);
  }).catch(function(e){
    if(e.message!=='not loaded')toast(t('保存失败','Save failed'),false);
  });
}

function saveMqtt(){
  postCfg().then(function(d){
    if(d.ok)toast(t('MQTT 配置已保存！','MQTT saved!'),true);
    else toast(t('保存失败','Save failed'),false);
  }).catch(function(e){
    if(e.message!=='not loaded')toast(t('保存失败','Save failed'),false);
  });
}

function addSub(){
  var v=document.getElementById('ns').value.trim();
  if(!v){toast(t('主题不能为空','Topic empty'),false);return;}
  if(v=='esp32/ota'){toast(t('该主题为系统保留，用于MQTT OTA升级','Reserved topic for MQTT OTA'),false);return;}
  subTopics.push({topic:v,qos:parseInt(document.getElementById('nq').value)});
  document.getElementById('ns').value='';
  renderSubs();
}

function removeSub(i){
  subTopics.splice(i,1);
  renderSubs();
}

function renderSubs(){
  var d=document.getElementById('sL');
  if(!subTopics.length){
    d.innerHTML='<div style="color:#64748b;font-size:12px;padding:8px 0">'+t('暂无订阅主题','No subscriptions')+'</div>';
    return;
  }
  var h='';
  for(var i=0;i<subTopics.length;i++){
    var tp=subTopics[i];
    h+='<div class="ti"><div class="nf"><div>'+tp.topic+'</div><small>QoS '+tp.qos+'</small></div><button class="bt bd" onclick="removeSub('+i+')">'+t('删除','Del')+'</button></div>';
  }
  d.innerHTML=h;
}

function addPub(){
  var v=document.getElementById('np').value.trim();
  if(!v){toast(t('主题不能为空','Topic empty'),false);return;}
  if(v=='esp32/ota'){toast(t('该主题为系统保留，用于MQTT OTA升级','Reserved topic for MQTT OTA'),false);return;}
  pubTopics.push({topic:v,qos:parseInt(document.getElementById('pq').value)});
  document.getElementById('np').value='';
  renderPubs();
}

function removePub(i){
  pubTopics.splice(i,1);
  renderPubs();
}

function renderPubs(){
  var d=document.getElementById('pL');
  if(!pubTopics.length){
    d.innerHTML='<div style="color:#64748b;font-size:12px;padding:8px 0">'+t('暂无发布主题','No publish topics')+'</div>';
    return;
  }
  var h='';
  for(var i=0;i<pubTopics.length;i++){
    var tp=pubTopics[i];
    h+='<div class="ti"><div class="nf"><div>'+tp.topic+'</div><small>QoS '+tp.qos+'</small></div><button class="bt bd" onclick="removePub('+i+')">'+t('删除','Del')+'</button></div>';
  }
  d.innerHTML=h;
}

function saveTopics(){
  postCfg().then(function(d){
    if(d.ok)toast(t('主题已保存！','Topics saved!'),true);
    else toast(t('保存失败','Save failed'),false);
  }).catch(function(e){
    if(e.message!=='not loaded')toast(t('保存失败','Save failed'),false);
  });
}

function toggleMqtt(){
  var en=document.getElementById('me').checked;
  fetch('/api/config',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({mqttEnabled:en})
  }).then(function(r){return r.json();}).then(function(d){
    if(d.ok){
      toast(en?t('MQTT 已开启','MQTT enabled'):t('MQTT 已关闭','MQTT disabled'),true);
    }else{
      toast(t('保存失败','Save failed'),false);
    }
  }).catch(function(){toast(t('保存失败','Save failed'),false);});
}


document.addEventListener('DOMContentLoaded',function(){
  loadConfig();
  loadDeviceInfo();
  document.getElementById('ah').addEventListener('change',function(){
    if(!loaded)return;
    cfg.apHidden=document.getElementById('ah').checked;
    applyLang();
    postCfg().then(function(d){
      if(d.ok)toast(cfg.apHidden?t('热点已隐藏','AP hidden'):t('热点已显示','AP visible'),true);
    }).catch(function(){});
  });
});

setInterval(loadDeviceInfo,10000);
</script>
</body>
</html>
)rawliteral";
static const char OTA_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OTA</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#0f172a;color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center}
.cd{background:#1e293b;border-radius:12px;padding:30px;width:90%;max-width:420px;border:1px solid #334155;text-align:center}
.bk{display:inline-block;margin-bottom:16px;color:#94a3b8;font-size:13px;text-decoration:none}
.bk:hover{color:#a78bfa}
h2{font-size:18px;margin-bottom:6px;color:#a78bfa}
.sb{font-size:11px;color:#64748b;margin-bottom:16px}
input[type=file]{margin:12px 0;font-size:13px;color:#94a3b8}
.bt{display:inline-block;padding:10px 20px;background:#4f46e5;color:#fff;border:none;border-radius:8px;font-size:13px;font-weight:600;cursor:pointer;width:100%;margin-top:8px;transition:.2s}
.bt:hover{background:#6366f1}
.bt:disabled{background:#475569;cursor:not-allowed}
.bt.g{background:#059669}.bt.g:hover{background:#10b981}
#pg{margin-top:14px;display:none}
#br{height:6px;background:#475569;border-radius:3px;overflow:hidden}
#bf{height:100%;width:0%;background:#7c3aed;transition:width .3s}
#st{margin-top:8px;font-size:12px;color:#64748b}
.dv{border:none;border-top:1px solid #334155;margin:18px 0}
.info{font-size:10px;color:#475569;line-height:1.8}
#mq{margin-top:14px;display:none}
#mqp{height:6px;background:#475569;border-radius:3px;overflow:hidden}
#mf{height:100%;width:0%;background:#059669;transition:width .3s}
#mst{margin-top:6px;font-size:11px;color:#64748b}
.lbtn{position:absolute;top:16px;right:16px;background:rgba(255,255,255,.15);border:1px solid rgba(255,255,255,.3);color:#fff;padding:5px 12px;border-radius:20px;font-size:11px;cursor:pointer;transition:.2s}
.lbtn:hover{background:rgba(255,255,255,.3)}
</style>
</head>
<body>
<button class="lbtn" onclick="toggleLang()" id="lbtn">EN</button>
<div class="cd">
  <a href="/" class="bk" id="bk">&#8592; Back</a>
<a href="/monitor" class="bk" id="monLink2">&#8592; Monitor</a>

  <h2 id="h2t">Firmware Update (OTA)</h2>
  <div class="sb" id="fi">Loading...</div>
  <label id="lbl_fw" style="margin-top:0">固件文件 (.bin)</label>
  <input type="file" id="fw" accept=".bin">
  <button class="bt" id="ub" onclick="doUpload()">Upload via WebSocket</button>
  <div id="pg"><div id="br"><div id="bf"></div></div><div id="st"></div></div>
  <hr class="dv">
  <div id="mq">
    <div style="font-size:12px;color:#a78bfa;margin-bottom:6px" id="mqt">MQTT OTA Progress</div>
    <div id="mqp"><div id="mf"></div></div>
    <div id="mst"></div>
  </div>
  <div class="info">
    <span id="i1">WebSocket OTA: upload .bin file directly</span><br>
    <span id="i2">MQTT OTA: publish to "esp32/ota" topic</span><br>
    <span id="i3">Protocol: START, binary chunks, DONE</span>
  </div>
</div>
<script>
var lang=localStorage.getItem('lang')||'zh';
function t(zh,en){return lang==='zh'?zh:en;}
function toggleLang(){
  lang=(lang==='zh')?'en':'zh';
  localStorage.setItem('lang',lang);
  applyLang();
}
function applyLang(){
  document.getElementById('monLink2').textContent=t('← 系统监控','← Monitor');
  document.title=t('固件升级','Firmware Update');
  document.getElementById('lbtn').textContent=lang==='zh'?'EN':'中文';
  document.getElementById('bk').textContent=lang==='zh'?'返回':'Back';
  document.getElementById('h2t').textContent=t('固件升级 (OTA)','Firmware Update (OTA)');
  document.getElementById('lbl_fw').textContent=t('固件文件 (.bin)','Firmware File (.bin)');
  document.getElementById('ub').textContent=t('通过 WebSocket 上传','Upload via WebSocket');
  document.getElementById('mqt').textContent=t('MQTT OTA 进度','MQTT OTA Progress');
  document.getElementById('i1').textContent=t('WebSocket OTA: 直接上传 .bin 文件','WebSocket OTA: upload .bin file directly');
  document.getElementById('i2').textContent=t('MQTT OTA: 发送到固定主题 "esp32/ota"','MQTT OTA: publish to "esp32/ota" topic');
  document.getElementById('i3').textContent=t('协议: START, 二进制数据块, DONE','Protocol: START, binary chunks, DONE');
  var st=document.getElementById('st');
  var txt=st.textContent;
  if(txt.indexOf('Finalizing')>=0||txt.indexOf('完成')>=0){st.textContent=t('正在完成...','Finalizing...');}
  else if(txt.indexOf('Success')>=0||txt.indexOf('成功')>=0){st.textContent=t('成功！设备正在重启...','Success! Device restarting...');}
  else if(txt.indexOf('Wait')>=0||txt.indexOf('等待')>=0){st.textContent=t('等待10秒后刷新页面','Wait 10s then refresh page');}
  else if(txt.indexOf('Connecting')>=0||txt.indexOf('连接中')>=0){st.textContent=t('连接中...','Connecting...');}
  else if(txt.indexOf('Uploading')>=0||txt.indexOf('上传中')>=0){st.textContent=t('上传中...','Uploading...');}
  else if(txt.indexOf('Connection failed')>=0||txt.indexOf('连接失败')>=0){st.textContent=t('连接失败','Connection failed');}
  else if(txt.indexOf('Connection closed')>=0||txt.indexOf('连接已关闭')>=0){st.textContent=t('连接已关闭','Connection closed');}
  else if(txt.indexOf('Lost')>=0||txt.indexOf('丢失')>=0){st.textContent=t('连接丢失','Connection lost');}
  else if(txt.indexOf('Read error')>=0||txt.indexOf('读取错误')>=0){st.textContent=t('文件读取错误','File read error');}
  else if(txt.indexOf('Error')>=0||txt.indexOf('错误')>=0){st.textContent=t('错误: ','Error: ')+txt.replace(/^(错误: |Error: )/,'');}
  else if(txt.indexOf('Retry')>=0||txt.indexOf('重试')>=0){
    var ub=document.getElementById('ub');
    ub.textContent=t('重试','Retry');
  }
  else if(txt.indexOf('Done')>=0||txt.indexOf('完成')>=0){
    var ub2=document.getElementById('ub');
    ub2.textContent=t('完成','Done');
  }
}

fetch('/api/info').then(function(r){return r.json();}).then(function(d){
  var lang2=localStorage.getItem('lang')||'zh';
  var free=d.heapFree?Math.round(d.heapFree/1024)+'KB':'?';
  document.getElementById('fi').textContent=(lang2==='zh'?'固件版本：v':'Firmware: v')+(d.firmware||'?')+' | '+(lang2==='zh'?'剩余内存：':'Free: ')+free;
}).catch(function(){});

applyLang();

var ws=null;
var CHUNK=4096;

function doUpload(){
  var f=document.getElementById('fw').files[0];
  if(!f){alert(t('请选择固件文件','Select firmware file'));return;}
  if(!f.name.endsWith('.bin')){alert(t('仅支持 .bin 文件','Only .bin supported'));return;}
  var b=document.getElementById('ub');
  var pg=document.getElementById('pg');
  var bf=document.getElementById('bf');
  var st=document.getElementById('st');
  b.disabled=true;b.textContent=t('连接中...','Connecting...');pg.style.display='block';

  ws=new WebSocket('ws://'+location.hostname+':8080/');
  ws.onopen=function(){
    st.textContent=t('已连接，等待设备...','Connected, waiting for device...');
    st.style.color='#94a3b8';
  };
  ws.onmessage=function(ev){
    var msg=ev.data;
        if(msg==='CONNECTED'){
        st.textContent=t('设备就绪，启动OTA...','Device ready, starting OTA...');
        ws.send('START');
    }

    if(msg==='READY'){
      st.textContent=t('上传中...','Uploading...');
      st.style.color='#94a3b8';
      sendFile(f);
    }else if(msg==='OK'){
      bf.style.width='100%';
      st.textContent=t('成功！设备正在重启...','Success! Device restarting...');
      st.style.color='#10b981';
      b.textContent=t('完成','Done');
      setTimeout(function(){
        st.textContent=t('等待10秒后刷新页面','Wait 10s then refresh page');
      },3000);
    }else if(msg.indexOf('ERR')===0){
      st.textContent=t('错误: ','Error: ')+msg;
      st.style.color='#ef4444';
      b.disabled=false;b.textContent=t('重试','Retry');
      ws.close();
    }
  };
  ws.onerror=function(){
    st.textContent=t('连接失败','Connection failed');
    st.style.color='#ef4444';
    b.disabled=false;b.textContent=t('重试','Retry');
  };
  ws.onclose=function(){
    if(st.style.color!=='rgb(16, 185, 129)'){
      st.textContent=t('连接已关闭','Connection closed');
      b.disabled=false;b.textContent=t('重试','Retry');
    }
  };
}

function sendFile(file){
  var reader=new FileReader();
  var offset=0;
  var total=file.size;
  var bf=document.getElementById('bf');
  var st=document.getElementById('st');

  function sendNext(){
    if(offset>=total){ws.send('DONE');st.textContent=t('正在完成...','Finalizing...');return;}
    var end=Math.min(offset+CHUNK,total);
    reader.readAsArrayBuffer(file.slice(offset,end));
  }

  reader.onload=function(ev){
    if(ws.readyState!==WebSocket.OPEN){st.textContent=t('连接丢失','Connection lost');return;}
    ws.send(ev.target.result);
    offset+=ev.target.result.byteLength;
    var p=Math.round((offset/total)*100);
    bf.style.width=p+'%';
    st.textContent=p+'% ('+Math.round(offset/1024)+'KB / '+Math.round(total/1024)+'KB)';
    sendNext();
  };

  reader.onerror=function(){
    st.textContent=t('文件读取错误','File read error');
    st.style.color='#ef4444';
  };

  sendNext();
}
</script>
</body>
</html>
)rawliteral";

#endif
