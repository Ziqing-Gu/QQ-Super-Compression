# QQ Super Compression 1.1.2 - Plan A Verification

**Date:** 2026-09-02
**Status:** Plan A complete; Windows x64 VST3 generated and installed for user verification
**Classification:** Stable; v1.1.1 Side Chain HPF is the previous Stable rollback baseline

## Implemented scope

- Redefined product-facing Gain Reduction to include Mix in the linear gain domain:
  - Mix 0% -> 0 dB effective GR.
  - Mix 100% -> full core GR.
  - Intermediate values use effectiveGain = 1 + (compressedGain - 1) * Mix; GR dB is not multiplied directly by Mix.
- Applied the same effective-GR definition to the right-side GR meter, two-second Hold, and Dynamic Display.
- Removed the visible Wet pre-Makeup history/readout. Wet remains internal for Match and the actual Dry/Wet signal blend.
- Rebuilt Dynamic Display history around pre-Input carrier plus actual future-window detector levels. Current Input, Ratio, Threshold, Mix, Makeup, and Output Gain reproject the complete 240-point visible history.
- Display now draws Dry/Input, a translucent effective-GR band and boundary, and projected Output post-Mix.
- EXT available draws a deliberately weak two-stroke post-Key-Gain/post-HPF future-window Key contour; unavailable EXT shows N/A and draws no false signal.
- Preserved ST/LR/MS, LIGHT/CLASSIC geometry/function parity, sidechain/HPF, future-window DSP, Match, PDC, Monitor, A/B, state, and Bypass contracts.
- No parameter or state migration change; schema remains 10.

## Build and tests

- Generator: Visual Studio 17 2022, x64 Release.
- Compiler: MSVC 19.44.35227.0.
- JUCE: 8.0.15.
- Target: QQSuperCompression_VST3.
- Build: PASS, no compiler warnings.
- Ten Python/source/math self-tests: PASS.
- Standalone BS.1770: PASS (-3.0036 LUFS; 6.00 dB Match).
- Steinberg validator/pluginval: not run because no executable is available in the checked local environment.

## Artifact identity and parity

- Binary FileVersion/ProductVersion: 1.1.2.
- VST3 module metadata: 1.1.2.
- Build/output/install: 2 files, 6,724,699 bytes each.
- Tree SHA-256: 2D6CE2509C70C9D697756F4AD264BCB6EC284619DB2B341A5A670AE11BDF061C.
- Main binary SHA-256: 240A9DBEBDE3D88B14A59096218525204CD43B020B486E390A1042330EAA1DE3.
- moduleinfo.json SHA-256: 80A93C57A131F00D775A321C25B1E0B9CF95ACDACEC28407E16C8A26F5976CCA.

Locations:

- Build: verified local Release VST3 bundle; machine-specific path omitted from public source.
- Output: verified local Plan A formal output; machine-specific path omitted from public source.
- Installed: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3
- Pre-install v1.1.1 backup: verified internal rollback copy; machine-specific path omitted from public source.

Cubase/DAW processes were absent immediately before installation. No DAW was launched by Codex.

## User validation boundary

Cubase scan/load and listening remain manual. Verify Mix-aware GR at 0/50/100%, full-history reprojection while moving Input/Ratio/Threshold/Mix, Makeup/Output changing only Output, EXT weak Key contour and N/A state, right-side GR meter/Hold agreement, ST/LR/MS, both themes, Match, sidechain HPF, PDC, Monitor, state/A-B, and true Bypass.

No Plan B/C/D, Stable promotion, GitHub sync, Actions, macOS artifact, package, or Release work was performed.
