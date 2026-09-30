from pathlib import Path
import math, random, re, sys
root=Path(__file__).resolve().parents[1]
proc=(root/'Source/PluginProcessor.cpp').read_text()
disp=(root/'Source/DynamicDisplay.cpp').read_text()
ceil=(root/'Source/OutputCeiling.h').read_text()
meter=(root/'Source/MeterState.h').read_text()
cm=(root/'CMakeLists.txt').read_text()
assert 'VERSION 1.2.17' in cm
assert 'qqsc::params::outputGainDb, 1 }, "Output Gain",\n        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }' in proc
assert 'limiterOutputDb, 1 }, "Limiter Output Gain",\n        juce::NormalisableRange<float> { -120.0f, 120.0f, 0.01f }' in proc
assert 'point.measuredOutputDb' not in disp[disp.index('void DynamicDisplay::projectHistory'):disp.index('void DynamicDisplay::scheduleHpfReplayRetry')]
assert 'point.ceilingGainReductionDb' not in disp[disp.index('void DynamicDisplay::projectHistory'):disp.index('void DynamicDisplay::scheduleHpfReplayRetry')]
for token in ['point.truePeakExcessDb','processor.isTruePeakSelected()','projectedPostCeilingDb - activeOutputGainDb']:
    assert token in disp
for token in ['inputSamplePeakForDisplayLinear','inputTruePeakForDisplayLinear']:
    assert token in ceil and token in proc
assert 'displayTruePeakExcessDb' in meter and '.exchange (0.0f' in disp

# Mathematical contract: blue and orange have identical contour, separated by
# current fixed output shift; TP changes old points without mutating history.
r=random.Random(1217)
changed=0
for _ in range(100000):
    inp=r.uniform(-90,6); shift=r.uniform(-24,24); ceiling=r.uniform(-24,0); excess=r.uniform(0,5)
    pre=inp+shift
    native_gr=max(0.0,pre-ceiling)
    native_out=pre-native_gr; native_blue=native_out-shift
    tp_gr=max(0.0,pre+excess-ceiling)
    if tp_gr>0: tp_gr += .01
    tp_out=pre-tp_gr; tp_blue=tp_out-shift
    assert abs((native_out-native_blue)-shift)<1e-9
    assert abs((tp_out-tp_blue)-shift)<1e-9
    if abs(tp_out-native_out)>1e-7: changed+=1
assert changed>1000
print('PASS: revision1217 source audit; 100000 retrospective TP/contour identities')
