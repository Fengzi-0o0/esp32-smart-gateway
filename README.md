# ESP32-S3 Smart Gateway

基于 ESP32-S3 的多协议智能家居网关系统，支持多总线设备接入、本地自动化引擎、
LAN+MQTT 双通道通信和 Web 可视化配置。

## 功能特性

- **多总线接入**：GPIO、PWM、I2C、SPI、UART、1-Wire、触摸传感器
- **本地自动化**：逻辑引擎、定时引擎、脚本引擎、数据引擎，规则存储于 NVS，断网不中断
- **双通道通信**：LAN 局域网 WebSocket 直连 + MQTT 云端中转，自动路由选择
- **设备发现**：UDP 广播自动发现局域网内其他网关设备
- **Web 管理界面**：设备配置、网络管理、传感器监控、定时任务、逻辑规则
- **积木式命令编辑器**：可视化拖拽构建复杂联动规则，支持嵌套条件和多设备控制
- **OTA 远程升级**：通过 Web 页面上传固件，无需物理连接
- **中英文双语**：所有界面支持中英文切换

## 硬件要求

- ESP32-S3 开发板
- 传感器/执行器模块（按需接入）

## 快速开始

1. 使用 Arduino IDE 或 PlatformIO 打开项目
2. 选择开发板 `ESP32S3 Dev Module`
3. 编译并上传固件
4. 连接 ESP32 的 Wi-Fi AP，在浏览器中打开 `192.168.4.1`
5. 配置 Wi-Fi 连接，进入管理界面

## 项目结构

- `esp32_smart_gateway.ino` — 主程序入口
- `config.h` / `config.cpp` — 配置管理
- `wifi_manager.h` / `.cpp` — Wi-Fi 管理与 UDP 设备发现
- `mqtt_client.h` / `.cpp` — MQTT 客户端与命令解析
- `dual_channel.h` / `.cpp` — 双通道路由与 LAN WebSocket 客户端
- `web_server.h` / `.cpp` — Web 服务器与 API
- `logic_engine.h` / `.cpp` — 逻辑规则引擎
- `timer_engine.h` / `.cpp` — 定时任务引擎
- `script_engine.h` / `.cpp` — 脚本引擎（变量、条件、数学运算）
- `data_engine.h` / `.cpp` — 数据处理引擎（HEX、CRC、协议解析）
- `rand_engine.h` / `.cpp` — 随机数引擎
- `web_page.h` — 主配置页面
- `cmd_builder_page.h` — 积木式命令编辑器
- `ota_page.h` — OTA 升级页面
- `msg_dedup.h` — MQTT 消息去重



## 技术栈

- **MCU**：ESP32-S3（双核 Xtensa LX7, 240MHz）
- **框架**：Arduino
- **通信**：Wi-Fi / WebSocket / MQTT / UDP
- **前端**：原生 HTML/CSS/JavaScript（无依赖）
- **存储**：NVS（Non-Volatile Storage）

## 许可证

MIT License
