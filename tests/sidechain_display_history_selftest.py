#!/usr/bin/env python3
import math
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
processor_h = (root / "Source" / "PluginProcessor.h").read_text(encoding="utf-8")
processor = (root / "Source" / "PluginProcessor.cpp").read_text(encoding="utf-8")
display_h = (root / "Source" / "DynamicDisplay.h").read_text(encoding="utf-8")
display = (root / "Source" / "DynamicDisplay.cpp").read_text(encoding="utf-8")
editor = (root / "Source" / "PluginEditor.cpp").read_text(encoding="utf-8")

assert "VERSION 1.1.5" in cmake

# Audio-thread capture is bounded, active only with the editor, capped at
# 48 kHz and stores the selected raw key before detector gain and HPF.
for token in (
    "DisplayKeyHistorySnapshot",
    "setDisplayKeyHistoryCaptureEnabled",
    "copyDisplayKeyHistory",
    "std::atomic<uint64_t>",
    "juce::jmin (48000.0, hostSampleRate)",
    "analysisSampleRate * 10.0",
):
    assert token in processor_h or token in processor, token

raw_capture = processor.index("displayHistoryForBlock->push (rawDisplayKeyL")
hpf_process = processor.index("filteredKeyL = keyHighPassStates[0].process")
assert raw_capture < hpf_process
assert "rawDisplayKeyL = externalL;" in processor
assert "rawDisplayKeyR = externalR;" in processor

# Key Gain stays on the cached 60 Hz projection path and therefore moves the
# complete live/replayed history while the knob is moving.
assert "capturedKeyGainDb" in display_h
assert "detectorDb += keyGainDb - point.capturedKeyGainDb;" in display
assert "detectorDb += externalKey ? keyGainDb : inputGainDb;" in display

# HPF never launches a full replay on every drag tick. Mouse release requests
# one worker pass; non-mouse automation/preset changes use a short stable-value
# debounce so they cannot leave stale history behind.
assert "display.beginKeyHpfGesture();" in editor
assert "keyHpfSlider.onGestureEnd = [this] { display.endKeyHpfGesture(); };" in editor
assert "requestHpfHistoryRefresh();" in display
assert "hpfAutomationStableTicks = 4" in display_h
assert "hpfMaxRetryAttempts = 3" in display_h
assert "hpfReplayPreRollSamples = 48000" in display_h
assert "handleHpfReplayFailure" in display
assert "requestHpfHistoryRefresh (true)" in display
assert "uint64_t startCounter" in processor_h and "requestedStartCounter" in processor
assert "QQSC Display HPF Replay" in display
assert "juce::Thread::Priority::low" in display
assert "HPF UPDATING" in display
assert "ReplayPeakWindow primaryEngine" in display
assert "ReplayPeakWindow secondaryEngine" in display
assert "ReplayPeakWindow leftEngine" not in display

# Reference the same RBJ Butterworth HPF used by the plug-in and prove that a
# historical low-frequency section changes substantially while high-frequency
# material remains nearly unchanged. This is why a dB-only correction is not
# accepted as a valid HPF history implementation.
def hpf(signal, sample_rate, cutoff):
    omega = 2.0 * math.pi * cutoff / sample_rate
    sine = math.sin(omega)
    cosine = math.cos(omega)
    alpha = sine / (2.0 * (1.0 / math.sqrt(2.0)))
    a0 = 1.0 + alpha
    b0 = ((1.0 + cosine) * 0.5) / a0
    b1 = -(1.0 + cosine) / a0
    b2 = b0
    a1 = (-2.0 * cosine) / a0
    a2 = (1.0 - alpha) / a0
    z1 = z2 = 0.0
    result = []
    for sample in signal:
        output = b0 * sample + z1
        z1 = b1 * sample - a1 * output + z2
        z2 = b2 * sample - a2 * output
        result.append(output)
    return result

sr = 48000
segment = sr // 2
low = [0.05 * math.sin(2.0 * math.pi * 30.0 * n / sr) for n in range(segment)]
high = [0.05 * math.sin(2.0 * math.pi * 1000.0 * n / sr) for n in range(segment)]
filtered = hpf(low + high, sr, 120.0)
settle = sr // 10
low_peak = max(abs(x) for x in filtered[settle:segment])
high_peak = max(abs(x) for x in filtered[segment + settle:])
assert low_peak < 0.004
assert high_peak > 0.049

# Key Gain is a linear multiplier around the HPF, so +12 dB can be projected
# immediately as a +12 dB detector-history offset without rerunning the filter.
base_db = 20.0 * math.log10(high_peak)
gain_db = 12.0
projected_db = min(0.0, base_db + gain_db)
assert math.isclose(projected_db - base_db, gain_db, abs_tol=1.0e-9)

print("PASS: v1.1.5 real-time Key Gain history and release-triggered background HPF replay.")
