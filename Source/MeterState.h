#pragma once

#include <atomic>

namespace qqsc
{
struct MeterState
{
    // The three dedicated meters use two channels. In ST/LR these are L/R;
    // in MS these are Mid/Side.
    // Post-Ceiling physical L/R true-peak estimate. Hold 20 s of processed
    // audio without decay; double-clicking TP clears the accumulated peak.
    // Final audible output, independent of MATCH and the A/B parameter banks.
    std::atomic<float> outputIntegratedLufs { -120.0f };
    std::atomic<float> outputLoudnessSeconds { 0.0f };
    std::atomic<bool> outputLoudnessMeasuring { false };

    std::atomic<float> truePeakHoldDb { -120.0f };
    std::atomic<float> inputDb0  { -120.0f };
    std::atomic<float> inputDb1  { -120.0f };
    std::atomic<float> outputDb0 { -120.0f };
    std::atomic<float> outputDb1 { -120.0f };

    // Dynamic Display retains the raw pre-Input carrier for current-Input replay, plus live detector
    // evidence. v1.1.3 additionally keeps a bounded pre-gain/pre-HPF raw key
    // ring while the editor is open: Key Gain reprojects immediately and an
    // HPF/Window gestures replay retained history on the bounded UI worker.
    // In ST, channel 0 is linked and channel 1 is ignored. LR uses L/R; MS M/S.
    std::atomic<float> displayInputDb0  { -120.0f };
    std::atomic<float> displayInputDb1  { -120.0f };
    // Live evidence covers the last host block, not the GUI polling interval.
    std::atomic<int> displayBlockSamples { 0 };
    std::atomic<uint64_t> displayCaptureRevision { 0 }, displayCaptureCounter { 0 }, displayCaptureGeneration { 0 };
    std::atomic<int> displayCaptureMode { 0 };
    std::array<std::atomic<float>,5> displayDetectorSettings {}; // Lookahead, Window, OS, HPF, Safe
    std::array<std::atomic<float>,4> displaySTInputDb {}, displaySTDetectorDb {};
    std::atomic<float> displayDetectorDb0 { -120.0f };
    std::atomic<float> displayDetectorDb1 { -120.0f };
    // Post-Filter/Key-Gain sidechain signal, aligned to the Display carrier.
    // Separate from the Window/Safe detector that controls compression.
    std::atomic<float> displayExternalKeyDb0 { -120.0f };
    std::atomic<float> displayExternalKeyDb1 { -120.0f };

    // Product-facing GAIN +/- meter: compressor GR after Mix plus the actual
    // stereo-linked OutputCeiling attenuation. Makeup and Output Gain remain
    // excluded. Signed: positive = net reduction, negative = net boost.
    // Therefore TP/Sample-Peak limiting is visible here as well as in Display.
    std::atomic<float> gainReductionDb0 { 0.0f };
    std::atomic<float> gainReductionDb1 { 0.0f };

    // Dynamic Display capture: extra time-dependent attenuation applied by the
    // final stereo-linked OutputCeiling. Display keeps this as a separate history
    // component because its compressor curve is reprojected from detector history;
    // the dedicated GAIN +/- meter above receives the same attenuation directly.
    std::atomic<float> displayCeilingGainReductionDb { 0.0f };

    // Maximum pre-Ceiling inter-sample excess observed since DynamicDisplay's
    // last 60 Hz capture. Unlike displayCeilingGainReductionDb this is a shape
    // descriptor, not a captured processing result. DynamicDisplay consumes it
    // and re-applies the CURRENT TP/Ceiling/processing parameters to the whole
    // visible history, preserving the product's retrospective projection model.
    std::atomic<float> displayTruePeakExcessDb { 0.0f };

    // Recent Gain Reduction peak hold. The processor captures every audio block
    // so short peaks cannot be missed by the slower GUI timer. A new larger absolute GR
    // restarts the 2 s hold; when the hold expires it refreshes to the current
    // block value automatically.
    std::atomic<float> gainReductionHoldDb0 { 0.0f };
    std::atomic<float> gainReductionHoldDb1 { 0.0f };

    // Clipper retains the deepest reduction for the playback pass. Independent
    // of UI lifetime and detector reconfiguration; reset by playback or a click.
    std::atomic<float> clipperGainReductionHoldDb0 { 0.0f };
    std::atomic<float> clipperGainReductionHoldDb1 { 0.0f };

    // v1.1.0 Candidate External Key panel. The compact meter follows the
    // detector source after Input Gain (INT) or dedicated Key Gain (EXT).
    // Availability distinguishes a disabled external bus from valid silence.
    std::atomic<float> keyInputDb { -120.0f };
    std::atomic<bool> externalKeyBusAvailable { false };

    std::atomic<int> processingMode { 0 };
};
}
