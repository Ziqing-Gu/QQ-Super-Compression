#!/usr/bin/env python3
from pathlib import Path
import hashlib
import re

root = Path(__file__).resolve().parents[1]
h = (root / "Source" / "DynamicDisplay.h").read_text(encoding="utf-8")
c = (root / "Source" / "DynamicDisplay.cpp").read_text(encoding="utf-8")
ph = (root / "Source" / "PluginProcessor.h").read_text(encoding="utf-8")
pc = (root / "Source" / "PluginProcessor.cpp").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")

checks = [
    ("1.2.27+ project version", any(f"VERSION 1.2.{v}" in cmake for v in range(27, 100))),
    ("60 Hz retained", "displayRefreshHz = 60" in h),
    ("480-point cap retained", "historyLength = 480" in h),
    ("8-second window derived from existing spec", "historyWindowSeconds" in h),
    ("audio counter retained in projected history", "captureCounter {}" in h),
    ("audio-time x helper", "historyXForCounter" in h and "ageSamples" in c and "renderReferenceCounter" in c),
    ("timer captures current audio counter", "renderReferenceCounter = position.counter" in c),
    ("timer uses key-history analysis rate", "renderReferenceSampleRate = position.sampleRate" in c),
    ("position exposes analysis sample rate", "double sampleRate = 44100.0" in ph and "storage->analysisSampleRate" in pc),
    ("real-time window trimming", "trimHistoryToVisibleWindow" in h and "maxAgeSamples" in c),
    ("dense 4097 LUT retained", "dynamicsLutSize = 4097" in h),
    ("detector grid precomputed once", "detectorLevelLut" in h and "dB->linear" in c),
    ("bulk dynamics projection helper declared", "fillDynamicsGainForDomain" in ph),
    ("bulk dynamics projection helper implemented", "void QQSuperCompressionAudioProcessor::fillDynamicsGainForDomain" in pc),
    ("Display LUT uses bulk helper", "processor.fillDynamicsGainForDomain" in c),
    ("old 4097 scalar parameter-read loop removed", "processor.getDynamicsGainForDomain (\n            dbToDetectorLevel" not in c),
    ("projection revision dirty gate retained", "projectionDirty || historyDirty || geometryDirty" in c),
]
for name, ok in checks:
    if not ok:
        raise SystemExit(f"FAIL: {name}")

engine = root / "Source" / "StaticCompressionEngine.h"
sha = hashlib.sha256(engine.read_bytes()).hexdigest().upper()
expected = "51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA"
if sha != expected:
    raise SystemExit(f"FAIL: StaticCompressionEngine changed: {sha}")

# Guard against accidentally reducing the Display timing/precision spec.
m_hz = re.search(r"displayRefreshHz\s*=\s*(\d+)", h)
m_len = re.search(r"historyLength\s*=\s*(\d+)", h)
m_lut = re.search(r"dynamicsLutSize\s*=\s*(\d+)", h)
if not (m_hz and m_len and m_lut):
    raise SystemExit("FAIL: Display constants missing")
if (int(m_hz.group(1)), int(m_len.group(1)), int(m_lut.group(1))) != (60, 480, 4097):
    raise SystemExit("FAIL: Display timing/history/LUT precision changed")

print("PASS: 1.2.27 audio-time timeline, 8-second trimming and bulk 4097-point LUT hooks are present.")
print("PASS: Display remains 60 Hz / 480 points / 4097 LUT entries.")
print("PASS: StaticCompressionEngine remains byte-identical to the established baseline.")
