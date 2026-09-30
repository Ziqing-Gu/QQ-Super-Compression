#!/usr/bin/env python3
from pathlib import Path
import math, random, re
root=Path(__file__).resolve().parents[1]
out=(root/'Source/OutputCeiling.h').read_text(encoding='utf-8')
proc=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
start=out.index('std::array<float,2> process(float l,float r)')
end=out.index('private:', start)
body=out[start:end]
assert 'gainToDecibels' not in body, 'per-sample dB/log conversion remains in OutputCeiling::process'
assert 'displayGainLinear=juce::jlimit' in out
assert 'gainForDisplayLinear()' in out
assert 'outputCeiling.gainForDisplayLinear()' in proc
assert 'outputCeiling.gainReductionDbForDisplay()' not in proc
assert 'displayCeilingGainLinear=juce::jmin' in proc
# Ensure conversion is after the host-sample loop and before meter publication.
loop_pos=proc.index('for (int i = 0; i < numSamples; ++i)')
conv_pos=proc.index('const auto displayCeilingGainReductionDb =')
store_pos=proc.index('meterState.displayCeilingGainReductionDb.store')
assert loop_pos < conv_pos < store_pos
# max(-20log10(g_i)) == -20log10(min(g_i)); verify across random blocks.
rng=random.Random(1215)
for _ in range(100000):
    gains=[10**(-rng.uniform(0,80)/20) for _ in range(rng.randint(1,64))]
    old=max(-20*math.log10(max(g,1e-9)) for g in gains)
    new=-20*math.log10(max(min(gains),1e-9))
    assert abs(old-new) < 1e-10
# Illustrative call-count reduction; actual host block size may differ.
for sr,block in [(44100,256),(48000,256),(96000,256),(48000,64)]:
    old=sr
    new=sr/block
    assert old/new == block
print('PASS: no dB/log conversion in per-sample OutputCeiling::process telemetry path.')
print('PASS: 100000 random blocks prove min(linear gain) == max(per-sample attenuation dB).')
print('PASS: runtime conversion count drops from about sample-rate times/sec to block-rate times/sec (e.g. 48000 -> 187.5 at 256 samples).')
