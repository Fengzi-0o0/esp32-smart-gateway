# Changelog

## [2.1.0] - 2025-04-27

### Added
- 积木式可视化命令编辑器，支持嵌套条件和多设备联动
- LAN WebSocket 直连通信，局域网设备间低延迟控制
- 双通道路由自动选择（LAN优先，MQTT兜底）
- UDP 设备发现协议，自动注册局域网内其他网关
- 全局 Target 下拉框和 NVS 持久化开关
- 嵌套 Target 注入，支持 Logic/Timer/Script 的 actions 指定远程设备
- 主页面固件版本动态显示

### Fixed
- MQTT断连后target命令丢失，现在自动转发到DualChannel
- publishResult被MQTT连接状态阻断，现在WS和MQTT独立回传
- 设备发现只有请求没有响应注册，补充discover_response处理

### Changed
- cmd_builder_page 从文本表单重构为积木式编辑器
- 主页面按钮从绝对定位改为flex布局，大小统一
- decideRoute() 兜底逻辑优化，MQTT断连时返回ROUTE_NONE

## [2.0.0] - 2025-04-20

### Added
- 逻辑规则引擎（and/or条件组合，NVS持久化）
- 定时任务引擎（interval/once/count模式）
- 脚本引擎（变量管理、数学运算、if/else条件判断）
- 数据引擎（HEX/CRC/协议解析）
- 随机数引擎
- Web配置管理页面
- 命令构建器
- OTA远程升级
- 中英文双语支持

## [1.0.0] - 2025-04-10

### Added
- ESP32-S3 基础固件
- Wi-Fi AP/STA 配网
- MQTT 连接与命令收发
- GPIO/PWM/I2C/SPI/UART/1-Wire/Touch 驱动
- 传感器采集与数字输入监控
