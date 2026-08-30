#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
h = (root / 'Source' / 'PluginEditor.h').read_text(encoding='utf-8')
cpp = (root / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')

for token in ('mixLR', 'mixMS', 'handleLinkedTextEntry'):
    assert token in h, token

for token in (
    'beginLinkedGesture (LinkedPair::mixLR',
    'beginLinkedGesture (LinkedPair::mixMS',
    'handleLinkedValueChange (LinkedPair::mixLR',
    'handleLinkedValueChange (LinkedPair::mixMS',
    'handleLinkedTextEntry (LinkedPair::ratioLR',
    'handleLinkedTextEntry (LinkedPair::thresholdLR',
    'handleLinkedTextEntry (LinkedPair::makeupLR',
    'handleLinkedTextEntry (LinkedPair::mixLR',
    'Ratio / Threshold / Makeup / Mix',
):
    assert token in cpp, token

# Direct text entry must use the same shared-delta range clamp, not a separate
# equality/snap implementation.
assert 'const auto requestedDelta = requestedSource - sourceStart;' in cpp
assert 'const auto appliedDelta = juce::jlimit (minDelta, maxDelta, requestedDelta);' in cpp
assert 'sourceOff != targetOff' in cpp

print('PASS: v1.0.2 UI source wires Mix into relative LINK and routes direct text entry through shared-delta linking.')
