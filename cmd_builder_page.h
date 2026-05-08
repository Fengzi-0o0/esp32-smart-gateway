#pragma once
const char CMD_BUILDER_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Command Builder</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600&family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#080c18;--surface:#0d1321;--card:#111827;--border:#1a2540;--text:#e2e8f0;--muted:#5a6a8a;--cyan:#00d4ff;--green:#10b981;--amber:#f59e0b;--red:#ef4444;--purple:#a78bfa}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);height:100vh;display:flex;flex-direction:column;overflow:hidden}
.app{display:flex;flex-direction:column;height:100%;overflow:hidden}
.header{display:flex;align-items:center;justify-content:space-between;padding:10px 16px;background:var(--card);border-bottom:1px solid var(--border);flex-shrink:0;flex-wrap:wrap;gap:6px}
.header h1{font-size:14px;font-weight:700}
.header-right{display:flex;gap:6px;align-items:center}
.btn{padding:5px 10px;border:1px solid var(--border);background:var(--surface);color:var(--text);border-radius:5px;font-size:10px;cursor:pointer;transition:.2s;text-decoration:none;font-family:inherit}
.btn:hover{border-color:var(--cyan);color:var(--cyan)}
.btn-primary{background:var(--cyan);color:#000;border-color:var(--cyan);font-weight:600}
.btn-primary:hover{background:#33ddff}
.conn-dot{width:8px;height:8px;border-radius:50%;background:var(--red);display:inline-block}
.conn-dot.connected{background:var(--green);box-shadow:0 0 6px var(--green)}
.settings-bar{background:var(--card);border-bottom:1px solid var(--border);padding:8px 16px;display:flex;gap:12px;align-items:center;flex-wrap:wrap;flex-shrink:0}
.setting-group{display:flex;align-items:center;gap:6px}
.setting-group label{font-size:10px;color:var(--muted);font-weight:600;white-space:nowrap}
.setting-group select{padding:5px 8px;background:var(--bg);border:1px solid var(--border);border-radius:5px;color:var(--text);font-size:10px;font-family:'JetBrains Mono',monospace;outline:none;cursor:pointer;min-width:140px}
.setting-group select:focus{border-color:var(--cyan)}
.setting-check{display:flex;align-items:center;gap:4px}
.setting-check input{accent-color:var(--cyan);width:13px;height:13px}
.setting-check label{font-size:10px;cursor:pointer}
.setting-group input[type=number]{width:70px;padding:5px 8px;background:var(--bg);border:1px solid var(--border);border-radius:5px;color:var(--text);font-size:10px;font-family:'JetBrains Mono',monospace;outline:none}
.setting-group input[type=number]:focus{border-color:var(--cyan)}
.workspace{display:flex;flex:1;overflow:hidden;min-height:0}
.palette{width:210px;background:var(--surface);border-right:1px solid var(--border);display:flex;flex-direction:column;flex-shrink:0;overflow:hidden}
.palette-tabs{display:flex;flex-wrap:wrap;gap:3px;padding:8px;border-bottom:1px solid var(--border);max-height:120px;overflow-y:auto}
.palette-tab{padding:3px 7px;border-radius:4px;font-size:9px;font-weight:600;cursor:pointer;border:1px solid var(--border);background:var(--card);color:var(--muted);transition:.15s;white-space:nowrap}
.palette-tab:hover{border-color:var(--cyan);color:var(--text)}
.palette-tab.active{background:rgba(0,212,255,.1);border-color:var(--cyan);color:var(--cyan)}
.palette-list{flex:1;overflow-y:auto;padding:6px;min-height:0}
.palette-item{padding:7px 8px;margin-bottom:3px;border-radius:6px;cursor:pointer;font-size:11px;border:1px solid var(--border);transition:.15s;display:flex;align-items:center;gap:5px}
.palette-item:hover{border-color:var(--cyan);background:rgba(0,212,255,.04)}
.palette-item .icon{font-size:12px;flex-shrink:0}
.palette-item .name{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.canvas{flex:1;overflow-y:auto;min-height:0;padding:12px;display:flex;flex-direction:column;gap:8px}
.canvas-empty{display:flex;align-items:center;justify-content:center;flex:1;color:var(--muted);font-size:12px;text-align:center;padding:30px}
.block{border-radius:8px;border:1px solid var(--border);border-left:3px solid var(--border);transition:.15s;overflow:hidden;flex-shrink:0}
.block.selected{border-color:var(--cyan);box-shadow:0 0 10px rgba(0,212,255,.1)}
.block.executing{border-color:var(--amber);box-shadow:0 0 14px rgba(245,158,11,.25)}
.block.executing .block-header{background:rgba(245,158,11,.12) !important}
.block-header{display:flex;align-items:center;gap:6px;padding:7px 10px;cursor:pointer;user-select:none}
.block-header .icon{font-size:13px;flex-shrink:0}
.block-header .name{font-size:11px;font-weight:600;flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.block-header .actions{display:flex;gap:3px;flex-shrink:0}
.block-action{width:18px;height:18px;border-radius:3px;border:1px solid var(--border);background:none;color:var(--muted);font-size:9px;cursor:pointer;display:flex;align-items:center;justify-content:center;transition:.15s}
.block-action:hover{border-color:var(--cyan);color:var(--cyan)}
.block-action.delete:hover{border-color:var(--red);color:var(--red)}
.block-body{padding:4px 10px 8px}
.block-gpio{border-left-color:#3b82f6}.block-gpio .block-header{background:rgba(59,130,246,.06)}
.block-sensor{border-left-color:#f59e0b}.block-sensor .block-header{background:rgba(245,158,11,.06)}
.block-input{border-left-color:#f97316}.block-input .block-header{background:rgba(249,115,22,.06)}
.block-timer{border-left-color:#a855f7}.block-timer .block-header{background:rgba(168,85,247,.06)}
.block-logic{border-left-color:#ec4899}.block-logic .block-header{background:rgba(236,72,153,.06)}
.block-condition{border-left-color:#06b6d4}.block-condition .block-header{background:rgba(6,182,212,.06)}
.block-i2c{border-left-color:#14b8a6}.block-i2c .block-header{background:rgba(20,184,166,.06)}
.block-onewire{border-left-color:#84cc16}.block-onewire .block-header{background:rgba(132,204,22,.06)}
.block-uart{border-left-color:#6366f1}.block-uart .block-header{background:rgba(99,102,241,.06)}
.block-spi{border-left-color:#10b981}.block-spi .block-header{background:rgba(16,185,129,.06)}
.block-touch{border-left-color:#06b6d4}.block-touch .block-header{background:rgba(6,182,212,.06)}
.block-rtc{border-left-color:#c084fc}.block-rtc .block-header{background:rgba(192,132,252,.06)}
.block-script{border-left-color:#eab308}.block-script .block-header{background:rgba(234,179,8,.06)}
.block-data{border-left-color:#94a3b8}.block-data .block-header{background:rgba(148,163,184,.06)}
.block-random{border-left-color:#38bdf8}.block-random .block-header{background:rgba(56,189,248,.06)}
.block-system{border-left-color:#6b7280}.block-system .block-header{background:rgba(107,114,128,.06)}
.block-module{border-left-color:#e879f9}.block-module .block-header{background:rgba(232,121,249,.06)}
.field{margin-bottom:6px}
.field label{display:block;font-size:9px;color:var(--muted);margin-bottom:2px;font-weight:500}
.field input,.field select{width:100%;padding:5px 7px;background:var(--bg);border:1px solid var(--border);border-radius:4px;color:var(--text);font-size:11px;font-family:'JetBrains Mono',monospace;outline:none;transition:.15s}
.field input:focus,.field select:focus{border-color:var(--cyan)}
.field select{cursor:pointer}
.field select option{background:var(--card)}
.field-check{display:flex;align-items:center;gap:4px;margin-bottom:4px}
.field-check input{accent-color:var(--cyan);width:12px;height:12px}
.field-check label{font-size:10px;cursor:pointer;color:var(--text)}
.target-override{margin-top:4px}
.target-override label{display:block;font-size:9px;color:var(--purple);margin-bottom:2px;font-weight:500}
.target-override input{width:100%;padding:5px 7px;background:var(--bg);border:1px solid rgba(167,139,250,.2);border-radius:4px;color:var(--text);font-size:10px;font-family:'JetBrains Mono',monospace;outline:none;transition:.15s}
.target-override input:focus{border-color:var(--purple)}
.child-slot{margin-top:6px;padding:6px;border:1px dashed var(--border);border-radius:6px;background:rgba(0,0,0,.15)}
.slot-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:4px;font-size:10px;font-weight:600;color:var(--muted)}
.slot-add{color:var(--cyan);cursor:pointer;font-size:10px;padding:1px 6px;border:1px solid rgba(0,212,255,.3);border-radius:3px;background:rgba(0,212,255,.06);transition:.15s}
.slot-add:hover{background:rgba(0,212,255,.15)}
.slot-list{display:flex;flex-direction:column;gap:4px}
.slot-empty{font-size:10px;color:var(--muted);text-align:center;padding:8px;font-style:italic}
.panel{background:var(--card);border-top:1px solid var(--border);flex-shrink:0}
.panel-header{display:flex;justify-content:space-between;align-items:center;padding:8px 16px;cursor:pointer}
.panel-header h3{font-size:11px;font-weight:600}
.panel-body{display:none;padding:0 16px 10px}
.panel-body.show{display:block}
.panel-content{background:var(--bg);border:1px solid var(--border);border-radius:6px;padding:10px;font-family:'JetBrains Mono',monospace;font-size:10px;line-height:1.5;max-height:150px;overflow:auto;white-space:pre-wrap;word-break:break-all}
.log-entry{border-bottom:1px solid rgba(255,255,255,.03);padding:1px 0}
.log-time{color:var(--muted)}
.log-ok{color:var(--green)}.log-error{color:var(--red)}.log-sent{color:var(--amber)}
.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.6);z-index:999;display:flex;align-items:center;justify-content:center}
.modal-box{background:var(--surface);border:1px solid var(--border);border-radius:12px;width:90%;max-width:420px;max-height:80vh;overflow-y:auto;padding:14px}
.modal-title{font-size:13px;font-weight:600;margin-bottom:10px}
.modal-category{font-size:10px;font-weight:600;color:var(--muted);margin:8px 0 4px;padding:0 4px}
.modal-item{padding:7px 10px;margin:2px 0;background:var(--card);border:1px solid var(--border);border-radius:6px;cursor:pointer;font-size:11px;transition:.15s}
.modal-item:hover{border-color:var(--cyan);background:rgba(0,212,255,.06)}
.modal-close{width:100%;padding:8px;margin-top:10px;border:1px solid var(--border);background:var(--card);color:var(--text);border-radius:6px;cursor:pointer;font-size:11px;font-family:inherit}
.raw-textarea{width:100%;min-height:100px;padding:10px;background:var(--bg);border:1px solid var(--border);border-radius:6px;color:var(--cyan);font-size:12px;font-family:'JetBrains Mono',monospace;outline:none;resize:vertical}
.btn-spin{animation:spinOnce .6s ease}
@keyframes spinOnce{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}
.toast{position:fixed;top:16px;left:50%;transform:translateX(-50%);padding:8px 18px;border-radius:6px;font-size:11px;font-weight:600;z-index:9999;opacity:0;transition:opacity .3s,transform .3s;pointer-events:none}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
.toast.hide{opacity:0;transform:translateX(-50%) translateY(-10px)}
.toast-ok{background:rgba(16,185,129,.15);border:1px solid var(--green);color:var(--green)}
.toast-err{background:rgba(239,68,68,.15);border:1px solid var(--red);color:var(--red)}
.toast-info{background:rgba(0,212,255,.12);border:1px solid var(--cyan);color:var(--cyan)}
.btn-loop-active{background:var(--red) !important;color:#fff !important;border-color:var(--red) !important;font-weight:700;animation:pulseStop 1.5s ease infinite}
@keyframes pulseStop{0%,100%{opacity:1}50%{opacity:.7}}
::-webkit-scrollbar{width:6px;height:6px}
::-webkit-scrollbar-track{background:var(--bg)}
::-webkit-scrollbar-thumb{background:var(--border);border-radius:3px}
::-webkit-scrollbar-thumb:hover{background:var(--muted)}
@media(max-width:700px){
  .workspace{flex-direction:column}
  .palette{width:100%;max-height:150px;border-right:none;border-bottom:1px solid var(--border)}
  .palette-tabs{flex-wrap:nowrap;overflow-x:auto;max-height:none}
}
</style>
</head>
<body>
<div class="app">
<div class="header">
  <div style="display:flex;align-items:center;gap:8px">
    <a href="/" class="btn" id="btnBack">&larr; Config</a>
    <h1 id="headerTitle">Command Builder</h1>
    <span class="conn-dot" id="connDot"></span>
    <span id="connText" style="font-size:10px;color:var(--muted)">--</span>
  </div>
  <div class="header-right">
    <button class="btn" id="btnLang" onclick="toggleLanguage()">EN</button>
    <button class="btn" id="btnRaw" onclick="showRawModal()">Raw</button>
    <button class="btn" id="btnRefresh" onclick="refreshTargets()">&#8635;</button>
    <button class="btn btn-primary" onclick="sendAll()" id="btnSend">Send</button>
    <button class="btn" onclick="clearCanvas()" id="btnClear">Clear</button>
  </div>
</div>
<div class="settings-bar">
  <div class="setting-group">
    <label id="labelTarget">Target:</label>
    <select id="targetSelect"><option value="">Local</option></select>
  </div>
  <div class="setting-check">
    <input type="checkbox" id="globalPersist" checked>
    <label for="globalPersist" id="labelPersist">Save NVS</label>
  </div>
<div class="setting-group">
    <label id="labelMode">Mode:</label>
    <select id="sendMode" onchange="onModeChange()">
      <option value="batch" id="optBatch">Batch</option>
      <option value="loop" id="optLoop">Loop</option>
      <option value="step" id="optStep">Step</option>
      <option value="sequential" id="optSeq">Sequential</option>
      <option value="seqloop" id="optSeqLoop">SeqLoop</option>
    </select>
  </div>

  <div class="setting-group">
    <label id="labelDelay">Delay(ms):</label>
    <input type="number" id="sendDelay" value="500" min="0" step="100">
  </div>
  <div style="flex:1"></div>
  <span id="deviceInfo" style="display:flex;align-items:center;gap:5px;padding:3px 8px;border-radius:4px;background:rgba(0,212,255,.06);border:1px solid var(--border);font-size:9px;color:var(--cyan);font-family:'JetBrains Mono',monospace;white-space:nowrap;letter-spacing:.3px;opacity:.8"></span>
</div>
<div class="workspace">

  <div class="palette">
    <div class="palette-tabs" id="paletteTabs"></div>
    <div class="palette-list" id="paletteList"></div>
  </div>
  <div class="canvas" id="canvas"></div>
</div>
<div class="panel">
  <div class="panel-header">
    <div style="display:flex;gap:8px;align-items:center;cursor:pointer" onclick="togglePanel('json')">
      <h3 style="color:var(--purple)" id="jsonTitle">JSON Preview</h3>
    </div>
    <div style="display:flex;gap:4px">
      <button class="btn" onclick="event.stopPropagation();copyJson()" style="font-size:9px;padding:2px 8px">Copy</button>
      <button class="btn" id="btnPanelMode" onclick="event.stopPropagation();togglePanelMode()" style="font-size:9px;padding:2px 8px">Log</button>
    </div>
  </div>
  <div class="panel-body" id="jsonPanel">
    <div class="panel-content" id="jsonContent" style="color:var(--cyan)"></div>
    <div class="panel-content" id="logContent" style="display:none"></div>
  </div>
</div>
</div>
<div class="modal-overlay" id="modalOverlay" style="display:none" onclick="if(event.target===this)hideModal()">
  <div class="modal-box" id="modalBox"></div>
</div>
<script>
/* ============================================================
   CONSTANTS
   ============================================================ */
var GPIO_PINS = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,38,39,40,41,42,43,44,45,46,47,48];
var TOUCH_PINS = [1,2,3,4,5,6,7,8,9,10,11,12,13,14];
var CATEGORIES = [
  { id:"gpio",     icon:"\uD83D\uDCA1", zh:"GPIO",     en:"GPIO" },
  { id:"sensor",   icon:"\uD83D\uDCCA", zh:"\u4F20\u611F\u5668",   en:"Sensor" },
  { id:"input",    icon:"\uD83D\uDD19", zh:"\u8F93\u5165",     en:"Input" },
  { id:"timer",    icon:"\u23F1",  zh:"\u5B9A\u65F6\u5668",   en:"Timer" },
  { id:"logic",    icon:"\uD83E\uDDE0", zh:"\u903B\u8F91",     en:"Logic" },
  { id:"condition",icon:"\u2753", zh:"\u6761\u4EF6",     en:"Condition" },
  { id:"i2c",      icon:"\uD83D\uDD0C", zh:"I2C",      en:"I2C" },
  { id:"onewire",  icon:"\uD83D\uDD17", zh:"1-Wire",   en:"1-Wire" },
  { id:"uart",     icon:"\uD83D\uDCE1", zh:"UART",     en:"UART" },
  { id:"spi",      icon:"\u26A1", zh:"SPI",      en:"SPI" },
  { id:"touch",    icon:"\uD83D\uDC46", zh:"\u89E6\u6478",     en:"Touch" },
  { id:"rtc",      icon:"\uD83D\uDD50", zh:"RTC",      en:"RTC" },
  { id:"script",   icon:"\uD83D\uDCDC", zh:"\u811A\u672C",     en:"Script" },
  { id:"data",     icon:"\uD83D\uDCBE", zh:"\u6570\u636E",     en:"Data" },
  { id:"random",   icon:"\uD83C\uDFB2", zh:"\u968F\u673A",     en:"Random" },
  { id:"system",   icon:"\u2699",  zh:"\u7CFB\u7EDF",     en:"System" },
  { id:"module",   icon:"\uD83D\uDCE6", zh:"\u6A21\u5757",     en:"Module" },
   { id:"display",  icon:"🖥", zh:"显示屏",   en:"Display" },
  { id:"preset",   icon:"\u2B50", zh:"\u9884\u8BBE",     en:"Presets" }
];

// ★ GPIO 二级分组
var GPIO_SUB_GROUPS = {
  "basic":   { zh:"基础IO",   en:"Basic IO" },
  "rgb":     { zh:"RGB灯",    en:"RGB LED" },
  "servo":   { zh:"舵机",     en:"Servo" },
  "audio":   { zh:"音频",     en:"Audio" },
  "pulse":   { zh:"脉冲测量", en:"Pulse" },
  "encoder": { zh:"编码器",   en:"Encoder" },
  "system":  { zh:"系统",     en:"System" }
};


/* ============================================================
   BLOCK DEFINITIONS (完整版 - 原始所有块 + 新增模块 + raw_cmd，已删除 RGB)
   ============================================================ */
var BLOCK_DEFS = {
  /* GPIO (原版保留, 删除 rgb, 新增 read_gpio / blink) */
  "set":          { cat:"gpio", sub:"basic", zh:"数字输出",   en:"Digital SET",  cmd:"set",
    fields:[["pin","引脚","Pin","pin"],["value","电平","Value","select",[["0","LOW (0)"],["1","HIGH (1)"]]]] },
  "toggle":       { cat:"gpio", sub:"basic", zh:"翻转电平",   en:"Toggle",       cmd:"toggle",
    fields:[["pin","引脚","Pin","pin"]] },
  "pwm":          { cat:"gpio", sub:"basic", zh:"PWM输出",    en:"PWM",          cmd:"pwm",
    fields:[["pin","引脚","Pin","pin"],["value","占空比 0-255","Value","number",null,128]] },
  "mode":         { cat:"gpio", sub:"basic", zh:"设置模式",   en:"Set Mode",     cmd:"mode",
    fields:[["pin","引脚","Pin","pin"],["mode","模式","Mode","select",[["output","OUTPUT"],["input","INPUT"],["input_pullup","INPUT_PULLUP"]]]] },
  "rgb":          { cat:"gpio", sub:"rgb",   zh:"RGB灯",      en:"RGB",          cmd:"rgb",
    fields:[["rPin","R引脚","R Pin","pin"],["gPin","G引脚","G Pin","pin"],["bPin","B引脚","B Pin","pin"],["r","R 0-255","R","number",null,255],["g","G 0-255","G","number",null,0],["b","B 0-255","B","number",null,0]] },
  "servo":        { cat:"gpio", sub:"servo", zh:"舵机角度",   en:"Servo",        cmd:"servo",
    fields:[["pin","引脚","Pin","pin"],["angle","角度 0-180","Angle","number",null,90]] },
  "servo_detach": { cat:"gpio", sub:"servo", zh:"释放舵机",   en:"Detach",       cmd:"servo_detach",
    fields:[["pin","引脚","Pin","pin"]] },
  "tone":         { cat:"gpio", sub:"audio", zh:"音频输出",   en:"Tone",         cmd:"tone",
    fields:[["pin","引脚","Pin","pin"],["freq","频率Hz(0=停)","Freq","number",null,1000]] },
  "pulse_in":     { cat:"gpio", sub:"pulse", zh:"脉冲测量",   en:"Pulse In",     cmd:"pulse_in",
    fields:[["pin","输入引脚","Pin","pin"],["trig","触发引脚(-1=无)","Trig","number",null,-1],["state","检测电平","State","select",[["high","HIGH"],["low","LOW"]]],["samples","采样次数","Samples","number",null,1],["timeout","超时μs","Timeout","number",null,30000]] },
  "enc_add":      { cat:"gpio", sub:"encoder", zh:"添加编码器", en:"Add Encoder", cmd:"encoder", action:"add",
    fields:[["id","ID","ID","text"],["clk","CLK引脚","CLK","pin"],["dt","DT引脚","DT","pin"],["label","标签","Label","text"]] },
  "enc_read":     { cat:"gpio", sub:"encoder", zh:"读取编码器", en:"Read Encoder", cmd:"encoder", action:"read",
    fields:[["id","ID","ID","text"]] },
  "enc_reset":    { cat:"gpio", sub:"encoder", zh:"重置编码器", en:"Reset Encoder", cmd:"encoder", action:"reset",
    fields:[["id","ID","ID","text"]] },
  "enc_remove":   { cat:"gpio", sub:"encoder", zh:"移除编码器", en:"Remove Encoder", cmd:"encoder", action:"remove",
    fields:[["id","ID","ID","text"]] },
  "clear":        { cat:"gpio", sub:"system", zh:"清除所有引脚", en:"Clear All",    cmd:"clear" },
  "status":       { cat:"gpio", sub:"system", zh:"查询引脚状态", en:"Status",       cmd:"status" },


  /* Sensor (完整原始体系) */
  "sensor_add":    { cat:"sensor", zh:"\u6DFB\u52A0\u4F20\u611F\u5668", en:"Add Sensor",   cmd:"sensor", action:"add",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["type","\u7C7B\u578B","Type","select",[["dht11","DHT11"],["dht22","DHT22"],["ds18b20","DS18B20"],["analog","Analog"],["custom","Custom"]]],["interval","\u91CF\u91CF\u95F4\u9694ms","Interval","number",null,5000],["label","\u6807\u7B7E","Label","text"],["persistent","NVS","Persistent","checkbox",null,true]] },
  "sensor_read":   { cat:"sensor", zh:"\u8BFB\u53D6\u4F20\u611F\u5668", en:"Read Sensor",  cmd:"sensor", action:"read",
    fields:[["pin","\u5F15\u811A","Pin","pin"]] },
  "sensor_remove": { cat:"sensor", zh:"\u79FB\u9664\u4F20\u611F\u5668", en:"Remove Sensor",cmd:"sensor", action:"remove",
    fields:[["pin","\u5F15\u811A","Pin","pin"]] },
  "sensor_enable": { cat:"sensor", zh:"\u542F\u7528/\u7981\u7528\u4F20\u611F\u5668", en:"Enable Sensor", cmd:"sensor", action:"enable",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["enabled","\u542F\u7528","Enabled","checkbox",null,true]] },
  "sensor_list":   { cat:"sensor", zh:"\u5217\u51FA\u4F20\u611F\u5668", en:"List Sensors", cmd:"sensor", action:"list" },

  /* Input (完整原始体系) */
  "input_add":    { cat:"input", zh:"\u6DFB\u52A0\u8F93\u5165", en:"Add Input",    cmd:"input", action:"add",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["mode","\u6A21\u5F0F","Mode","select",[["1","INPUT"],["2","INPUT_PULLUP"]]],["debounce","\u6D88\u6296ms","Debounce","number",null,50],["label","\u6807\u7B7E","Label","text"]] },
  "input_remove": { cat:"input", zh:"\u79FB\u9664\u8F93\u5165", en:"Remove Input", cmd:"input", action:"remove",
    fields:[["pin","\u5F15\u811A","Pin","pin"]] },
  "input_enable": { cat:"input", zh:"\u542F\u7528/\u7981\u7528\u8F93\u5165", en:"Enable Input", cmd:"input", action:"enable",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["enabled","\u542F\u7528","Enabled","checkbox",null,true]] },
  "input_list":   { cat:"input", zh:"\u5217\u51FA\u8F93\u5165", en:"List Inputs",  cmd:"input", action:"list" },

  /* Timer (完整原始体系，含子命令槽) */
  "timer_add":    { cat:"timer", zh:"\u6DFB\u52A0\u5B9A\u65F6\u5668", en:"Add Timer", cmd:"timer", action:"add",
    fields:[["id","ID","ID","text"],["type","\u7C7B\u578B","Type","select",[["interval","interval"],["once","once"],["count","count"]]],["interval","\u95F4\u9694ms","Interval","number",null,5000],["count","\u6B21\u6570(-1=\u65E0\u9650)","Count","number",null,-1],["duration","\u6301\u7EEDms(0=\u6C38\u4E45)","Duration","number",null,0],["enabled","\u542F\u7528","Enabled","checkbox",null,true],["persistent","NVS","Persistent","checkbox",null,true]],
    children:[{key:"commands",zh:"\u5B9A\u65F6\u5668\u547D\u4EE4",en:"Timer Commands",accepts:"command"}] },
  "timer_remove": { cat:"timer", zh:"\u79FB\u9664\u5B9A\u65F6\u5668", en:"Remove Timer", cmd:"timer", action:"remove",
    fields:[["id","ID","ID","text"]] },
  "timer_enable": { cat:"timer", zh:"\u542F\u7528/\u7981\u7528\u5B9A\u65F6\u5668", en:"Enable Timer", cmd:"timer", action:"enable",
    fields:[["id","ID","ID","text"],["enabled","\u542F\u7528","Enabled","checkbox",null,true]] },
  "timer_reset":  { cat:"timer", zh:"\u91CD\u7F6E\u5B9A\u65F6\u5668", en:"Reset Timer", cmd:"timer", action:"reset",
    fields:[["id","ID","ID","text"]] },
  "timer_list":   { cat:"timer", zh:"\u5217\u51FA\u5B9A\u65F6\u5668", en:"List Timers", cmd:"timer", action:"list" },

  /* Logic (完整原始体系，含子命令槽) */
  "logic_add":    { cat:"logic", zh:"\u6DFB\u52A0\u903B\u8F91\u89C4\u5219", en:"Add Rule", cmd:"logic", action:"add",
    fields:[["id","ID","ID","text"],["operator","\u8FD0\u7B97\u7B26","Operator","select",[["and","and"],["or","or"]]],["cooldown","\u51B7\u5374ms","Cooldown","number",null,1000],["enabled","\u542F\u7528","Enabled","checkbox",null,true],["persistent","NVS","Persistent","checkbox",null,true]],
    children:[{key:"conditions",zh:"\u6761\u4EF6",en:"Conditions",accepts:"condition"},{key:"actions",zh:"\u52A8\u4F5C",en:"Actions",accepts:"command"}] },
  "logic_remove": { cat:"logic", zh:"\u79FB\u9664\u903B\u8F91\u89C4\u5219", en:"Remove Rule", cmd:"logic", action:"remove",
    fields:[["id","ID","ID","text"]] },
  "logic_enable": { cat:"logic", zh:"\u542F\u7528/\u7981\u7528\u89C4\u5219", en:"Enable Rule", cmd:"logic", action:"enable",
    fields:[["id","ID","ID","text"],["enabled","\u542F\u7528","Enabled","checkbox",null,true]] },
  "logic_list":   { cat:"logic", zh:"\u5217\u51FA\u903B\u8F91\u89C4\u5219", en:"List Rules",  cmd:"logic", action:"list" },

  /* I2C (原始完整版) */
  "i2c_scan":  { cat:"i2c", zh:"I2C \u626B\u63CF", en:"I2C Scan",  cmd:"i2c", action:"scan",
    fields:[["sda","SDA","SDA","pin"],["scl","SCL","SCL","pin"]] },
  "i2c_read":  { cat:"i2c", zh:"I2C \u8BFB\u53D6", en:"I2C Read",  cmd:"i2c", action:"read",
    fields:[["sda","SDA","SDA","pin"],["scl","SCL","SCL","pin"],["address","\u5730\u5740","Addr","number",null,64],["register","\u5BC4\u5B58\u5668","Reg","number",null,0],["count","\u5B57\u8282\u6570","Count","number",null,1]] },
  "i2c_write": { cat:"i2c", zh:"I2C \u5199\u5165", en:"I2C Write", cmd:"i2c", action:"write",
    fields:[["sda","SDA","SDA","pin"],["scl","SCL","SCL","pin"],["address","\u5730\u5740","Addr","number",null,64],["register","\u5BC4\u5B58\u5668","Reg","number",null,0],["data","\u6570\u636E(JSON)","Data","text"]] },

  /* 1-Wire (原始完整版) */
  "onewire_search": { cat:"onewire", zh:"1-Wire \u641C\u7D22", en:"Search",   cmd:"onewire", action:"search",
    fields:[["pin","\u5F15\u811A","Pin","pin"]] },
  "onewire_temp":   { cat:"onewire", zh:"1-Wire \u8BFB\u6E29\u5EA6", en:"Read Temp", cmd:"onewire", action:"read_temp",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["rom","ROM","ROM","text"]] },
  "onewire_reset":  { cat:"onewire", zh:"1-Wire \u91CD\u7F6E", en:"Reset",    cmd:"onewire", action:"reset",
    fields:[["pin","\u5F15\u811A","Pin","pin"]] },

  /* UART (原始完整版) */
  "uart_config":      { cat:"uart", zh:"UART \u914D\u7F6E", en:"Config",   cmd:"uart", action:"config",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]],["tx","TX","TX","pin"],["rx","RX","RX","pin"],["baud","\u6CE2\u7279\u7387","Baud","number",null,9600],["dataBits","\u6570\u636E\u4F4D","Bits","select",[["5","5"],["6","6"],["7","7"],["8","8"]]],["stopBits","\u505C\u6B62\u4F4D","Stop","select",[["1","1"],["2","2"]]],["parity","\u6821\u9A8C","Parity","select",[["0","None"],["1","Even"],["2","Odd"]]]] },
  "uart_write":       { cat:"uart", zh:"UART \u53D1\u9001\u6587\u672C", en:"Write",    cmd:"uart", action:"write",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]],["data","\u6570\u636E","Data","text"]] },
  "uart_hex":         { cat:"uart", zh:"UART \u53D1\u9001HEX", en:"Write HEX", cmd:"uart", action:"write_hex",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]],["hex","HEX","HEX","text"]] },
  "uart_read":        { cat:"uart", zh:"UART \u8BFB\u53D6",   en:"Read",     cmd:"uart", action:"read",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_listen_start":{ cat:"uart", zh:"UART \u5F00\u59CB\u76D1\u542C", en:"Listen+",  cmd:"uart", action:"listen_start",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_listen_stop": { cat:"uart", zh:"UART \u505C\u6B62\u76D1\u542C", en:"Listen-",  cmd:"uart", action:"listen_stop",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_close":       { cat:"uart", zh:"UART \u5173\u95ED",   en:"Close",    cmd:"uart", action:"close",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_list":        { cat:"uart", zh:"UART \u5217\u8868",   en:"List",     cmd:"uart", action:"list" },

  /* SPI (原始完整版) */
  "spi_config":   { cat:"spi", zh:"SPI \u914D\u7F6E",    en:"Config",   cmd:"spi", action:"config",
    fields:[["port","\u7AEF\u53E3","Port","select",[["2","FSPI"],["3","HSPI"]]],["mosi","MOSI","MOSI","pin"],["miso","MISO","MISO","pin"],["sclk","SCLK","SCLK","pin"],["speed","\u901F\u7387Hz","Speed","number",null,1000000],["mode","\u6A21\u5F0F","Mode","select",[["0","Mode0"],["1","Mode1"],["2","Mode2"],["3","Mode3"]]]] },
  "spi_transfer": { cat:"spi", zh:"SPI \u4F20\u8F93",    en:"Transfer", cmd:"spi", action:"transfer",
    fields:[["port","\u7AEF\u53E3","Port","select",[["2","FSPI"],["3","HSPI"]]],["cs","CS","CS","pin"],["data","\u6570\u636E(JSON)","Data","text"]] },
  "spi_read":     { cat:"spi", zh:"SPI \u8BFB\u53D6",    en:"Read",     cmd:"spi", action:"read",
    fields:[["port","\u7AEF\u53E3","Port","select",[["2","FSPI"],["3","HSPI"]]],["cs","CS","CS","pin"],["count","\u5B57\u8282\u6570","Count","number",null,4]] },
  "spi_close":    { cat:"spi", zh:"SPI \u5173\u95ED",    en:"Close",    cmd:"spi", action:"close" },
  "spi_list":     { cat:"spi", zh:"SPI \u5217\u8868",    en:"List",     cmd:"spi", action:"list" },

  /* Touch (原始完整版) */
  "touch_add":       { cat:"touch", zh:"\u6DFB\u52A0\u89E6\u6478",   en:"Add Touch",    cmd:"touch", action:"add",
    fields:[["pin","\u5F15\u811A","Pin","touch_pin"],["threshold","\u9608\u503C(0=\u81EA\u52A8)","Threshold","number",null,0],["debounceMs","\u6D88\u6296ms","Debounce","number",null,50],["label","\u6807\u7B7E","Label","text"]] },
  "touch_read":      { cat:"touch", zh:"\u8BFB\u53D6\u89E6\u6478",   en:"Read Touch",   cmd:"touch", action:"read",
    fields:[["pin","\u5F15\u811A","Pin","touch_pin"]] },
  "touch_calibrate": { cat:"touch", zh:"\u6821\u51C6\u89E6\u6478",   en:"Calibrate",    cmd:"touch", action:"calibrate",
    fields:[["pin","\u5F15\u811A","Pin","touch_pin"],["samples","\u6837\u672C\u6570","Samples","number",null,100]] },
  "touch_enable":    { cat:"touch", zh:"\u542F\u7528\u89E6\u6478",   en:"Enable Touch",  cmd:"touch", action:"enable",
    fields:[["pin","\u5F15\u811A","Pin","touch_pin"]] },
  "touch_disable":   { cat:"touch", zh:"\u7981\u7528\u89E6\u6478",   en:"Disable Touch", cmd:"touch", action:"disable",
    fields:[["pin","\u5F15\u811A","Pin","touch_pin"]] },
  "touch_list":      { cat:"touch", zh:"\u5217\u51FA\u89E6\u6478",   en:"List Touch",    cmd:"touch", action:"list" },
  "touch_read_all":  { cat:"touch", zh:"\u8BFB\u53D6\u6240\u6709\u89E6\u6478", en:"Read All Touch", cmd:"touch", action:"read_all" },

  /* RTC (原始完整版，含定时计划子槽) */
  "rtc_gettime": { cat:"rtc", zh:"\u83B7\u53D6\u65F6\u95F4", en:"Get Time",  cmd:"rtc", action:"get_time" },
  "rtc_sync":    { cat:"rtc", zh:"NTP \u540C\u6B65", en:"NTP Sync",  cmd:"rtc", action:"sync" },
  "rtc_settime": { cat:"rtc", zh:"\u8BBE\u7F6E\u65F6\u95F4", en:"Set Time",  cmd:"rtc", action:"set_time",
    fields:[["year","\u5E74","Year","number",null,2025],["month","\u6708","Month","number",null,1],["day","\u65E5","Day","number",null,1],["hour","\u65F6","Hour","number",null,0],["minute","\u5206","Min","number",null,0],["second","\u79D2","Sec","number",null,0]] },
  "rtc_addsch":  { cat:"rtc", zh:"\u6DFB\u52A0\u5B9A\u65F6\u8BA1\u5212", en:"Add Schedule", cmd:"rtc", action:"add_schedule",
    fields:[["id","ID","ID","text"],["cron","Cron","Cron","text"],["repeat","\u91CD\u590D","Repeat","checkbox",null,true],["enabled","\u542F\u7528","Enabled","checkbox",null,true]],
    children:[{key:"commands",zh:"\u8BA1\u5212\u547D\u4EE4",en:"Scheduled Commands",accepts:"command"}] },
  "rtc_remsch":  { cat:"rtc", zh:"\u79FB\u9664\u5B9A\u65F6\u8BA1\u5212", en:"Remove Schedule", cmd:"rtc", action:"remove_schedule",
    fields:[["id","ID","ID","text"]] },
  "rtc_listsch": { cat:"rtc", zh:"\u5217\u51FA\u5B9A\u65F6\u8BA1\u5212", en:"List Schedules",  cmd:"rtc", action:"list_schedules" },

  /* Script (原始完整版，含子槽) */
  "scr_varset":  { cat:"script", zh:"\u8BBE\u7F6E\u53D8\u91CF", en:"Set Var",    cmd:"script", action:"var_set",
    fields:[["name","\u53D8\u91CF\u540D","Name","text"],["value","\u503C","Value","number",null,0],["persistent","NVS","Persistent","checkbox",null,false]] },
  "scr_varget":  { cat:"script", zh:"\u83B7\u53D6\u53D8\u91CF", en:"Get Var",    cmd:"script", action:"var_get",
    fields:[["name","\u53D8\u91CF\u540D","Name","text"]] },
  "scr_varinc":  { cat:"script", zh:"\u81EA\u589E\u53D8\u91CF", en:"Inc Var",    cmd:"script", action:"var_inc",
    fields:[["name","\u53D8\u91CF\u540D","Name","text"],["step","\u6B65\u957F","Step","number",null,1]] },
  "scr_varrm":   { cat:"script", zh:"\u5220\u9664\u53D8\u91CF", en:"Remove Var", cmd:"script", action:"var_remove",
    fields:[["name","\u53D8\u91CF\u540D","Name","text"]] },
  "scr_varclr":  { cat:"script", zh:"\u6E05\u9664\u6240\u6709\u53D8\u91CF", en:"Clear Vars", cmd:"script", action:"var_clear" },
  "scr_math":    { cat:"script", zh:"\u6570\u5B66\u8FD0\u7B97", en:"Math",       cmd:"script", action:"math",
    fields:[["op","\u8FD0\u7B97","Op","select",[["add","add"],["sub","sub"],["mul","mul"],["div","div"],["mod","mod"],["abs","abs"],["min","min"],["max","max"],["round","round"],["floor","floor"],["ceil","ceil"]]],["a","a","a","number",null,0],["b","b","b","number",null,0],["result","\u7ED3\u679C\u53D8\u91CF","Result","text"],["persistent","NVS","Persistent","checkbox",null,false]] },
  "scr_if":      { cat:"script", zh:"IF/ELSE \u6761\u4EF6", en:"If/Else",    cmd:"script", action:"if",
    children:[{key:"condition",zh:"\u6761\u4EF6",en:"Condition",accepts:"condition"},{key:"then",zh:"\u6EE1\u8DB3\u65F6",en:"Then",accepts:"command"},{key:"else",zh:"\u4E0D\u6EE1\u8DB3\u65F6",en:"Else",accepts:"command"}] },
  "scr_exec":    { cat:"script", zh:"\u6267\u884C\u547D\u4EE4\u5757", en:"Exec Block", cmd:"script", action:"exec",
    children:[{key:"commands",zh:"\u547D\u4EE4",en:"Commands",accepts:"command"}] },
  "scr_list":    { cat:"script", zh:"\u5217\u51FA\u53D8\u91CF", en:"List Vars",  cmd:"script", action:"list" },

  /* Data (原始完整版) */
  "data_hex":  { cat:"data", zh:"HEX \u8F6C Bytes", en:"HEX to Bytes", cmd:"data", action:"hex_to_bytes",
    fields:[["hex","HEX","HEX","text"]] },
  "data_bhex": { cat:"data", zh:"Bytes \u8F6C HEX", en:"Bytes to HEX", cmd:"data", action:"bytes_to_hex",
    fields:[["data","\u6570\u636E(JSON)","Data","text"]] },
  "data_bint": { cat:"data", zh:"Bytes \u8F6C Int", en:"Bytes to Int", cmd:"data", action:"bytes_to_int",
    fields:[["data","\u6570\u636E(JSON)","Data","text"],["endian","\u5B57\u8282\u5E8F","Endian","select",[["big","big"],["little","little"]]],["signed","\u6709\u7B26\u53F7","Signed","checkbox",null,false]] },
  "data_ib":   { cat:"data", zh:"Int \u8F6C Bytes", en:"Int to Bytes", cmd:"data", action:"int_to_bytes",
    fields:[["value","\u503C","Value","number",null,0],["endian","\u5B57\u8282\u5E8F","Endian","select",[["big","big"],["little","little"]]],["length","\u957F\u5EA6","Length","number",null,2]] },
  "data_crc":  { cat:"data", zh:"CRC16 \u6821\u9A8C", en:"CRC16",      cmd:"data", action:"crc16",
    fields:[["data","\u6570\u636E(JSON)","Data","text"],["append","\u8FFD\u52A0CRC","Append","checkbox",null,false]] },
  "data_cat":  { cat:"data", zh:"\u62FC\u63A5\u6570\u7EC4", en:"Concat",     cmd:"data", action:"concat",
    fields:[["arrays","\u6570\u7EC4(JSON)","Arrays","text"]] },
  "data_slice":{ cat:"data", zh:"\u5207\u7247",       en:"Slice",      cmd:"data", action:"slice",
    fields:[["data","\u6570\u636E(JSON)","Data","text"],["offset","\u504F\u79FB","Offset","number",null,0],["length","\u957F\u5EA6","Length","number",null,2]] },
  "data_ext":  { cat:"data", zh:"\u63D0\u53D6\u5B57\u6BB5", en:"Extract",    cmd:"data", action:"extract",
    fields:[["data","\u6570\u636E(JSON)","Data","text"],["fields","\u5B57\u6BB5(JSON)","Fields","text"]] },
  "data_bit":  { cat:"data", zh:"\u4F4D\u64CD\u4F5C",     en:"Bit Op",     cmd:"data", action:"bit",
    fields:[["value","\u503C","Value","number",null,0],["bit","\u4F4D","Bit","number",null,0],["op","\u64CD\u4F5C","Op","select",[["get","get"],["set","set"],["clear","clear"],["toggle","toggle"]]]] },
  "data_cmp":  { cat:"data", zh:"\u6BD4\u8F83\u6570\u7EC4", en:"Compare",    cmd:"data", action:"compare",
    fields:[["data_a","A(JSON)","A","text"],["data_b","B(JSON)","B","text"]] },

  /* Random (原始完整版，含子槽) */
  "rand_int":   { cat:"random", zh:"\u968F\u673A\u6574\u6570", en:"Random Int",   cmd:"rand", action:"int",
    fields:[["min","\u6700\u5C0F","Min","number",null,0],["max","\u6700\u5927","Max","number",null,100]] },
  "rand_float": { cat:"random", zh:"\u968F\u673A\u6D6E\u70B9", en:"Random Float", cmd:"rand", action:"float",
    fields:[["min","\u6700\u5C0F","Min","number",null,0],["max","\u6700\u5927","Max","number",null,1]] },
  "rand_bool":  { cat:"random", zh:"\u968F\u673A\u5E03\u5C14", en:"Random Bool",  cmd:"rand", action:"bool" },
  "rand_seed":  { cat:"random", zh:"\u8BBE\u7F6E\u79CD\u5B50", en:"Set Seed",     cmd:"rand", action:"seed",
    fields:[["value","\u79CD\u5B50","Seed","number",null,0]] },
  "rand_exec":  { cat:"random", zh:"\u968F\u673A\u6267\u884C", en:"Random Exec",  cmd:"rand", action:"exec",
    fields:[["min","\u6700\u5C0F","Min","number",null,0],["max","\u6700\u5927","Max","number",null,255]],
    children:[{key:"template",zh:"\u6A21\u677F\u547D\u4EE4",en:"Template",accepts:"command",max:1}] },

  /* System (原始完整版 + raw_cmd) */
  "sys_status":  { cat:"system", zh:"\u7CFB\u7EDF\u72B6\u6001", en:"System Status", cmd:"status" },
  "sys_dump":    { cat:"system", zh:"\u5BFC\u51FA\u914D\u7F6E", en:"Dump Config",   cmd:"dump" },
  "sys_restart": { cat:"system", zh:"\u91CD\u542F\u8BBE\u5907", en:"Restart",       cmd:"restart" },
  "sys_batch":   { cat:"system", zh:"\u6279\u5904\u7406\u72B6\u6001", en:"Batch Status", cmd:"batch_status" },
  "sys_pub":     { cat:"system", zh:"MQTT \u53D1\u5E03", en:"MQTT Publish", cmd:"publish",
    fields:[["topic","\u4E3B\u9898","Topic","text"],["payload","\u8D1F\u8F7D","Payload","text"]] },
  "sys_custom":  { cat:"system", zh:"\u81EA\u5B9A\u4E49\u52A8\u4F5C", en:"Custom",    cmd:"custom",
    fields:[["action","\u52A8\u4F5C","Action","text"]] },
  "sys_log":     { cat:"system", zh:"\u65E5\u5FD7\u8F93\u51FA", en:"Log",          cmd:"log",
    fields:[["message","\u6D88\u606F","Msg","text"],["level","\u7EA7\u522B","Level","select",[["info","INFO"],["warn","WARN"],["error","ERROR"]]]] },

  /* Raw JSON (新增) */
  "raw_cmd":     { cat:"system", zh:"\u539F\u59CB JSON \u547D\u4EE4", en:"Raw JSON Command", cmd:"__raw__",
    fields:[["json","JSON \u6570\u636E","JSON Data","textarea"]] },

  /* Module (新增分类) */
  "mod_ds18b20":  { cat:"module", zh:"DS18B20 \u6E29\u5EA6\u4F20\u611F\u5668", en:"DS18B20 Temp", cmd:"module", action:"add", moduleType:"ds18b20",
    fields:[["pin","\u6570\u636E\u5F15\u811A","Pin","pin"],["interval","\u8BFB\u53D6\u95F4\u9694ms","Interval","number",null,5000],["label","\u6807\u7B7E","Label","text"],["rom","ROM\u5730\u5740(\u53EF\u9009)","ROM","text"],["persistent","NVS","Persistent","checkbox",null,true]] },
  "mod_dht11":    { cat:"module", zh:"DHT11 \u6E29\u6E7F\u5EA6\u4F20\u611F\u5668", en:"DHT11 Temp/Hum", cmd:"module", action:"add", moduleType:"dht11",
    fields:[["pin","\u6570\u636E\u5F15\u811A","Pin","pin"],["interval","\u8BFB\u53D6\u95F4\u9694ms","Interval","number",null,5000],["label","\u6807\u7B7E","Label","text"],["persistent","NVS","Persistent","checkbox",null,true]] },
  "mod_remove":   { cat:"module", zh:"\u79FB\u9664\u6A21\u5757",     en:"Remove Module", cmd:"module", action:"remove",
    fields:[["pin","\u5F15\u811A","Pin","pin"]] },
  "mod_enable":   { cat:"module", zh:"\u542F\u7528/\u7981\u7528\u6A21\u5757", en:"Enable/Disable Module", cmd:"module", action:"enable",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["enabled","\u542F\u7528","Enabled","checkbox",null,true]] },
  "mod_read":     { cat:"module", zh:"\u8BFB\u53D6\u6A21\u5757\u6570\u636E", en:"Read Module Data", cmd:"module", action:"read",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["result_var","\u5B58\u5165\u53D8\u91CF","Result Var","text"]] },
  "mod_list":     { cat:"module", zh:"\u5217\u51FA\u6240\u6709\u6A21\u5757", en:"List All Modules", cmd:"module", action:"list" },

  // ========== Display 显示屏 ==========
  "disp_init": { cat:"display", zh:"\u521D\u59CB\u5316\u663E\u793A\u5C4F", en:"Init Display",
    cmd:"display", action:"init",
    fields:[
      ["sda","SDA\u5F15\u811A","SDA","pin"],
      ["scl","SCL\u5F15\u811A","SCL","pin"],
      ["address","I2C\u5730\u5740","Addr","number",null,60],
      ["width","\u5BBD\u5EA6(\u50CF\u7D20)","Width","number",null,128],
      ["height","\u9AD8\u5EA6(\u50CF\u7D20)","Height","number",null,64],
      ["flip","\u7FFB\u8F6C","Flip","checkbox",null,false],
      ["contrast","\u5BF9\u6BD4\u5EA6 0-255","Contrast","number",null,128],
      ["minFlushMs","\u6700\u5C0F\u5237\u65B0\u95F4\u9694ms","MinFlush","number",null,50],
      ["persistent","NVS","Persistent","checkbox",null,true]
    ] },
  "disp_text": { cat:"display", zh:"\u663E\u793A\u6587\u5B57", en:"Draw Text",
    cmd:"display", action:"text",
    fields:[
      ["x","X\u5750\u6807","X","number",null,0],
      ["y","Y\u5750\u6807(\u50CF\u7D20)","Y","number",null,0],
      ["size","\u5B57\u53F7","Size","select",[["1","\u5C0F(6\u00D78)"],["2","\u5927(12\u00D716)"]]],
      ["text","\u5185\u5BB9(\u652F\u6301$V:)","Text","text"],
      ["color","\u989C\u8272","Color","select",[["1","\u4EAE(\u767D)"],["0","\u706D(\u9ED1)"]]],
      ["wrap","\u81EA\u52A8\u6362\u884C","Wrap","checkbox",null,false]
    ] },
  "disp_rect": { cat:"display", zh:"\u7ED8\u5236\u77E9\u5F62", en:"Draw Rect",
    cmd:"display", action:"rect",
    fields:[
      ["x","X","X","number",null,0],
      ["y","Y","Y","number",null,0],
      ["w","\u5BBD","W","number",null,50],
      ["h","\u9AD8","H","number",null,20],
      ["fill","\u586B\u5145","Fill","checkbox",null,false],
      ["color","\u989C\u8272","Color","select",[["1","\u4EAE"],["0","\u706D"]]]
    ] },
  "disp_line": { cat:"display", zh:"\u7ED8\u5236\u76F4\u7EBF", en:"Draw Line",
    cmd:"display", action:"line",
    fields:[
      ["x1","X1","X1","number",null,0],
      ["y1","Y1","Y1","number",null,0],
      ["x2","X2","X2","number",null,127],
      ["y2","Y2","Y2","number",null,63],
      ["color","\u989C\u8272","Color","select",[["1","\u4EAE"],["0","\u706D"]]]
    ] },
  "disp_hline": { cat:"display", zh:"\u6C34\u5E73\u7EBF", en:"H-Line",
    cmd:"display", action:"hline",
    fields:[
      ["x","X","X","number",null,0],
      ["y","Y","Y","number",null,18],
      ["w","\u957F\u5EA6","W","number",null,128],
      ["color","\u989C\u8272","Color","select",[["1","\u4EAE"],["0","\u706D"]]]
    ] },
  "disp_pixel": { cat:"display", zh:"\u753B\u70B9", en:"Pixel",
    cmd:"display", action:"pixel",
    fields:[
      ["x","X","X","number",null,64],
      ["y","Y","Y","number",null,32],
      ["color","\u989C\u8272","Color","select",[["1","\u4EAE"],["0","\u706D"]]]
    ] },
  "disp_progress": { cat:"display", zh:"\u8FDB\u5EA6\u6761", en:"Progress Bar",
    cmd:"display", action:"progress",
    fields:[
      ["x","X","X","number",null,0],
      ["y","Y","Y","number",null,56],
      ["w","\u5BBD","W","number",null,128],
      ["h","\u9AD8","H","number",null,8],
      ["value","\u5F53\u524D\u503C(\u652F\u6301$V:)","Value","text"],
      ["max","\u6700\u5927\u503C","Max","number",null,100]
    ] },
  "disp_bitmap": { cat:"display", zh:"\u4F4D\u56FE", en:"Bitmap",
    cmd:"display", action:"bitmap",
    fields:[
      ["x","X","X","number",null,0],
      ["y","Y","Y","number",null,0],
      ["w","\u5BBD(\u50CF\u7D20)","W","number",null,32],
      ["h","\u9AD8(8\u500D\u6570)","H","number",null,32],
      ["data","\u6570\u636E(JSON\u6570\u7EC4)","Data","textarea"]
    ] },
  "disp_clear": { cat:"display", zh:"\u6E05\u5C4F", en:"Clear",
    cmd:"display", action:"clear" },
  "disp_clear_rect": { cat:"display", zh:"\u6E05\u9664\u533A\u57DF", en:"Clear Rect",
    cmd:"display", action:"clear_rect",
    fields:[
      ["x","X","X","number",null,0],
      ["y","Y","Y","number",null,0],
      ["w","\u5BBD","W","number",null,128],
      ["h","\u9AD8","H","number",null,64]
    ] },
  "disp_flush": { cat:"display", zh:"\u5237\u65B0\u5230\u5C4F\u5E55", en:"Flush",
    cmd:"display", action:"flush",
    fields:[
      ["force","\u5F3A\u5236\u5168\u5C4F","Force","checkbox",null,false]
    ] },
  "disp_on": { cat:"display", zh:"\u5F00\u5C4F", en:"Display On",
    cmd:"display", action:"on" },
  "disp_off": { cat:"display", zh:"\u5173\u5C4F", en:"Display Off",
    cmd:"display", action:"off" },
  "disp_contrast": { cat:"display", zh:"\u5BF9\u6BD4\u5EA6", en:"Contrast",
    cmd:"display", action:"contrast",
    fields:[
      ["value","\u503C 0-255","Value","number",null,128]
    ] },
  "disp_invert": { cat:"display", zh:"\u53CD\u8272", en:"Invert",
    cmd:"display", action:"invert",
    fields:[
      ["enabled","\u542F\u7528","Enabled","checkbox",null,false]
    ] },
  "disp_flip": { cat:"display", zh:"\u7FFB\u8F6C\u663E\u793A", en:"Flip",
    cmd:"display", action:"flip",
    fields:[
      ["enabled","\u542F\u7528","Enabled","checkbox",null,false]
    ] },
  "disp_scene": { cat:"display", zh:"\u573A\u666F\u7ED8\u5236", en:"Scene Draw",
    cmd:"display", action:"scene",
    fields:[],
    children:[{key:"commands",zh:"\u7ED8\u56FE\u547D\u4EE4",en:"Draw Commands",accepts:"command"}] },
  "disp_status": { cat:"display", zh:"\u663E\u793A\u5C4F\u72B6\u6001", en:"Display Status",
    cmd:"display", action:"status" }
};


/* Condition definitions */
var CONDITION_DEFS = {
  "sensor": { zh:"\u4F20\u611F\u5668\u6570\u636E\u6761\u4EF6", en:"Sensor Condition", source:"sensor",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["op","\u8FD0\u7B97\u7B26","Op","select",[["gt",">"],["lt","<"],["eq","=="],["ne","!="],[">=",">="],["<=","<="]]],["value","\u503C","Value","number",null,0]] },
  "touch":  { zh:"\u89E6\u6478\u6761\u4EF6",   en:"Touch Condition",  source:"touch",
    fields:[["pin","\u5F15\u811A","Pin","touch_pin"],["op","\u8FD0\u7B97\u7B26","Op","select",[["gt",">"],["lt","<"],["eq","=="],["ne","!="]]],["value","\u503C","Value","number",null,0]] },
  "input":  { zh:"\u8F93\u5165\u5F15\u811A\u6761\u4EF6", en:"Input Condition",  source:"input",
    fields:[["pin","\u5F15\u811A","Pin","pin"],["op","\u8FD0\u7B97\u7B26","Op","select",[["eq","="],["ne","!="]]],["value","\u503C","Value","select",[["0","LOW"],["1","HIGH"]]]] },
  "var":    { zh:"\u53D8\u91CF\u6761\u4EF6",   en:"Variable Condition", source:"var",
    fields:[["name","\u53D8\u91CF\u540D","Name","text"],["op","\u8FD0\u7B97\u7B26","Op","select",[["gt",">"],["lt","<"],["eq","=="],["ne","!="],[">=",">="],["<=","<="]]],["value","\u503C","Value","number",null,0]] },
  "time":   { zh:"\u65F6\u95F4\u6761\u4EF6",     en:"Time Condition",   source:"time",
    fields:[["op","\u64CD\u4F5C","Op","select",[["hour_eq","hour =="],["hour_gte","hour >="],["hour_lte","hour <="],["between","between"]]],["value","\u503C1","V1","number",null,0],["value2","\u503C2","V2","number",null,0]] },
  "rand":   { zh:"\u968F\u673A\u6761\u4EF6", en:"Random Condition", source:"rand",
    fields:[["op","\u64CD\u4F5C","Op","select",[["lt","<"],["gt",">"]]],["value","\u9608\u503C 0-100","Value","number",null,50]] },
  "mqtt":   { zh:"MQTT \u6D88\u606F\u6761\u4EF6", en:"MQTT Condition",   source:"mqtt",
    fields:[["topic","\u4E3B\u9898","Topic","text"],["op","\u64CD\u4F5C","Op","select",[["contains","contains"],["eq","=="]]],["value","\u503C","Value","text"]] },
  "uart":   { zh:"UART \u6570\u636E\u6761\u4EF6", en:"UART Condition",   source:"uart",
    fields:[["port","\u7AEF\u53E3","Port","select",[["1","UART1"],["2","UART2"]]],["op","\u64CD\u4F5C","Op","select",[["contains","contains"],["eq","=="],["changed","changed"]]],["value","\u503C","Value","text"]] }
};

var OP_MAP = {"gt":">","lt":"<","eq":"==","ne":"!=","lte":"<=","gte":">="};
/* ============================================================
   STATE
   ============================================================ */
var blockTree = [];
var nextBlockId = 1;
var selectedBlockId = null;
var activeCategory = "gpio";
var cachedDevices = [];
var websocket = null;
var websocketConnected = false;
var language = localStorage.getItem("lang") || "zh";
var panelMode = "json";
var loopTimer = null;
var stepIndex = 0;
var loopIteration = 0;
var seqLoopTimer = null;


function translate(zh, en) { return language === "zh" ? zh : en; }
function escapeHtml(s) { return String(s).replace(/&/g,"&amp;").replace(/"/g,"&quot;").replace(/</g,"&lt;"); }

/* ============================================================
   TOAST
   ============================================================ */
function showToast(msg, type, duration) {
  type = type || "info";
  duration = duration || 2000;
  var existing = document.querySelector(".toast");
  if (existing) existing.remove();
  var el = document.createElement("div");
  el.className = "toast toast-" + type;
  el.textContent = msg;
  document.body.appendChild(el);
  requestAnimationFrame(function() { el.classList.add("show"); });
  setTimeout(function() {
    el.classList.remove("show");
    el.classList.add("hide");
    setTimeout(function() { el.remove(); }, 300);
  }, duration);
}

/* ============================================================
   PANEL MODE TOGGLE (JSON / Log)
   ============================================================ */
function togglePanelMode() {
  panelMode = panelMode === "json" ? "log" : "json";
  document.getElementById("jsonContent").style.display = panelMode === "json" ? "" : "none";
  document.getElementById("logContent").style.display = panelMode === "log" ? "" : "none";
  document.getElementById("btnPanelMode").textContent = panelMode === "json"
    ? translate("\u65E5\u5FD7","Log") : "JSON";
  document.getElementById("jsonTitle").textContent = panelMode === "json"
    ? translate("JSON \u9884\u89C8","JSON Preview")
    : translate("\u65E5\u5FD7","Log");
}

/* ============================================================
   DEVICE MANAGEMENT
   ============================================================ */
function refreshTargets() {
  var btn = document.getElementById("btnRefresh");
  if (btn) { btn.classList.remove("btn-spin"); void btn.offsetWidth; btn.classList.add("btn-spin"); }
  showToast(translate("\u6B63\u5728\u5237\u65B0\u76EE\u6807...","Refreshing targets..."), "info", 1000);
  fetch("/api/devices").then(function(r){ return r.json(); }).then(function(data) {
    var select = document.getElementById("targetSelect");
    var currentValue = select.value;
    cachedDevices = (data.devices || []).filter(function(d) { return d.id !== data.deviceId; });
    select.innerHTML = "<option value=\"\">" + translate("\u672C\u5730","Local") + "</option>";
    var count = 0;
    for (var i = 0; i < cachedDevices.length; i++) {
      var dev = cachedDevices[i];
      select.innerHTML += "<option value=\"" + dev.id + "\">" + (dev.name || dev.id) + " [" + dev.ip + "]</option>";
      count++;
    }
    select.value = currentValue;
    showToast(translate("\u5DF2\u53D1\u73B0 ","Found ") + count + translate(" \u4E2A\u8BBE\u5907"," device(s)"), "ok");
  }).catch(function() {
    showToast(translate("\u5237\u65B0\u5931\u8D25","Refresh failed"), "err");
  });
}

function getGlobalTarget() {
  return document.getElementById("targetSelect").value || null;
}

/* ============================================================
   BLOCK OPERATIONS (原始完整版)
   ============================================================ */
function getBlockDef(type) {
  if (type.indexOf("cond_") === 0) {
    var condType = type.substring(5);
    var condDef = CONDITION_DEFS[condType];
    if (!condDef) return null;
    return { cat:"condition", zh:condDef.zh, en:condDef.en, fields:condDef.fields, isCondition:true, source:condDef.source };
  }
  if (type === "__raw__" || (BLOCK_DEFS[type] && BLOCK_DEFS[type].cmd === "__raw__")) {
    return BLOCK_DEFS[type] || null;
  }
  return BLOCK_DEFS[type] || null;
}

function getCategoryIcon(categoryId) {
  for (var i = 0; i < CATEGORIES.length; i++) {
    if (CATEGORIES[i].id === categoryId) return CATEGORIES[i].icon;
  }
  return "\u2699";
}

function createBlock(type) {
  var def = getBlockDef(type);
  if (!def) return null;
  var block = { id: nextBlockId++, type: type, params: {}, children: {} };
  if (def.isCondition && def.source) block.params.source = def.source;
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      var field = def.fields[i];
      var key = field[0], fieldType = field[3], defaultVal = field.length > 5 ? field[5] : null;
      if (fieldType === "checkbox" && defaultVal !== null) block.params[key] = defaultVal;
      else if (fieldType === "number" && defaultVal !== null) block.params[key] = defaultVal;
    }
  }
  if (def.children) {
    for (var i = 0; i < def.children.length; i++) {
      block.children[def.children[i].key] = [];
    }
  }
  var globalTarget = getGlobalTarget();
  if (globalTarget) block.params.target = globalTarget;
  var globalPersist = document.getElementById("globalPersist");
  if (globalPersist && globalPersist.checked && block.params.hasOwnProperty("persistent")) {
    block.params.persistent = true;
  }
  return block;
}

function findBlockById(id, list) {
  if (!list) list = blockTree;
  for (var i = 0; i < list.length; i++) {
    if (list[i].id === id) return list[i];
    if (list[i].children) {
      for (var key in list[i].children) {
        var found = findBlockById(id, list[i].children[key]);
        if (found) return found;
      }
    }
  }
  return null;
}

function findBlockParent(id, list, parentInfo) {
  if (!list) list = blockTree;
  for (var i = 0; i < list.length; i++) {
    if (list[i].id === id) return { list: list, index: i, parent: parentInfo };
    if (list[i].children) {
      for (var key in list[i].children) {
        var found = findBlockParent(id, list[i].children[key], { block: list[i], key: key });
        if (found) return found;
      }
    }
  }
  return null;
}

function addBlock(type) {
  var block = createBlock(type);
  if (!block) return;
  blockTree.push(block);
  selectedBlockId = block.id;
  renderCanvas();
  updateJsonPreview();
  var def = getBlockDef(type);
  var label = def ? (language === "zh" ? def.zh : def.en) : type;
  addLogMessage(translate("\u5DF2\u6DFB\u52A0: ","Added: ") + label, "ok");
}

function addChildBlock(parentId, slotKey, childType) {
  var block = createBlock(childType);
  if (!block) return;
  var parent = findBlockById(parentId);
  if (!parent || !parent.children[slotKey]) return;
  parent.children[slotKey].push(block);
  hideModal();
  renderCanvas();
  updateJsonPreview();
  var def = getBlockDef(childType);
  var label = def ? (language === "zh" ? def.zh : def.en) : childType;
  addLogMessage(translate("\u5D4C\u5957: ","Nested: ") + label, "ok");
}

function removeBlock(id) {
  var info = findBlockParent(id);
  if (!info) return;
  info.list.splice(info.index, 1);
  if (selectedBlockId === id) selectedBlockId = null;
  renderCanvas();
  updateJsonPreview();
}

function moveBlock(id, direction) {
  var info = findBlockParent(id);
  if (!info) return;
  var newIndex = info.index + direction;
  if (newIndex < 0 || newIndex >= info.list.length) return;
  var temp = info.list[info.index];
  info.list[info.index] = info.list[newIndex];
  info.list[newIndex] = temp;
  renderCanvas();
  updateJsonPreview();
}

function selectBlock(id) {
  selectedBlockId = (selectedBlockId === id) ? null : id;
  renderCanvas();
}

function updateField(blockId, key, value) {
  var block = findBlockById(blockId);
  if (!block) return;
  if (key === "target" && !value) delete block.params.target;
  else block.params[key] = value;
  updateJsonPreview();
}

function highlightBlock(id) {
  clearAllHighlights();
  var el = document.getElementById('block-' + id);
  if (el) {
    el.classList.add('executing');
    el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }
}

function clearAllHighlights() {
  var els = document.querySelectorAll('.block.executing');
  for (var i = 0; i < els.length; i++) els[i].classList.remove('executing');
}

function flashAllBlocks() {
  var els = document.querySelectorAll('.block');
  for (var i = 0; i < els.length; i++) els[i].classList.add('executing');
  setTimeout(clearAllHighlights, 350);
}

function updateSendButton(isLoopRunning) {
  var btn = document.getElementById('btnSend');
  if (!btn) return;
  if (isLoopRunning) {
    btn.textContent = translate('\u23F9 \u505C\u6B62\u5FAA\u73AF', '\u23F9 Stop Loop');
    btn.className = 'btn btn-loop-active';
  } else {
    btn.textContent = translate('\u53D1\u9001', 'Send');
    btn.className = 'btn btn-primary';
  }
}


function clearCanvas() {
  stopLoop();
  stopSeqLoop();  
  clearAllHighlights();
  stepIndex = 0;
  blockTree = [];
  selectedBlockId = null;
  stepIndex = 0;
  renderCanvas();
  updateJsonPreview();
  addLogMessage(translate("\u5DF2\u6E05\u7A7A","Cleared"), "ok");
}



/* ============================================================
   RENDERING - PALETTE (含预设分类)
   ============================================================ */
function renderPalette() {
  var tabsEl = document.getElementById("paletteTabs");
  var listEl = document.getElementById("paletteList");
  if (!tabsEl || !listEl) return;

  // Category tabs
  var tabsHtml = "";
  for (var i = 0; i < CATEGORIES.length; i++) {
    var cat = CATEGORIES[i];
    var isActive = (activeCategory === cat.id);
    tabsHtml += '<button class="palette-tab' + (isActive ? ' active' : '') + '" ';
    tabsHtml += 'onclick="selectCategory(\'' + cat.id + '\')">';
    tabsHtml += cat.icon + ' ' + (language === "zh" ? cat.zh : cat.en);
    tabsHtml += '</button>';
  }
  tabsEl.innerHTML = tabsHtml;

  // Block list for active category (支持二级分组)
  var listHtml = "";
  var count = 0;

  if (activeCategory === "preset") {
    listHtml += '<div style="padding:8px">';
    listHtml += '<div style="margin-bottom:8px;font-size:11px;color:var(--muted);text-align:center">';
    listHtml += translate('\u5C06\u5F53\u524D\u753B\u5E03\u5BFC\u51FA\u4E3A\u6587\u4EF6\uFF0C\u6216\u4ECE\u6587\u4EF6\u5BFC\u5165\u9884\u8BBE\u5230\u753B\u5E03', 'Export canvas to file, or import preset from file to canvas');
    listHtml += '</div>';
    listHtml += '<button class="btn btn-primary" style="width:100%;margin-bottom:6px;font-size:11px;padding:8px" onclick="exportPresets()">';
    listHtml += '\u2B07 ' + translate('\u5BFC\u51FA\u5F53\u524D\u753B\u5E03', 'Export Canvas');
    listHtml += '</button>';
    listHtml += '<button class="btn" style="width:100%;font-size:11px;padding:8px;border-color:var(--amber);color:var(--amber)" onclick="document.getElementById(\'importFileInput\').click()">';
    listHtml += '\u2B06 ' + translate('\u4ECE\u6587\u4EF6\u5BFC\u5165', 'Import from File');
    listHtml += '</button>';
    listHtml += '<input type="file" id="importFileInput" accept=".json" style="display:none" onchange="handleImportFile(this)">';
    listHtml += '</div>';
    count = blockTree.length;
  }
  else if (activeCategory === "condition") {


 
    for (var key in CONDITION_DEFS) {
      if (!CONDITION_DEFS.hasOwnProperty(key)) continue;
      var def = CONDITION_DEFS[key];
      var label = language === "zh" ? def.zh : def.en;
      listHtml += '<div class="palette-item" onclick="addBlock(\'cond_' + key + '\')">';
      listHtml += '<span class="icon">◆</span>';
      listHtml += '<span class="name">' + escapeHtml(label) + '</span>';
      listHtml += '</div>';
      count++;
    }
  } else {
    // 收集当前分类的所有 block，按 sub 分组
    var groups = {};
    var groupOrder = [];
    for (var key in BLOCK_DEFS) {
      if (!BLOCK_DEFS.hasOwnProperty(key)) continue;
      var def = BLOCK_DEFS[key];
      if (def.cat !== activeCategory) continue;
      var subKey = def.sub || "_default";
      if (!groups[subKey]) {
        groups[subKey] = [];
        groupOrder.push(subKey);
      }
      groups[subKey].push({ key: key, def: def });
      count++;
    }

    if (count === 0) {
      listHtml = '<div style="padding:12px;text-align:center;color:var(--muted);font-size:11px">';
      listHtml += translate('此分类暂无模块','No blocks in this category') + '</div>';
    } else {
      var hasSubGroups = (groupOrder.length > 1 || groupOrder[0] !== "_default");
      for (var gi = 0; gi < groupOrder.length; gi++) {
        var subKey = groupOrder[gi];
        var items = groups[subKey];

        // 有子分组且不是默认分组 → 显示子分组标题
        if (hasSubGroups && subKey !== "_default") {
          var subDef = null;
          if (typeof GPIO_SUB_GROUPS !== 'undefined' && GPIO_SUB_GROUPS[subKey]) {
            subDef = GPIO_SUB_GROUPS[subKey];
          }
          var subLabel = subDef ? (language === "zh" ? subDef.zh : subDef.en) : subKey;
          listHtml += '<div style="font-size:9px;color:var(--cyan);font-weight:600;padding:8px 8px 3px;letter-spacing:.5px;text-transform:uppercase;opacity:.7">';
          listHtml += subLabel + '</div>';
        }

        for (var bi = 0; bi < items.length; bi++) {
          var bDef = items[bi].def;
          var bKey = items[bi].key;
          var label = language === "zh" ? bDef.zh : bDef.en;
          var icon = getCategoryIcon(bDef.cat);
          listHtml += '<div class="palette-item" onclick="addBlock(\'' + bKey + '\')">';
          listHtml += '<span class="icon">' + icon + '</span>';
          listHtml += '<span class="name">' + escapeHtml(label) + '</span>';
          listHtml += '</div>';
        }
      }
    }
  }

  listEl.innerHTML = listHtml;
  listEl.scrollTop = 0;
}


function selectCategory(categoryId) {
  activeCategory = categoryId;
  renderPalette();
}

/* ============================================================
   RENDERING - CANVAS (原始完整版)
   ============================================================ */
function renderCanvas() {
  var canvasEl = document.getElementById("canvas");
  if (!blockTree.length) {
    canvasEl.innerHTML = '<div class="canvas-empty">' + translate("\u70B9\u51FB\u5DE6\u4FA7\u79EF\u6728\u5757\u5F00\u59CB\u6784\u5EFA","Click a block from the palette on the left") + '</div>';
    return;
  }
  var html = "";
  for (var i = 0; i < blockTree.length; i++) {
    html += renderBlock(blockTree[i], false);
  }
  canvasEl.innerHTML = html;
}

function renderBlock(block, isChild) {
  var def = getBlockDef(block.type);
  if (!def) return "";
  var category = def.cat || "system";
  var label = language === "zh" ? def.zh : def.en;
  var icon = getCategoryIcon(category);
  var isSelected = block.id === selectedBlockId;

  var html = '<div id="block-' + block.id + '" class="block block-' + category + (isSelected ? ' selected' : '') + '">';
  html += '<div class="block-header" onclick="selectBlock(' + block.id + ')">';
  html += '<span class="icon">' + icon + '</span>';
  html += '<span class="name">' + escapeHtml(label) + '</span>';
  html += '<div class="actions">';
  if (!isChild) {
    html += '<button class="block-action" onclick="event.stopPropagation();moveBlock(' + block.id + ',-1)" title="Up">\u2191</button>';
    html += '<button class="block-action" onclick="event.stopPropagation();moveBlock(' + block.id + ',1)" title="Down">\u2193</button>';
  }
  html += '<button class="block-action delete" onclick="event.stopPropagation();removeBlock(' + block.id + ')" title="Delete">\u2715</button>';
  html += '</div></div>';

  html += '<div class="block-body">';

  /* raw_cmd 特殊渲染 */
  if (def.cmd === "__raw__") {
    var jsonVal = block.params.json || "";
    html += '<div class="field"><label>' + translate('JSON \u6570\u636E','JSON Data') + '</label>';
    html += '<textarea style="width:100%;min-height:60px;padding:5px 7px;background:var(--bg);border:1px solid var(--border);border-radius:4px;color:var(--cyan);font-size:11px;font-family:monospace;outline:none;resize:vertical" onchange="updateField(' + block.id + ',\'json\',this.value)">' + escapeHtml(jsonVal) + '</textarea></div>';
    html += '<div class="target-override"><label>' + translate('Target \u8986\u76D6 (\u7A7A=\u5168\u5C40)','Target override (empty=global)') + '</label>';
    html += '<input value="' + escapeHtml(block.params.target || '') + '" placeholder="' + translate('\u8DDF\u968F\u5168\u5C40','global') + '" onchange="updateField(' + block.id + ',\'target\',this.value || null)"></div>';
    html += '</div></div>';
    return html;
  }

  /* 常规字段 */
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      html += renderField(block, def.fields[i]);
    }
  }

  /* Target override (非条件块) */
  if (!def.isCondition) {
    html += '<div class="target-override">';
    html += '<label>' + translate('Target \u8986\u76D6 (\u7A7A=\u5168\u5C40)','Target override (empty=global)') + '</label>';
    html += '<input value="' + escapeHtml(block.params.target || '') + '" placeholder="' + translate('\u8DDF\u968F\u5168\u5C40','global') + '" onchange="updateField(' + block.id + ',\'target\',this.value || null)">';
    html += '</div>';
  }

  /* 子命令槽 */
  if (def.children) {
    for (var c = 0; c < def.children.length; c++) {
      html += renderChildSlot(block, def.children[c]);
    }
  }

  html += '</div></div>';
  return html;
}

function renderField(block, fieldDef) {
  var key = fieldDef[0];
  var zhLabel = fieldDef[1];
  var enLabel = fieldDef[2];
  var fieldType = fieldDef[3];
  var options = fieldDef.length > 4 ? fieldDef[4] : null;
  var label = language === "zh" ? zhLabel : enLabel;
  var value = block.params[key];
  var fieldId = "field_" + block.id + "_" + key;

  if (fieldType === "checkbox") {
    return '<div class="field-check">' +
      '<input type="checkbox" id="' + fieldId + '" ' + (value ? 'checked' : '') +
      ' onchange="updateField(' + block.id + ',\'' + key + '\',this.checked)">' +
      '<label for="' + fieldId + '">' + escapeHtml(label) + '</label></div>';
  }

  var html = '<div class="field"><label>' + escapeHtml(label) + '</label>';

  if (fieldType === "number") {
    var min = fieldDef.length > 5 ? fieldDef[5] : "";
    var max = fieldDef.length > 6 ? fieldDef[6] : "";
    html += '<input type="number" id="' + fieldId + '" value="' + (value !== undefined ? value : '') + '"' +
      (min !== '' ? ' min="' + min + '"' : '') + (max !== '' ? ' max="' + max + '"' : '') +
      ' onchange="updateField(' + block.id + ',\'' + key + '\',parseFloat(this.value)||0)">';
  }
  else if (fieldType === "text") {
    html += '<input type="text" id="' + fieldId + '" value="' + escapeHtml(value || '') +
      '" onchange="updateField(' + block.id + ',\'' + key + '\',this.value)">';
  }
  else if (fieldType === "textarea") {
    html += '<textarea id="' + fieldId + '" style="width:100%;min-height:60px;padding:5px 7px;background:var(--bg);border:1px solid var(--border);border-radius:4px;color:var(--cyan);font-size:11px;font-family:monospace;outline:none;resize:vertical" onchange="updateField(' + block.id + ',\'' + key + '\',this.value)">' + escapeHtml(value || '') + '</textarea>';
  }
  else if (fieldType === "pin") {
    html += '<select id="' + fieldId + '" onchange="updateField(' + block.id + ',\'' + key + '\',parseInt(this.value))">';
    for (var i = 0; i < GPIO_PINS.length; i++) {
      html += '<option value="' + GPIO_PINS[i] + '"' + (value === GPIO_PINS[i] ? ' selected' : '') + '>GPIO ' + GPIO_PINS[i] + '</option>';
    }
    html += '</select>';
  }
  else if (fieldType === "touch_pin") {
    html += '<select id="' + fieldId + '" onchange="updateField(' + block.id + ',\'' + key + '\',parseInt(this.value))">';
    for (var i = 0; i < TOUCH_PINS.length; i++) {
      html += '<option value="' + TOUCH_PINS[i] + '"' + (value === TOUCH_PINS[i] ? ' selected' : '') + '>GPIO ' + TOUCH_PINS[i] + '</option>';
    }
    html += '</select>';
  }
  else if (fieldType === "select" && options) {
    html += '<select id="' + fieldId + '" onchange="updateSelectField(' + block.id + ',\'' + key + '\',this.value,' + JSON.stringify(options).replace(/"/g, '&quot;') + ')">';
    for (var i = 0; i < options.length; i++) {
      var optValue = options[i][0];
      var optLabel = options[i][1];
      var selected = (String(value) === String(optValue)) ? ' selected' : '';
      html += '<option value="' + escapeHtml(String(optValue)) + '"' + selected + '>' + escapeHtml(optLabel) + '</option>';
    }
    html += '</select>';
  }

  html += '</div>';
  return html;
}

function updateSelectField(blockId, key, htmlValue, options) {
  var convertedValue = htmlValue;
  for (var i = 0; i < options.length; i++) {
    if (String(options[i][0]) === htmlValue) { convertedValue = options[i][0]; break; }
  }
  updateField(blockId, key, convertedValue);
}

function renderChildSlot(block, slotDef) {
  var childList = block.children[slotDef.key] || [];
  var slotLabel = language === "zh" ? slotDef.zh : slotDef.en;
  var canAdd = !slotDef.max || childList.length < slotDef.max;

  var html = '<div class="child-slot">';
  html += '<div class="slot-header">';
  html += '<span>' + escapeHtml(slotLabel) + ' (' + childList.length + ')</span>';
  if (canAdd) {
    html += '<span class="slot-add" onclick="showChildPicker(' + block.id + ',\'' + slotDef.key + '\',\'' + slotDef.accepts + '\')">+ ' + translate('\u6DFB\u52A0','Add') + '</span>';
  }
  html += '</div>';
  if (childList.length) {
    html += '<div class="slot-list">';
    for (var i = 0; i < childList.length; i++) {
      html += renderBlock(childList[i], true);
    }
    html += '</div>';
  } else {
    html += '<div class="slot-empty">' + translate('\u7A7A','empty') + '</div>';
  }
  html += '</div>';
  return html;
}

/* ============================================================
   MODAL (原始完整版 + raw_cmd 保存预设)
   ============================================================ */
function showModal(html) {
  document.getElementById("modalBox").innerHTML = html;
  document.getElementById("modalOverlay").style.display = "flex";
}
function hideModal() {
  document.getElementById("modalOverlay").style.display = "none";
}

function showChildPicker(parentId, slotKey, accepts) {
  var html = '<div class="modal-title">' + translate('\u9009\u62E9\u79EF\u6728\u5757','Choose Block') + '</div>';
  if (accepts === "condition") {
    for (var condType in CONDITION_DEFS) {
      var condDef = CONDITION_DEFS[condType];
      var label = language === "zh" ? condDef.zh : condDef.en;
      html += '<div class="modal-item" onclick="addChildBlock(' + parentId + ',\'' + slotKey + '\',\'cond_' + condType + '\')">';
      html += '\u2753 ' + escapeHtml(label) + '</div>';
    }
  } else {
    for (var ci = 0; ci < CATEGORIES.length; ci++) {
      var cat = CATEGORIES[ci];
      if (cat.id === "condition") continue;
      var items = [];
      for (var blockType in BLOCK_DEFS) {
        if (BLOCK_DEFS[blockType].cat === cat.id) items.push(blockType);
      }
      if (!items.length) continue;
      html += '<div class="modal-category">' + cat.icon + ' ' + (language === "zh" ? cat.zh : cat.en) + '</div>';
      for (var bi = 0; bi < items.length; bi++) {
        var def = BLOCK_DEFS[items[bi]];
        var label = language === "zh" ? def.zh : def.en;
        html += '<div class="modal-item" onclick="addChildBlock(' + parentId + ',\'' + slotKey + '\',\'' + items[bi] + '\')">' + escapeHtml(label) + '</div>';
      }
    }
  }
  html += '<button class="modal-close" onclick="hideModal()">' + translate('\u5173\u95ED','Close') + '</button>';
  showModal(html);
}

function showRawModal() {
  var html = '<div class="modal-title">' + translate('\u539F\u59CB JSON','Raw JSON') + '</div>';
  html += '<textarea class="raw-textarea" id="rawInput" placeholder=\'{"cmd":"set","pin":12,"value":1}\'></textarea>';
  html += '<button class="btn btn-primary" style="width:100%;margin-top:8px" onclick="sendRawJson()">' + translate('\u53D1\u9001','Send') + '</button>';
  html += '<button class="modal-close" onclick="hideModal()">' + translate('\u5173\u95ED','Close') + '</button>';
  showModal(html);
}


function sendRawJson() {
  var textarea = document.getElementById("rawInput");
  if (!textarea) return;
  var text = textarea.value.trim();
  if (!text) { addLogMessage(translate("\u5185\u5BB9\u4E3A\u7A7A","Empty"), "error"); return; }
  try { var obj = JSON.parse(text); } catch(e) { addLogMessage("JSON: " + e.message, "error"); return; }
  var target = getGlobalTarget();
  if (target) obj.target = target;
  websocketSend(obj);
  hideModal();
}

/* ============================================================
   JSON GENERATION (原始完整版 + raw_cmd 处理)
   ============================================================ */
function blockToJson(block) {
  var def = getBlockDef(block.type);
  if (!def) return null;

  /* raw_cmd: 直接解析 JSON */
  if (def.cmd === "__raw__") {
    try { return JSON.parse(block.params.json || "{}"); }
    catch(e) { addLogMessage("Raw JSON error: " + e.message, "error"); return null; }
  }

  if (def.isCondition) return conditionToJson(block, def);

  var obj = {};
  obj.cmd = def.cmd;
  if (def.action) obj.action = def.action;
  if (def.moduleType) obj.module_type = def.moduleType;

  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      var field = def.fields[i];
      var key = field[0], fieldType = field[3], value = block.params[key];
      if (value === undefined || value === null || value === "") continue;
      if (fieldType === "number") value = parseFloat(value) || 0;
      else if (fieldType === "pin" || fieldType === "touch_pin") value = parseInt(value) || 0;
      else if (fieldType === "select") {
        var options = field[4];
        if (options) {
          for (var j = 0; j < options.length; j++) {
            if (String(options[j][0]) === String(value)) { value = options[j][0]; break; }
          }
        }
      }
      obj[key] = value;
    }
  }

  if (def.children) {
    for (var c = 0; c < def.children.length; c++) {
      var slotDef = def.children[c];
      var childList = block.children[slotDef.key] || [];
      if (!childList.length) continue;
      if (slotDef.key === "template") {
        obj.template = blockToJson(childList[0]);
      } else if (slotDef.key === "condition") {
        if (childList.length === 1) obj.condition = blockToJson(childList[0]);
        else {
          var condArr = [];
          for (var j = 0; j < childList.length; j++) { var cj = blockToJson(childList[j]); if (cj) condArr.push(cj); }
          obj.condition = condArr;
        }
      } else {
        var arr = [];
        for (var j = 0; j < childList.length; j++) { var cj = blockToJson(childList[j]); if (cj) arr.push(cj); }
        obj[slotDef.key] = arr;
      }
    }
  }

  if (block.params.target) obj.target = block.params.target;
  var globalPersist = document.getElementById("globalPersist");
  if (globalPersist && globalPersist.checked && obj.hasOwnProperty("persistent")) obj.persistent = true;
  return obj;
}

function conditionToJson(block, def) {
  var obj = {};
  if (def.source) obj.source = def.source;
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      var field = def.fields[i], key = field[0], fieldType = field[3], value = block.params[key];
      if (value === undefined || value === null || value === "") continue;
      if (key === "op" && OP_MAP[value]) value = OP_MAP[value];
      if (fieldType === "number") value = parseFloat(value) || 0;
      else if (fieldType === "pin" || fieldType === "touch_pin") value = parseInt(value) || 0;
      else if (fieldType === "select") {
        var options = field[4];
        if (options) { for (var j = 0; j < options.length; j++) { if (String(options[j][0]) === String(value)) { value = options[j][0]; break; } } }
      }
      obj[key] = value;
    }
  }
  return obj;
}

function buildFinalJson() {
  if (!blockTree.length) return null;
  var commands = [];
  for (var i = 0; i < blockTree.length; i++) {
    var json = blockToJson(blockTree[i]);
    if (json) commands.push(json);
  }
  if (!commands.length) return null;
  if (commands.length === 1) return commands[0];
  return { cmd: "batch", commands: commands };
}

function updateJsonPreview() {
  var json = buildFinalJson();
  var el = document.getElementById("jsonContent");
  if (el) el.textContent = json ? JSON.stringify(json, null, 2) : translate("\u7A7A","Empty");
}

function onModeChange() {
  var mode = document.getElementById("sendMode").value;
  var delayGroup = document.getElementById("sendDelay").parentElement;
  if (mode === "batch") {
    delayGroup.style.display = "none";
  } else {
    delayGroup.style.display = "";
  }
  stopLoop();
}


/* ============================================================
   SEND
   ============================================================ */
function stopLoop() {
  stopSeqLoop();
  if (loopTimer !== null) {
    clearInterval(loopTimer);
    loopTimer = null;
    loopIteration = 0;
    clearAllHighlights();
    updateSendButton(false);
    addLogMessage(translate("\u5FAA\u73AF\u5DF2\u505C\u6B62","Loop stopped"), "ok");
  }
}

function stopSeqLoop() {
  if (seqLoopTimer !== null) {
    clearTimeout(seqLoopTimer);
    seqLoopTimer = null;
    loopIteration = 0;
    clearAllHighlights();
    updateSendButton(false);
    addLogMessage(translate("\u987A\u5E8F\u5FAA\u73AF\u5DF2\u505C\u6B62","SeqLoop stopped"), "ok");
  }
}


function sendAll() {
  var json = buildFinalJson();
  if (!json) { addLogMessage(translate("\u6CA1\u6709\u547D\u4EE4","No commands"), "error"); return; }
  var mode = document.getElementById("sendMode").value;
  var delayMs = parseInt(document.getElementById("sendDelay").value) || 500;

  if (mode === "batch") {
    stopLoop();
    clearAllHighlights();
    websocketSend(json);
  }
  else if (mode === "step") {
    stopLoop();
    if (stepIndex >= blockTree.length) { stepIndex = 0; clearAllHighlights(); }
    highlightBlock(blockTree[stepIndex].id);
    var blockJson = blockToJson(blockTree[stepIndex]);
    if (blockJson) {
      var def = getBlockDef(blockTree[stepIndex].type);
      var label = def ? (language === "zh" ? def.zh : def.en) : blockTree[stepIndex].type;
      addLogMessage("Step " + (stepIndex + 1) + "/" + blockTree.length + ": " + label, "sent");
      websocketSend(blockJson);
    }
    stepIndex++;
    if (stepIndex >= blockTree.length) {
      addLogMessage(translate("\u6240\u6709\u6B65\u9AA4\u5DF2\u6267\u884C\u5B8C\u6BD5","All steps completed"), "ok");
      setTimeout(clearAllHighlights, 800);
    }
  }
  else if (mode === "loop") {
    if (loopTimer !== null) {
      stopLoop();
      return;
    }
    loopIteration = 0;
    updateSendButton(true);
    flashAllBlocks();
    websocketSend(json);
    addLogMessage(translate("\u5FAA\u73AF\u5DF2\u542F\u52A8\uFF0C\u95F4\u9694 ","Loop started, interval ") + delayMs + "ms", "ok");
    loopTimer = setInterval(function() {
      loopIteration++;
      flashAllBlocks();
      var again = buildFinalJson();
      if (again) {
        websocketSend(again);
        addLogMessage(translate("\u5FAA\u73AF #","Loop #") + loopIteration, "sent");
      }
    }, delayMs);
  }
  else if (mode === "sequential") {
    stopLoop();
    var total = blockTree.length;
    for (var i = 0; i < total; i++) {
      (function(index) {
        setTimeout(function() {
          highlightBlock(blockTree[index].id);
          var def = getBlockDef(blockTree[index].type);
          var label = def ? (language === "zh" ? def.zh : def.en) : blockTree[index].type;
          addLogMessage("Seq " + (index + 1) + "/" + total + ": " + label, "sent");
          var blockJson = blockToJson(blockTree[index]);
          if (blockJson) websocketSend(blockJson);
          if (index === total - 1) {
            addLogMessage(translate("\u987A\u5E8F\u6267\u884C\u5B8C\u6BD5","Sequential completed"), "ok");
            setTimeout(clearAllHighlights, 600);
          }
        }, index * delayMs);
      })(i);
    }
  }
    else if (mode === "sequential") {
    stopLoop();
    stopSeqLoop();
    var total = blockTree.length;
    for (var i = 0; i < total; i++) {
      (function(index) {
        setTimeout(function() {
          highlightBlock(blockTree[index].id);
          var def = getBlockDef(blockTree[index].type);
          var label = def ? (language === "zh" ? def.zh : def.en) : blockTree[index].type;
          addLogMessage("Seq " + (index + 1) + "/" + total + ": " + label, "sent");
          var blockJson = blockToJson(blockTree[index]);
          if (blockJson) websocketSend(blockJson);
          if (index === total - 1) {
            addLogMessage(translate("\u987A\u5E8F\u6267\u884C\u5B8C\u6BD5","Sequential completed"), "ok");
            setTimeout(clearAllHighlights, 600);
          }
        }, index * delayMs);
      })(i);
    }
  }

  /* ▼▼▼ 新增：SeqLoop — 逐条顺序循环 ▼▼▼ */
  else if (mode === "seqloop") {
    if (seqLoopTimer !== null) {
      stopSeqLoop();
      return;
    }
    if (blockTree.length === 0) return;
    loopIteration = 0;
    updateSendButton(true);
    sendNextSeqLoop(0, delayMs);
  }
  /* ▲▲▲ 新增结束 ▲▲▲ */
}

function sendNextSeqLoop(index, delayMs) {
  if (index >= blockTree.length) {
    index = 0;
    loopIteration++;
    addLogMessage(translate("\u7B2C ","#") + loopIteration + translate(" \u8F6E\u5FAA\u73AF\u5B8C\u6210"," loop round done"), "ok");
  }
  highlightBlock(blockTree[index].id);
  var def = getBlockDef(blockTree[index].type);
  var label = def ? (language === "zh" ? def.zh : def.en) : blockTree[index].type;
  var round = loopIteration + 1;
  addLogMessage("SL " + round + "." + (index + 1) + "/" + blockTree.length + ": " + label, "sent");
  var blockJson = blockToJson(blockTree[index]);
  if (blockJson) websocketSend(blockJson);

  var nextIndex = index + 1;
  seqLoopTimer = setTimeout(function() {
    seqLoopTimer = null;
    sendNextSeqLoop(nextIndex, delayMs);
  }, delayMs);
}



/* ============================================================
   WEBSOCKET
   ============================================================ */
function connectWebSocket() {
  if (websocket && websocket.readyState <= 1) return;
  try { websocket = new WebSocket("ws://" + location.hostname + ":8080/"); } catch(e) { return; }
  websocket.onopen = function() {
    websocketConnected = true;
    document.getElementById("connDot").classList.add("connected");
    document.getElementById("connText").textContent = translate("\u5DF2\u8FDE\u63A5","Connected");
    addLogMessage(translate("\u5DF2\u8FDE\u63A5","Connected"), "ok");
  };
  websocket.onmessage = function(event) {
    var msg = event.data;
    if (msg === "CONNECTED") return;
    addLogMessage(msg, "ok");
  };
  websocket.onerror = function() { addLogMessage(translate("\u9519\u8BEF","Error"), "error"); };
  websocket.onclose = function() {
    websocketConnected = false;
    document.getElementById("connDot").classList.remove("connected");
    document.getElementById("connText").textContent = translate("\u5DF2\u65AD\u5F00","Disconnected");
    setTimeout(connectWebSocket, 3000);
  };
}

function websocketSend(json) {
  if (!websocket || websocket.readyState !== 1) {
    addLogMessage(translate("\u672A\u8FDE\u63A5","Not connected"), "error");
    return false;
  }
  var str = typeof json === "string" ? json : JSON.stringify(json);
  websocket.send(str);
  addLogMessage("TX: " + str, "sent");
  return true;
}

/* ============================================================
   LOG
   ============================================================ */
function addLogMessage(message, type) {
  var el = document.getElementById("logContent");
  var timestamp = new Date().toLocaleTimeString();
  var className = "log-" + (type || "ok");
  el.innerHTML = '<div class="log-entry"><span class="log-time">[' + timestamp + ']</span> <span class="' + className + '">' + escapeHtml(message) + '</span></div>' + el.innerHTML;
  if (el.children.length > 30) el.removeChild(el.lastChild);
}

function togglePanel(name) {
  document.getElementById(name + "Panel").classList.toggle("show");
}

function copyJson() {
  var json = buildFinalJson();
  if (!json) return;
  navigator.clipboard.writeText(JSON.stringify(json, null, 2)).then(function() {
    addLogMessage(translate("\u5DF2\u590D\u5236","Copied"), "ok");
  });
}

/* ============================================================
   PRESETS (文件导入导出)
   ============================================================ */
function exportPresets() {
  if (blockTree.length === 0) {
    showToast(translate('\u753B\u5E03\u4E3A\u7A7A\uFF0C\u65E0\u6CD5\u5BFC\u51FA','Canvas empty, nothing to export'), "err");
    return;
  }
  var blocks = JSON.parse(JSON.stringify(blockTree));
  function stripIds(b) {
    delete b.id;
    if (b.children) { for (var k in b.children) b.children[k].forEach(stripIds); }
  }
  blocks.forEach(stripIds);
  var data = { version: 1, blocks: blocks, timestamp: Date.now() };
  var json = JSON.stringify(data, null, 2);
  var blob = new Blob([json], { type: "application/json" });
  var url = URL.createObjectURL(blob);
  var a = document.createElement("a");
  a.href = url;
  a.download = "esp32_preset_" + new Date().toISOString().slice(0,10) + ".json";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
  addLogMessage(translate('\u5BFC\u51FA\u6210\u529F: ','Exported: ') + blocks.length + translate(' \u4E2A\u79EF\u6728\u5757',' block(s)'), "ok");
  showToast(translate('\u5BFC\u51FA\u6210\u529F','Export successful'), "ok");
}


function handleImportFile(input) {
  if (!input.files || input.files.length === 0) return;
  var file = input.files[0];
  if (!file.name.endsWith('.json')) {
    showToast(translate('\u8BF7\u9009\u62E9.json\u6587\u4EF6','Please select a .json file'), "err");
    input.value = '';
    return;
  }
  var reader = new FileReader();
  reader.onload = function(e) {
    try {
      var data = JSON.parse(e.target.result);
      var importBlocks = null;
      if (data.blocks && Array.isArray(data.blocks)) {
        importBlocks = data.blocks;
      } else if (data.presets && Array.isArray(data.presets)) {
        var allBlocks = [];
        data.presets.forEach(function(p) {
          if (p.blocks && Array.isArray(p.blocks)) {
            allBlocks = allBlocks.concat(p.blocks);
          }
        });
        importBlocks = allBlocks;
      }
      if (!importBlocks || importBlocks.length === 0) {
        showToast(translate('\u65E0\u6548\u7684\u9884\u8BBE\u6587\u4EF6\u683C\u5F0F','Invalid preset file format'), "err");
        return;
      }
      function restoreIds(b) {
        b.id = nextBlockId++;
        if (!b.params) b.params = {};
        if (!b.children) b.children = {};
        var def = getBlockDef(b.type);
        if (def && def.children) {
          def.children.forEach(function(slot) {
            if (!b.children[slot.key]) b.children[slot.key] = [];
            b.children[slot.key].forEach(restoreIds);
          });
        }
      }
      importBlocks.forEach(restoreIds);
      for (var i = 0; i < importBlocks.length; i++) {
        blockTree.push(importBlocks[i]);
      }
      selectedBlockId = null;
      renderCanvas();
      updateJsonPreview();
      addLogMessage(translate('\u5BFC\u5165\u6210\u529F: ','Imported: ') + importBlocks.length + translate(' \u4E2A\u79EF\u6728\u5757\u5DF2\u8FFD\u52A0\u5230\u753B\u5E03',' block(s) appended to canvas'), "ok");
      showToast(translate('\u5BFC\u5165\u6210\u529F','Import successful'), "ok");
    } catch(ex) {
      showToast(translate('\u6587\u4EF6\u89E3\u6790\u5931\u8D25','File parse error'), "err");
    }
    input.value = '';
  };
  reader.readAsText(file);
}


/* ============================================================
   LANGUAGE
   ============================================================ */
function applyLanguage() {
    document.getElementById("optBatch").textContent = translate("\u6279\u91CF\uFF08\u5355\u6B21\uFF09","Batch");
  document.getElementById("optLoop").textContent = translate("\u5FAA\u73AF\uFF08\u91CD\u590D\uFF09","Loop");
  document.getElementById("optStep").textContent = translate("\u6B65\u8FDB\uFF08\u8C03\u8BD5\uFF09","Step");
  document.getElementById("optSeq").textContent = translate("\u987A\u5E8F\uFF08\u9010\u6761\uFF09","Sequential");
   document.getElementById("optSeqLoop").textContent = translate("\u987A\u5E8F\u5FAA\u73AF","SeqLoop");
  document.getElementById("headerTitle").textContent = translate("\u547D\u4EE4\u6784\u5EFA\u5668","Command Builder");
  document.getElementById("btnBack").innerHTML = "&larr; " + translate("\u914D\u7F6E","Config");
  document.getElementById("btnRaw").textContent = translate("\u539F\u59CB","Raw");
  updateSendButton(loopTimer !== null);
    document.getElementById("btnClear").textContent = translate("\u6E05\u7A7A","Clear");
  document.getElementById("btnLang").textContent = language === "zh" ? "EN" : "\u4E2D";
  document.getElementById("labelTarget").textContent = translate("\u76EE\u6807:","Target:");
  document.getElementById("labelPersist").textContent = translate("\u4FDD\u5B58NVS","Save NVS");
  document.getElementById("labelMode").textContent = translate("\u6A21\u5F0F:","Mode:");
  document.getElementById("labelDelay").textContent = translate("\u5EF6\u8FDF(ms):","Delay(ms):");
  document.getElementById("jsonTitle").textContent = panelMode === "json"
    ? translate("JSON \u9884\u89C8","JSON Preview") : translate("\u65E5\u5FD7","Log");
  document.getElementById("btnPanelMode").textContent = panelMode === "json"
    ? translate("\u65E5\u5FD7","Log") : "JSON";
  document.getElementById("connText").textContent = websocketConnected
    ? translate("\u5DF2\u8FDE\u63A5","Connected") : translate("\u5DF2\u65AD\u5F00","Disconnected");
  refreshTargets();
  renderPalette();
  renderCanvas();
  updateJsonPreview();
}

function toggleLanguage() {
  language = (language === "zh") ? "en" : "zh";
  localStorage.setItem("lang", language);
  applyLanguage();
}

/* ============================================================
   INIT
   ============================================================ */
function loadDeviceInfo() {
  fetch("/api/info").then(function(r){ return r.json(); }).then(function(data) {
    var name = data.deviceName || data.deviceId || "";
    var mac = data.mac || data.deviceId || "";
    var text = name;
    if (mac && mac !== name) text += " | " + mac;
    document.getElementById("deviceInfo").textContent = text;
  }).catch(function() {
    fetch("/api/devices").then(function(r){ return r.json(); }).then(function(data) {
      var id = data.deviceId || "";
      if (id) document.getElementById("deviceInfo").textContent = id;
    }).catch(function(){});
  });
}

document.addEventListener("DOMContentLoaded", function() {
  applyLanguage();
  connectWebSocket();
  loadDeviceInfo();
});

</script>
</body>
</html>
)rawliteral";
