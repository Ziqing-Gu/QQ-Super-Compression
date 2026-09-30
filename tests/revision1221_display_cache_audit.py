#!/usr/bin/env python3
from pathlib import Path
import hashlib, math, random, re, sys

root = Path(__file__).resolve().parents[1]
h = (root/'Source/DynamicDisplay.h').read_text(encoding='utf-8')
c = (root/'Source/DynamicDisplay.cpp').read_text(encoding='utf-8')
ph = (root/'Source/PluginProcessor.h').read_text(encoding='utf-8')
pc = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
test = (root/'tests/retrospective_tp_display_checks.inc').read_text(encoding='utf-8')

required = [
    ('projection revision atomic', 'displayProjectionRevision { 1 }' in ph),
    ('projection revision getter', 'getDisplayProjectionRevision()' in ph),
    ('revision bumped in parameter callback', 'displayProjectionRevision.fetch_add' in pc),
    ('stopped transport duplicate guard', 'position.counter != lastCapturedHistoryCounter' in c),
    ('hidden display render bypass', 'if (! isShowing())\n        return;' in c),
    ('history/projection dirty gate', 'projectionDirty || historyDirty || geometryDirty' in c),
    ('dense transfer LUT', 'dynamicsLutSize = 4097' in h),
    ('LUT rebuilt only when dynamics signature changes', 'dynamicsLutSignature != dynamicsSignature' in c),
    ('history projection uses LUT', 'dynamicsGainFromLut (cache, detectorDb)' in c),
    ('valid Limiter ratio regression 200', 'set(p,"limiterRatio",200.0f)' in test),
    ('valid Limiter ratio regression 800', 'set(p,"limiterRatio",800.0f)' in test),
    ('Ceiling revision invalidation regression', 'Ceiling did not invalidate Display projection revision' in test),
    ('Ceiling blue invariance regression', 'Ceiling incorrectly changed blue TP GR' in test),
    ('Ceiling output shift regression', 'Ceiling did not retrospectively shift projected final output' in test),
]
for name, ok in required:
    if not ok:
        raise SystemExit(f'FAIL: {name}')
print('PASS: projection revision/dirty gating, stopped-transport guard, hidden-render bypass and 4097-point transfer LUT hooks are present.')

# Verify the display LUT is visually transparent for the two single-band static
# laws over representative finite-threshold settings. This does not replace the
# real JUCE revision1221 test; it bounds interpolation error only.
N = 4097
DB0, DB1 = -120.0, 0.0
grid = [DB0 + (DB1-DB0)*i/(N-1) for i in range(N)]

def gain_classic(db, th, ratio):
    level = 10**(db/20)
    T = 10**(max(-90.0, th)/20)
    if level <= T: return 1.0
    return (T/level)**(1.0-1.0/ratio)

def gain_super(db, th, ratio):
    level = 10**(db/20)
    T = 0.0 if th <= -120 else 10**(th/20)
    if T <= 0: return 1.0/(1.0+(ratio-1.0)*level)
    if level <= T: return 1.0
    return (1.0+(ratio-1.0)*T)/(1.0+(ratio-1.0)*level)

def db_gain(g):
    return 20*math.log10(max(g, 1e-18))

rng = random.Random(1221)
worst = 0.0
for _ in range(240):
    th = rng.uniform(-90.0, -0.05)
    ratio = 10**rng.uniform(math.log10(1.01), math.log10(1000.0))
    fn = gain_classic if rng.randrange(2) == 0 else gain_super
    lut = [fn(db, th, ratio) for db in grid]
    for _ in range(220):
        db = rng.uniform(DB0, DB1)
        pos = (db-DB0)/(DB1-DB0)*(N-1)
        lo = int(pos); hi = min(N-1, lo+1); f = pos-lo
        approx = lut[lo] + f*(lut[hi]-lut[lo])
        exact = fn(db, th, ratio)
        worst = max(worst, abs(db_gain(approx)-db_gain(exact)))
if worst > 0.01:
    raise SystemExit(f'FAIL: LUT interpolation worst error {worst:.6f} dB > 0.01 dB')
print(f'PASS: 4097-point transfer LUT stays within {worst:.6f} dB in randomized Classic/Super finite-threshold probes.')

engine = root/'Source/StaticCompressionEngine.h'
sha = hashlib.sha256(engine.read_bytes()).hexdigest().upper()
expected = '51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA'
if sha != expected:
    raise SystemExit(f'FAIL: StaticCompressionEngine changed: {sha}')
print('PASS: StaticCompressionEngine remains byte-identical to the established baseline.')
print('LIMITATION: Windows/JUCE revision1221 remains the integration/performance acceptance gate.')
