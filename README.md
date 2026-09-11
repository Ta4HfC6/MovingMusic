# MovingMusic：ESP32-S3 手势 BLE-MIDI 控制器

基于开源项目 [ESP32_Host_MIDI](https://github.com/sauloverissimo/ESP32_Host_MIDI)
进行的二次开发。项目目标是让 ESP32-S3 读取手势和距离传感器，将动作转换为标准
MIDI 消息，再通过 Bluetooth Low Energy MIDI 控制电脑或移动设备上的合成器软件。

当前阶段尚未接入传感器，已经使用串口命令和 Android 应用 Piano MIDI Legend
打通以下链路：

```text
ESP32-S3
  -> BLE-MIDI 外设（MovingMusic S3）
  -> Bluetooth MIDI Connect
  -> Android MIDI Service
  -> Piano MIDI Legend
```

## 当前进度

- ESP32-S3 BLE-MIDI 广播、连接和断线重连正常。
- Note On/Off 和力度（Velocity）正常。
- CC7 音量控制正常。
- CC64 延音踏板控制正常。
- Program Change 0–41 可以切换 Piano MIDI Legend 音色。
- Pitch Bend 会影响下一次 Note On；Piano MIDI Legend 实测不会连续改变已经发声音符的音高。
- CC91 混响和 CC92 Delay 在当前应用状态下未听到明显效果，可能需要先在应用内启用或解锁。

## 仓库结构

```text
.
├─ src/                         ESP32_Host_MIDI 核心库源码
├─ examples/
│  └─ Piano-MIDI-Legend-BLE-Lab/
│     ├─ Piano-MIDI-Legend-BLE-Lab.ino
│     └─ README.md
├─ extras/tests/                平台无关的核心测试
├─ .github/workflows/ci.yml     自动编译和测试
├─ library.properties           Arduino Library 元数据
├─ library.json                 PlatformIO Library 元数据
└─ LICENSE                      MIT 许可证
```

上游自带的 USB、UART、RTP-MIDI、Ethernet、OSC、ESP-NOW 和显示屏演示已从本仓库
移除，以便聚焦本次 BLE-MIDI 传感器控制器开发。相关传输实现仍保留在 `src/`，以后可以
按需扩展。

## 硬件与软件

- ESP32-S3（当前测试板带 8 MB PSRAM）
- 支持数据传输的 USB 线
- Arduino IDE，或 Arduino CLI 1.5.1
- Espressif Arduino Core 3.3.10（最低建议 3.0.0）
- Android 设备
- Bluetooth MIDI Connect
- Piano MIDI Legend

目前不需要外接传感器。

## 编译和上传

Arduino IDE 中选择与实际开发板对应的型号；通用开发板可先选择
`ESP32S3 Dev Module`。打开：

```text
examples/Piano-MIDI-Legend-BLE-Lab/Piano-MIDI-Legend-BLE-Lab.ino
```

## Android 连接

需要 Bluetooth MIDI Connect 和 Piano MIDI Legend。

1. 给 Bluetooth MIDI Connect 授予“附近设备/蓝牙”权限；部分 Android 版本还要求位置权限。
2. 在 Bluetooth MIDI Connect 中扫描并连接 `MovingMusic S3`。
3. 不要退出连接工具，切换到 Piano MIDI Legend。
4. 打开 115200 波特率串口监视器；看到 `[BLE] connected` 后输入 `t`。
5. 正常情况下会依次听到 C4、E4、G4、C5。

BLE-MIDI 不是蓝牙音频协议，因此无需在 Android 的普通蓝牙音频设备页面完成配对。

## 串口调试命令

串口监视器使用 115200 波特率和 Newline 行尾：

```text
h                 显示帮助
s                 显示 BLE 状态
t                 播放/停止四音测试
n 60 100 500      MIDI 60，力度 100，持续 500 ms
vol 40            CC7 音量
rev 100           CC91 混响
del 80            CC92 Delay
pc 5              切换至音色 5
next / prev       下一个/上一个音色
pb 4096           Pitch Bend
sus 1 / sus 0     开启/关闭延音
pressure 100      Channel Pressure
panic             All Notes Off / All Sound Off
```

更完整的联调步骤和实测能力矩阵见
[`examples/Piano-MIDI-Legend-BLE-Lab/README.md`](examples/Piano-MIDI-Legend-BLE-Lab/README.md)。

## 传感器映射方向

计划优先验证以下映射：

| 传感器动作 | MIDI 消息 | 音乐行为 |
|---|---|---|
| 手与传感器的距离 | CC7 或可配置 CC | 连续控制音量/参数 |
| 动作速度 | Velocity | 控制音符力度 |
| 张手/握拳 | CC64 | 延音开关 |
| 左右挥动 | Program Change | 切换音色 |
| 离散手势 | Note On/Off | 触发音符、和弦或片段 |
| 空间位置 | Pitch Bend | 设置下一音的音高偏移 |

传感器接入后会增加采样限速、低通滤波、死区、迟滞和 MIDI 发送节流，避免测量抖动
产生过量 BLE 消息。

## 上游与许可证

本项目基于 Saulo Veríssimo 的 ESP32_Host_MIDI v7.0.0，保留原项目的 MIT
许可证和版权声明。二次开发内容同样按仓库中的 [LICENSE](LICENSE) 使用。
