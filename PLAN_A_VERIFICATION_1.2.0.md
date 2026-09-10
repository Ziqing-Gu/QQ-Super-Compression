# Current checkpoint: 1.2.0 Rev4 Stable — 2026-09-10

Rev4 supersedes the historical checkpoint below. See [STABLE_1.2.0.md](STABLE_1.2.0.md) and [VERIFICATION_REV4_1.2.0.md](VERIFICATION_REV4_1.2.0.md). The user authorized B, then C and D with the completed 23-page PDFs retained unchanged. Current production schema is 14.

---

# Plan A — 1.2.0 Rev3 inherited verification

2026-09-10 Stable promotion reuses the completed Rev3 Windows x64 Release build;
no DSP, UI, parameter, asset or CMake change is part of this promotion.
Toolchain: Visual Studio 2022 x64, CMake, JUCE 8.0.15. Plug-in 1.2.0, schema 13.

The compiled processor and production editor tests passed: five-domain UP gate
and DOWN handoff, fifteen unity defaults, Ratio LINK relative-product coupling,
range limits, real gestures, settings/state precedence, three-theme geometry and
Ratio illumination. Ten inherited source/math checks also passed.
599.9 Hz regression at 44.1/48/96 kHz and 10/26/80 ms: zero unity bursts;
maximum sample error versus the retained legacy engine 5.96046e-8.
See `VERIFICATION_1.2.0.md` for measured steady-tone and display results/limits.

Output: `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`.
Raw test logs: output `Verification/dynamics-check.txt`, `visual-check.txt`,
`inherited-checks.txt`, `display-performance-1.2.0.txt`.
Original verified installation: 2026-09-10T07:28:52+08:00, after host-closure checks.
Plan B rechecks the saved evidence and current build/output/installed identity;
its fresh verification result is `Verification/PlanB/PLAN_B_COMPLETION.json`.

| File | Bytes | SHA256 |
|---|---:|---|
| VST3 binary |10426880|9AC4E801390563AE082B91CE6CE475F8D5F2F2BD44BA6EDA6DB7CB8687218642|
| moduleinfo.json |1115|D2F5F28CDBD827225930681ED0F12161C390960D06C91A1FD9693B6B18FDE9FB|
| Windows VST3 ZIP |6519415|55485CD3A7FC7C581EE8F9DEEE5FDEA43987CA8819954CF272A0B372D814940D|

No new host listening or third-party full VST3 validator run is claimed for the
status-only promotion. User acceptance of Rev3 is recorded separately in the
Stable checkpoint. Existing test limitations are retained in the detailed report.
