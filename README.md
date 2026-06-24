# 🏠 校园宿舍智能安防门

基于 **ESP32 + EMQX + Spring Boot + HarmonyOS(ArkTS)** 的智能门禁安防系统。

---

## 📋 项目架构

```
┌─────────────────┐     MQTT      ┌──────────────┐     MQTT      ┌──────────────────┐
│   ESP32 硬件层   │ ───────────→ │  EMQX Broker  │ ───────────→ │  Spring Boot 后端 │
│                  │ ←─────────── │    (MQTT)     │ ←─────────── │    (Java 17)     │
│ · MFRC522 RFID   │              └──────────────┘              │                  │
│ · HC-SR501 PIR   │                                            │ · MySQL 存储     │
│ · DHT11 温湿度   │                                            │ · REST API       │
│ · SG90 舵机      │                                            │ · 业务逻辑       │
│ · 蜂鸣器         │                                            └────────┬─────────┘
└─────────────────┘                                                     │
                                                                        │ HTTP REST API
                                                                        ▼
                                                              ┌──────────────────┐
                                                              │  HarmonyOS 应用   │
                                                              │   (ArkTS)        │
                                                              │                  │
                                                              │ · 仪表盘         │
                                                              │ · 门禁记录       │
                                                              │ · 温湿度监测     │
                                                              │ · 告警推送       │
                                                              │ · 授权卡管理     │
                                                              └──────────────────┘
```

## 🚀 快速开始

### 1. 环境准备

| 组件 | 版本要求 |
|------|---------|
| Java | JDK 17+ |
| Maven | 3.6+ |
| MySQL | 8.0+ |
| EMQX | 5.x (开源版) |
| PlatformIO | 最新版 (VSCode插件) |
| DevEco Studio | 5.0+ (鸿蒙开发工具) |
| Python | 3.8+ (ESP32串口工具依赖) |

---

### 2. EMQX MQTT Broker 部署

#### 2.1 安装 EMQX

**Windows:**
```powershell
# 下载 EMQX 5.x 开源版
# https://www.emqx.com/zh/downloads-and-install/broker

# 解压后进入目录，启动
./bin/emqx start
```

**Linux (Docker):**
```bash
docker run -d --name emqx \
  -p 1883:1883 \
  -p 8083:8083 \
  -p 8084:8084 \
  -p 8883:8883 \
  -p 18083:18083 \
  emqx/emqx:latest
```

**macOS:**
```bash
brew install emqx
emqx start
```

#### 2.2 访问 Dashboard

1. 浏览器打开: `http://localhost:18083`
2. 默认账号: `admin` / 密码: `public`
3. 首次登录后请修改密码

#### 2.3 验证 MQTT 服务

使用 **MQTTX** 客户端工具测试:
- 下载: https://mqttx.app/
- 创建连接 → 地址 `localhost:1883`
- 订阅主题 `dorm/door/#` 测试接收消息

#### 2.4 EMQX 主题权限配置（可选）

进入 Dashboard → 访问控制 → 授权:
- 允许所有客户端发布到 `dorm/door/#`
- 允许所有客户端订阅 `dorm/door/#`

> 开发阶段可直接使用匿名访问，生产环境建议开启认证。

---

### 3. MySQL 数据库配置

```sql
-- 创建数据库
CREATE DATABASE IF NOT EXISTS dorm_security
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;
```

数据库表会在 Spring Boot 首次启动时自动创建 (JPA ddl-auto: update)。
也可手动执行 `backend/src/main/resources/db/schema.sql`。

---

### 4. Spring Boot 后端

#### 4.1 修改配置

编辑 `backend/src/main/resources/application.yml`:

```yaml
spring:
  datasource:
    url: jdbc:mysql://localhost:3306/dorm_security?useSSL=false&serverTimezone=Asia/Shanghai
    username: root
    password: 你的MySQL密码          # ← 修改这里

mqtt:
  broker-url: tcp://localhost:1883    # EMQX地址 (默认本机)
  # username: your_mqtt_user         # 如果EMQX开启了认证则填写
  # password: your_mqtt_password
```

#### 4.2 启动后端

```bash
cd backend
mvn clean package -DskipTests
mvn spring-boot:run
```

启动成功后访问: `http://localhost:8080`

#### 4.3 API 测试

```bash
# 获取最新温湿度
curl http://localhost:8080/api/sensor/dht/latest

# 获取刷卡记录
curl http://localhost:8080/api/rfid/records?page=0&size=10

# 获取告警记录
curl http://localhost:8080/api/alarm/logs/latest

# 添加授权卡
curl -X POST http://localhost:8080/api/cards \
  -H "Content-Type: application/json" \
  -d '{"cardUid":"A3 7F 2C 10","holderName":"张三","studentId":"2024001"}'
```

---

### 5. ESP32 硬件部署

#### 5.1 硬件接线

| ESP32 引脚 | 外设 | 信号 |
|-----------|------|------|
| GPIO5 | MFRC522 SDA (SS) | SPI 片选 |
| GPIO18 | MFRC522 SCK | SPI 时钟 |
| GPIO23 | MFRC522 MOSI | SPI MOSI |
| GPIO19 | MFRC522 MISO | SPI MISO |
| GPIO22 | MFRC522 RST | 复位 |
| GPIO4 | DHT11 DATA | 单总线 |
| GPIO15 | HC-SR501 OUT | 数字输入 |
| GPIO2 | 蜂鸣器 I/O | PWM输出 |
| GPIO13 | SG90 PWM | 舵机控制 |

> ⚠️ 注意: 蜂鸣器共用一个，PIR告警、温湿度告警、未授权刷卡告警共用。

#### 5.2 修改配置并烧录

编辑 `esp32/src/config.h`:

```cpp
// 修改WiFi信息
#define WIFI_SSID       "你的WiFi名称"
#define WIFI_PASSWORD   "你的WiFi密码"

// 修改EMQX Broker地址
#define MQTT_BROKER     "192.168.1.100"   // 你的EMQX服务器IP

// 可选: 修改温湿度阈值
#define TEMP_MAX        40.0   // 温度上限
#define HUMIDITY_MAX    80.0   // 湿度上限
```

编辑 `esp32/src/authorized_cards.h`，将你的实际卡号替换进去:

```cpp
// 将你的卡UID替换到下面
static const char* AUTHORIZED_UIDS[AUTHORIZED_CARD_COUNT] = {
    "A3 7F 2C 10",   // ← 换成你的实际卡号
    "B4 8E 3D 21",
    "C5 9F 4E 32"
};
```

#### 5.3 烧录

使用 VSCode + PlatformIO 插件:
1. 打开 `esp32/` 目录
2. 点击底部状态栏 `→` (Upload and Monitor)
3. 观察串口输出，确认 WiFi 和 MQTT 连接成功

#### 5.4 如何获取卡号UID

首次使用时，修改 `esp32/src/main.cpp` 在 `setup()` 后添加一段简单的读取代码重新烧录：
刷一下卡，从串口监视器中就能看到类似 `Card UID: A3 7F 2C 10` 的输出。

---

### 6. HarmonyOS 鸿蒙前端

#### 6.1 修改 API 地址

编辑 `frontend/entry/src/main/ets/common/Constants.ets`:

```typescript
export const BASE_URL: string = 'http://192.168.1.200:8080'  // 改为你的后端IP
```

#### 6.2 导入运行

1. 打开 DevEco Studio
2. 导入 `frontend/` 目录
3. 连接鸿蒙设备或启动模拟器
4. 点击 Run 运行

---

## 📡 MQTT 消息格式

### 主题: `dorm/door/rfid` (ESP32 → Backend)

```json
{
  "uid": "A3 7F 2C 10",
  "authorized": true,
  "holder_name": "张三",
  "timestamp": 1234567890
}
```

### 主题: `dorm/door/pir` (ESP32 → Backend)

```json
{
  "detected": true,
  "timestamp": 1234567890
}
```

### 主题: `dorm/door/dht` (ESP32 → Backend)

```json
{
  "temperature": 25.5,
  "humidity": 60.0,
  "timestamp": 1234567890
}
```

### 主题: `dorm/door/alarm` (ESP32 → Backend)

```json
{
  "type": "PIR",
  "message": "疑似有人非法闯入",
  "timestamp": 1234567890
}
```

### 主题: `dorm/door/servo/cmd` (Backend → ESP32, 备用远程开门)

```json
{
  "cmd": "open"
}
```

---

## 🔄 业务流程

### 1. RFID 门禁开门
```
刷卡 → 读UID → 白名单比对
  ├─ 授权: 舵机转90°→ 延时2s → 0°关门 → 上报信息
  └─ 未授权: 蜂鸣器响3声 → 上报告警
→ 后端保存记录
→ 前端展示
```

### 2. PIR 人体红外告警
```
检测到人 → 蜂鸣器长响2秒 → 上报PIR事件 + 告警事件
→ 后端保存告警 → 前端弹窗: "⚠️ 疑似有人非法闯入"
```

### 3. DHT11 温湿度监测
```
每30秒采集 → 上报数据 → 后端保存
  └─ 超标: 蜂鸣器间歇响 → 上报告警事件 → 前端展示
```

---

## 📂 项目结构

```
2340711113_rfid_security/
├── esp32/                        # ESP32 硬件代码
│   ├── platformio.ini
│   └── src/
│       ├── main.cpp              # 主程序
│       ├── config.h              # 全局配置
│       ├── wifi_manager.h/cpp    # WiFi管理
│       ├── mqtt_manager.h/cpp    # MQTT通信
│       ├── rfid_reader.h/cpp     # MFRC522读卡
│       ├── dht_sensor.h/cpp      # DHT11温湿度
│       ├── pir_sensor.h/cpp      # 人体红外
│       ├── buzzer.h/cpp          # 蜂鸣器
│       ├── servo_control.h/cpp   # SG90舵机
│       └── authorized_cards.h    # 本地白名单
├── backend/                      # Spring Boot 后端
│   ├── pom.xml
│   └── src/main/
│       ├── java/com/dormsecurity/
│       │   ├── entity/           # 数据实体
│       │   ├── repository/       # JPA仓库
│       │   ├── service/          # 业务逻辑
│       │   ├── controller/       # REST API
│       │   ├── config/           # MQTT/WebSocket/CORS配置
│       │   └── handler/          # MQTT入站处理器
│       └── resources/
│           ├── application.yml
│           └── db/schema.sql
├── frontend/                     # 鸿蒙ArkTS前端
│   └── entry/src/main/ets/
│       ├── model/                # 数据模型
│       ├── common/               # 公共工具/API封装
│       ├── pages/                # 5个页面
│       └── components/           # 可复用组件
└── README.md
```

---

## 🔧 常见问题

**Q: ESP32 连接不上 EMQX？**
- 确认 EMQX 已启动且端口 1883 可访问
- 检查 ESP32 和 EMQX 是否在同一局域网
- 用 MQTTX 客户端连接 EMQX 确认服务正常

**Q: 后端启动报 MQTT 连接错误？**
- 确认 EMQX 已启动
- 检查 `application.yml` 中 `mqtt.broker-url` 地址正确

**Q: 鸿蒙应用无法获取数据？**
- 确认后端已启动且 API 可访问
- 确认手机与后端在同一网络
- 检查 `Constants.ets` 中 `BASE_URL` 配置正确

**Q: DHT11 读取数据为 0 或 NaN？**
- DHT11 偶尔读取失败属于正常现象
- 代码中已做 NaN 处理，会显示为 0
- 建议确保 DHT11 供电稳定

---

## 📝 后续扩展建议

1. **人脸识别**: 接入摄像头 + AI 人脸识别模块
2. **远程开门**: 应用端一键远程开门（已有 MQTT 通道 `dorm/door/servo/cmd`）
3. **多宿舍管理**: 扩展为多宿舍楼宇级系统
4. **微信推送**: 告警信息通过企业微信/公众号推送
5. **数据大屏**: 增加 Web 管理后台数据大屏
