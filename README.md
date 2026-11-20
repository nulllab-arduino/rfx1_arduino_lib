# nulllab_rfx1

English | [简体中文](README.zh-CN.md)

## Overview

`nulllab_rfx1` is an Arduino driver library for the RFX1 433.92MHz OOK EV1527 RF receiver module. The host talks to the module over I2C (default address `0x45`); reception, decoding, and frame filtering all happen on the module itself, so the Arduino side only needs a few configuration calls and then simply reads the results.

Key features:

- Six RF profiles (2.4k / 4.8k / 9.6k / 19.2k / 30k / 40k bps, all at 433.92MHz) to match different types of transmitters;
- Built-in window-based frame filtering that rejects repeated bursts, noise, and corrupted frames, and outputs stable key-press frames;
- Dual read paths for raw and filtered frames, covering both debugging and production use;
- Configurable I2C address and RX indicator;
- Header-only C++ implementation with no dynamic memory allocation, built on cyf (cyfney-cpp).

## Hardware Requirements

### Hardware

- An RFX1 433.92MHz OOK EV1527 RF receiver module;
- Any Arduino-compatible board with an I2C bus;
- A 433.92MHz OOK EV1527 RF transmitter (a remote control, a self-powered switch, etc.). This library covers the receiving side only.

### Wiring

The RFX1 module connects to the host over I2C. A typical wiring (UNO shown as an example):

| RFX1 module | Host board (UNO example) |
| ----------- | ------------------------ |
| VCC         | 5V                       |
| GND         | GND                      |
| SDA         | A4                       |
| SCL         | A5                       |

> For other boards, refer to their own I2C pin definitions. The module's default I2C address is `0x45`; if the address has been changed, pass the custom address when constructing the `Rfx1` object.

## Installation

This library depends on [cyfney-cpp](https://github.com/cyfney/cyfney-cpp-arduino) (the cyf base library, version 5.3.0 or later). Install both libraries yourself through the Arduino Library Manager:

1. In the Arduino IDE, open **Tools → Manage Libraries**;
2. Search for `cyfney-cpp` and install the cyf base library;
3. Search for `nulllab_rfx1` and install this library;
4. Once installed, `#include "nulllab_rfx1.h"` in your sketch to start using it.

> If you use arduino-cli or PlatformIO, install both `cyfney-cpp` and `nulllab_rfx1` before compiling.

## Example

The bundled example `examples/rfx1/rfx1.ino` walks through the complete workflow from configuration to frame reception — it is the best place to start. Open it in the Arduino IDE via **File → Examples → nulllab_rfx1 → rfx1**, or browse the source directly: [examples/rfx1/rfx1.ino](examples/rfx1/rfx1.ino).

### How it works

**Configuration (top-of-file constants and `setup()`)**

- The `kRfProfile` constant at the top selects the receiving profile; only one line is active at a time, and the current default is `k433MHz2400bps`. The profile must match the transmitter, otherwise no frames will be received at all — see [RF profiles](#rf-profiles-rfprofile) for how to choose one;
- `setup()` runs through the full bring-up: reading the device ID and firmware version (which also verifies the I2C link) → enabling the RX indicator → applying the RF profile → setting the filter parameters (`min_count = 1`, `timeout_ms = 300`, matching the library defaults).

**Reception (`loop()`)**

- `PopRawFrame()`: pops and prints raw frames, which include the duplicated or corrupted frames caused by transmission bursts, noise, and bit errors;
- `FlushFilteredFrame()`: settles the filter window and prints the result, only when the window yields a stable frame.

### Expected serial output (115200 bps)

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

> Raw frames include the repeated and corrupted frames caused by transmission bursts and noise; filtered frames are only emitted when a window settles on a stable result. While debugging, watch the raw stream first to confirm that signals are arriving, then use filtered frames as the input for your application logic.

## Configuration and API

### RF profiles (RfProfile)

A profile sets the receiver's data rate and its matching demodulation parameters in one call. Available values (all at 433.92MHz):

| Enumerator        | Data rate | Typical devices                  |
| ----------------- | --------- | -------------------------------- |
| `k433MHz2400bps`  | 2.4 kbps  | Lithium-battery devices (common) |
| `k433MHz4800bps`  | 4.8 kbps  | Choose by real testing           |
| `k433MHz9600bps`  | 9.6 kbps  | Self-powered switches (common)   |
| `k433MHz19200bps` | 19.2 kbps | Choose by real testing           |
| `k433MHz30000bps` | 30 kbps   | Self-powered switches (common)   |
| `k433MHz40000bps` | 40 kbps   | Choose by real testing           |

Guidelines for choosing a profile:

- If the transmitter's parameters are unknown, try the profiles one by one from the lowest rate upward and keep the **smallest** profile that gives stable communication and meets your range requirement;
- A higher-rate profile can still receive a lower-rate transmitter, at the cost of shorter range and a higher bit error rate;
- The typical devices listed above are common references only — always confirm by real testing.

### Filter settings (SetFilterSettings)

```cpp
rfx1.SetFilterSettings(uint8_t min_count, uint16_t timeout_ms);
```

- `min_count`: the minimum number of identical frames the winning frame must reach within one window; set it to `0` to skip the count check and only require the winning frame to strictly outnumber every other candidate;
- `timeout_ms`: the window length in milliseconds, counted from the first frame the window receives;
- Defaults: `min_count = 1`, `timeout_ms = 300`.

Tuning trade-offs:

- A larger `min_count` rejects more noise but may drop valid frames;
- A longer `timeout_ms` collects more samples but adds latency, and may merge two consecutive presses into one;
- New settings take effect immediately and the current window restarts empty, so frames still being accumulated are dropped.

### Other API

| Method                                          | Description                                                |
| ----------------------------------------------- | ---------------------------------------------------------- |
| `Rfx1(TwoWire& wire, uint8_t address = 0x45)`   | Constructor; takes the I2C instance and the device address |
| `device_id()`                                   | Read the device ID                                         |
| `firmware_version()`                            | Read the firmware version (major.minor.patch)              |
| `SetRfProfile(RfProfile profile)`               | Set the RF profile                                         |
| `SetFilterSettings(min_count, timeout_ms)`      | Set the window filter parameters                           |
| `SetRxIndicatorEnabled(bool enabled)`           | Enable or disable the RX indicator                         |
| `PopRawFrame(uint32_t& frame)`                  | Pop a raw frame; returns `true` if a frame was retrieved   |
| `FlushFilteredFrame(uint32_t& frame)`           | Settle the filter window and read the result; returns `true` if a frame settled |
| `Rfx1::ToString(profile)`                       | Get a readable profile description, e.g. `30k (433.92MHz)` |

## Troubleshooting

**Q: No frames received at all?**

Check the following in order:

1. Make sure the transmitter operates at 433.92MHz;
2. A profile mismatch with the transmitter is the most common cause — start from the lowest profile, `k433MHz2400bps`, and step upward;
3. Make sure the I2C address matches the actual module (default `0x45`);
4. While debugging, call `PopRawFrame()` first to see whether raw frames are arriving at all, then check whether the filter settings are too strict.

**Q: No device found at `0x45` on the I2C bus?**

Check the VCC/GND/SDA/SCL wiring and power supply, confirm the host board's I2C pins (A4/A5 on an UNO), and verify that the module's address has not been modified.

**Q: Unstable frames or many corrupted values?**

This is usually caused by a too-high profile, which weakens noise immunity — try a lower profile. Alternatively, increase `min_count` so a result is only emitted after more identical frames agree.

**Q: One key press produces several frames, or presses are occasionally missed?**

A single press is normally transmitted as a burst of repeated frames. To collapse the duplicates, increase `min_count`; if presses are occasionally missed, the window may be too short or `min_count` too high — lengthen `timeout_ms` or lower `min_count`.

**Q: Nothing at all on the serial monitor?**

The example uses a 115200 bps baud rate — make sure the serial monitor matches. If the `device_id` and `firmware_version` lines printed in `setup()` are also missing, the problem is the I2C connection, not the RF link.

## Project Layout

```text
nulllab_rfx1/
├── src/nulllab_rfx1.h      # Library source (header-only)
├── examples/rfx1/rfx1.ino  # Basic reception example
├── library.properties      # Arduino library metadata
├── LICENSE                 # License
├── README.md               # English documentation
└── README.zh-CN.md         # Chinese documentation
```

## License

This project is released under the [MIT License](LICENSE).
