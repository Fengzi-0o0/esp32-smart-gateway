# ESP32-S3 Smart Gateway

基于 ESP32-S3 的多协议智能网关系统，支持多总线设备接入、本地自动化引擎、
LAN+MQTT 双通道无感切换通信和 Web 可视化积木式命令构建。

## ✨ 功能特性

### 🔌 多总线设备接入

| 总线 | 功能 | 最大实例 |
|------|------|---------|
| GPIO | 数字输出、PWM、RGB、舵机、音频、脉冲测量 | 20 |
| I²C | 扫描、读写寄存器、SSD1306 显示屏 | 20 |
| SPI | FSPI/HSPI 配置与传输 | 2 |
| UART | 双串口配置、文本/HEX 收发、持续监听 | 2 |
| 1-Wire | 设备搜索、DS18B20 温度读取 | 10 |
| Touch | 电容触摸检测、自动校准、阈值触发 | 10 |
| Encoder | 旋转编码器增删读重置 | — |
| Display | SSD1306 OLED 文字/图形/位图/进度条/场景绘制 | 1 |

### 🧠 本地自动化引擎（断网不中断）

- **定时引擎**：interval / once / count 三种模式，支持持久化到 NVS
- **逻辑引擎**：AND/OR 多条件组合，传感器/输入/变量/时间/MQTT/UART 条件源，冷却防抖
- **脚本引擎**：变量存取、自增、数学运算、IF/ELSE 条件分支、命令块执行
- **数据引擎**：HEX↔Bytes↔Int 转换、CRC16、数组拼接/切片/提取、位操作、比较
- **随机引擎**：整数/浮点/布尔随机数、随机执行
- **RTC 引擎**：NTP 同步、手动设时、Cron 定时计划

### 🛤️ LAN + MQTT 双通道无感切换

```
ESP32A ──── LAN ──── ESP32B ──── MQTT ──── ESP32C
 (无MQTT)    (双通道)              (不同局域网)
```

- **LAN 优先**：局域网内 WebSocket 直连，低延迟零流量
- **MQTT 兜底**：跨网段通过 MQTT Broker 中转
- **自动切换**：LAN 失败自动回退 MQTT，冷却指数退避（10s→20s→40s），恢复后自动切回 LAN
- **三层设备发现**：
  - 被动发现：`_from` 字段自动注册（零额外开销）
  - 主动心跳：60s 间隔 `esp32/status` 心跳续期
  - LWT 遗嘱：设备异常断线秒级感知
- **独立超时**：LAN/MQTT 各 120s 超时，互不影响
- **最多管理 10 台设备**

### 🧱 积木式命令构建器

Web 端可视化命令编辑器，覆盖全部 17 个分类、80+ 积木块：

| 分类 | 积木块示例 |
|------|-----------|
| GPIO | 数字输出、PWM、RGB、舵机、音频、脉冲、编码器、清除 |
| 传感器 | 添加/读取/移除/启用 DHT11/DHT22/DS18B20/Analog |
| 输入 | 添加/移除/启用输入引脚，消抖配置 |
| 定时器 | 添加/移除/启用/重置定时器，支持嵌套命令槽 |
| 逻辑 | 添加/移除/启用逻辑规则，条件+动作双槽嵌套 |
| 条件 | 传感器/触摸/输入/变量/时间/随机/MQTT/UART 条件 |
| I2C / SPI / UART / 1-Wire / Touch / RTC | 各总线操作 |
| 脚本 | 变量操作、数学运算、IF/ELSE、命令块执行 |
| 数据 | HEX/Bytes/Int 转换、CRC16、切片、位操作 |
| 随机 | 整数/浮点/布尔随机、随机执行 |
| 系统 | 状态查询、配置导出、重启、MQTT 发布、自定义动作、原始 JSON |
| 显示屏 | 初始化、文字、矩形、直线、位图、进度条、场景绘制 |
| 模块 | DS18B20/DHT11 一站式模块管理 |

**构建器高级功能**：
- 🚫 **积木块禁用**：点击 ⊘ 禁用任意块，执行时自动跳过，需要时 ✓ 启用
- ⏱ **自定义延时器**：在积木间插入延时块，仅 sequential/seqloop 模式生效，覆盖全局延迟
- 📦 **预设导出/导入**：画布导出为 JSON 文件，导入追加到画布（不覆盖），跨设备复用
- 🎯 **Target 覆盖**：每个块可单独指定目标设备，不跟随全局
- 5 种执行模式：批量单次 / 循环重复 / 步进调试 / 顺序逐条 / 顺序循环

### 📡 MQTT 支持

- TCP / SSL(TLS) 连接
- LWT 遗嘱消息（含 deviceId，秒级断线感知）
- 多主题订阅/发布（最多 20 组）
- 批量状态上报（可配置间隔 0-3600 秒）
- MQTT OTA 远程固件升级
- 消息去重（`_mid` + `_from` 防回声）

### 🌐 Web 管理界面

- **配置页**：Wi-Fi、MQTT、设备名、传感器/输入/定时/逻辑/触摸/UART/SPI/显示屏管理
- **命令构建器**：积木式可视化编辑，JSON 预览，实时日志
- **引脚能力页**：ESP32-S3 各引脚功能矩阵（数字IO/PWM/ADC/触摸）
- **监控页**：实时传感器数据、输入状态、设备在线状态
- **OTA 页**：Web 上传固件升级
- 中英文双语切换

### 🔒 安全与稳定性

- NVS 脏标记机制：配置修改只标记 dirty，主循环统一写入，减少 Flash 磨损
- DHT 读取 yield() 防看门狗重启
- 串口缓冲区 2048 字节上限防 OOM
- 清除命令使用引脚白名单，避免误操作 Flash/PSRAM 引脚
- WebSocket 结果转发竞态消除（clearResultSink）
- batchInterval 输入校验（0-3600 范围）
- MQTT String 预分配防堆碎片

## 🛠️ 硬件要求

- **MCU**：ESP32-S3（推荐 N16R8，双核 Xtensa LX7, 240MHz）
- **Flash Mode**：DIO（Arduino IDE 板设置）
- 传感器/执行器模块（按需接入）

## 🚀 快速开始

1. 使用 Arduino IDE 打开项目
2. 选择开发板 `ESP32S3 Dev Module`
3. Flash Mode 设置为 **DIO**
4. 编译并上传固件
5. 连接 ESP32 的 Wi-Fi AP（默认 SSID：`esp32byQFQ`，密码：`12345678`）
6. 浏览器打开 `192.168.4.1`，配置 Wi-Fi 连接
7. 进入管理界面，开始配置设备

## 📁 项目结构

```
esp32_smart_gateway/
├── esp32_smart_gateway.ino   # 主程序入口
├── config.h / .cpp           # 配置管理（NVS 持久化 + 脏标记）
├── wifi_manager.h / .cpp     # Wi-Fi 管理（STA/AP 自动切换）
├── mqtt_client.h / .cpp      # MQTT 客户端（SSL/LWT/OTA/命令解析）
├── dual_channel.h / .cpp     # 双通道路由（LAN+MQTT 自动切换）
├── web_server.h / .cpp       # Web 服务器与 REST API
├── gpio_control.h / .cpp     # GPIO 控制（数字/PWM/RGB/舵机/音频/脉冲）
├── sensor_engine             # 传感器管理（内嵌于 mqtt_client）
├── timer_engine.h / .cpp     # 定时任务引擎
├── logic_engine.h / .cpp     # 逻辑规则引擎
├── script_engine.h / .cpp    # 脚本引擎（变量/条件/数学）
├── data_engine.h / .cpp      # 数据处理引擎（HEX/CRC/协议）
├── rand_engine.h / .cpp      # 随机数引擎
├── rtc_engine.h / .cpp       # RTC 时钟与 Cron 计划
├── i2c_engine.h / .cpp       # I²C 总线操作
├── onewire_engine.h / .cpp   # 1-Wire 总线操作
├── uart_engine.h / .cpp      # UART 串口操作
├── spi_engine.h / .cpp       # SPI 总线操作
├── touch_engine.h / .cpp     # 电容触摸检测
├── encoder_engine.h / .cpp   # 旋转编码器
├── display_engine.h / .cpp   # SSD1306 OLED 显示引擎
├── pin_caps.h                # ESP32-S3 引脚能力定义
├── msg_dedup.h               # MQTT 消息去重
├── web_page.h                # 主配置页面
├── cmd_builder_page.h        # 积木式命令构建器页面
├── monitor_page.h            # 实时监控页面
└── pins_page.h               # 引脚能力页面
```

## 🔗 API 端点

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/` | 主配置页 |
| GET | `/builder` | 命令构建器页 |
| GET | `/monitor` | 实时监控页 |
| GET | `/pins` | 引脚能力页 |
| GET | `/ota` | OTA 升级页 |
| GET | `/api/config` | 获取当前配置 |
| POST | `/api/config` | 更新配置 |
| GET | `/api/info` | 设备信息（ID/MAC/版本） |
| GET | `/api/device` | 设备状态 |
| POST | `/api/device` | 设备控制 |
| GET | `/api/devices` | 双通道设备列表 |
| GET | `/api/pins` | 引脚能力矩阵 |
| GET | `/api/scan` | Wi-Fi 扫描 |
| GET | `/api/system` | 系统状态（堆/运行时间） |
| GET | `/api/batch_status` | 批量传感器状态 |
| POST | `/api/command` | 发送命令 |

WebSocket 端口 `8080`：命令构建器实时通信

## 💡 技术栈

- **MCU**：ESP32-S3（双核 Xtensa LX7, 240MHz, 16MB Flash, 8MB PSRAM）
- **框架**：Arduino
- **通信**：Wi-Fi / WebSocket / MQTT / UDP / I²C / SPI / UART / 1-Wire
- **前端**：原生 HTML/CSS/JavaScript（零依赖，单文件嵌入）
- **存储**：NVS（Non-Volatile Storage，脏标记优化写入）
- **JSON**：ArduinoJson

## 📜 许可证

MIT License
