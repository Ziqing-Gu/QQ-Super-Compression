#pragma once

namespace qqsc
{
inline constexpr float normalMaximumMakeupDb = 30.0f;
inline constexpr float maximumMakeupDb = 120.0f;
inline constexpr float classicThresholdMinimumDb = -90.0f;
inline constexpr float classicThresholdMinimumGain = 3.162277660168379e-5f; // -90 dB
inline constexpr float minimumUpRatio = 1.0f / 200.0f;
inline constexpr float normalMaximumDownRatio = 200.0f;
inline constexpr float limiterMinimumUpRatio = 1.0f / 8.0f; // Limiter SINGLE upward side: 1:8 .. 1:1.
inline constexpr float limiterDualMinimumUpRatio = 1.0f / 8.0f; // Limiter DUAL UP: 1:8 .. 1:1.
inline constexpr float limiterMinimumDownRatio = 1.0f; // Limiter DOWN: 1:1 .. 1000:1; Ceiling remains the final limiter.
// The shared engine must still accommodate the Limiter bank.
inline constexpr float maximumDownRatio = 1000.0f;
inline constexpr float maximumUpwardGain = 1000000.0f;
}
