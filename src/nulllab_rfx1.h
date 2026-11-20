#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "Wire.h"
#include "cyf.h"
#include "cyf/array.h"
#include "cyf/i2c.h"

namespace nulllab {

class Rfx1 {
 public:
  enum class RfProfile : uint8_t {
    k433MHz2400bps = 0,   // 2.4 kbps（433.92MHz）
    k433MHz4800bps = 1,   // 4.8 kbps（433.92MHz）
    k433MHz9600bps = 2,   // 9.6 kbps（433.92MHz）
    k433MHz19200bps = 3,  // 19.2 kbps（433.92MHz）
    k433MHz30000bps = 4,  // 30 kbps（433.92MHz）
    k433MHz40000bps = 5,  // 40 kbps（433.92MHz）
  };

  /**
   * @~English
   * @brief Get the user-readable description of an RF profile.
   * @param profile The RF profile.
   * @return The readable description, e.g. "30k (433.92MHz)".
   */
  /**
   * @~Chinese
   * @brief 获取 RF 配置档位的用户可读描述。
   * @param profile RF 配置档位。
   * @return 可读描述，如 "30k (433.92MHz)"。
   */
  static String ToString(RfProfile profile) {
    switch (profile) {
      case RfProfile::k433MHz2400bps: {
        return "2.4k (433.92MHz)";
      }
      case RfProfile::k433MHz4800bps: {
        return "4.8k (433.92MHz)";
      }
      case RfProfile::k433MHz9600bps: {
        return "9.6k (433.92MHz)";
      }
      case RfProfile::k433MHz19200bps: {
        return "19.2k (433.92MHz)";
      }
      case RfProfile::k433MHz30000bps: {
        return "30k (433.92MHz)";
      }
      case RfProfile::k433MHz40000bps: {
        return "40k (433.92MHz)";
      }
      default: {
        return "";
      }
    }
  }

  static constexpr uint8_t kDefaultI2cAddress = 0x45;

  /**
   * @~English
   * @brief Constructor
   * @param wire The TwoWire instance.
   * @param i2c_address The 7-bit I2C device address, defaults to kDefaultI2cAddress.
   */
  /**
   * @~Chinese
   * @brief 构造函数
   * @param wire TwoWire 实例。
   * @param i2c_address 7 位 I2C 设备地址，默认为 kDefaultI2cAddress。
   */
  constexpr Rfx1(TwoWire& wire, uint8_t i2c_address = kDefaultI2cAddress) : wire_(wire), i2c_address_(i2c_address) {}

  /**
   * @~English
   * @brief Read the device ID.
   * @return The device ID.
   */
  /**
   * @~Chinese
   * @brief 读取设备 ID。
   * @return 设备 ID。
   */
  inline uint8_t device_id() const { return cyf::i2c::Read<uint8_t>(wire_, i2c_address_, kAddressDeviceId); }

  /**
   * @~English
   * @brief Read the firmware version.
   * @return The firmware version (major, minor, patch).
   */
  /**
   * @~Chinese
   * @brief 读取固件版本。
   * @return 固件版本（主版本、次版本、修订号）。
   */
  inline cyf::Array<uint8_t, 3> firmware_version() const {
    cyf::Array<uint8_t, 3> version;
    cyf::i2c::Read(wire_, i2c_address_, kAddressMajorVersion, version.data, version.size());
    return version;
  }

  /**
   * @~English
   * @brief Set the RF profile.
   * @param profile The RF profile to apply.
   *
   * If the transmitter's profile (especially its frequency) is unknown, try the
   * profiles one by one from the lowest frequency to the highest, and keep the
   * one that gives the most stable communication while meeting the required range.
   *
   * A higher-frequency profile can still receive a lower-frequency transmitter,
   * but it may shorten the communication range and raise the bit error rate.
   * Prefer to confirm the transmitter's parameters first, or use the smallest
   * profile that meets the requirement.
   *
   * Common references: lithium-battery devices are usually 2.4k, and
   * self-powered switches are usually 9.6k or 30k. Always confirm by real testing.
   */
  /**
   * @~Chinese
   * @brief 设置 RF 配置档位。
   * @param profile 要应用的 RF 配置档位。
   *
   * 若不清楚发射器的 Profile（尤其是频率），应从低频到高频逐档设置并实测，
   * 选择通信最稳定且满足距离要求的档位。
   *
   * 较高频率的 Profile 可兼容较低频率的发射器，但可能导致通信距离缩短、
   * 误码率升高；建议明确发射器参数，或优先使用满足要求的最小 Profile。
   *
   * 常见参考：锂电池设备通常为 2.4k，自发电开关通常为 9.6k 或 30k，
   * 最终以实际测试为准。
   */
  inline void SetRfProfile(RfProfile profile) {
    cyf::i2c::Write(wire_, i2c_address_, kAddressRfProfile, static_cast<uint8_t>(profile));
  }

  /**
   * @~English
   * @brief Set the frame filter settings.
   * @param min_count Minimum number of identical frames the winning frame must
   *        reach within one window; 0 disables the count check and only requires
   *        the frame to lead all other candidates.
   * @param timeout_ms Window length in milliseconds, counted from the first frame
   *        the window receives.
   *
   * Window filtering in plain terms: one key press is normally sent as a burst of
   * repeated frames, mixed with noise and corrupted bits. The filter groups the
   * frames received during a window by content and counts each group; when the
   * window closes, only the most frequent group may be reported, and only if it
   * reached min_count frames and is strictly more frequent than any other group.
   * Otherwise the whole window is discarded. The settled result is read with
   * FlushFilteredFrame().
   *
   * The two parameters answer different questions: timeout_ms is how long the
   * filter listens, min_count is how many identical frames it must hear before
   * trusting them. A larger min_count rejects more noise but may drop valid
   * frames; a longer timeout_ms collects more samples but adds latency and may
   * merge two consecutive presses.
   *
   * Defaults are min_count = 1 and timeout_ms = 300. The new settings take effect
   * immediately and the current window restarts empty, so frames still being
   * accumulated are dropped. min_count is a uint8_t (0-255), timeout_ms is a
   * uint16_t (0-65535).
   */
  /**
   * @~Chinese
   * @brief 设置帧滤波参数。
   * @param min_count 一个窗口中胜出帧需要达到的最少相同帧数；为 0 时不做数量校验，
   *        只要求它严格多于其它候选帧。
   * @param timeout_ms 窗口时长（毫秒），从窗口收到第一帧开始计时。
   *
   * 什么是窗口滤波：一次按键通常会被发射器连发成多帧相同内容，空中还混杂着噪声和
   * 误码。滤波把一段时间内收到的帧按内容分组并分别计数，窗口结束时只有出现次数最多
   * 的那一组才可能被输出，而且必须同时满足“数量达到 min_count”与“严格多于其它任何
   * 一组”；不满足时整个窗口作废重来。结算结果由 FlushFilteredFrame() 读取。
   *
   * 两个参数回答的是不同问题：timeout_ms 是“听多久”，min_count 是“听到几帧才敢信”。
   * 调大 min_count 更能抑制噪声，但可能漏掉有效帧；调长 timeout_ms 能积累更多样本，
   * 但会增加延迟，甚至把前后两次按键合并成一次。
   *
   * 默认值为 min_count = 1、timeout_ms = 300。新设置立即生效，且当前窗口会清空重启，
   * 仍在累积的帧将被丢弃。min_count 类型为 uint8_t（0~255），timeout_ms 类型为
   * uint16_t（0~65535）。
   */
  inline void SetFilterSettings(uint8_t min_count, uint16_t timeout_ms) {
    const uint8_t payload[kCmdParamSize + 1] = {
        min_count,                                // p0
        static_cast<uint8_t>(timeout_ms & 0xFF),  // p1
        static_cast<uint8_t>(timeout_ms >> 8),    // p2
        0,                                        // p3
        0,                                        // p4
        0,                                        // p5
        0,                                        // p6
        static_cast<uint8_t>(Cmd::kApplyFilter),  // cmd
    };
    cyf::i2c::Write(wire_, i2c_address_, kAddressCmdParam, payload, sizeof(payload));
  }

  /**
   * @~English
   * @brief Pop a raw frame from the device FIFO.
   * @param frame The retrieved 24-bit frame data.
   * @return True if a frame was retrieved, false otherwise.
   */
  /**
   * @~Chinese
   * @brief 从设备 FIFO 弹出一帧原始数据。
   * @param frame 输出的 24 位帧数据。
   * @return 取到帧时返回 true，否则返回 false。
   */
  inline bool PopRawFrame(uint32_t& frame) {
    cyf::i2c::Write(wire_, i2c_address_, kAddressCmd, static_cast<uint8_t>(Cmd::kRefreshRaw));
    frame = cyf::i2c::Read<uint32_t>(wire_, i2c_address_, kAddressRawFrame);
    if ((frame & kFrameValidBit) != 0) {
      frame &= kFrameDataMask;
      return true;
    } else {
      frame = 0;
      return false;
    }
  }

  /**
   * @~English
   * @brief Flush the filter window and read the filtered frame.
   * @param frame The retrieved 24-bit filtered frame data.
   * @return True if a filtered frame was settled, false otherwise.
   */
  /**
   * @~Chinese
   * @brief 结算滤波窗口并读取滤波后的帧。
   * @param frame 输出的 24 位滤波帧数据。
   * @return 有滤波结果时返回 true，否则返回 false。
   */
  inline bool FlushFilteredFrame(uint32_t& frame) {
    cyf::i2c::Write(wire_, i2c_address_, kAddressCmd, static_cast<uint8_t>(Cmd::kRefreshFiltered));
    frame = cyf::i2c::Read<uint32_t>(wire_, i2c_address_, kAddressFilteredFrame);
    if ((frame & kFrameValidBit) != 0) {
      frame &= kFrameDataMask;
      return true;
    } else {
      frame = 0;
      return false;
    }
  }

  /**
   * @~English
   * @brief Enable or disable the RX indicator.
   * @param enabled True to enable, false to disable.
   */
  /**
   * @~Chinese
   * @brief 使能或关闭 RX 指示。
   * @param enabled true 为使能，false 为关闭。
   */
  inline void SetRxIndicatorEnabled(bool enabled) {
    cyf::i2c::Write(wire_, i2c_address_, kAddressRxIndicatorEnabled, enabled ? uint8_t{1} : uint8_t{0});
  }

 private:
  static constexpr uint8_t kAddressDeviceId = 0x00;
  static constexpr uint8_t kAddressMajorVersion = 0x01;
  static constexpr uint8_t kAddressMinorVersion = 0x02;
  static constexpr uint8_t kAddressPatchVersion = 0x03;
  static constexpr uint8_t kAddressCmdParam = 0x10;
  static constexpr uint8_t kAddressCmd = 0x17;
  static constexpr uint8_t kAddressRfProfile = 0x18;
  static constexpr uint8_t kAddressRxIndicatorEnabled = 0x1A;
  static constexpr uint8_t kAddressRawFrame = 0x20;
  static constexpr uint8_t kAddressFilteredFrame = 0x24;

  static constexpr uint32_t kFrameValidBit = 0x80000000;
  static constexpr uint32_t kFrameDataMask = 0xFFFFFF;
  static constexpr uint8_t kCmdParamSize = 7;

  enum class Cmd : uint8_t {
    kRefreshRaw = 1,
    kRefreshFiltered = 2,
    kApplyFilter = 3,
  };

  TwoWire& wire_;
  const uint8_t i2c_address_;
};

}  // namespace nulllab
