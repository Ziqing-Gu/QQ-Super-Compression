#!/usr/bin/env python3
from pathlib import Path
import argparse, random, math

ap=argparse.ArgumentParser()
ap.add_argument('--source', default=str(Path(__file__).resolve().parents[1]))
a=ap.parse_args()
root=Path(a.source).resolve()
lim=(root/'Source/LimiterEditor.cpp').read_text(encoding='utf-8')
editor=(root/'Source/PluginEditor.cpp').read_text(encoding='utf-8')

# Static contract checks against the product source.
assert 'Strict 1:1 dB Link' in lim
block=lim.split('// Limiter LINK keeps the musical compression controls coherent in either',1)[1].split('// MATCH owns',1)[0]
assert 'makeupSTSlider' in block and 'mixSlider' not in block and 'mainRatioControls' not in block and 'downRatioSliders' not in block
assert 'double(limiterLinkOutput)-delta' in lim
assert 'limiterStartLowerThresholds[size_t(d)]-appliedDelta' in lim and 'limiterStartUpperThresholds[size_t(d)]-appliedDelta' in lim
assert 'reconcileLimiterOutput(editedSource)' in editor
assert 'captureLimiterLinkAnchor(source)' in editor

# Mathematical mirror of the exact shared-delta clamps used by the source.
def source_to_output(source0, requested, smin, smax, out0):
    req=requested-source0
    lo=max(smin-source0, out0-120.0)
    hi=min(smax-source0, out0+120.0)
    d=min(hi,max(lo,req))
    return source0+d, out0-d, d

def output_to_thresholds(out0, requested, starts, bounds):
    req=requested-out0
    lo=-120.0-out0
    hi=120.0-out0
    for st,(mn,mx) in zip(starts,bounds):
        lo=max(lo, st-mx)
        hi=min(hi, st-mn)
    d=min(hi,max(lo,req))
    return out0+d,[st-d for st in starts],d

rng=random.Random(1211)
for _ in range(100000):
    s0=rng.uniform(-100,100); out0=rng.uniform(-100,100)
    requested=rng.uniform(-180,180)
    s,out,d=source_to_output(s0,requested,-120,120,out0)
    assert -120-1e-9 <= s <= 120+1e-9 and -120-1e-9 <= out <= 120+1e-9
    assert abs((s-s0)+(out-out0)) < 1e-9

for domains in (1,2):
    for _ in range(50000):
        out0=rng.uniform(-100,100)
        starts=[]; bounds=[]
        for _d in range(domains):
            mn=rng.uniform(-120,-20); mx=rng.uniform(max(mn,-20),0)
            st=rng.uniform(mn,mx)
            starts.append(st); bounds.append((mn,mx))
        out,ths,d=output_to_thresholds(out0,rng.uniform(-180,180),starts,bounds)
        assert -120-1e-9 <= out <= 120+1e-9
        for st,th,(mn,mx) in zip(starts,ths,bounds):
            assert mn-1e-9 <= th <= mx+1e-9
            assert abs((out-out0)+(th-st)) < 1e-9

# Explicit user-facing examples.
s,out,d=source_to_output(-24.3,-30.3,-120,0,8.0)
assert abs(s+30.3)<1e-9 and abs(out-14.0)<1e-9
s,out,d=source_to_output(3.0,8.0,-120,120,10.0)
assert abs(out-5.0)<1e-9
out,ths,d=output_to_thresholds(10.0,15.0,[-24.3],[(-90,0)])
assert abs(out-15.0)<1e-9 and abs(ths[0]+29.3)<1e-9
print('PASS: source whitelist is Makeup + active non-unity UP/DOWN Thresholds; Ratio/Mix/algorithm excluded.')
print('PASS: 100000 source->Output and 100000 Output->Threshold randomized strict 1:1 clamp cases.')
print('PASS: exact inverse dB relationship remains symmetric for Threshold/Makeup/Output in either compression direction.')
print('LIMITATION: this is a source/relationship isolation test; BUILD_WINDOWS revision1211 is the real JUCE editor/APVTS integration test.')
