#pragma once

#include <atomic>

namespace qqsc
{
struct MeterState
{
    // Dynamic-history aggregate values are always measured in the audible L/R
    // output domain so the graph remains comparable while modes change.
    std::atomic<float> inputDb  { -120.0f };
    std::atomic<float> wetDb    { -120.0f };
    std::atomic<float> outputDb { -120.0f };

    // The three dedicated meters use two channels. In ST/LR these are L/R;
    // in MS these are Mid/Side.
    std::atomic<float> inputDb0  { -120.0f };
    std::atomic<float> inputDb1  { -120.0f };
    std::atomic<float> outputDb0 { -120.0f };
    std::atomic<float> outputDb1 { -120.0f };
    std::atomic<float> gainReductionDb0 { 0.0f };
    std::atomic<float> gainReductionDb1 { 0.0f };

    // Recent Gain Reduction peak hold. The processor captures every audio block
    // so short peaks cannot be missed by the slower GUI timer. A new larger GR
    // restarts the 2 s hold; when the hold expires it refreshes to the current
    // block value automatically.
    std::atomic<float> gainReductionHoldDb0 { 0.0f };
    std::atomic<float> gainReductionHoldDb1 { 0.0f };

    std::atomic<int> processingMode { 0 };
};
}
