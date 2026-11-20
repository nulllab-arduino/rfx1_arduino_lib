#include "Wire.h"
#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/log.h"
#include "nulllab_rfx1.h"

namespace {
constexpr uint8_t kFilterMinCount = 1;
constexpr uint16_t kFilterTimeoutMs = 300;  // ms
constexpr bool kRxIndicatorEnabled = true;

// RF profile: sets the receiver data rate and its matching demodulation
// parameters in one go. It must match the transmitter, or at least fall within
// a range this profile can still receive, otherwise no frame is decoded.
//
// Available values (all at 433.92MHz): 2.4k / 4.8k / 9.6k / 19.2k / 30k / 40k bps,
// i.e. every profile listed below; only one line is active, the rest stay commented.
//
// Typical use: lithium-battery devices are usually 2.4k, self-powered (battery-free)
// switches are usually 9.6k or 30k; pick the remaining ones by real testing.
// A higher profile keeps each frame on air for less time, but tolerates less noise.
//
// How to choose: if the transmitter is unknown, try the profiles from the lowest to
// the highest rate, and keep the smallest one that is stable and meets the range
// requirement. Higher profiles can still receive lower-rate transmitters, at the
// cost of shorter range and a higher bit error rate.
//
// The current default is 2.4k (k433MHz2400bps).
//
// RF 配置档位（RfProfile）：一次性设定接收端的速率及配套解调参数，
// 必须与发射器一致或落在该档位可兼容的范围内，否则收不到帧。
//
// 可选值（频率均为 433.92MHz）：2.4k / 4.8k / 9.6k / 19.2k / 30k / 40k bps，
// 即下面逐行列出的全部档位；同一时刻只有一行生效，其余保持注释。
//
// 适用场景：锂电池供电设备通常为 2.4k；自发电（免电池）开关通常为 9.6k 或 30k；
// 其余档位按实测选择。档位越高单帧占用的空口时间越短，抗噪能力也越弱。
//
// 选择要点：不清楚发射器参数时，由低到高逐档设置并实测，取通信最稳定且满足
// 距离要求的最小档位；较高档位可兼容较低速率的发射器，但距离缩短、误码率升高。
//
// 当前默认使用 2.4k（k433MHz2400bps）。
constexpr auto kRfProfile = nulllab::Rfx1::RfProfile::k433MHz2400bps;  // 2.4 kbps（433.92MHz）
// constexpr auto kRfProfile = nulllab::Rfx1::RfProfile::k433MHz4800bps; // 4.8 kbps（433.92MHz）
// constexpr auto kRfProfile = nulllab::Rfx1::RfProfile::k433MHz9600bps; // 9.6 kbps（433.92MHz）
// constexpr auto kRfProfile = nulllab::Rfx1::RfProfile::k433MHz19200bps; // 19.2 kbps（433.92MHz）
// constexpr auto kRfProfile = nulllab::Rfx1::RfProfile::k433MHz30000bps; // 30 kbps（433.92MHz）
// constexpr auto kRfProfile = nulllab::Rfx1::RfProfile::k433MHz40000bps; // 40 kbps（433.92MHz）

nulllab::Rfx1 g_rfx1(Wire, nulllab::Rfx1::kDefaultI2cAddress);
}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin();
  CLOGI("RFX1 demo");
  CLOGI("device id: 0x%02X", g_rfx1.device_id());

  const auto firmware_version = g_rfx1.firmware_version();
  CLOGI("firmware version: %u.%u.%u", firmware_version[0], firmware_version[1], firmware_version[2]);

  g_rfx1.SetRxIndicatorEnabled(kRxIndicatorEnabled);
  CLOGI("set rx indicator enabled: %u", static_cast<unsigned>(kRxIndicatorEnabled));

  g_rfx1.SetRfProfile(kRfProfile);
  CLOGI("set rf profile: %s", nulllab::Rfx1::ToString(kRfProfile).c_str());

  g_rfx1.SetFilterSettings(kFilterMinCount, kFilterTimeoutMs);
  CLOGI("set filter settings: min_count=%u, timeout=%u ms", static_cast<unsigned>(kFilterMinCount),
        static_cast<unsigned>(kFilterTimeoutMs));

  CLOGI("start receving...");
}

void loop() {
  uint32_t raw_frame = 0;
  uint32_t filtered_frame = 0;

  if (g_rfx1.PopRawFrame(raw_frame)) {
    static uint32_t s_raw_count = 0;
    CLOGI("[raw %lu] 0x%06lX", static_cast<unsigned long>(s_raw_count++), static_cast<unsigned long>(raw_frame));
  }

  if (g_rfx1.FlushFilteredFrame(filtered_frame)) {
    static uint32_t s_filtered_count = 0;
    CLOGI("[filtered %lu] 0x%06lX", static_cast<unsigned long>(s_filtered_count++), static_cast<unsigned long>(filtered_frame));
  }
}
