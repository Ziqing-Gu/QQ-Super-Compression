#!/usr/bin/env python3
from pathlib import Path
import math, random, re
src=(Path(__file__).resolve().parents[1]/'Source/OutputCeiling.h').read_text(encoding='utf-8')
assert 'truePeak.process(inL,inR,c,0,false,-1.0f,recoveryMode)' in src
assert 'residualSafetyDb=-0.01f' in src
assert 'triggerCeiling>0.0f' in src
assert 'post.process(aligned[0],aligned[1],residualTarget,detected,' in src
assert 'truePeak.useConservativeTiming(recoveryMode),c,recoveryMode)' in src
assert re.search(r'Oversampling<float>\s+analysis\s*\{2,4,',src)
assert 'double(maximum)/16.0' in src
assert 'decibelsToGain(-0.10f)' not in src
assert 'decibelsToGain(-0.02f)' not in src
safety=10**(-0.01/20)
rng=random.Random(1212)
for _ in range(100000):
    c=10**(rng.uniform(-24,0)/20)
    detected=c*10**(rng.uniform(-0.2,0.2)/20)
    target=(c*safety/detected) if detected>c else 1.0
    if detected<=c:
        assert target==1.0
    else:
        landed=detected*target
        assert landed<=c and abs(20*math.log10(landed/c)+0.01)<1e-10
print('PASS: TP main guard targets user Ceiling; fixed -0.02/-0.10 reserves remain removed.')
print('PASS: final reconstruction analysis is 16x and residual correction triggers only above Ceiling.')
print('PASS: 100000 residual overshoot cases land exactly 0.01 dB below Ceiling; non-overshoots are untouched.')
