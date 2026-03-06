# ESP32 远程温室大棚模拟控制系统 
基于ESP32开发板与巴法云平台的物联网系统，实现温湿度数据采集、云端传输、远程设备控制及微信小程序交互，全程单人独立完成硬件搭建、程序开发、平台配置与前端交互全流程。

## 项目简介
本项目以ESP32为核心控制器，模拟远程温室大棚监控场景：
- 硬件层：通过DHT11采集温湿度数据，驱动小灯泡模拟设备远程控制；
- 网络层：基于WiFi+TCP协议与巴法云平台实现数据双向传输；
- 应用层：开发微信小程序实现移动端数据可视化与远程控制。
所有功能均满足设计指标：数据传输延迟≤2.3秒、设备响应≤1.5秒、小程序刷新≤8秒/次。

## 技术栈
| 分类         | 技术/工具                          |
|--------------|------------------------------------|
| 硬件         | ESP32-WROOM-32、DHT11、LED、限流电阻 |
| 开发环境     | Arduino IDE、微信开发者工具        |
| 通信协议     | WiFi(STA模式)、TCP、HTTP API       |
| 核心库/平台  | SimpleDHT、巴法云TCP创客云         |
| 前端技术     | 微信小程序（WXML/WXSS/JS）         |

## 核心功能
### 1. 温湿度数据采集与云端上传
- 基于SimpleDHT库读取DHT11传感器数据，采样频率2秒/次；
- 按`#温度#湿度#设备状态#`格式封装数据，通过TCP协议发布至巴法云`temp`主题；
- 实现WiFi断线自动重连、TCP心跳保活机制，数据上传成功率98%。

### 2. 远程设备控制
- 监听巴法云`light`主题，解析`on/off`控制指令，驱动LED亮灭；
- 支持手动远程控制（优先级最高）+ 自动阈值控制（温度＞30℃时LED闪烁）。

### 3. 微信小程序交互
- 开发轻量化小程序界面，支持温湿度数据实时显示、LED控制指令发送；
- 配置合法域名与HTTP API调用，界面加载≤2秒，自动刷新频率8秒/次。

## 快速开始
### 1. 硬件连接
| 组件         | ESP32引脚 | 连接说明                  |
|--------------|-----------|---------------------------|
| DHT11数据线  | GPIO4     | 电源接3.3V，接地接GND     |
| LED正极      | GPIO2     | 串联220Ω限流电阻后接GND   |
| WiFi         | -         | 接入手机热点/路由器（同局域网） |

### 2. 软件配置
#### （1）ESP32程序（Arduino IDE）
1. 安装ESP32开发板支持库、SimpleDHT库；
2. 修改代码中WiFi账号/密码、巴法云UID/主题名：
   ```c
   #define WIFI_SSID "你的WiFi名称"
   #define WIFI_PASS "你的WiFi密码"
   #define BEMFA_UID "你的巴法云UID"
   #define TOPIC_TEMP "temp"  // 数据主题
   #define TOPIC_LIGHT "light" // 控制主题
   #define TOPIC_TEMP "temp"  // 数据主题
   ```
3. 上传程序至ESP32，打开串口监视器（设置115200波特率）查看设备运行状态。

#### （2）巴法云平台配置
1. 注册巴法云账号，在平台创建两个TCP主题：temp（用于数据发布）、light（用于指令订阅）；
2. 记录平台分配的唯一UID，该UID需同步至ESP32程序和小程序代码中，作为接口调用的核心标识。

#### （3）微信小程序配置
1. 下载项目中miniprogram目录下的小程序源码，使用微信开发者工具打开；
2. 登录微信小程序后台，在“开发-开发设置-服务器域名”中添加合法域名：https://api.bemfa.com；
3. 修改小程序代码中的巴法云UID与主题名（temp/light），完成配置后编译并运行小程序。

## 核心代码模块说明
### 1. 温湿度采集模块
```c
#define pinDHT11 4
SimpleDHT11 dht11(pinDHT11);
void readDHT11Data() {
  byte temp = 0, humi = 0;
  int err = dht11.read(&temp, &humi, NULL);
  if (err == SimpleDHTErrSuccess) {
    String data = "#" + String(temp) + "#" + String(humi) + "#" + String(ledState) + "#";
    sendtoTCPServer(data); // 上传至云端
  }
}
```
### 2. TCP指令解析模块
```c
String TcpClient_Buff = "";
void parseTCPCmd() {
  if (TcpClient_Buff.indexOf("&msg=on") > 0) {
    digitalWrite(LED_Pin, LOW); // 点亮 LED
  } else if (TcpClient_Buff.indexOf("&msg=off") > 0) {
    digitalWrite(LED_Pin, HIGH); // 熄灭 LED
  }
  TcpClient_Buff = ""; // 清空缓冲区
}
```
## 测试结果
| 测试项         | 实测值               | 设计指标       | 结果   |
|----------------|----------------------|----------------|--------|
| 温湿度采集误差 | 温度≤1℃、湿度≤3%     | -              | 达标   |
| 数据上传延迟   | 平均2.3秒            | ≤5秒           | 达标   |
| LED响应时间   | 平均1.5秒            | ≤3秒           | 达标   |
| 小程序刷新频率 | 8秒/次               | ≥1次/10秒      | 达标   |

## 问题与解决
| 问题场景                | 解决方案                                  |
|-------------------------|-------------------------------------------|
| WiFi连接不稳定、频繁断线 | 增加WiFi状态轮询机制，添加断线自动重连逻辑，实时检测连接状态并触发重连 |
| 巴法云指令解析错误      | 仔细研读巴法云官方文档，修正TCP协议参数的封装格式，确保指令字段匹配 |
| 小程序域名配置失败      | 在微信小程序后台的服务器域名配置项中，添加“https://api.bemfa.com”为合法域名 |

## 改进方向
1. 硬件层面：增加ESD保护二极管，防止静电干扰DHT11传感器的数据采集精度；
2. 程序层面：引入MQTT协议与现有TCP协议做对比测试，优化设备功耗与数据传输效率；
3. 小程序层面：添加温湿度阈值预警动态效果（如温度超限时界面变色），引入本地存储功能缓存历史数据，减少重复API请求。

## 参考资料
- 巴法云TCP创客云官方文档：https://cloud.bemfa.com/docs/src/
- ESP32 WiFi开发官方指南：https://docs.espressif.com/projects/esp-idf/
- 微信小程序官方开发文档：https://developers.weixin.qq.com/miniprogram/dev/
