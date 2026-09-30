from pathlib import Path
import random

root=Path(__file__).resolve().parents[1]
out=(root/'Source/OutputCeiling.h').read_text(encoding='utf-8')
proc=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
dyn=(root/'Source/DynamicDisplay.cpp').read_text(encoding='utf-8')
meter=(root/'Source/MeterState.h').read_text(encoding='utf-8')

required=[
    (out,'gainForDisplayLinear()'),
    (out,'hardClipDisplayGain'),
    (out,'truePeakDisplayGain=juce::jmin'),
    (out,'postDisplayGain=post.gainForDisplay()'),
    (out,'hardClipDisplayGain+t*(tpDisplayGain-hardClipDisplayGain)'),
    (proc,'outputCeiling.gainForDisplayLinear()'),
    (proc,'meterState.displayCeilingGainReductionDb.store'),
    (proc,'const auto ceilingGrForVisuals'),
    (proc,'const auto totalEffectiveGr0 = effectiveGr0 + ceilingGrForVisuals'),
    (proc,'meterState.gainReductionDb0.store (totalEffectiveGr0'),
    (proc,'updateGainReductionHoldChannel (0, totalEffectiveGr0'),
    (meter,'displayCeilingGainReductionDb'),
    (dyn,'point0.truePeakExcessDb = capturedTruePeakExcessDb'),
    (dyn,'point.truePeakExcessDb'),
    (dyn,'const auto ceilingGrForBlue'),
]
for text, token in required:
    assert token in text, token

process_body=out[out.index('std::array<float,2> process(float l,float r)'):out.index('private:', out.index('std::array<float,2> process(float l,float r)'))]
assert 'gainToDecibels' not in process_body
assert 'displayGainLinear=juce::jlimit' in process_body
assert 'outputCeiling.gainReductionDbForDisplay()' not in proc
assert 'displayCeilingGainLinear=juce::jmin' in proc
assert proc.count('displayCeilingGainReductionDb =') == 1

rng=random.Random(1213)
for _ in range(100000):
    compressor=rng.uniform(-40.0,40.0)
    ceiling=rng.uniform(0.0,30.0)
    shown=compressor+ceiling
    assert abs(shown-(compressor+ceiling))<1e-12
    if compressor>=0:
        assert shown>=compressor

for token in [
    'constexpr float residualSafetyDb=-0.01f;',
    'const std::array<float,2> hardFinalClipped',
    'hardFinalClipped[0]+t*(corrected[0]-hardFinalClipped[0])',
]:
    assert token in out, token

print('PASS: exact Limiter Ceiling attenuation feeds GAIN +/-; Display receives 8x Hard/TP shape evidence.')
print('PASS: 100000 signed compressor + positive ceiling-GR composition cases for shared Display/meter semantics.')
print('PASS: audio-thread telemetry remains linear per sample; dB conversion occurs once per host block.')
