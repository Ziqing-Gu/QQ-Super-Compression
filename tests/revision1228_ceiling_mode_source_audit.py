from pathlib import Path
root=Path(__file__).resolve().parents[1]
proc=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
head=(root/'Source/PluginProcessor.h').read_text(encoding='utf-8')
ceil=(root/'Source/OutputCeiling.h').read_text(encoding='utf-8')
dyn=(root/'Source/DynamicDisplay.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')

assert any(f'VERSION 1.2.{v}' in cmake for v in range(28,100))
assert 'bool currentLimiterCeilingActive = false;' in head
assert 'if (currentLimiterCeilingActive)' in proc
assert 'finalOutput=outputCeiling.process' in proc

# TP changes at a fixed quality remain target-only. No filter prepare/allocation
# belongs in the realtime set() path.
start=ceil.index('void set(bool tp,float ceilingDb')
end=ceil.index('void set(bool /*enabled*/',start)
setter=ceil[start:end]
for forbidden in ('initProcessing','prepare(','reset()','assign(','resize(','setSize('):
    assert forbidden not in setter, f'TP setter rebuilds pipeline via {forbidden}'
assert 'truePeakBlend.setTargetValue' in setter

# 1.2.28 8x Hard/TP semantics remain available as choice index 1, while later
# versions may additionally expose 1x/16x and matched Core/Ceiling sharing.
assert 'oversampling8' in ceil
assert 'const std::array<float,2> hardClipped' in ceil
assert 'juce::jlimit(-c,c,inL)' in ceil and 'juce::jlimit(-c,c,inR)' in ceil
assert 'hardOversampledAlignment' in ceil and 'hardPostAlignment' in ceil
assert 'hardFinalClipped' in ceil
assert 'inputSamplePeakForDisplayLinear' in ceil and 'inputTruePeakForDisplayLinear' in ceil

# Display still follows Ceiling attenuation and the 1.2.27 timing hooks remain.
assert '+ juce::jmax (0.0f, point.truePeakExcessDb);' in dyn
assert 'const auto ceilingGrForBlue = (limiter && ! bypassed) ? projectedCeilingGrDb : 0.0f;' in dyn
assert 'historyXForCounter' in dyn
assert 'trimHistoryToVisibleWindow' in dyn
assert 'fillDynamicsGainForDomain' in dyn
print('PASS: 1.2.28 Limiter-only Ceiling Hard/TP invariants and synchronized Display hooks remain present.')
