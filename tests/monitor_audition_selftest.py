#!/usr/bin/env python3
import math
import random
from pathlib import Path

root = Path(__file__).resolve().parents[1]
processor = (root / 'Source' / 'PluginProcessor.cpp').read_text(encoding='utf-8')
editor_h = (root / 'Source' / 'PluginEditor.h').read_text(encoding='utf-8')
editor = (root / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')
params = (root / 'Source' / 'Parameters.h').read_text(encoding='utf-8')
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')

# Product identity/state wiring.
assert 'VERSION 1.0.4' in cmake
for token in ('monitorAll', 'monitorFirst', 'monitorSecond'):
    assert token in params, token
for token in ('qqscMonitorLRSelection', 'qqscMonitorMSSelection', 'currentStateSchemaVersion = 8'):
    assert token in processor, token

# Exact centered audition contract agreed with the user.
for token in (
    'constexpr float centeredChannelMonitorGain = 0.70710678118654752440f',
    'audibleOutL = audibleOutR = outL * centeredChannelMonitorGain',
    'audibleOutL = audibleOutR = outR * centeredChannelMonitorGain',
    'const float activeM = 0.5f * (outL + outR)',
    'const float activeS = 0.5f * (outL - outR)',
    'audibleOutL = audibleOutR = activeM;',
    'audibleOutL = audibleOutR = activeS * centeredChannelMonitorGain',
):
    assert token in processor, token

# The actual audio buffer gets the monitor result, while analysis remains based on
# the normal pre-monitor output. This prevents -3.01 dB listening compensation
# from contaminating Display/Meters/Match semantics.
write_pos = processor.index('buffer.setSample (0, i, audibleOutL);')
graph_pos = processor.index('graphOutputPeak = juce::jmax')
assert write_pos < graph_pos
assert 'maxAbs (outL, outR)' in processor[graph_pos:graph_pos + 220]
assert 'std::abs (outM)' in processor
assert 'std::abs (outS)' in processor

# UI is mode-local: hidden in ST, labelled L/R in LR and M/S in MS.
for token in (
    'juce::Label monitorLabel',
    'monitorAllButton', 'monitorFirstButton', 'monitorSecondButton',
    'monitorFirstButton.setButtonText (lr ? "L" : "M")',
    'monitorSecondButton.setButtonText (lr ? "R" : "S")',
    'const bool visible = lr || ms;',
    'processor.setDomainMonitorSelection (mode, selection)',
):
    assert token in editor_h or token in editor, token

# Numeric math checks. L/R/S get equal-power -3.0103 dB when copied to both ears;
# Mid is already M=(L+R)/2 and remains unity when centred.
k = 1.0 / math.sqrt(2.0)
assert math.isclose(k, 0.7071067811865476, rel_tol=0.0, abs_tol=1e-15)
random.seed(103)
for _ in range(10000):
    l = random.uniform(-2.0, 2.0)
    r = random.uniform(-2.0, 2.0)
    m = 0.5 * (l + r)
    s = 0.5 * (l - r)

    lr_l = (l * k, l * k)
    lr_r = (r * k, r * k)
    ms_m = (m, m)
    ms_s = (s * k, s * k)

    assert math.isclose(lr_l[0], lr_l[1], abs_tol=0.0)
    assert math.isclose(lr_r[0], lr_r[1], abs_tol=0.0)
    assert math.isclose(ms_m[0], m, abs_tol=0.0)  # no extra -3.01 dB on M
    assert math.isclose(ms_s[0], ms_s[1], abs_tol=0.0)

print('PASS: v1.0.3 centered LR/MS audition monitor formulas, UI/state wiring, and analysis isolation.')
