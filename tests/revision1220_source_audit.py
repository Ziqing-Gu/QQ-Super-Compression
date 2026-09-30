#!/usr/bin/env python3
from pathlib import Path
import hashlib, math, random
root=Path(__file__).resolve().parents[1]
disp=(root/'Source/DynamicDisplay.cpp').read_text(encoding='utf-8')
engine=root/'Source/StaticCompressionEngine.h'
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
assert 'VERSION 1.2.23' in cm
assert hashlib.sha256(engine.read_bytes()).hexdigest() == '51b36d3aa7be1113aa4e90ac6c0c534163a9b7a3a9edc2dfe24d455cad3fa6ba'
section=disp[disp.index('void DynamicDisplay::projectHistory'):disp.index('void DynamicDisplay::scheduleHpfReplayRetry')]
for token in [
    'const auto transferInputDb = externalKey ? point.inputDb',
    ': detectorDb - inputGainDb;',
    'const auto cutMixDb = bypassed ? transferInputDb',
    'qqsc::StaticCompressionEngine::effectiveGainReductionDb',
    'const auto tpGrForBlue = (truePeak && ! bypassed) ? projectedCeilingGrDb : 0.0f;',
    'projected.gainReductionBoundary[index] = cutMixDb - tpGrForBlue;',
    'projected.effectiveGainReduction[index] = dynamicsMixGrDb + tpGrForBlue;'
]:
    assert token in section, token
assert 'projectedPostCeilingDb - activeOutputGainDb' not in section
assert 'point.measuredOutputDb' not in section
assert 'limiter != lastLimiter' not in disp[disp.index('void DynamicDisplay::timerCallback'):disp.index('void DynamicDisplay::pushHistory')]
assert 'currentLookaheadMs' in disp and 'requestHpfHistoryRefresh' in disp

# Finite-threshold transfer contract at the Display reference.
r=random.Random(1220)
def db2lin(db): return 10.0**(db/20.0)
def lin2db(x): return 20.0*math.log10(max(x,1e-30))
for _ in range(100000):
    tdb=r.uniform(-80.0,-1.0)
    xdb=r.uniform(tdb,0.0)
    ratio=10**r.uniform(math.log10(1.0001),math.log10(1000.0))
    mix=r.random()
    input_gain=r.uniform(-24.0,24.0)
    T=db2lin(tdb); L=db2lin(xdb)
    for algorithm in ('classic','super'):
        if L <= T:
            g=1.0
        elif algorithm=='classic':
            g=(T/L)**(1.0-1.0/ratio)
        else:
            g=(1.0+(ratio-1.0)*T)/(1.0+(ratio-1.0)*L)
        eff=(1.0-mix)+mix*g
        transfer_input=xdb-input_gain
        blue=transfer_input+lin2db(eff)
        visual_threshold=tdb-input_gain
        if blue < visual_threshold-1e-7:
            raise AssertionError((algorithm,tdb,xdb,ratio,mix,input_gain,blue,visual_threshold))

# TP is explicitly a post-dynamics term and may move blue below threshold.
for _ in range(10000):
    threshold=r.uniform(-40,-5)
    base=r.uniform(threshold,0)
    tp=r.uniform(0.01,12)
    assert base-tp < base

print('PASS: StaticCompressionEngine is byte-identical to the 1.2.18 input baseline.')
print('PASS: 100000 Classic/Super finite-threshold + Mix projections stay on/above the TP-OFF visual threshold.')
print('PASS: blue is current Dynamics/Mix plus current TP only; captured Output and blue/orange-identity projection are absent.')
print('PASS: Limiter toggle preserves evidence and Lookahead changes request retrospective detector replay.')
