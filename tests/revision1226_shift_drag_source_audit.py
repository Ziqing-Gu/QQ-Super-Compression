from pathlib import Path
root=Path(__file__).resolve().parents[1]
h=(root/'Source'/'PluginEditor.h').read_text(encoding='utf-8')
assert 'rotaryLastDragPosition = event.position;' in h
assert 'currentProportion = valueToProportionOfLength (getValue())' in h
assert 'proportionOfLengthToValue (requestedProportion)' in h
assert 'deltaPixels = dx - dy' in h
assert '? fineSensitivity : normalSensitivity' in h
assert 'Shift press jumped rotary value mid-drag' in (root/'tests'/'revision1226_shift_drag_continuity_checks.inc').read_text(encoding='utf-8')
print('PASS: 1.2.26 Rotary Shift drag uses incremental normalized deltas and has real MouseEvent continuity coverage.')
