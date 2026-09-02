#!/usr/bin/env python3
import math
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
engine = (root / "Source" / "StaticCompressionEngine.h").read_text(encoding="utf-8")
meter = (root / "Source" / "MeterState.h").read_text(encoding="utf-8")
processor = (root / "Source" / "PluginProcessor.cpp").read_text(encoding="utf-8")
display_h = (root / "Source" / "DynamicDisplay.h").read_text(encoding="utf-8")
display = (root / "Source" / "DynamicDisplay.cpp").read_text(encoding="utf-8")

assert "VERSION 1.1.5" in cmake

# Product-facing GR is Dry/Wet compression depth in the linear gain domain.
def effective_gr(core_gr_db, wet_mix):
    compressed_gain = 10.0 ** (-max(0.0, core_gr_db) / 20.0)
    effective_gain = 1.0 + (compressed_gain - 1.0) * min(1.0, max(0.0, wet_mix))
    return max(0.0, -20.0 * math.log10(max(effective_gain, 1.0e-9)))

assert math.isclose(effective_gr(12.0, 0.0), 0.0, abs_tol=1.0e-12)
assert math.isclose(effective_gr(12.0, 1.0), 12.0, abs_tol=1.0e-9)
assert math.isclose(effective_gr(12.0, 0.5), 4.074144, abs_tol=1.0e-5)
assert not math.isclose(effective_gr(12.0, 0.5), 6.0)

for token in (
    "effectiveGainForMix",
    "effectiveGainReductionDb",
    "1.0f + (compressedGain - 1.0f) * wetMix",
):
    assert token in engine, token

# Both the live GR meter and the historical Display use the same Mix-aware law.
assert "effectiveGrForMeter" in processor
assert "effectiveGainReductionDb (compressedGain, wetMix)" in processor
assert "gainReductionDb0.store (effectiveGr0" in processor
assert "updateGainReductionHoldChannel (0, effectiveGr0" in processor
assert "GR incl. Mix" in display
assert "GR (MIX)" in display

# The visible history stores raw carrier plus detector evidence and reprojects
# all prior points with current parameters. Wet is no longer a visible trace.
for token in (
    "capturedInputGainDb",
    "displayDetectorDb0",
    "projectHistory",
    "ratioForDomain",
    "mixForDomain",
    "gainReductionBoundary",
):
    assert token in display_h or token in display or token in meter, token
assert "Wet pre-makeup" not in display
assert "WET PRE-MAKEUP" not in display
assert "displayWetDb" not in meter
assert "displayWetDb" not in processor

# EXT shows a deliberately weak two-stroke detector ghost. Key Gain now moves
# complete live/replayed history in real time; Input remains independent of EXT.
assert "externalKey && externalAvailable" in display
assert "External key" in display
assert "withAlpha (0.10f)" in display and "withAlpha (0.34f)" in display
assert "capturedKeyGainDb" in display_h
assert "detectorDb += keyGainDb - point.capturedKeyGainDb;" in display
assert "detectorDb += externalKey ? keyGainDb : inputGainDb;" in display
assert "displayDetectorPeak[0]" in processor
assert "midEngine.getCurrentLevel()" in processor
assert "linkedLevel" in processor

# Makeup and Output affect only the projected Output curve; effective GR is
# computed before either is applied.
gr_pos = display.index("const auto effectiveGr =")
makeup_mix_pos = display.index("const auto mixedGain =")
assert gr_pos < makeup_mix_pos

print("PASS: v1.1.5 Mix-aware GR, real-time Key Gain history, Wet removal and EXT Key ghost display.")
