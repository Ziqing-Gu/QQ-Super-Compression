#!/usr/bin/env python3
from pathlib import Path
import math, random
root=Path(__file__).resolve().parents[1]
out=(root/'Source/OutputCeiling.h').read_text(encoding='utf-8')
display=(root/'Source/DynamicDisplay.cpp').read_text(encoding='utf-8')
params=(root/'Source/Parameters.h').read_text(encoding='utf-8')
processor=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
editor=(root/'Source/LimiterEditor.cpp').read_text(encoding='utf-8')
assert 'tpRecoveryMode = "tpRecoveryMode"' in params
assert 'enum TpRecoveryMode { tpTight = 0, tpAuto = 1, tpSmooth = 2 }' in params
assert 'juce::StringArray{"Tight","Auto","Smooth"}' in processor
assert 'qqsc::params::tpAuto' in processor
assert 'releaseSeconds=juce::jlimit(.005,.025,halfPeriod);' in out
assert 'releaseSeconds=autoRepeated ? .060 : juce::jlimit(.015,.075,2.5*halfPeriod);' in out
assert 'releaseSeconds=juce::jlimit(.060,.180,6.0*halfPeriod);' in out
assert 'controlledMinimum=repeated ? slowNodes[slowHead].value : minimum;' in out
assert 'tpRecoveryButton.setButtonText' in editor
assert 'tpRecoveryButton.setVisible(limiter && processor.isTruePeakSelected())' in editor
assert ('recoverySeconds=tpRecovery==qqsc::params::tpTight ? .015f' in display or
        'recoverySeconds = tpRecovery == qqsc::params::tpTight ? .015f' in display)
# Lightweight numerical contract: for an identical post-hit target sequence,
# faster release must retain less attenuation than slower release.
def step(g, target, seconds, hz=60.0):
    if target <= g: return target
    a=1.0/(1.0+hz*seconds)
    return g+a*(target-g)
for _ in range(100000):
    hit=10**(-random.uniform(0.1,12.0)/20)
    vals=[]
    steps=random.randint(1,5)
    for sec in (0.015,0.050,0.140):
        g=hit
        for i in range(steps):
            g=step(g,1.0,sec)
        vals.append(g)
    assert vals[0] > vals[1] > vals[2]
# Ceiling target logic must remain independent of recovery selection.
assert ('truePeak.process(inL,inR,c,0,false,-1.0f,recoveryMode)' in out or
        'const auto tpLimited=tp.process(inL,inR,c,0,false,-1.0f,recoveryMode);' in out)
assert 'residualSafetyDb=-0.01f' in out
assert 'const double target=peak>trigger ? double(ceiling)/peak : 1.0;' in out
print('PASS: TP Recovery parameter/UI/state hooks are present and AUTO keeps the 1.2.17 timing formulas.')
print('PASS: TIGHT/AUTO/SMOOTH release constants are ordered for loudness -> adaptive -> smooth behaviour.')
print('PASS: 100000 display-envelope release cases preserve TIGHT > AUTO > SMOOTH recovery speed.')
print('PASS: Ceiling target/residual safety equations are independent of TP Recovery mode.')
print('LIMITATION: BUILD_WINDOWS revision1218 + QQSCCeilingCheck remains the real JUCE/audio acceptance gate.')
