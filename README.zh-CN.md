# Nulllab RFX1 Arduino Lib

[English](README.md) | 简体中文

## 概述

**Nulllab RFX1 Arduino Lib**（`nulllab_rfx1`）是用于 **RFX1** 射频接收模块的 Arduino 库，支持 433.92MHz OOK 信号与 EV1527 编码协议。主机侧通过 I2C 总线（默认地址 0x45）访问 RFX1 模块，射频信号的接收、解码与滤波均由模块完成，只需简单配置并读取结果，即可获得空中的射频帧数据。

主要能力：

- 提供 6 个 RF 配置档位（2.4k / 4.8k / 9.6k / 19.2k / 30k / 40k bps，均为 433.92MHz），用于匹配不同类型的发射器；
- 内置窗口滤波机制，抑制按键连发、噪声与误码，直接输出稳定的按键帧；
- 支持原始帧与滤波帧双通道读取，便于调试与产品化两种场景；
- 可配置 I2C 地址与 RX 指示开关；
- 纯头文件 C++ 实现，无动态内存分配，基于 cyf（cyfney-cpp）构建。

## 前置条件

### 硬件

- `RFX1` 433.92MHz OOK EV1527 射频接收模块；
- 任意支持 I2C 的 Arduino 兼容开发板；
- 一个 433.92MHz OOK EV1527 射频发射器（遥控器、自发电开关等），本库只负责接收侧。发射器上标注的“433”通常即指 433.92MHz 频段。

### 接线

RFX1 模块通过 I2C 与主机连接，典型接法如下（以 UNO 为例）：

| RFX1 模块 | 主控板（UNO 示例） |
| :-------- | :----------------- |
| VCC       | 5V                 |
| GND       | GND                |
| SDA       | A4                 |
| SCL       | A5                 |

> 其它板卡请参照各自的 I2C 引脚定义。模块默认 I2C 地址为 `0x45`，如地址被修改，可在构造 `Rfx1` 对象时传入自定义地址。

## 安装

本库依赖 [cyfney-cpp](https://github.com/cyfney/cyfney-cpp-arduino)（cyf 基础库，版本不低于 5.3.0），请通过 Arduino 库管理器自行安装以下两个库：

1. 打开 Arduino IDE，依次进入 **工具 → 管理库**（Tools → Manage Libraries）；
2. 搜索 `cyfney-cpp`，安装 cyf 基础库；
3. 搜索 `nulllab_rfx1`，安装本库；
4. 安装完成后，在工程中 `#include "nulllab_rfx1.h"` 即可使用。

> 若使用 arduino-cli 或 PlatformIO，请分别安装 `cyfney-cpp` 与 `nulllab_rfx1` 两个库后再编译。

## 示例

`examples/rfx1/rfx1.ino` 完整演示了从配置到收帧的全流程，建议从这里入手。可在 Arduino IDE 中通过 **文件 → 示例 → nulllab_rfx1 → rfx1** 打开，或直接查看源码：[examples/rfx1/rfx1.ino](examples/rfx1/rfx1.ino)。

### 工作流程

**配置阶段（文件顶部常量与 `setup()`）**

- 文件顶部的 `kRfProfile` 决定接收档位，同一时刻只有一行生效，当前默认为 `k433MHz2400bps`。档位必须与发射器匹配，否则收不到任何帧，选择方法见「配置与 API 说明」；
- `setup()` 按顺序完成：读取设备 ID 与固件版本（同时验证 I2C 连通性）→ 开启 RX 指示 → 应用 RF 档位 → 设置滤波参数（`min_count = 1`、`timeout_ms = 300`，与库默认一致）。

**收帧阶段（`loop()`）**

- `PopRawFrame()`：弹出原始帧并打印，其中包含按键连发、噪声与误码产生的重复或异常帧；
- `FlushFilteredFrame()`：结算滤波窗口并打印，仅在窗口得到稳定结果时输出。

### 预期串口输出（115200 bps）

```text
00.00.00.000 I rfx1.ino:24 setup] RFX1 demo
00.00.00.001 I rfx1.ino:25 setup] device id: 0x45
00.00.00.003 I rfx1.ino:28 setup] firmware version: 1.1.0
00.00.00.008 I rfx1.ino:31 setup] set rx indicator enabled: 1
00.00.00.013 I rfx1.ino:34 setup] set rf profile: 2.4k (433.92MHz)
00.00.00.019 I rfx1.ino:38 setup] set filter settings: min_count=1, timeout=300 ms
00.00.00.025 I rfx1.ino:40 setup] start receving...
00.00.03.538 I rfx1.ino:49 loop] [raw 0] 0x2E63C2
00.00.03.577 I rfx1.ino:49 loop] [raw 1] 0x2E63C2
00.00.03.840 I rfx1.ino:54 loop] [filtered 0] 0x2E63C2
```

> 原始帧会包含按键连发、噪声与误码产生的重复或异常帧；滤波帧只在窗口结算出稳定结果时输出。调试时可先观察原始帧确认信号到达，再以滤波帧作为业务输入。

## 配置与 API 说明

### RF 配置档位（RfProfile）

档位一次性设定接收端的速率及配套解调参数，可选值如下（频率均为 433.92MHz）：

| 枚举值                  | 速率      | 典型场景                 |
| ----------------------- | --------- | ------------------------ |
| `k433MHz2400bps`        | 2.4 kbps  | 锂电池供电设备（常见）   |
| `k433MHz4800bps`        | 4.8 kbps  | 按实测选择               |
| `k433MHz9600bps`        | 9.6 kbps  | 自发电开关（常见）       |
| `k433MHz19200bps`       | 19.2 kbps | 按实测选择               |
| `k433MHz30000bps`       | 30 kbps   | 自发电开关（常见）       |
| `k433MHz40000bps`       | 40 kbps   | 按实测选择               |

选择原则：

- 不清楚发射器参数时，从低档到高档逐档设置并实测，取通信最稳定且满足距离要求的**最小档位**；
- 较高档位可兼容较低速率的发射器，但通信距离缩短、误码率升高；
- 以上典型场景仅为常见参考，最终以实际测试为准。

### 滤波参数（SetFilterSettings）

```cpp
rfx1.SetFilterSettings(uint8_t min_count, uint16_t timeout_ms);
```

- `min_count`：一个窗口中胜出帧需要达到的最少相同帧数；为 `0` 时不做数量校验，只要求它严格多于其它候选帧；
- `timeout_ms`：窗口时长（毫秒），从窗口收到第一帧开始计时；
- 默认值为 `min_count = 1`、`timeout_ms = 300`。

调参权衡：

- 调大 `min_count` 更能抑制噪声，但可能漏掉有效帧；
- 调长 `timeout_ms` 能积累更多样本，但会增加延迟，甚至把前后两次按键合并成一次；
- 新设置立即生效，当前窗口会清空重启，仍在累积的帧将被丢弃。

### 其余 API

| API                                             | 说明                                     |
| ----------------------------------------------- | ---------------------------------------- |
| `Rfx1(TwoWire& wire, uint8_t address = 0x45)`   | 构造函数，指定 I2C 实例与设备地址        |
| `device_id()`                                   | 读取设备 ID                              |
| `firmware_version()`                            | 读取固件版本（主版本.次版本.修订号）     |
| `SetRfProfile(RfProfile profile)`               | 设置 RF 配置档位                         |
| `SetFilterSettings(min_count, timeout_ms)`      | 设置窗口滤波参数                         |
| `SetRxIndicatorEnabled(bool enabled)`           | 使能或关闭 RX 指示                       |
| `PopRawFrame(uint32_t& frame)`                  | 弹出一帧原始数据，取到帧返回 `true`      |
| `FlushFilteredFrame(uint32_t& frame)`           | 结算滤波窗口并读取结果，有结果返回 `true` |
| `Rfx1::ToString(profile)`                       | 获取档位的可读描述，如 `30k (433.92MHz)` |

## 常见问题

**Q：完全收不到帧？**

按以下顺序排查：

1. 确认发射器工作在 433.92MHz（发射器上标注的“433”通常即指该频段）；
2. 档位与发射器不匹配是最常见原因，从最低档 `k433MHz2400bps` 开始逐档实测；
3. 确认 I2C 地址与实际模块一致（默认 `0x45`）；
4. 调试时可先调用 `PopRawFrame()` 观察原始帧是否到达，再检查滤波参数是否过严。

**Q：I2C 扫描不到 `0x45` 设备？**

检查 VCC/GND/SDA/SCL 接线与供电，确认主控板 I2C 引脚正确（UNO 为 A4/A5），必要时核对模块地址是否被修改过。

**Q：帧值不稳定、误码多？**

通常是档位过高导致抗噪能力下降，可尝试降低档位；或调大 `min_count` 要求更多相同帧才输出结果。

**Q：一次按键输出多帧，或者按键偶尔没反应？**

一次按键通常被发射器连发成多帧相同内容。多帧输出可调大 `min_count`；偶尔无反应可能是 `timeout_ms` 太短或 `min_count` 过高，可适当调长窗口或降低 `min_count`。

**Q：串口看不到任何输出？**

示例使用 115200 bps 波特率，请确认串口监视器波特率一致，且 `device_id` 与 `firmware_version` 在 `setup()` 中已正常打印（若这两行都没有，问题在 I2C 连接而非射频）。

## 项目结构

```text
nulllab_rfx1/
├── src/nulllab_rfx1.h      # 库源码（纯头文件）
├── examples/rfx1/rfx1.ino  # 基础接收示例
├── library.properties      # Arduino 库描述文件
├── LICENSE                 # 许可证
├── README.md               # 英文文档
└── README.zh-CN.md         # 中文文档
```

## 许可证

本项目基于 [MIT License](LICENSE) 开源。
