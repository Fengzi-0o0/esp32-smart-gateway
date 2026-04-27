static const char CMD_BUILDER_PAGE[] PROGMEM = R"rawliteral(
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
.app{display:flex;flex-direction:column;height:100%}

/* Header */
.header{display:flex;align-items:center;justify-content:space-between;padding:10px 16px;background:var(--card);border-bottom:1px solid var(--border);flex-shrink:0;flex-wrap:wrap;gap:6px}
.header h1{font-size:14px;font-weight:700}
.header-right{display:flex;gap:6px;align-items:center}
.btn{padding:5px 10px;border:1px solid var(--border);background:var(--surface);color:var(--text);border-radius:5px;font-size:10px;cursor:pointer;transition:.2s;text-decoration:none;font-family:inherit}
.btn:hover{border-color:var(--cyan);color:var(--cyan)}
.btn-primary{background:var(--cyan);color:#000;border-color:var(--cyan);font-weight:600}
.btn-primary:hover{background:#33ddff}
.conn-dot{width:8px;height:8px;border-radius:50%;background:var(--red);display:inline-block}
.conn-dot.connected{background:var(--green);box-shadow:0 0 6px var(--green)}

/* Settings Bar */
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

/* Workspace */
.workspace{display:flex;flex:1;overflow:hidden}

/* Palette */
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

/* Canvas */
.canvas{flex:1;overflow-y:auto;padding:12px;display:flex;flex-direction:column;gap:8px}
.canvas-empty{display:flex;align-items:center;justify-content:center;flex:1;color:var(--muted);font-size:12px;text-align:center;padding:30px}

/* Block Card */
.block{border-radius:8px;border:1px solid var(--border);border-left:3px solid var(--border);transition:.15s;overflow:hidden}
.block.selected{border-color:var(--cyan);box-shadow:0 0 10px rgba(0,212,255,.1)}
.block-header{display:flex;align-items:center;gap:6px;padding:7px 10px;cursor:pointer;user-select:none}
.block-header .icon{font-size:13px;flex-shrink:0}
.block-header .name{font-size:11px;font-weight:600;flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.block-header .actions{display:flex;gap:3px;flex-shrink:0}
.block-action{width:18px;height:18px;border-radius:3px;border:1px solid var(--border);background:none;color:var(--muted);font-size:9px;cursor:pointer;display:flex;align-items:center;justify-content:center;transition:.15s}
.block-action:hover{border-color:var(--cyan);color:var(--cyan)}
.block-action.delete:hover{border-color:var(--red);color:var(--red)}
.block-body{padding:4px 10px 8px}

/* Block Category Colors */
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

/* Fields */
.field{margin-bottom:6px}
.field label{display:block;font-size:9px;color:var(--muted);margin-bottom:2px;font-weight:500}
.field input,.field select{width:100%;padding:5px 7px;background:var(--bg);border:1px solid var(--border);border-radius:4px;color:var(--text);font-size:11px;font-family:'JetBrains Mono',monospace;outline:none;transition:.15s}
.field input:focus,.field select:focus{border-color:var(--cyan)}
.field select{cursor:pointer}
.field select option{background:var(--card)}
.field-check{display:flex;align-items:center;gap:4px;margin-bottom:4px}
.field-check input{accent-color:var(--cyan);width:12px;height:12px}
.field-check label{font-size:10px;cursor:pointer;color:var(--text)}

/* Target Override */
.target-override{margin-top:4px}
.target-override label{display:block;font-size:9px;color:var(--purple);margin-bottom:2px;font-weight:500}
.target-override input{width:100%;padding:5px 7px;background:var(--bg);border:1px solid rgba(167,139,250,.2);border-radius:4px;color:var(--text);font-size:10px;font-family:'JetBrains Mono',monospace;outline:none;transition:.15s}
.target-override input:focus{border-color:var(--purple)}

/* Child Slot */
.child-slot{margin-top:6px;padding:6px;border:1px dashed var(--border);border-radius:6px;background:rgba(0,0,0,.15)}
.slot-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:4px;font-size:10px;font-weight:600;color:var(--muted)}
.slot-add{color:var(--cyan);cursor:pointer;font-size:10px;padding:1px 6px;border:1px solid rgba(0,212,255,.3);border-radius:3px;background:rgba(0,212,255,.06);transition:.15s}
.slot-add:hover{background:rgba(0,212,255,.15)}
.slot-list{display:flex;flex-direction:column;gap:4px}
.slot-empty{font-size:10px;color:var(--muted);text-align:center;padding:8px;font-style:italic}

/* Panels */
.panel{background:var(--card);border-top:1px solid var(--border);flex-shrink:0}
.panel-header{display:flex;justify-content:space-between;align-items:center;padding:8px 16px;cursor:pointer}
.panel-header h3{font-size:11px;font-weight:600}
.panel-body{display:none;padding:0 16px 10px}
.panel-body.show{display:block}
.panel-content{background:var(--bg);border:1px solid var(--border);border-radius:6px;padding:10px;font-family:'JetBrains Mono',monospace;font-size:10px;line-height:1.5;max-height:150px;overflow:auto;white-space:pre-wrap;word-break:break-all}

/* Log */
.log-entry{border-bottom:1px solid rgba(255,255,255,.03);padding:1px 0}
.log-time{color:var(--muted)}
.log-ok{color:var(--green)}.log-error{color:var(--red)}.log-sent{color:var(--amber)}

/* Modal */
.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.6);z-index:999;display:flex;align-items:center;justify-content:center}
.modal-box{background:var(--surface);border:1px solid var(--border);border-radius:12px;width:90%;max-width:420px;max-height:80vh;overflow-y:auto;padding:14px}
.modal-title{font-size:13px;font-weight:600;margin-bottom:10px}
.modal-category{font-size:10px;font-weight:600;color:var(--muted);margin:8px 0 4px;padding:0 4px}
.modal-item{padding:7px 10px;margin:2px 0;background:var(--card);border:1px solid var(--border);border-radius:6px;cursor:pointer;font-size:11px;transition:.15s}
.modal-item:hover{border-color:var(--cyan);background:rgba(0,212,255,.06)}
.modal-close{width:100%;padding:8px;margin-top:10px;border:1px solid var(--border);background:var(--card);color:var(--text);border-radius:6px;cursor:pointer;font-size:11px;font-family:inherit}
.raw-textarea{width:100%;min-height:100px;padding:10px;background:var(--bg);border:1px solid var(--border);border-radius:6px;color:var(--cyan);font-size:12px;font-family:'JetBrains Mono',monospace;outline:none;resize:vertical}

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
    <h1 id="headerTitle">Command Builder</h1>
    <span class="conn-dot" id="connDot"></span>
    <span id="connText" style="font-size:10px;color:var(--muted)">--</span>
  </div>
  <div class="header-right">
    <a href="/" class="btn" id="btnBack">← Config</a>
    <button class="btn" onclick="showRawModal()" id="btnRaw">Raw</button>
    <button class="btn btn-primary" onclick="sendAll()" id="btnSend">Send</button>
    <button class="btn" onclick="clearCanvas()" id="btnClear">Clear</button>
    <button class="btn" onclick="toggleLanguage()" id="btnLang">EN</button>
  </div>
</div>
<div class="settings-bar">
  <div class="setting-group">
    <label id="labelTarget">Target:</label>
    <select id="targetSelect"><option value="">Local</option></select>
    <button class="btn" onclick="refreshTargets()">↻</button>
  </div>
  <div class="setting-check">
    <input type="checkbox" id="globalPersist" checked>
    <label for="globalPersist" id="labelPersist">Save NVS</label>
  </div>
  <div class="setting-group">
    <label id="labelMode">Mode:</label>
    <select id="sendMode">
      <option value="batch">Batch</option>
      <option value="sequential">Sequential</option>
    </select>
  </div>
  <div class="setting-group">
    <label id="labelDelay">Delay(ms):</label>
    <input type="number" id="sendDelay" value="500" min="0" step="100">
  </div>
</div>
<div class="workspace">
  <div class="palette">
    <div class="palette-tabs" id="paletteTabs"></div>
    <div class="palette-list" id="paletteList"></div>
  </div>
  <div class="canvas" id="canvas"></div>
</div>
<div class="panel">
  <div class="panel-header" onclick="togglePanel('json')">
    <h3 style="color:var(--purple)" id="jsonTitle">JSON Preview</h3>
    <button class="btn" onclick="event.stopPropagation();copyJson()">Copy</button>
  </div>
  <div class="panel-body" id="jsonPanel"><div class="panel-content" id="jsonContent" style="color:var(--cyan)"></div></div>
</div>
<div class="panel">
  <div class="panel-header" onclick="togglePanel('log')">
    <h3 style="color:var(--green)" id="logTitle">Log</h3>
    <button class="btn" onclick="event.stopPropagation();clearLog()">Clear</button>
  </div>
  <div class="panel-body" id="logPanel"><div class="panel-content" id="logContent"></div></div>
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
  { id:"gpio",     icon:"⚡", zh:"GPIO",     en:"GPIO" },
  { id:"sensor",   icon:"📡", zh:"传感器",   en:"Sensor" },
  { id:"input",    icon:"🔘", zh:"输入",     en:"Input" },
  { id:"timer",    icon:"⏱",  zh:"定时器",   en:"Timer" },
  { id:"logic",    icon:"🧠", zh:"逻辑",     en:"Logic" },
  { id:"condition",icon:"📊", zh:"条件",     en:"Condition" },
  { id:"i2c",      icon:"🔌", zh:"I2C",      en:"I2C" },
  { id:"onewire",  icon:"🌡",  zh:"1-Wire",  en:"1-Wire" },
  { id:"uart",     icon:"📟", zh:"UART",     en:"UART" },
  { id:"spi",      icon:"🔗", zh:"SPI",      en:"SPI" },
  { id:"touch",    icon:"👆", zh:"触摸",     en:"Touch" },
  { id:"rtc",      icon:"🕐", zh:"RTC",      en:"RTC" },
  { id:"script",   icon:"📜", zh:"脚本",     en:"Script" },
  { id:"data",     icon:"🔢", zh:"数据",     en:"Data" },
  { id:"random",   icon:"🎲", zh:"随机",     en:"Random" },
  { id:"system",   icon:"⚙",  zh:"系统",     en:"System" }
];

/* ============================================================
   BLOCK DEFINITIONS
   Each: { category, label:{zh,en}, cmd, action?, fields?, children? }
   Field: [key, zhLabel, enLabel, type, options?, default?]
   Child: { key, zhLabel, enLabel, accepts:"command"|"condition", max? }
   ============================================================ */
var BLOCK_DEFS = {
  /* GPIO */
  "set":          { cat:"gpio", zh:"数字输出",   en:"Digital SET",  cmd:"set",
    fields:[["pin","引脚","Pin","pin"],["value","电平","Value","select",[["0","LOW (0)"],["1","HIGH (1)"]]]] },
  "toggle":       { cat:"gpio", zh:"翻转电平",   en:"Toggle",       cmd:"toggle",
    fields:[["pin","引脚","Pin","pin"]] },
  "pwm":          { cat:"gpio", zh:"PWM输出",    en:"PWM",          cmd:"pwm",
    fields:[["pin","引脚","Pin","pin"],["value","占空比 0-255","Value","number",null,128]] },
  "mode":         { cat:"gpio", zh:"引脚模式",   en:"Set Mode",     cmd:"mode",
    fields:[["pin","引脚","Pin","pin"],["mode","模式","Mode","select",[["output","OUTPUT"],["input","INPUT"],["input_pullup","INPUT_PULLUP"]]]] },
  "rgb":          { cat:"gpio", zh:"RGB三色",    en:"RGB",          cmd:"rgb",
    fields:[["rPin","R引脚","R Pin","pin"],["gPin","G引脚","G Pin","pin"],["bPin","B引脚","B Pin","pin"],["r","R 0-255","R","number",null,255],["g","G 0-255","G","number",null,0],["b","B 0-255","B","number",null,0]] },
  "servo":        { cat:"gpio", zh:"舵机",       en:"Servo",        cmd:"servo",
    fields:[["pin","引脚","Pin","pin"],["angle","角度 0-180","Angle","number",null,90]] },
  "servo_detach": { cat:"gpio", zh:"舵机释放",   en:"Detach",       cmd:"servo_detach",
    fields:[["pin","引脚","Pin","pin"]] },
  "clear":        { cat:"gpio", zh:"清除全部",   en:"Clear All",    cmd:"clear" },
  "status":       { cat:"gpio", zh:"引脚状态",   en:"Status",       cmd:"status" },

  /* Sensor */
  "sensor_add":    { cat:"sensor", zh:"添加传感器", en:"Add Sensor",   cmd:"sensor", action:"add",
    fields:[["pin","引脚","Pin","pin"],["interval","间隔ms","Interval","number",null,5000],["label","标签","Label","text"]] },
  "sensor_read":   { cat:"sensor", zh:"读取传感器", en:"Read Sensor",  cmd:"sensor", action:"read",
    fields:[["pin","引脚","Pin","pin"]] },
  "sensor_remove": { cat:"sensor", zh:"删除传感器", en:"Remove Sensor",cmd:"sensor", action:"remove",
    fields:[["pin","引脚","Pin","pin"]] },
  "sensor_enable": { cat:"sensor", zh:"启用/禁用",  en:"Enable",       cmd:"sensor", action:"enable",
    fields:[["pin","引脚","Pin","pin"],["enabled","启用","Enabled","checkbox",null,true]] },
  "sensor_list":   { cat:"sensor", zh:"列出传感器", en:"List",         cmd:"sensor", action:"list" },

  /* Input */
  "input_add":    { cat:"input", zh:"添加输入", en:"Add Input",    cmd:"input", action:"add",
    fields:[["pin","引脚","Pin","pin"],["mode","模式","Mode","select",[["1","INPUT"],["2","INPUT_PULLUP"]]],["debounce","防抖ms","Debounce","number",null,50],["label","标签","Label","text"]] },
  "input_remove": { cat:"input", zh:"删除输入", en:"Remove Input", cmd:"input", action:"remove",
    fields:[["pin","引脚","Pin","pin"]] },
  "input_enable": { cat:"input", zh:"启用/禁用",en:"Enable",       cmd:"input", action:"enable",
    fields:[["pin","引脚","Pin","pin"],["enabled","启用","Enabled","checkbox",null,true]] },
  "input_list":   { cat:"input", zh:"列出输入", en:"List",         cmd:"input", action:"list" },

  /* Timer */
  "timer_add":    { cat:"timer", zh:"添加定时器", en:"Add Timer", cmd:"timer", action:"add",
    fields:[["id","ID","ID","text"],["type","类型","Type","select",[["interval","interval"],["once","once"],["count","count"]]],["interval","间隔ms","Interval","number",null,5000],["count","次数(-1=∞)","Count","number",null,-1],["duration","时长限制ms(0=不限)","Duration","number",null,0],["enabled","启用","Enabled","checkbox",null,true],["persistent","NVS","Persistent","checkbox",null,true]],
    children:[{key:"commands",zh:"执行命令",en:"Commands",accepts:"command"}] },
  "timer_remove": { cat:"timer", zh:"删除定时器", en:"Remove",  cmd:"timer", action:"remove",
    fields:[["id","ID","ID","text"]] },
  "timer_enable": { cat:"timer", zh:"启用/禁用",  en:"Enable",  cmd:"timer", action:"enable",
    fields:[["id","ID","ID","text"],["enabled","启用","Enabled","checkbox",null,true]] },
  "timer_reset":  { cat:"timer", zh:"重置",       en:"Reset",   cmd:"timer", action:"reset",
    fields:[["id","ID","ID","text"]] },
  "timer_list":   { cat:"timer", zh:"列出",       en:"List",    cmd:"timer", action:"list" },

  /* Logic */
  "logic_add":    { cat:"logic", zh:"添加规则", en:"Add Rule", cmd:"logic", action:"add",
    fields:[["id","ID","ID","text"],["operator","运算符","Operator","select",[["and","and"],["or","or"]]],["cooldown","冷却ms","Cooldown","number",null,1000],["enabled","启用","Enabled","checkbox",null,true],["persistent","NVS","Persistent","checkbox",null,true]],
    children:[{key:"conditions",zh:"条件",en:"Conditions",accepts:"condition"},{key:"actions",zh:"动作",en:"Actions",accepts:"command"}] },
  "logic_remove": { cat:"logic", zh:"删除规则", en:"Remove",   cmd:"logic", action:"remove",
    fields:[["id","ID","ID","text"]] },
  "logic_enable": { cat:"logic", zh:"启用/禁用",en:"Enable",   cmd:"logic", action:"enable",
    fields:[["id","ID","ID","text"],["enabled","启用","Enabled","checkbox",null,true]] },
  "logic_list":   { cat:"logic", zh:"列出规则", en:"List",     cmd:"logic", action:"list" },

  /* I2C */
  "i2c_scan":  { cat:"i2c", zh:"扫描设备", en:"Scan",  cmd:"i2c", action:"scan",
    fields:[["sda","SDA","SDA","pin"],["scl","SCL","SCL","pin"]] },
  "i2c_read":  { cat:"i2c", zh:"读取数据", en:"Read",  cmd:"i2c", action:"read",
    fields:[["sda","SDA","SDA","pin"],["scl","SCL","SCL","pin"],["address","地址","Addr","number",null,64],["register","寄存器","Reg","number",null,0],["count","字节数","Count","number",null,1]] },
  "i2c_write": { cat:"i2c", zh:"写入数据", en:"Write", cmd:"i2c", action:"write",
    fields:[["sda","SDA","SDA","pin"],["scl","SCL","SCL","pin"],["address","地址","Addr","number",null,64],["register","寄存器","Reg","number",null,0],["data","数据(JSON)","Data","text"]] },

  /* 1-Wire */
  "onewire_search": { cat:"onewire", zh:"搜索设备", en:"Search",   cmd:"onewire", action:"search",
    fields:[["pin","引脚","Pin","pin"]] },
  "onewire_temp":   { cat:"onewire", zh:"读温度",   en:"Read Temp",cmd:"onewire", action:"read_temp",
    fields:[["pin","引脚","Pin","pin"],["rom","ROM","ROM","text"]] },
  "onewire_reset":  { cat:"onewire", zh:"复位总线", en:"Reset",    cmd:"onewire", action:"reset",
    fields:[["pin","引脚","Pin","pin"]] },

  /* UART */
  "uart_config":      { cat:"uart", zh:"配置串口", en:"Config",   cmd:"uart", action:"config",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]],["tx","TX","TX","pin"],["rx","RX","RX","pin"],["baud","波特率","Baud","number",null,9600],["dataBits","数据位","Bits","select",[["5","5"],["6","6"],["7","7"],["8","8"]]],["stopBits","停止位","Stop","select",[["1","1"],["2","2"]]],["parity","校验","Parity","select",[["0","None"],["1","Even"],["2","Odd"]]]] },
  "uart_write":       { cat:"uart", zh:"发送字符串", en:"Write",    cmd:"uart", action:"write",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]],["data","数据","Data","text"]] },
  "uart_hex":         { cat:"uart", zh:"发送HEX",   en:"HEX",      cmd:"uart", action:"write_hex",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]],["hex","HEX","HEX","text"]] },
  "uart_read":        { cat:"uart", zh:"读取缓冲",   en:"Read",     cmd:"uart", action:"read",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_listen_start":{ cat:"uart", zh:"开启监听",   en:"Listen+",  cmd:"uart", action:"listen_start",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_listen_stop": { cat:"uart", zh:"停止监听",   en:"Listen-",  cmd:"uart", action:"listen_stop",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_close":       { cat:"uart", zh:"关闭串口",   en:"Close",    cmd:"uart", action:"close",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]]] },
  "uart_list":        { cat:"uart", zh:"列出配置",   en:"List",     cmd:"uart", action:"list" },

  /* SPI */
  "spi_config":   { cat:"spi", zh:"配置SPI",    en:"Config",   cmd:"spi", action:"config",
    fields:[["port","端口","Port","select",[["2","FSPI"],["3","HSPI"]]],["mosi","MOSI","MOSI","pin"],["miso","MISO","MISO","pin"],["sclk","SCLK","SCLK","pin"],["speed","频率Hz","Speed","number",null,1000000],["mode","模式","Mode","select",[["0","Mode0"],["1","Mode1"],["2","Mode2"],["3","Mode3"]]]] },
  "spi_transfer": { cat:"spi", zh:"传输",       en:"Transfer", cmd:"spi", action:"transfer",
    fields:[["port","端口","Port","select",[["2","FSPI"],["3","HSPI"]]],["cs","CS","CS","pin"],["data","数据(JSON)","Data","text"]] },
  "spi_read":     { cat:"spi", zh:"读取",       en:"Read",     cmd:"spi", action:"read",
    fields:[["port","端口","Port","select",[["2","FSPI"],["3","HSPI"]]],["cs","CS","CS","pin"],["count","字节数","Count","number",null,4]] },
  "spi_close":    { cat:"spi", zh:"关闭",       en:"Close",    cmd:"spi", action:"close" },
  "spi_list":     { cat:"spi", zh:"列出配置",   en:"List",     cmd:"spi", action:"list" },

  /* Touch */
  "touch_add":       { cat:"touch", zh:"添加触摸",   en:"Add",       cmd:"touch", action:"add",
    fields:[["pin","引脚","Pin","touch_pin"],["threshold","阈值(0=自动)","Threshold","number",null,0],["debounceMs","防抖ms","Debounce","number",null,50],["label","标签","Label","text"]] },
  "touch_read":      { cat:"touch", zh:"读取触摸",   en:"Read",      cmd:"touch", action:"read",
    fields:[["pin","引脚","Pin","touch_pin"]] },
  "touch_calibrate": { cat:"touch", zh:"校准",       en:"Calibrate",  cmd:"touch", action:"calibrate",
    fields:[["pin","引脚","Pin","touch_pin"],["samples","采样数","Samples","number",null,100]] },
  "touch_enable":    { cat:"touch", zh:"启用",       en:"Enable",     cmd:"touch", action:"enable",
    fields:[["pin","引脚","Pin","touch_pin"]] },
  "touch_disable":   { cat:"touch", zh:"禁用",       en:"Disable",    cmd:"touch", action:"disable",
    fields:[["pin","引脚","Pin","touch_pin"]] },
  "touch_list":      { cat:"touch", zh:"列出",       en:"List",       cmd:"touch", action:"list" },
  "touch_read_all":  { cat:"touch", zh:"读取全部",   en:"Read All",   cmd:"touch", action:"read_all" },

  /* RTC */
  "rtc_gettime": { cat:"rtc", zh:"获取时间", en:"Get Time",  cmd:"rtc", action:"get_time" },
  "rtc_sync":    { cat:"rtc", zh:"NTP同步", en:"Sync",      cmd:"rtc", action:"sync" },
  "rtc_settime": { cat:"rtc", zh:"设置时间", en:"Set Time",  cmd:"rtc", action:"set_time",
    fields:[["year","年","Year","number",null,2025],["month","月","Month","number",null,1],["day","日","Day","number",null,1],["hour","时","Hour","number",null,0],["minute","分","Min","number",null,0],["second","秒","Sec","number",null,0]] },
  "rtc_addsch":  { cat:"rtc", zh:"添加调度", en:"Add Schedule", cmd:"rtc", action:"add_schedule",
    fields:[["id","ID","ID","text"],["cron","Cron","Cron","text"],["repeat","重复","Repeat","checkbox",null,true],["enabled","启用","Enabled","checkbox",null,true]],
    children:[{key:"commands",zh:"执行命令",en:"Commands",accepts:"command"}] },
  "rtc_remsch":  { cat:"rtc", zh:"删除调度", en:"Remove",       cmd:"rtc", action:"remove_schedule",
    fields:[["id","ID","ID","text"]] },
  "rtc_listsch": { cat:"rtc", zh:"列出调度", en:"List",         cmd:"rtc", action:"list_schedules" },

  /* Script */
  "scr_varset":  { cat:"script", zh:"设置变量", en:"Set Var",    cmd:"script", action:"var_set",
    fields:[["name","变量名","Name","text"],["value","数值","Value","number",null,0],["persistent","NVS","Persistent","checkbox",null,false]] },
  "scr_varget":  { cat:"script", zh:"获取变量", en:"Get Var",    cmd:"script", action:"var_get",
    fields:[["name","变量名","Name","text"]] },
  "scr_varinc":  { cat:"script", zh:"递增变量", en:"Inc Var",    cmd:"script", action:"var_inc",
    fields:[["name","变量名","Name","text"],["step","步长","Step","number",null,1]] },
  "scr_varrm":   { cat:"script", zh:"删除变量", en:"Remove Var", cmd:"script", action:"var_remove",
    fields:[["name","变量名","Name","text"]] },
  "scr_varclr":  { cat:"script", zh:"清除变量", en:"Clear Vars", cmd:"script", action:"var_clear" },
  "scr_math":    { cat:"script", zh:"数学运算", en:"Math",       cmd:"script", action:"math",
    fields:[["op","运算","Op","select",[["add","add"],["sub","sub"],["mul","mul"],["div","div"],["mod","mod"],["abs","abs"],["min","min"],["max","max"],["round","round"],["floor","floor"],["ceil","ceil"]]],["a","a","a","number",null,0],["b","b","b","number",null,0],["result","结果变量","Result","text"],["persistent","NVS","Persistent","checkbox",null,false]] },
  "scr_if":      { cat:"script", zh:"条件判断", en:"If/Else",    cmd:"script", action:"if",
    children:[{key:"condition",zh:"条件",en:"Condition",accepts:"condition"},{key:"then",zh:"满足执行",en:"Then",accepts:"command"},{key:"else",zh:"否则执行",en:"Else",accepts:"command"}] },
  "scr_exec":    { cat:"script", zh:"执行命令", en:"Exec",       cmd:"script", action:"exec",
    children:[{key:"commands",zh:"命令",en:"Commands",accepts:"command"}] },
  "scr_list":    { cat:"script", zh:"列出变量", en:"List",       cmd:"script", action:"list" },

  /* Data */
  "data_hex":  { cat:"data", zh:"HEX→Bytes",  en:"HEX→Bytes",  cmd:"data", action:"hex_to_bytes",
    fields:[["hex","HEX","HEX","text"]] },
  "data_bhex": { cat:"data", zh:"Bytes→HEX",  en:"Bytes→HEX",  cmd:"data", action:"bytes_to_hex",
    fields:[["data","数据(JSON)","Data","text"]] },
  "data_bint": { cat:"data", zh:"Bytes→Int",  en:"Bytes→Int",  cmd:"data", action:"bytes_to_int",
    fields:[["data","数据(JSON)","Data","text"],["endian","字节序","Endian","select",[["big","big"],["little","little"]]],["signed","有符号","Signed","checkbox",null,false]] },
  "data_ib":   { cat:"data", zh:"Int→Bytes",  en:"Int→Bytes",  cmd:"data", action:"int_to_bytes",
    fields:[["value","整数","Value","number",null,0],["endian","字节序","Endian","select",[["big","big"],["little","little"]]],["length","长度","Length","number",null,2]] },
  "data_crc":  { cat:"data", zh:"CRC16",      en:"CRC16",      cmd:"data", action:"crc16",
    fields:[["data","数据(JSON)","Data","text"],["append","追加CRC","Append","checkbox",null,false]] },
  "data_cat":  { cat:"data", zh:"拼接",       en:"Concat",     cmd:"data", action:"concat",
    fields:[["arrays","数组(JSON)","Arrays","text"]] },
  "data_slice":{ cat:"data", zh:"截取",       en:"Slice",      cmd:"data", action:"slice",
    fields:[["data","数据(JSON)","Data","text"],["offset","起始","Offset","number",null,0],["length","长度","Length","number",null,2]] },
  "data_ext":  { cat:"data", zh:"协议解析",   en:"Extract",    cmd:"data", action:"extract",
    fields:[["data","原始(JSON)","Data","text"],["fields","字段(JSON)","Fields","text"]] },
  "data_bit":  { cat:"data", zh:"位操作",     en:"Bit Op",     cmd:"data", action:"bit",
    fields:[["value","值","Value","number",null,0],["bit","位号","Bit","number",null,0],["op","操作","Op","select",[["get","get"],["set","set"],["clear","clear"],["toggle","toggle"]]]] },
  "data_cmp":  { cat:"data", zh:"比较",       en:"Compare",    cmd:"data", action:"compare",
    fields:[["data_a","A(JSON)","A","text"],["data_b","B(JSON)","B","text"]] },

  /* Random */
  "rand_int":   { cat:"random", zh:"随机整数", en:"Int",   cmd:"rand", action:"int",
    fields:[["min","最小","Min","number",null,0],["max","最大","Max","number",null,100]] },
  "rand_float": { cat:"random", zh:"随机浮点", en:"Float", cmd:"rand", action:"float",
    fields:[["min","最小","Min","number",null,0],["max","最大","Max","number",null,1]] },
  "rand_bool":  { cat:"random", zh:"随机布尔", en:"Bool",  cmd:"rand", action:"bool" },
  "rand_seed":  { cat:"random", zh:"随机种子", en:"Seed",  cmd:"rand", action:"seed",
    fields:[["value","种子","Seed","number",null,0]] },
  "rand_exec":  { cat:"random", zh:"随机执行", en:"Exec",  cmd:"rand", action:"exec",
    fields:[["min","最小","Min","number",null,0],["max","最大","Max","number",null,255]],
    children:[{key:"template",zh:"模板命令",en:"Template",accepts:"command",max:1}] },

  /* System */
  "sys_status":  { cat:"system", zh:"引脚状态", en:"Status",    cmd:"status" },
  "sys_dump":    { cat:"system", zh:"导出配置", en:"Dump",      cmd:"dump" },
  "sys_restart": { cat:"system", zh:"重启",     en:"Restart",   cmd:"restart" },
  "sys_batch":   { cat:"system", zh:"批量状态", en:"Batch",     cmd:"batch_status" },
  "sys_pub":     { cat:"system", zh:"MQTT发布", en:"Publish",   cmd:"publish",
    fields:[["topic","主题","Topic","text"],["payload","内容","Payload","text"]] },
  "sys_custom":  { cat:"system", zh:"自定义",   en:"Custom",    cmd:"custom",
    fields:[["action","动作","Action","text"]] }
};

/* Condition definitions (separate category) */
var CONDITION_DEFS = {
  "sensor": { zh:"传感器值", en:"Sensor", source:"sensor",
    fields:[["pin","引脚","Pin","pin"],["op","运算符","Op","select",[["gt",">"],["lt","<"],["eq","=="],["ne","!="],[">=",">="],["<=","<="]]],["value","阈值","Value","number",null,0]] },
  "touch":  { zh:"触摸值",   en:"Touch",  source:"touch",
    fields:[["pin","引脚","Pin","touch_pin"],["op","运算符","Op","select",[["gt",">"],["lt","<"],["eq","=="],["ne","!="]]],["value","阈值","Value","number",null,0]] },
  "input":  { zh:"输入状态", en:"Input",  source:"input",
    fields:[["pin","引脚","Pin","pin"],["op","运算符","Op","select",[["eq","="],["ne","!="]]],["value","电平","Value","select",[["0","LOW"],["1","HIGH"]]]] },
  "var":    { zh:"变量值",   en:"Variable",source:"var",
    fields:[["name","变量名","Name","text"],["op","运算符","Op","select",[["gt",">"],["lt","<"],["eq","=="],["ne","!="],[">=",">="],["<=","<="]]],["value","值","Value","number",null,0]] },
  "time":   { zh:"时间",     en:"Time",   source:"time",
    fields:[["op","运算","Op","select",[["hour_eq","hour =="],["hour_gte","hour >="],["hour_lte","hour <="],["between","between"]]],["value","值1","V1","number",null,0],["value2","值2","V2","number",null,0]] },
  "rand":   { zh:"随机概率", en:"Random", source:"rand",
    fields:[["op","运算","Op","select",[["lt","<"],["gt",">"]]],["value","概率0-100","Value","number",null,50]] },
  "mqtt":   { zh:"MQTT主题", en:"MQTT",   source:"mqtt",
    fields:[["topic","主题","Topic","text"],["op","运算","Op","select",[["contains","contains"],["eq","=="]]],["value","值","Value","text"]] },
  "uart":   { zh:"UART数据", en:"UART",   source:"uart",
    fields:[["port","端口","Port","select",[["1","UART1"],["2","UART2"]]],["op","运算","Op","select",[["contains","contains"],["eq","=="],["changed","changed"]]],["value","值","Value","text"]] }
};

/* Op display-to-code mapping for conditions */
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

function translate(zh, en) { return language === "zh" ? zh : en; }
function escapeHtml(s) { return String(s).replace(/&/g,"&amp;").replace(/"/g,"&quot;").replace(/</g,"&lt;"); }

/* ============================================================
   DEVICE MANAGEMENT
   ============================================================ */
function refreshTargets() {
  fetch("/api/devices").then(function(r){ return r.json(); }).then(function(data) {
    var select = document.getElementById("targetSelect");
    var currentValue = select.value;
    cachedDevices = (data.devices || []).filter(function(d) { return d.id !== data.deviceId; });
    select.innerHTML = "<option value=\"\">" + translate("本机","Local") + "</option>";
    for (var i = 0; i < cachedDevices.length; i++) {
      var dev = cachedDevices[i];
      select.innerHTML += "<option value=\"" + dev.id + "\">" + (dev.name || dev.id) + " [" + dev.ip + "]</option>";
    }
    select.value = currentValue;
  }).catch(function() {});
}

function getGlobalTarget() {
  return document.getElementById("targetSelect").value || null;
}

/* ============================================================
   BLOCK OPERATIONS
   ============================================================ */
function getBlockDef(type) {
  if (type.indexOf("cond_") === 0) {
    var condType = type.substring(5);
    var condDef = CONDITION_DEFS[condType];
    if (!condDef) return null;
     return {
      cat: "condition",
      zh: condDef.zh,
      en: condDef.en,
      fields: condDef.fields,
      isCondition: true,
      source: condDef.source
    };

  }
  return BLOCK_DEFS[type] || null;
}

function getCategoryIcon(categoryId) {
  for (var i = 0; i < CATEGORIES.length; i++) {
    if (CATEGORIES[i].id === categoryId) return CATEGORIES[i].icon;
  }
  return "📦";
}

function createBlock(type) {
  var def = getBlockDef(type);
  if (!def) return null;

  var block = {
    id: nextBlockId++,
    type: type,
    params: {},
    children: {}
  };

  // Set condition source for condition blocks
  if (def.isCondition && def.source) {
    block.params.source = def.source;
  }

  // Set field defaults
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      var field = def.fields[i];
      var key = field[0], fieldType = field[3], defaultVal = field.length > 5 ? field[5] : null;
      if (fieldType === "checkbox" && defaultVal !== null) {
        block.params[key] = defaultVal;
      } else if (fieldType === "number" && defaultVal !== null) {
        block.params[key] = defaultVal;
      }
    }
  }

  // Initialize children slots
  if (def.children) {
    for (var i = 0; i < def.children.length; i++) {
      block.children[def.children[i].key] = [];
    }
  }

  // Apply global target
  var globalTarget = getGlobalTarget();
  if (globalTarget) block.params.target = globalTarget;

  // Apply global persist
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

  addLogMessage(translate("已添加: ","Added: ") + label, "ok");
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

  addLogMessage(translate("已嵌入 ","Nested: ") + label, "ok");
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
  if (key === "target" && !value) {
    delete block.params.target;
  } else {
    block.params[key] = value;
  }
  updateJsonPreview();
}

function clearCanvas() {
  blockTree = [];
  selectedBlockId = null;
  renderCanvas();
  updateJsonPreview();
  addLogMessage(translate("已清空","Cleared"), "ok");
}

/* ============================================================
   RENDERING - PALETTE
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

  // Block list for active category
  var listHtml = "";
  var count = 0;

  if (activeCategory === "condition") {
    for (var key in CONDITION_DEFS) {
      if (!CONDITION_DEFS.hasOwnProperty(key)) continue;
      var def = CONDITION_DEFS[key];
      var label = language === "zh" ? def.zh : def.en;
      listHtml += '<div class="palette-item" onclick="addBlock(\'cond_' + key + '\')">';
      listHtml += '<span class="icon">📊</span>';
      listHtml += '<span class="name">' + escapeHtml(label) + '</span>';
      listHtml += '</div>';
      count++;
    }
  } else {
    for (var key in BLOCK_DEFS) {
      if (!BLOCK_DEFS.hasOwnProperty(key)) continue;
      var def = BLOCK_DEFS[key];
      if (def.cat !== activeCategory) continue;
      var label = language === "zh" ? def.zh : def.en;
      var icon = getCategoryIcon(def.cat);
      listHtml += '<div class="palette-item" onclick="addBlock(\'' + key + '\')">';
      listHtml += '<span class="icon">' + icon + '</span>';
      listHtml += '<span class="name">' + escapeHtml(label) + '</span>';
      listHtml += '</div>';
      count++;
    }
  }

  if (count === 0) {
    listHtml = '<div style="padding:12px;text-align:center;color:var(--muted);font-size:11px">';
    listHtml += translate('该分类无积木','No blocks in this category') + '</div>';
  }

  listEl.innerHTML = listHtml;
  listEl.scrollTop = 0;
}

function selectCategory(categoryId) {
  activeCategory = categoryId;
  renderPalette();
}

/* ============================================================
   RENDERING - CANVAS
   ============================================================ */

function renderCanvas() {
  var canvasEl = document.getElementById("canvas");
  if (!blockTree.length) {
    canvasEl.innerHTML = '<div class="canvas-empty">' + translate("从左侧选择积木块添加到这里","Click a block from the palette on the left") + '</div>';
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

  var html = '<div class="block block-' + category + (isSelected ? ' selected' : '') + '">';

  // Header
  html += '<div class="block-header" onclick="selectBlock(' + block.id + ')">';
  html += '<span class="icon">' + icon + '</span>';
  html += '<span class="name">' + escapeHtml(label) + '</span>';
  html += '<div class="actions">';
  if (!isChild) {
    html += '<button class="block-action" onclick="event.stopPropagation();moveBlock(' + block.id + ',-1)" title="Up">▲</button>';
    html += '<button class="block-action" onclick="event.stopPropagation();moveBlock(' + block.id + ',1)" title="Down">▼</button>';
  }
  html += '<button class="block-action delete" onclick="event.stopPropagation();removeBlock(' + block.id + ')" title="Delete">✕</button>';
  html += '</div></div>';

  // Body
  html += '<div class="block-body">';

  // Fields
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      html += renderField(block, def.fields[i]);
    }
  }

  // Target override (skip for condition blocks)
  if (!def.isCondition) {
    html += '<div class="target-override">';
    html += '<label>' + translate('Target覆盖 (留空=全局)','Target override (empty=global)') + '</label>';
    html += '<input value="' + escapeHtml(block.params.target || '') + '" placeholder="' + translate('跟随全局','global') + '" onchange="updateField(' + block.id + ',\'target\',this.value || null)">';
    html += '</div>';
  }

  // Child slots
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

  // Checkbox
  if (fieldType === "checkbox") {
    return '<div class="field-check">' +
      '<input type="checkbox" id="' + fieldId + '" ' + (value ? 'checked' : '') +
      ' onchange="updateField(' + block.id + ',\'' + key + '\',this.checked)">' +
      '<label for="' + fieldId + '">' + escapeHtml(label) + '</label></div>';
  }

  var html = '<div class="field"><label>' + escapeHtml(label) + '</label>';

  // Number
  if (fieldType === "number") {
    html += '<input type="number" id="' + fieldId + '" value="' + (value !== undefined ? value : '') +
      '" onchange="updateField(' + block.id + ',\'' + key + '\',parseFloat(this.value)||0)">';
  }
  // Text
  else if (fieldType === "text") {
    html += '<input type="text" id="' + fieldId + '" value="' + escapeHtml(value || '') +
      '" onchange="updateField(' + block.id + ',\'' + key + '\',this.value)">';
  }
  // GPIO Pin
  else if (fieldType === "pin") {
    html += '<select id="' + fieldId + '" onchange="updateField(' + block.id + ',\'' + key + '\',parseInt(this.value))">';
    for (var i = 0; i < GPIO_PINS.length; i++) {
      html += '<option value="' + GPIO_PINS[i] + '"' + (value === GPIO_PINS[i] ? ' selected' : '') + '>GPIO ' + GPIO_PINS[i] + '</option>';
    }
    html += '</select>';
  }
  // Touch Pin
  else if (fieldType === "touch_pin") {
    html += '<select id="' + fieldId + '" onchange="updateField(' + block.id + ',\'' + key + '\',parseInt(this.value))">';
    for (var i = 0; i < TOUCH_PINS.length; i++) {
      html += '<option value="' + TOUCH_PINS[i] + '"' + (value === TOUCH_PINS[i] ? ' selected' : '') + '>GPIO ' + TOUCH_PINS[i] + '</option>';
    }
    html += '</select>';
  }
  // Select
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
  // Convert HTML string value back to original type from options
  var convertedValue = htmlValue;
  for (var i = 0; i < options.length; i++) {
    if (String(options[i][0]) === htmlValue) {
      convertedValue = options[i][0];
      break;
    }
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
    html += '<span class="slot-add" onclick="showChildPicker(' + block.id + ',\'' + slotDef.key + '\',\'' + slotDef.accepts + '\')">+ ' + translate('添加','Add') + '</span>';
  }
  html += '</div>';

  if (childList.length) {
    html += '<div class="slot-list">';
    for (var i = 0; i < childList.length; i++) {
      html += renderBlock(childList[i], true);
    }
    html += '</div>';
  } else {
    html += '<div class="slot-empty">' + translate('空','empty') + '</div>';
  }

  html += '</div>';
  return html;
}

/* ============================================================
   MODAL - CHILD PICKER & RAW JSON
   ============================================================ */
function showModal(html) {
  document.getElementById("modalBox").innerHTML = html;
  document.getElementById("modalOverlay").style.display = "flex";
}

function hideModal() {
  document.getElementById("modalOverlay").style.display = "none";
}

function showChildPicker(parentId, slotKey, accepts) {
  var html = '<div class="modal-title">' + translate('选择积木块','Choose Block') + '</div>';

  if (accepts === "condition") {
    for (var condType in CONDITION_DEFS) {
      var condDef = CONDITION_DEFS[condType];
      var label = language === "zh" ? condDef.zh : condDef.en;
      html += '<div class="modal-item" onclick="addChildBlock(' + parentId + ',\'' + slotKey + '\',\'cond_' + condType + '\')">';
      html += '📊 ' + escapeHtml(label) + '</div>';
    }
  } else {
    // Show all command blocks grouped by category
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

        html += '<div class="modal-item" onclick="addChildBlock(' + parentId + ',\'' + slotKey + '\',\'' + items[bi] + '\')">';
        html += escapeHtml(label) + '</div>';
      }
    }
  }

  html += '<button class="modal-close" onclick="hideModal()">' + translate('关闭','Close') + '</button>';
  showModal(html);
}

function showRawModal() {
  var html = '<div class="modal-title">' + translate('原始 JSON','Raw JSON') + '</div>';
  html += '<textarea class="raw-textarea" id="rawInput" placeholder=\'{"cmd":"set","pin":12,"value":1}\'></textarea>';
  html += '<button class="btn btn-primary" style="width:100%;margin-top:8px" onclick="sendRawJson()">' + translate('发送','Send') + '</button>';
  html += '<button class="modal-close" onclick="hideModal()">' + translate('关闭','Close') + '</button>';
  showModal(html);
}

function sendRawJson() {
  var textarea = document.getElementById("rawInput");
  if (!textarea) return;
  var text = textarea.value.trim();
  if (!text) { addLogMessage(translate("输入为空","Empty"), "error"); return; }
  try {
    var obj = JSON.parse(text);
  } catch(e) {
    addLogMessage("JSON: " + e.message, "error");
    return;
  }
  var target = getGlobalTarget();
  if (target) obj.target = target;
  websocketSend(obj);
  hideModal();
}

/* ============================================================
   JSON GENERATION
   ============================================================ */
function blockToJson(block) {
  var def = getBlockDef(block.type);
  if (!def) return null;

  // Condition blocks
  if (def.isCondition) {
    return conditionToJson(block, def);
  }

  var obj = {};

  // Command routing
  obj.cmd = def.cmd;
  if (def.action) obj.action = def.action;

  // Fields
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      var field = def.fields[i];
      var key = field[0];
      var fieldType = field[3];
      var value = block.params[key];
      if (value === undefined || value === null || value === "") continue;

      // Type conversion
      if (fieldType === "number") {
        value = parseFloat(value) || 0;
      } else if (fieldType === "pin" || fieldType === "touch_pin") {
        value = parseInt(value) || 0;
      } else if (fieldType === "select") {
        // Find original type from options
        var options = field[4];
        if (options) {
          for (var j = 0; j < options.length; j++) {
            if (String(options[j][0]) === String(value)) {
              value = options[j][0]; // Preserve original type
              break;
            }
          }
        }
      }
      // checkbox: already boolean
      // text: already string

      obj[key] = value;
    }
  }

  // Children
  if (def.children) {
    for (var c = 0; c < def.children.length; c++) {
      var slotDef = def.children[c];
      var childList = block.children[slotDef.key] || [];
      if (!childList.length) continue;

      if (slotDef.key === "template") {
        // Single object
        obj.template = blockToJson(childList[0]);
      } else if (slotDef.key === "condition") {
        // Single condition object (or array if somehow multiple)
        if (childList.length === 1) {
          obj.condition = blockToJson(childList[0]);
        } else {
          var condArr = [];
          for (var j = 0; j < childList.length; j++) {
            var cj = blockToJson(childList[j]);
            if (cj) condArr.push(cj);
          }
          obj.condition = condArr;
        }
      } else {
        // Array: commands, actions, then, else
        var arr = [];
        for (var j = 0; j < childList.length; j++) {
          var cj = blockToJson(childList[j]);
          if (cj) arr.push(cj);
        }
        obj[slotDef.key] = arr;
      }
    }
  }

  // Target override
  if (block.params.target) {
    obj.target = block.params.target;
  }

  // Global persistent override
  var globalPersist = document.getElementById("globalPersist");
  if (globalPersist && globalPersist.checked && obj.hasOwnProperty("persistent")) {
    obj.persistent = true;
  }

  return obj;
}

function conditionToJson(block, def) {
  var obj = {};

  // Source
  if (def.source) obj.source = def.source;

  // Fields with proper type conversion
  if (def.fields) {
    for (var i = 0; i < def.fields.length; i++) {
      var field = def.fields[i];
      var key = field[0];
      var fieldType = field[3];
      var value = block.params[key];
      if (value === undefined || value === null || value === "") continue;

      // Special handling for "op" field - convert display values to code
      if (key === "op" && OP_MAP[value]) {
        value = OP_MAP[value];
      }

      if (fieldType === "number") {
        value = parseFloat(value) || 0;
      } else if (fieldType === "pin" || fieldType === "touch_pin") {
        value = parseInt(value) || 0;
      } else if (fieldType === "select") {
        var options = field[4];
        if (options) {
          for (var j = 0; j < options.length; j++) {
            if (String(options[j][0]) === String(value)) {
              value = options[j][0];
              break;
            }
          }
        }
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
  el.textContent = json ? JSON.stringify(json, null, 2) : translate("空","Empty");
}

/* ============================================================
   SEND
   ============================================================ */
function sendAll() {
  var json = buildFinalJson();
  if (!json) { addLogMessage(translate("没有命令","No commands"), "error"); return; }

  var mode = document.getElementById("sendMode").value;

  if (mode === "batch") {
    websocketSend(json);
  } else {
    // Sequential: send top-level blocks one at a time
    var delayMs = parseInt(document.getElementById("sendDelay").value) || 500;
    for (var i = 0; i < blockTree.length; i++) {
      (function(index) {
        setTimeout(function() {
          var blockJson = blockToJson(blockTree[index]);
          if (blockJson) websocketSend(blockJson);
        }, index * delayMs);
      })(i);
    }
  }
}

/* ============================================================
   WEBSOCKET
   ============================================================ */
function connectWebSocket() {
  if (websocket && websocket.readyState <= 1) return;
  try {
    websocket = new WebSocket("ws://" + location.hostname + ":8080/");
  } catch(e) { return; }

  websocket.onopen = function() {
    websocketConnected = true;
    document.getElementById("connDot").classList.add("connected");
    document.getElementById("connText").textContent = translate("已连接","Connected");
    addLogMessage(translate("已连接","Connected"), "ok");
  };
  websocket.onmessage = function(event) {
    var msg = event.data;
    if (msg === "CONNECTED") return;
    addLogMessage(msg, "ok");
  };
  websocket.onerror = function() {
    addLogMessage(translate("连接错误","Error"), "error");
  };
  websocket.onclose = function() {
    websocketConnected = false;
    document.getElementById("connDot").classList.remove("connected");
    document.getElementById("connText").textContent = translate("断开","Disconnected");
    setTimeout(connectWebSocket, 3000);
  };
}

function websocketSend(json) {
  if (!websocket || websocket.readyState !== 1) {
    addLogMessage(translate("未连接","Not connected"), "error");
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

function clearLog() {
  document.getElementById("logContent").innerHTML = "";
}

function togglePanel(name) {
  document.getElementById(name + "Panel").classList.toggle("show");
}

function copyJson() {
  var json = buildFinalJson();
  if (!json) return;
  navigator.clipboard.writeText(JSON.stringify(json, null, 2)).then(function() {
    addLogMessage(translate("已复制","Copied"), "ok");
  });
}

/* ============================================================
   LANGUAGE
   ============================================================ */
function applyLanguage() {
  document.getElementById("headerTitle").textContent = translate("命令构建器","Command Builder");
  document.getElementById("btnBack").textContent = translate("← 配置","← Config");
  document.getElementById("btnRaw").textContent = translate("原始","Raw");
  document.getElementById("btnSend").textContent = translate("发送","Send");
  document.getElementById("btnClear").textContent = translate("清空","Clear");
  document.getElementById("btnLang").textContent = language === "zh" ? "EN" : "中文";
  document.getElementById("labelTarget").textContent = translate("目标:","Target:");
  document.getElementById("labelPersist").textContent = translate("持久化NVS","Save NVS");
  document.getElementById("labelMode").textContent = translate("模式:","Mode:");
  document.getElementById("labelDelay").textContent = translate("间隔(ms):","Delay(ms):");
  document.getElementById("jsonTitle").textContent = translate("JSON 预览","JSON Preview");
  document.getElementById("logTitle").textContent = translate("日志","Log");
  document.getElementById("connText").textContent = websocketConnected ? translate("已连接","Connected") : translate("断开","Disconnected");

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
document.addEventListener("DOMContentLoaded", function() {
  applyLanguage();
  connectWebSocket();
});
</script>
</body>
</html>
)rawliteral";
