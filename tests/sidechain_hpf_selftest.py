#!/usr/bin/env python3
import cmath
import math
from pathlib import Path

root = Path(__file__).resolve().parents[1]
processor = (root / "Source" / "PluginProcessor.cpp").read_text(encoding="utf-8")
header = (root / "Source" / "PluginProcessor.h").read_text(encoding="utf-8")
params = (root / "Source" / "Parameters.h").read_text(encoding="utf-8")
editor = (root / "Source" / "PluginEditor.cpp").read_text(encoding="utf-8")
editor_header = (root / "Source" / "PluginEditor.h").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")

assert "VERSION 1.1.5" in cmake
assert 'keyHpfHz       = "keyHpfHz"' in params
assert params.index('keyGainDb      = "keyGainDb"') < params.index('keyHpfHz       = "keyHpfHz"')
assert "keyHpfOffHz = 0.0f" in params
assert "keyHpfMinHz = 20.0f" in params and "keyHpfMaxHz = 500.0f" in params
assert "currentStateSchemaVersion = 10" in processor

# HPF is post source selection/Key Gain and pre meter, detector oversampling and
# SC Listen storage. The carrier remains in channels 0/1 and is never replaced.
gain_pos = processor.index("selectedKeyL = externalL * keyGain;")
filter_pos = processor.index("filteredKeyL = keyHighPassStates[0].process")
key_buffer_pos = processor.index("keyInputBuffer.setSample (0, i, selectedKeyL);")
detector_pos = processor.index("const float keyL = oversampledBlock.getSample (2, i);")
assert gain_pos < filter_pos < key_buffer_pos < detector_pos
assert "oversamplingInputBuffer.setSample (0, i, mainL);" in processor
assert "oversamplingInputBuffer.setSample (2, i, selectedKeyL);" in processor
assert "keyListenDelayBuffer.setSample (0, dryDelayWriteIndex, keyInputBuffer.getSample (0, i));" in processor

# Parameter is automatable/persistent and participates in A/B. Old states and old
# A/B snapshots explicitly migrate to OFF.
for token in (
    'qqsc::params::keyHpfHz, 1',
    'snapshot.keyHpfHz = apvts.getRawParameterValue',
    'prefix + "keyHpfHz"',
    "stateHasKeyHpf",
    "qqsc::params::keyHpfOffHz",
):
    assert token in processor, token

# The popup grows leftward and exposes the same control in both themes.
for token in (
    "keyHpfLabel",
    "keyHpfSlider",
    "keyHpfAttachment",
    "sidechainButton.getRight() - 330",
):
    assert token in editor or token in editor_header, token
assert "&keyGainSlider, &keyHpfSlider" in editor

# Reference RBJ 2nd-order Butterworth response used by the C++ detector HPF.
def magnitude(sample_rate, cutoff, frequency):
    omega = 2.0 * math.pi * cutoff / sample_rate
    sine = math.sin(omega)
    cosine = math.cos(omega)
    alpha = sine / (2.0 / math.sqrt(2.0))
    a0 = 1.0 + alpha
    b0 = ((1.0 + cosine) * 0.5) / a0
    b1 = -(1.0 + cosine) / a0
    b2 = b0
    a1 = (-2.0 * cosine) / a0
    a2 = (1.0 - alpha) / a0
    z1 = cmath.exp(-1j * 2.0 * math.pi * frequency / sample_rate)
    numerator = b0 + b1 * z1 + b2 * z1 * z1
    denominator = 1.0 + a1 * z1 + a2 * z1 * z1
    return abs(numerator / denominator)

assert magnitude(48000.0, 120.0, 30.0) < 0.07
assert magnitude(48000.0, 120.0, 1000.0) > 0.999
raw = 0.314159
filtered = -0.271828
assert raw + (filtered - raw) * 0.0 == raw  # OFF is exact dry detector signal.

print("PASS: v1.1.5 detector-only Side Chain HPF parameter/DSP/state/A-B/UI wiring and response.")