#ifndef PINS_PAGE_H
#define PINS_PAGE_H

#include <Arduino.h>

static const char PINS_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-S3 Pin Reference</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600&family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{
  --bg:#0a0e1a;--surface:#111827;--card:#1a2235;
  --border:#2a3550;--text:#e2e8f0;--muted:#6b7a99;
  --accent:#3b82f6;--accent2:#8b5cf6;
  --adc:#f59e0b;--touch:#10b981;--pwm:#3b82f6;
  --warn:#ef4444;--special:#f97316;
}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);min-height:100vh;padding:20px}
.container{max-width:960px;margin:0 auto}

/* Header */
.hdr{background:linear-gradient(135deg,#1e3a5f,#2d1b69);border-radius:16px;padding:28px 32px;margin-bottom:24px;position:relative;overflow:hidden}
.hdr::before{content:'';position:absolute;top:-50%;right:-20%;width:300px;height:300px;background:radial-gradient(circle,rgba(59,130,246,.15),transparent 70%);pointer-events:none}
.hdr h1{font-size:22px;font-weight:700;letter-spacing:.5px}
.hdr p{font-size:13px;color:var(--muted);margin-top:6px}
.chip-badge{display:inline-block;background:rgba(59,130,246,.15);border:1px solid rgba(59,130,246,.3);color:var(--accent);padding:3px 10px;border-radius:20px;font-size:11px;font-family:'JetBrains Mono',monospace;margin-top:8px}

/* Legend */
.legend{display:flex;flex-wrap:wrap;gap:12px;margin-bottom:20px;padding:14px 18px;background:var(--surface);border-radius:10px;border:1px solid var(--border)}
.legend-item{display:flex;align-items:center;gap:5px;font-size:11px;color:var(--muted)}
.legend-dot{width:10px;height:10px;border-radius:50%;flex-shrink:0}

/* Section */
.section{margin-bottom:24px}
.section h2{font-size:15px;font-weight:600;margin-bottom:12px;padding-left:12px;border-left:3px solid var(--accent)}

/* Pin Grid */
.pin-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:10px}
.pin-card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:14px;transition:.2s;position:relative;overflow:hidden}
.pin-card:hover{border-color:var(--accent);transform:translateY(-1px);box-shadow:0 4px 20px rgba(59,130,246,.1)}
.pin-num{font-family:'JetBrains Mono',monospace;font-size:18px;font-weight:700;color:var(--accent);margin-bottom:6px}
.pin-num .prefix{font-size:12px;color:var(--muted);font-weight:400}
.pin-badges{display:flex;flex-wrap:wrap;gap:4px;margin-bottom:8px}
.badge{display:inline-block;padding:2px 7px;border-radius:4px;font-size:10px;font-weight:600;font-family:'JetBrains Mono',monospace}
.badge-adc{background:rgba(245,158,11,.12);color:var(--adc);border:1px solid rgba(245,158,11,.25)}
.badge-touch{background:rgba(16,185,129,.12);color:var(--touch);border:1px solid rgba(16,185,129,.25)}
.badge-pwm{background:rgba(59,130,246,.12);color:var(--pwm);border:1px solid rgba(59,130,246,.25)}
.badge-dio{background:rgba(139,92,246,.12);color:var(--accent2);border:1px solid rgba(139,92,246,.25)}
.badge-warn{background:rgba(239,68,68,.12);color:var(--warn);border:1px solid rgba(239,68,68,.25)}
.badge-special{background:rgba(249,115,22,.12);color:var(--special);border:1px solid rgba(249,115,22,.25)}
.pin-alt{font-size:10px;color:var(--muted);line-height:1.5}

/* Special pins warning */
.special-warn{background:rgba(239,68,68,.06);border:1px solid rgba(239,68,68,.2);border-radius:10px;padding:14px 18px;margin-bottom:20px}
.special-warn h3{font-size:13px;color:var(--warn);margin-bottom:8px}
.special-warn ul{list-style:none;font-size:12px;color:var(--muted);line-height:1.8}
.special-warn li::before{content:'⚠ ';color:var(--warn)}

/* Validator */
.validator{background:var(--surface);border-radius:12px;padding:24px;border:1px solid var(--border)}
.validator h3{font-size:14px;margin-bottom:12px}
.val-row{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.val-input{width:100px;padding:8px 12px;background:var(--bg);border:1px solid var(--border);border-radius:6px;color:var(--text);font-size:14px;font-family:'JetBrains Mono',monospace;outline:none}
.val-input:focus{border-color:var(--accent)}
.val-btn{padding:8px 18px;background:var(--accent);color:#fff;border:none;border-radius:6px;font-size:13px;font-weight:600;cursor:pointer;transition:.2s}
.val-btn:hover{background:#2563eb}
.val-result{margin-top:12px;padding:10px 14px;border-radius:8px;font-size:13px;display:none}
.val-result.ok{display:block;background:rgba(16,185,129,.1);border:1px solid rgba(16,185,129,.3);color:var(--touch)}
.val-result.fail{display:block;background:rgba(239,68,68,.1);border:1px solid rgba(239,68,68,.3);color:var(--warn)}
.val-result .pin-info{margin-top:6px;font-size:11px;color:var(--muted);line-height:1.6}

/* Footer */
.ftr{text-align:center;font-size:10px;color:#3a4560;margin-top:30px;padding-bottom:20px}
</style>
</head>
<body>
<div class="container">

<div class="hdr">
  <h1>ESP32-S3-N16R8 引脚参考</h1>
  <p>Pin Reference &amp; Capability Guide</p>
  <span class="chip-badge">ESP32-S3-N16R8 &middot; 30 可用引脚</span>
</div>

<div class="legend">
  <div class="legend-item"><span class="legend-dot" style="background:var(--pwm)"></span>DIO 数字IO</div>
  <div class="legend-item"><span class="legend-dot" style="background:var(--pwm)"></span>PWM 模拟输出</div>
  <div class="legend-item"><span class="legend-dot" style="background:var(--adc)"></span>ADC 模拟输入</div>
  <div class="legend-item"><span class="legend-dot" style="background:var(--touch)"></span>TOUCH 触摸</div>
  <div class="legend-item"><span class="legend-dot" style="background:var(--warn)"></span>特殊功能</div>
</div>

<div class="special-warn">
  <h3>⚠ 注意事项</h3>
  <ul>
    <li>GPIO 0: BOOT按钮引脚，低电平进入下载模式，避免外部下拉</li>
    <li>GPIO 19/20: 板载USB接口（USB_D-/D+），使用后可能影响USB通信</li>
    <li>GPIO 43/44: 默认UART0串口（TXD/RXD），用于调试日志</li>
    <li>GPIO 47: 板载PSRAM时钟，不建议使用</li>
    <li>GPIO 48: 板载RGB LED，可控制但注意冲突</li>
    <li>GPIO 33-37: Flash SPI专用，<b>绝对禁止使用</b>（不在列表中）</li>
  </ul>
</div>

<div class="section">
  <h2>可用引脚列表</h2>
  <div class="pin-grid" id="grid"></div>
</div>

<div class="validator">
  <h3>🔍 引脚验证</h3>
  <div class="val-row">
    <input type="number" class="val-input" id="vpin" placeholder="引脚号" min="0" max="48">
    <button class="val-btn" onclick="validate()">验证</button>
  </div>
  <div class="val-result" id="vres"></div>
</div>

<div class="ftr">ESP32-S3 Smart Gateway &middot; Pin Reference v1.3.0</div>
</div>

<script>
var pins=[
{pin:0,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"BOOT按钮, 低电平进下载模式",warn:true},
{pin:1,digitalIO:true,pwm:true,adc:true,adcChannel:0,touch:true,alt:"ADC1_CH0, TOUCH1"},
{pin:2,digitalIO:true,pwm:true,adc:true,adcChannel:1,touch:true,alt:"ADC1_CH1, TOUCH2"},
{pin:3,digitalIO:true,pwm:true,adc:true,adcChannel:2,touch:true,alt:"JTAG, ADC1_CH2, TOUCH3",warn:true},
{pin:4,digitalIO:true,pwm:true,adc:true,adcChannel:3,touch:true,alt:"ADC1_CH3, TOUCH4"},
{pin:5,digitalIO:true,pwm:true,adc:true,adcChannel:4,touch:true,alt:"ADC1_CH4, TOUCH5"},
{pin:6,digitalIO:true,pwm:true,adc:true,adcChannel:5,touch:true,alt:"ADC1_CH5, TOUCH6"},
{pin:7,digitalIO:true,pwm:true,adc:true,adcChannel:6,touch:true,alt:"ADC1_CH6, TOUCH7"},
{pin:8,digitalIO:true,pwm:true,adc:true,adcChannel:7,touch:true,alt:"ADC1_CH7, TOUCH8"},
{pin:9,digitalIO:true,pwm:true,adc:true,adcChannel:8,touch:true,alt:"ADC1_CH8, TOUCH9"},
{pin:10,digitalIO:true,pwm:true,adc:true,adcChannel:9,touch:true,alt:"ADC1_CH9, TOUCH10"},
{pin:11,digitalIO:true,pwm:true,adc:true,adcChannel:0,touch:true,alt:"ADC2_CH0, TOUCH11"},
{pin:12,digitalIO:true,pwm:true,adc:true,adcChannel:1,touch:true,alt:"ADC2_CH1, TOUCH12"},
{pin:13,digitalIO:true,pwm:true,adc:true,adcChannel:2,touch:true,alt:"ADC2_CH2, TOUCH13, FSPIQ"},
{pin:14,digitalIO:true,pwm:true,adc:true,adcChannel:3,touch:true,alt:"ADC2_CH3, TOUCH14, FSPIWP"},
{pin:15,digitalIO:true,pwm:true,adc:true,adcChannel:4,touch:false,alt:"ADC2_CH4, U0RTS"},
{pin:16,digitalIO:true,pwm:true,adc:true,adcChannel:5,touch:false,alt:"ADC2_CH5, U0CTS"},
{pin:17,digitalIO:true,pwm:true,adc:true,adcChannel:6,touch:false,alt:"ADC2_CH6, U1TXD"},
{pin:18,digitalIO:true,pwm:true,adc:true,adcChannel:7,touch:false,alt:"ADC2_CH7, U1RXD, CLK_OUT3"},
{pin:19,digitalIO:true,pwm:true,adc:true,adcChannel:8,touch:false,alt:"ADC2_CH8, U1RTS, USB_D- (板载)",warn:true},
{pin:20,digitalIO:true,pwm:true,adc:true,adcChannel:9,touch:false,alt:"ADC2_CH9, U1CTS, USB_D+ (板载)",warn:true},
{pin:21,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"RTC GPIO"},
{pin:38,digitalIO:true,pwm:true,adc:true,adcChannel:15,touch:false,alt:"ADC1_CH15, FSPIWP"},
{pin:39,digitalIO:true,pwm:true,adc:true,adcChannel:14,touch:false,alt:"ADC1_CH14, MTCK"},
{pin:40,digitalIO:true,pwm:true,adc:true,adcChannel:13,touch:false,alt:"ADC1_CH13, MTDO, CLK_OUT2"},
{pin:41,digitalIO:true,pwm:true,adc:true,adcChannel:12,touch:false,alt:"ADC1_CH12, MTDI, CLK_OUT3"},
{pin:42,digitalIO:true,pwm:true,adc:true,adcChannel:11,touch:false,alt:"ADC1_CH11, MTMS"},
{pin:43,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"U0TXD (串口发送)",warn:true},
{pin:44,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"U0RXD (串口接收)",warn:true},
{pin:45,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"无特殊功能"},
{pin:46,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"LOG (建议浮空或输出)"},
{pin:47,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"SPICLK_P (板载PSRAM)",warn:true},
{pin:48,digitalIO:true,pwm:true,adc:false,adcChannel:-1,touch:false,alt:"SPICLK_N (板载RGB LED)",warn:true}
];

function render(){
  var g=document.getElementById('grid');
  var h='';
  for(var i=0;i<pins.length;i++){
    var p=pins[i];
    h+='<div class="pin-card">';
    h+='<div class="pin-num"><span class="prefix">GPIO </span>'+p.pin+'</div>';
    h+='<div class="pin-badges">';
    h+='<span class="badge badge-dio">DIO</span>';
    h+='<span class="badge badge-pwm">PWM</span>';
    if(p.adc)h+='<span class="badge badge-adc">ADC CH'+p.adcChannel+'</span>';
    if(p.touch)h+='<span class="badge badge-touch">TOUCH</span>';
    if(p.warn)h+='<span class="badge badge-warn">特殊</span>';
    h+='</div>';
    h+='<div class="pin-alt">'+p.alt+'</div>';
    h+='</div>';
  }
  g.innerHTML=h;
}

function validate(){
  var v=parseInt(document.getElementById('vpin').value);
  var el=document.getElementById('vres');
  if(isNaN(v)){el.className='val-result fail';el.innerHTML='请输入有效数字';return;}
  var found=null;
  for(var i=0;i<pins.length;i++){
    if(pins[i].pin===v){found=pins[i];break;}
  }
  if(!found){
    el.className='val-result fail';
    el.innerHTML='GPIO '+v+' 不可用！<div class="pin-info">该引脚不在外部引脚图中，可能是 Flash SPI 专用引脚或不存在。</div>';
  }else{
    el.className='val-result ok';
    var info='<b>GPIO '+v+'</b> 可用';
    var caps=[];
    if(found.adc)caps.push('ADC CH'+found.adcChannel);
    if(found.touch)caps.push('TOUCH');
    caps.push('PWM','DIO');
    info+='<div class="pin-info">功能: '+caps.join(' | ');
    info+='<br>说明: '+found.alt;
    if(found.warn)info+='<br><span style="color:#ef4444">⚠ 该引脚有特殊用途，请谨慎使用</span>';
    info+='</div>';
    el.innerHTML=info;
  }
}

document.getElementById('vpin').addEventListener('keydown',function(e){
  if(e.key==='Enter')validate();
});

render();
</script>
</body>
</html>
)rawliteral";

#endif
