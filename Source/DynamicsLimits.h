#pragma once

namespace qqsc
{
inline constexpr float maximumMakeupDb = 120.0f;
inline constexpr float classicThresholdMinimumDb = -90.0f;
inline constexpr float classicThresholdMinimumGain = 3.162277660168379e-5f; // -90 dB
inline constexpr float minimumUpRatio = 1.0f / 200.0f;
inline constexpr float normalMaximumDownRatio = 200.0f;
inline constexpr float limiterMinimumUpRatio = 1.0f / 200.0f;
inline constexpr float limiterMinimumDownRatio = 200.0f;
// The shared engine must still accommodate the Limiter bank.
inline constexpr float maximumDownRatio = 1000.0f;
inline constexpr float maximumUpwardGain = 1000000.0f;
}
