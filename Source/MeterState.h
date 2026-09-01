#pragma once

#include <atomic>

namespace qqsc
{
struct MeterState
{
    // The three dedicated meters use two channels. In ST/LR these are L/R;
    // in MS these are Mid/Side.
    std::atomic<float> inputDb0  { -120.0f };
    std::atomic<float> inputDb1  { -120.0f };
    std::atomic<float> outputDb0 { -120.0f };
    std::atomic<float> outputDb1 { -120.0f };

    // v1.1.2 Dynamic Display stores the pre-Input-Gain carrier plus the actual
    // future-window detector level. The UI reprojects the complete visible
    // history with current Input/Ratio/Threshold/Mix/Makeup/Output values, so
    // parameter edits remain explanatory even after the audio has passed.
    // In ST, channel 0 is linked and channel 1 is ignored. LR uses L/R; MS M/S.
    std::atomic<float> displayInputDb0  { -120.0f };
    std::atomic<float> displayInputDb1  { -120.0f };
    std::atomic<float> displayDetectorDb0 { -120.0f };
    std::atomic<float> displayDetectorDb1 { -120.0f };

    // Product-facing GR includes Mix (Makeup and Output Gain remain excluded).
    std::atomic<float> gainReductionDb0 { 0.0f };
    std::atomic<float> gainReductionDb1 { 0.0f };

    // Recent Gain Reduction peak hold. The processor captures every audio block
    // so short peaks cannot be missed by the slower GUI timer. A new larger GR
    // restarts the 2 s hold; when the hold expires it refreshes to the current
    // block value automatically.
    std::atomic<float> gainReductionHoldDb0 { 0.0f };
    std::atomic<float> gainReductionHoldDb1 { 0.0f };

    // v1.1.0 Candidate External Key panel. The compact meter follows the
    // detector source after Input Gain (INT) or dedicated Key Gain (EXT).
    // Availability distinguishes a disabled external bus from valid silence.
    std::atomic<float> keyInputDb { -120.0f };
    std::atomic<bool> externalKeyBusAvailable { false };

    std::atomic<int> processingMode { 0 };
};
}
