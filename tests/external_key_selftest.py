from pathlib import Path
import math

root = Path(__file__).resolve().parents[1]
processor = (root / "Source" / "PluginProcessor.cpp").read_text(encoding="utf-8")
header = (root / "Source" / "PluginProcessor.h").read_text(encoding="utf-8")
params = (root / "Source" / "Parameters.h").read_text(encoding="utf-8")
editor = (root / "Source" / "PluginEditor.cpp").read_text(encoding="utf-8")
editor_header = (root / "Source" / "PluginEditor.h").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")

assert "VERSION 1.1.2" in cmake
assert '.withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)' in processor
assert 'keySource      = "keySource"' in params
assert 'keyGainDb      = "keyGainDb"' in params
assert processor.index('addMix (qqsc::params::mixS') < processor.index('qqsc::params::keySource, 1')
assert 'qqsc::params::keyInternal' in processor and 'qqsc::params::keyExternal' in processor

# INT must continue to feed the exact post-Input-Gain carrier samples to the
# established detector; EXT may replace only those detector samples.
assert 'const float keyL = oversampledBlock.getSample (2, i);' in processor
assert 'const float keyR = oversampledBlock.getSample (3, i);' in processor
assert 'leftEngine.processSample  (keyL' in processor
assert 'rightEngine.processSample (keyR' in processor
assert 'oversampledLookaheadDelayBuffer.setSample (0, oversampledDelayWriteIndex, inputL);' in processor
assert 'const float wetLinkedL = dryLInternal * linkedGain;' in processor

# A disabled external bus is a zero key (unity/no GR), never a silent carrier.
assert 'const float externalL = externalKeyBusAvailable ? externalKeyBuffer.getSample (0, i) : 0.0f;' in processor
assert 'const float outL = forceBypass ? displayDryL : activeOutL;' in processor

# Mono external key intentionally drives all independent domains in common.
assert ': (useExternalKey ? keyL : 0.0f);' in processor

# Source and gain are sound parameters (state/A-B/automation); Listen remains a
# non-persistent audition override and cannot defeat true bypass.
for token in ['prefix + "keySource"', 'prefix + "keyGainDb"', 'stateHasKeySource', 'stateHasKeyGain']:
    assert token in processor
assert 'if (! forceBypass && sidechainListen.load' in processor
assert 'sidechainListen.store (false' in processor
assert 'state.setProperty ("sidechainListen"' not in processor
assert 'SC: EXT' in editor and 'SC LISTEN' in editor_header

# Reference future-window mapping: an external key peak must reduce the aligned
# carrier before the key event, while silence returns exact unity.
def mapped(carrier, key, lookahead, ratio):
    out = []
    for n, x in enumerate(carrier):
        level = max(abs(v) for v in key[n:min(len(key), n + lookahead + 1)])
        gain = 1.0 / (1.0 + (ratio - 1.0) * min(1.0, level))
        out.append(x * gain)
    return out

carrier = [0.5] * 8
silent = mapped(carrier, [0.0] * 8, 2, 8.0)
keyed = mapped(carrier, [0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0], 2, 8.0)
assert silent == carrier
assert math.isclose(keyed[0], 0.5 / 8.0) and math.isclose(keyed[1], 0.5 / 8.0)
assert math.isclose(keyed[2], 0.5 / 8.0) and keyed[3:] == carrier[3:]

print("PASS: v1.1.2 External Key bus/source/gain/A-B/state/listen wiring and future-window mapping.")