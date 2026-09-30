from pathlib import Path
import hashlib, math, random, re

root=Path(__file__).resolve().parents[1]
limits=(root/'Source/DynamicsLimits.h').read_text()
params=(root/'Source/Parameters.h').read_text()
proc=(root/'Source/PluginProcessor.cpp').read_text()
editor=(root/'Source/LimiterEditor.cpp').read_text()
main_editor=(root/'Source/PluginEditor.cpp').read_text()
header=(root/'Source/PluginEditor.h').read_text()
cmake=(root/'CMakeLists.txt').read_text()
engine=(root/'Source/StaticCompressionEngine.h').read_bytes()

assert 'VERSION 1.2.25' in cmake
assert 'limiterMinimumUpRatio = 1.0f / 8.0f' in limits
assert 'limiterMinimumDownRatio = 1.0f' in limits
assert 'maximumDownRatio = 1000.0f' in limits
assert 'const float initial=(group<=2) ? 1.0f' in proc
assert 'Downward Ratio: 1:1 to 1000:1' in editor
assert 'Alt-click resets to 1:1.' in editor
assert 'main[d]->setResetValue(1.0);' in editor
assert 'active UP or DOWN Threshold and Makeup' in editor
assert 'limiterStartLowerThresholds' in header and 'limiterStartUpperThresholds' in header
assert 'moveLower' in editor and 'moveUpper' in editor
assert 'dualUpRatio<1.0f-1.0e-6f' in main_editor
assert 'dualDownRatio>1.0f+1.0e-6f' in main_editor
assert 'std::abs(singleRatio-1.0f)>1.0e-6f' in main_editor
assert hashlib.sha256(engine).hexdigest().upper() == '51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA'

# Reproduce the 1.2.25 SINGLE range mapping: unity is exact midpoint and
# every downward ratio 1..1000 is valid (no old 1..200 dead zone).
lo=1/8
hi=1000.0
def from_n(n):
    return lo*((1/lo)**(2*n)) if n<=.5 else hi**(2*n-1)
def to_n(v):
    v=max(lo,min(hi,v))
    return .5*math.log(max(lo,v)/lo)/math.log(1/lo) if v<=1 else .5+.5*math.log(v)/math.log(hi)
assert abs(from_n(.5)-1)<1e-12
assert abs(to_n(1)-.5)<1e-12
last=0
worst=0
for i in range(100001):
    n=i/100000
    v=from_n(n)
    assert v+1e-12>=last
    last=v
    worst=max(worst,abs(from_n(to_n(v))-v))
for _ in range(100000):
    v=math.exp(random.uniform(math.log(lo),math.log(hi)))
    worst=max(worst,abs(from_n(to_n(v))-v))
assert worst < 1e-8
print(f'PASS: 1.2.25 Limiter ratio unity mapping and bidirectional link source audit; range round-trip worst={worst:.3g}.')
