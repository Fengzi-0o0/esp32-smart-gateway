static const char MONITOR_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>System Monitor - ESP32-S3</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600;700&family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{
  --bg:#080c18;--surface:#0d1321;--card:#111827;
  --border:#1a2540;--text:#e2e8f0;--muted:#5a6a8a;
  --cyan:#00d4ff;--green:#10b981;--amber:#f59e0b;--red:#ef4444;
}
body{
  font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);
  min-height:100vh;padding:16px;
  background-image:
    radial-gradient(ellipse at 20% 50%,rgba(0,212,255,.03),transparent 50%),
    radial-gradient(ellipse at 80% 20%,rgba(16,185,129,.03),transparent 50%);
}
.container{max-width:800px;margin:0 auto}
.hdr{
  display:flex;align-items:center;justify-content:space-between;
  padding:16px 20px;background:var(--card);border:1px solid var(--border);
  border-radius:12px;margin-bottom:14px;flex-wrap:wrap;gap:10px;
}
.hdr h1{font-size:16px;font-weight:700;letter-spacing:.5px}
.hdr .sub{font-size:11px;color:var(--muted);margin-top:2px}
.hdr .fw{
  display:inline-block;padding:2px 8px;border-radius:10px;font-size:10px;
  font-family:'JetBrains Mono',monospace;background:rgba(0,212,255,.1);
  border:1px solid rgba(0,212,255,.2);color:var(--cyan);margin-left:8px;
}
.hdr-r{display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.live{display:flex;align-items:center;gap:6px}
.live-dot{
  width:8px;height:8px;border-radius:50%;background:var(--green);
  animation:pulse 2s ease-in-out infinite;flex-shrink:0;
}
@keyframes pulse{0%,100%{opacity:1;box-shadow:0 0 4px var(--green)}50%{opacity:.3;box-shadow:none}}
.live-txt{font-size:10px;font-weight:600;color:var(--green);letter-spacing:1px}
.back{
  color:var(--muted);text-decoration:none;font-size:11px;
  padding:5px 12px;border:1px solid var(--border);border-radius:6px;transition:.2s;
}
.back:hover{border-color:var(--cyan);color:var(--cyan)}
.lbtn{
  background:rgba(255,255,255,.15);border:1px solid rgba(255,255,255,.3);
  color:#fff;padding:5px 12px;border-radius:20px;font-size:11px;cursor:pointer;
  transition:.2s;backdrop-filter:blur(4px);
}
.lbtn:hover{background:rgba(255,255,255,.3)}
.rate-sel{
  background:var(--surface);border:1px solid var(--border);color:var(--text);
  padding:4px 8px;border-radius:5px;font-size:10px;outline:none;cursor:pointer;
}
.g-row{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin-bottom:14px}
.g-card{
  background:var(--card);border:1px solid var(--border);border-radius:12px;
  padding:18px 8px 14px;text-align:center;transition:.2s;
}
.g-card:hover{border-color:rgba(0,212,255,.3);transform:translateY(-1px)}
.g-svg{width:100%;max-width:120px;margin:0 auto;display:block}
.g-bg{fill:none;stroke:var(--border);stroke-width:8}
.g-arc{
  fill:none;stroke:var(--cyan);stroke-width:8;stroke-linecap:round;
  stroke-dasharray:326.73;stroke-dashoffset:326.73;
  transform:rotate(-90deg);transform-origin:center;
  transition:stroke-dashoffset .8s cubic-bezier(.4,0,.2,1),stroke .5s ease;
  filter:drop-shadow(0 0 6px currentColor);
}
.g-val{
  font-family:'JetBrains Mono',monospace;font-size:22px;font-weight:700;
  fill:var(--text);transition:fill .5s ease;
}
.g-lbl{font-family:'Inter',sans-serif;font-size:10px;fill:var(--muted);font-weight:600;letter-spacing:1px}
.g-sub{
  font-family:'JetBrains Mono',monospace;font-size:10px;color:var(--muted);
  margin-top:8px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;
}
.chart-card{
  background:var(--card);border:1px solid var(--border);border-radius:12px;
  padding:16px;margin-bottom:14px;
}
.chart-hdr{
  display:flex;justify-content:space-between;align-items:center;
  margin-bottom:10px;font-size:12px;font-weight:600;
}
.chart-legend{display:flex;gap:14px;font-weight:400;font-size:11px;color:var(--muted)}
.leg-dot{
  display:inline-block;width:12px;height:3px;border-radius:2px;margin-right:4px;vertical-align:middle;
}
#chart{width:100%;height:180px;display:block;border-radius:8px}
.s-row{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin-bottom:14px}
.s-card{
  background:var(--card);border:1px solid var(--border);border-radius:10px;
  padding:14px;transition:.2s;
}
.s-card:hover{border-color:rgba(0,212,255,.2)}
.s-icon{font-size:20px;margin-bottom:8px}
.s-lbl{font-size:10px;color:var(--muted);font-weight:600;letter-spacing:.5px;text-transform:uppercase}
.s-val{font-family:'JetBrains Mono',monospace;font-size:13px;margin-top:4px;font-weight:600}
.s-dot{
  display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px;vertical-align:middle;
}
.sig-bars{display:flex;gap:3px;align-items:flex-end;height:18px;margin-top:8px}
.sig-bar{width:5px;border-radius:1px;background:var(--border);transition:background .3s}
.sig-bar.on{background:var(--green)}
.sig-bar:nth-child(1){height:5px}
.sig-bar:nth-child(2){height:9px}
.sig-bar:nth-child(3){height:13px}
.sig-bar:nth-child(4){height:18px}
.d-card{
  background:var(--card);border:1px solid var(--border);border-radius:12px;
  padding:16px;margin-bottom:14px;
}
.d-card h3{
  font-size:12px;color:var(--cyan);margin-bottom:12px;
  padding-bottom:8px;border-bottom:1px solid var(--border);
  font-weight:600;letter-spacing:.5px;
}
.d-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
.d-item{
  background:var(--surface);border-radius:8px;padding:10px 12px;
  border:1px solid transparent;transition:.2s;
}
.d-item:hover{border-color:var(--border)}
.d-lbl{font-size:10px;color:var(--muted);display:block;margin-bottom:4px}
.d-val{font-family:'JetBrains Mono',monospace;font-size:13px;font-weight:600}
.d-val.sm{font-size:10px}
.frag-bar{height:5px;background:var(--border);border-radius:3px;overflow:hidden;margin-top:6px}
.frag-fill{height:100%;border-radius:3px;transition:width .6s ease,background .5s ease}
.ftr{
  text-align:center;font-size:10px;color:#2a3550;padding:14px 0;
  display:flex;justify-content:space-between;
}
@media(max-width:600px){
  .g-row,.s-row{grid-template-columns:repeat(2,1fr)}
  .d-grid{grid-template-columns:repeat(2,1fr)}
  .d-item.wide{grid-column:span 2}
  .hdr{flex-direction:column;text-align:center}
  .hdr-r{justify-content:center}
}
</style>
</head>
<body>
<div class="container">

<div class="hdr">
  <div>
    <h1 id="hTitle">System Monitor</h1>
    <div class="sub">
      <span id="devName">--</span>
      <span class="fw" id="fwVer">--</span>
    </div>
  </div>
  <div class="hdr-r">
    <a href="/" class="back" id="bkLink">← Config</a>
    <div class="live">
      <span class="live-dot"></span>
      <span class="live-txt">LIVE</span>
    </div>
    <select class="rate-sel" id="rateSel" onchange="setRate(this.value)">
      <option value="1000">1s</option>
      <option value="2000" selected>2s</option>
      <option value="5000">5s</option>
      <option value="10000">10s</option>
    </select>
    <button class="lbtn" onclick="toggleLang()" id="lbtn">EN</button>
  </div>
</div>

<!-- Gauges -->
<div class="g-row">
  <div class="g-card">
    <svg class="g-svg" viewBox="0 0 120 120">
      <circle cx="60" cy="60" r="52" class="g-bg"/>
      <circle id="g-heap" cx="60" cy="60" r="52" class="g-arc"/>
      <text id="v-heap" x="60" y="55" text-anchor="middle" class="g-val">--</text>
      <text x="60" y="72" text-anchor="middle" class="g-lbl">HEAP</text>
    </svg>
    <div class="g-sub" id="s-heap">-- / --</div>
  </div>
  <div class="g-card">
    <svg class="g-svg" viewBox="0 0 120 120">
      <circle cx="60" cy="60" r="52" class="g-bg"/>
      <circle id="g-psram" cx="60" cy="60" r="52" class="g-arc"/>
      <text id="v-psram" x="60" y="55" text-anchor="middle" class="g-val">--</text>
      <text x="60" y="72" text-anchor="middle" class="g-lbl">PSRAM</text>
    </svg>
    <div class="g-sub" id="s-psram">-- / --</div>
  </div>
  <div class="g-card">
    <svg class="g-svg" viewBox="0 0 120 120">
      <circle cx="60" cy="60" r="52" class="g-bg"/>
      <circle id="g-flash" cx="60" cy="60" r="52" class="g-arc"/>
      <text id="v-flash" x="60" y="55" text-anchor="middle" class="g-val">--</text>
      <text x="60" y="72" text-anchor="middle" class="g-lbl" id="gSketch">SKETCH</text>
    </svg>
    <div class="g-sub" id="s-flash">-- / --</div>
  </div>
  <div class="g-card">
    <svg class="g-svg" viewBox="0 0 120 120">
      <circle cx="60" cy="60" r="52" class="g-bg"/>
      <circle id="g-sig" cx="60" cy="60" r="52" class="g-arc"/>
      <text id="v-sig" x="60" y="55" text-anchor="middle" class="g-val">--</text>
      <text x="60" y="72" text-anchor="middle" class="g-lbl">SIGNAL</text>
    </svg>
    <div class="g-sub" id="s-sig">-- dBm</div>
  </div>
</div>

<!-- Chart -->
<div class="chart-card">
  <div class="chart-hdr">
    <span id="chartTitle">Memory Usage Trend</span>
    <div class="chart-legend">
      <span><span class="leg-dot" style="background:var(--cyan)"></span><span id="legHeap">Heap</span></span>
      <span><span class="leg-dot" style="background:var(--green)"></span><span id="legPsram">PSRAM</span></span>
    </div>
  </div>
  <canvas id="chart"></canvas>
</div>

<!-- Status Cards -->
<div class="s-row">
  <div class="s-card">
    <div class="s-icon">📶</div>
    <div class="s-lbl">WiFi</div>
    <div class="s-val" id="wifiVal">--</div>
    <div class="sig-bars">
      <div class="sig-bar"></div><div class="sig-bar"></div>
      <div class="sig-bar"></div><div class="sig-bar"></div>
    </div>
  </div>
  <div class="s-card">
    <div class="s-icon">📡</div>
    <div class="s-lbl">MQTT</div>
    <div class="s-val" id="mqttVal">--</div>
  </div>
  <div class="s-card">
    <div class="s-icon">⏱</div>
    <div class="s-lbl" id="lblUptime">Uptime</div>
    <div class="s-val" id="uptimeVal">--</div>
  </div>
  <div class="s-card">
    <div class="s-icon">⚙</div>
    <div class="s-lbl" id="lblTasks">Tasks</div>
    <div class="s-val" id="tasksVal">--</div>
  </div>
</div>

<!-- Memory Detail -->
<div class="d-card">
  <h3 id="memTitle">Memory Detail</h3>
  <div class="d-grid">
    <div class="d-item">
      <span class="d-lbl" id="lblHfree">Heap Free</span>
      <span class="d-val" id="d-hfree">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblHmin">Heap Min Free (Watermark)</span>
      <span class="d-val" id="d-hmin">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblHalloc">Heap Max Alloc Block</span>
      <span class="d-val" id="d-halloc">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblPfree">PSRAM Free</span>
      <span class="d-val" id="d-pfree">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblPmin">PSRAM Min Free</span>
      <span class="d-val" id="d-pmin">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblFrag">Heap Fragmentation</span>
      <span class="d-val" id="d-frag">--</span>
      <div class="frag-bar"><div class="frag-fill" id="fragFill"></div></div>
    </div>
  </div>
</div>

<!-- System Info -->
<div class="d-card">
  <h3 id="sysTitle">System Info</h3>
  <div class="d-grid">
    <div class="d-item">
      <span class="d-lbl" id="lblChip">Chip</span>
      <span class="d-val" id="d-chip">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblCpu">CPU Frequency</span>
      <span class="d-val" id="d-cpu">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblFlash">Flash Size</span>
      <span class="d-val" id="d-flash">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblSketch">Sketch Size</span>
      <span class="d-val" id="d-sketch">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblSkfree">Sketch Free Space</span>
      <span class="d-val" id="d-skfree">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl">Device ID</span>
      <span class="d-val sm" id="d-devid">--</span>
    </div>
  </div>
</div>

<!-- Network Detail -->
<div class="d-card">
  <h3 id="netTitle">Network Detail</h3>
  <div class="d-grid">
    <div class="d-item">
      <span class="d-lbl" id="lblStaIp">STA IP</span>
      <span class="d-val" id="d-staip">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblApIp">AP IP</span>
      <span class="d-val" id="d-apip">--</span>
    </div>
    <div class="d-item">
      <span class="d-lbl" id="lblRssi">WiFi RSSI</span>
      <span class="d-val" id="d-rssi">--</span>
    </div>
  </div>
</div>

<div class="ftr">
  <span><span id="lblLast">Last update</span>: <span id="ts">--</span></span>
  <span>ESP32-S3 Smart Gateway · Monitor</span>
</div>

</div>

<script>
var C=326.73,MX=60,hA=[],pA=[],tm=null,rt=2000;
var lang=localStorage.getItem('lang')||'zh';

function t(zh,en){return lang==='zh'?zh:en;}

function applyLang(){
  document.title=t('系统监控 - ESP32-S3','System Monitor - ESP32-S3');
  document.getElementById('hTitle').textContent=t('系统监控','System Monitor');
  document.getElementById('bkLink').textContent=t('← 配置','← Config');
  document.getElementById('lbtn').textContent=lang==='zh'?'EN':'中文';
  document.getElementById('gSketch').textContent=t('固件','SKETCH');
  document.getElementById('chartTitle').textContent=t('内存使用趋势','Memory Usage Trend');
  document.getElementById('legHeap').textContent=t('堆内存','Heap');
  document.getElementById('legPsram').textContent=t('PSRAM','PSRAM');
  document.getElementById('lblUptime').textContent=t('运行时间','Uptime');
  document.getElementById('lblTasks').textContent=t('任务','Tasks');
  document.getElementById('memTitle').textContent=t('内存详情','Memory Detail');
  document.getElementById('lblHfree').textContent=t('堆空闲','Heap Free');
  document.getElementById('lblHmin').textContent=t('堆最低水位 (历史最低)','Heap Min Free (Watermark)');
  document.getElementById('lblHalloc').textContent=t('堆最大连续块','Heap Max Alloc Block');
  document.getElementById('lblPfree').textContent=t('PSRAM 空闲','PSRAM Free');
  document.getElementById('lblPmin').textContent=t('PSRAM 最低水位','PSRAM Min Free');
  document.getElementById('lblFrag').textContent=t('堆碎片率','Heap Fragmentation');
  document.getElementById('sysTitle').textContent=t('系统信息','System Info');
  document.getElementById('lblChip').textContent=t('芯片','Chip');
  document.getElementById('lblCpu').textContent=t('CPU 频率','CPU Frequency');
  document.getElementById('lblFlash').textContent=t('Flash 容量','Flash Size');
  document.getElementById('lblSketch').textContent=t('固件大小','Sketch Size');
  document.getElementById('lblSkfree').textContent=t('固件剩余空间','Sketch Free Space');
  document.getElementById('netTitle').textContent=t('网络详情','Network Detail');
  document.getElementById('lblStaIp').textContent=t('STA 地址','STA IP');
  document.getElementById('lblApIp').textContent=t('AP 地址','AP IP');
  document.getElementById('lblRssi').textContent=t('WiFi 信号','WiFi RSSI');
  document.getElementById('lblLast').textContent=t('最后更新','Last update');
}

function toggleLang(){
  lang=(lang==='zh')?'en':'zh';
  localStorage.setItem('lang',lang);
  applyLang();
}

/* ===== 使用率颜色：<50% 青色(健康) | 50-75% 琥珀(注意) | >75% 红色(危险) ===== */
function gc(p){return p<50?'#00d4ff':p<75?'#f59e0b':'#ef4444';}

/* ===== Signal颜色：强度越高越绿（与使用率逻辑相反） ===== */
function gcSig(p){return p>60?'#00d4ff':p>30?'#f59e0b':'#ef4444';}

function fmt(b){
  if(b>=1048576)return(b/1048576).toFixed(1)+' MB';
  if(b>=1024)return Math.round(b/1024)+' KB';
  return b+' B';
}

/* sg 支持第4个参数自定义颜色，不传则用默认 gc() */
function sg(id,p,sub,clr){
  var a=document.getElementById('g-'+id),
      v=document.getElementById('v-'+id),
      s=document.getElementById('s-'+id),
      c=clr||gc(p);
  a.style.strokeDashoffset=C*(1-p/100);
  a.style.stroke=c;
  v.textContent=Math.round(p)+'%';
  v.style.fill=c;
  s.textContent=sub;
}

function drawChart(){
  var cv=document.getElementById('chart'),w=cv.parentElement.clientWidth;
  if(w<20)return;
  cv.width=w;cv.height=180;
  var ctx=cv.getContext('2d'),H=180;
  ctx.clearRect(0,0,w,H);
  var P={t:12,r:12,b:5,l:44},cw=w-P.l-P.r,ch=H-P.t-P.b;
  ctx.strokeStyle='rgba(255,255,255,.06)';ctx.lineWidth=1;
  ctx.font='10px JetBrains Mono,monospace';ctx.fillStyle='#5a6a8a';ctx.textAlign='right';
  for(var i=0;i<=4;i++){
    var y=P.t+ch*i/4;
    ctx.beginPath();ctx.moveTo(P.l,y);ctx.lineTo(w-P.r,y);ctx.stroke();
    ctx.fillText((100-i*25)+'%',P.l-6,y+4);
  }
  function dl(data,color){
    if(data.length<2)return;
    ctx.strokeStyle=color;ctx.lineWidth=2;ctx.beginPath();
    for(var i=0;i<data.length;i++){
      var x=P.l+(i/(MX-1))*cw,y=P.t+ch-(data[i]/100)*ch;
      i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
    }
    ctx.stroke();
    var lx=P.l+((data.length-1)/(MX-1))*cw;
    ctx.lineTo(lx,P.t+ch);ctx.lineTo(P.l,P.t+ch);ctx.closePath();
    ctx.fillStyle=color+'15';ctx.fill();
    var ly=P.t+ch-(data[data.length-1]/100)*ch;
    ctx.beginPath();ctx.arc(lx,ly,4,0,Math.PI*2);ctx.fillStyle=color;ctx.fill();
    ctx.beginPath();ctx.arc(lx,ly,7,0,Math.PI*2);
    ctx.strokeStyle=color;ctx.lineWidth=1;ctx.globalAlpha=.3;ctx.stroke();ctx.globalAlpha=1;
    ctx.font='600 11px JetBrains Mono,monospace';ctx.fillStyle=color;ctx.textAlign='left';
    ctx.fillText(Math.round(data[data.length-1])+'%',lx+10,ly+4);
  }
  if(hA.length<2){
    ctx.fillStyle='#3a4a60';ctx.font='12px Inter,sans-serif';ctx.textAlign='center';
    ctx.fillText(t('采集中...','Collecting data...'),w/2,H/2);return;
  }
  dl(hA,'#00d4ff');dl(pA,'#10b981');
}

function update(d){
  /* ===== 使用率 = 100% - 空闲率 ===== */
  var hp=d.heapSize>0?(100-d.heapFree/d.heapSize*100):0;
  var pp=d.psramSize>0?(100-d.psramFree/d.psramSize*100):0;
  var ts=d.sketchSize+d.sketchFree;
  var fp=ts>0?(d.sketchFree/ts*100):0;

  /* ===== Signal: STA未连接时显示0，否则 RSSI 映射 0-100% ===== */
  var rp=0;
  if(d.staIP!=='N/A' && d.rssi<0){
    rp=Math.max(0,Math.min(100,2*(d.rssi+100)));
  }

  /* 副标题：已用 / 总量 */
  var heapUsed=d.heapSize-d.heapFree;
  sg('heap',hp,t('已用 ','Used ')+fmt(heapUsed)+' / '+fmt(d.heapSize));

  if(d.psramSize>0){
    var psramUsed=d.psramSize-d.psramFree;
    sg('psram',pp,t('已用 ','Used ')+fmt(psramUsed)+' / '+fmt(d.psramSize));
  }else{
    sg('psram',0,'N/A');
  }

  var sketchFree=d.sketchFree;
  sg('flash',fp,fmt(sketchFree)+' / '+fmt(ts));

  /* Signal 使用专用颜色函数 gcSig */
  var sigText=d.staIP!=='N/A'?d.rssi+' dBm':t('未连接','N/A');
  sg('sig',rp,sigText,gcSig(rp));

  hA.push(hp);pA.push(pp);
  if(hA.length>MX)hA.shift();if(pA.length>MX)pA.shift();
  drawChart();

  document.getElementById('devName').textContent=d.deviceName||d.deviceId;
  document.getElementById('fwVer').textContent='v'+d.firmware;

  /* WiFi */
  var wSta=d.staIP==='N/A';
  document.getElementById('wifiVal').textContent=wSta?t('AP 模式','AP Mode'):d.staIP;
  var bars=document.querySelectorAll('.sig-bar');
  var ab=rp>75?4:rp>50?3:rp>25?2:rp>10?1:0;
  for(var i=0;i<bars.length;i++)bars[i].classList.toggle('on',i<ab);

  /* MQTT */
  var mc=d.mqttStatus==='connected';
  document.getElementById('mqttVal').innerHTML=
    '<span class="s-dot" style="background:'+(mc?'var(--green)':'var(--red)')+'"></span>'+
    (mc?t('已连接','Online'):t('离线','Offline'));

  /* Uptime */
  var s=d.uptime,dn=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);
  document.getElementById('uptimeVal').textContent=(dn>0?dn+t('天','d '):'')+h+'h '+m+'m';

  /* Tasks */
  document.getElementById('tasksVal').innerHTML=
    t('定','T')+':'+d.activeTimers+'/'+d.totalTimers+
    ' &nbsp;'+t('规','R')+':'+d.activeRules+'/'+d.totalRules;

  /* Memory detail */
  document.getElementById('d-hfree').textContent=fmt(d.heapFree);
  document.getElementById('d-hmin').textContent=fmt(d.heapMinFree);
  document.getElementById('d-halloc').textContent=fmt(d.heapMaxAlloc);
  document.getElementById('d-pfree').textContent=d.psramSize>0?fmt(d.psramFree):'N/A';
  document.getElementById('d-pmin').textContent=d.psramSize>0?fmt(d.psramMinFree):'N/A';

  var frag=d.heapFree>0?Math.round((1-d.heapMaxAlloc/d.heapFree)*100):0;
  document.getElementById('d-frag').textContent=frag+'%';
  var ff=document.getElementById('fragFill');
  ff.style.width=frag+'%';
  ff.style.background=frag>50?'var(--red)':frag>25?'var(--amber)':'var(--green)';

  /* System info */
  document.getElementById('d-chip').textContent=d.chipModel+' Rev'+d.chipRev;
  document.getElementById('d-cpu').textContent=d.cpuFreq+' MHz';
  document.getElementById('d-flash').textContent=fmt(d.flashSize);
  document.getElementById('d-sketch').textContent=fmt(d.sketchSize);
  document.getElementById('d-skfree').textContent=fmt(d.sketchFree);
  document.getElementById('d-devid').textContent=d.deviceId;

  /* Network detail */
  document.getElementById('d-staip').textContent=d.staIP;
  document.getElementById('d-apip').textContent=d.apIP;
  document.getElementById('d-rssi').textContent=d.rssi+' dBm';

  document.getElementById('ts').textContent=new Date().toLocaleTimeString();
}

function poll(){
  fetch('/api/system').then(function(r){return r.json();}).then(update).catch(function(){});
}

function setRate(ms){
  rt=parseInt(ms);localStorage.setItem('monRate',ms);
  if(tm)clearInterval(tm);tm=setInterval(poll,rt);
}

window.addEventListener('resize',function(){if(hA.length>1)drawChart();});

document.addEventListener('DOMContentLoaded',function(){
  var sv=localStorage.getItem('monRate');
  if(sv){rt=parseInt(sv);document.getElementById('rateSel').value=sv;}
  applyLang();drawChart();poll();tm=setInterval(poll,rt);
});
</script>
</body>
</html>
)rawliteral";
