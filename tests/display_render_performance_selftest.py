#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
header = (root / "Source" / "DynamicDisplay.h").read_text(encoding="utf-8")
source = (root / "Source" / "DynamicDisplay.cpp").read_text(encoding="utf-8")

assert "VERSION 1.1.5" in cmake

# Keep the previous eight-second history while doubling temporal resolution.
assert "displayRefreshHz = 60" in header
assert "historyLength = 480" in header
assert "startTimerHz (displayRefreshHz)" in source
assert 480 / 60 == 240 / 30

# Projection storage and JUCE paths are retained members, not temporary paint
# allocations. Parameter moves still refresh the full history every timer tick.
for token in (
    "std::array<float, historyLength>",
    "std::array<RenderCache, 2> renderCaches",
    "preallocateSpace",
    "refreshRenderCaches (mode)",
    "cache.currentGainReductionDb",
):
    assert token in header or token in source, token

draw_start = source.index("void DynamicDisplay::drawDomainPanel")
paint_start = source.index("void DynamicDisplay::paint", draw_start)
draw_body = source[draw_start:paint_start]
assert "projectHistory (" not in draw_body
assert "makePath (" not in draw_body

# The expensive full-area translucent polygon is gone. One sparse cached path
# carries the GR shading, so render work no longer jumps with the filled area.
assert "makeBandPath" not in header
assert "makeBandPath" not in source
assert "fillPath (makeBandPath" not in source
assert "gainReductionShadeSegments = 160" in header
assert "cache.gainReductionShadePath" in draw_body

# An opaque child prevents its 60 Hz repaint from invalidating the parent UI.
assert "setOpaque (true)" in source
assert "g.fillAll (qqsc::ui::canvas())" in source

# Moving from 30 to 60 Hz must not shorten HPF automation/retry debounce time.
assert "hpfAutomationStableTicks = 4" in header
assert "hpfRetryDelayTicks = 4" in header

print("PASS: v1.1.5 cached 60 Hz Display rendering and bounded GR shading.")
