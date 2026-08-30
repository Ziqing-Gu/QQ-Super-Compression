from pathlib import Path

root = Path(__file__).resolve().parents[1]
s = (root / "Source" / "DynamicDisplay.cpp").read_text(encoding="utf-8")
assert "constexpr float minDb = -90.0f;" in s
assert "constexpr float maxDb = 0.0f;" in s
assert "{ 0.0f, -15.0f, -30.0f, -45.0f, -60.0f, -75.0f, -90.0f }" in s
assert "thresholdOffDb = -120.0f" in (root / "Source" / "Parameters.h").read_text(encoding="utf-8")
print("display_scale_selftest: PASS")
