# QQ Super Compression 1.2.11 — Verification Notes

This candidate is based on the clean 1.2.9 source snapshot. 1.2.10 Algorithm-Link was intentionally abandoned.

## Intended Limiter LINK contract
Only DOWN Threshold, Makeup and Output Gain participate. The UI relationship is an exact opposite dB delta, not a compression-curve/loudness compensation. Ratio, Mix and Classic/Super changes are explicitly excluded.

Direct Output edits continue the established behavior of driving DOWN Threshold only; they do not also move Makeup, avoiding double correction.

## Windows acceptance
Run BUILD_WINDOWS.cmd. It defaults to D:\Codex\Temp\QQSC1211-Build to avoid Windows long-path issues, builds the VST3 and QQSCLimiterCheck, then runs revision1211, continuity and dual. It does not install the plugin or execute Plan B.

The revision1211 test uses the real JUCE editor/parameters and checks Single/Dual, ST/LR/MS, Classic/Super, LINK on/off, strict Threshold/Makeup deltas, Output->Threshold inverse motion, and confirms Ratio/Mix/algorithm edits leave Output unchanged.

## Isolation results executed in this session
- `strict_one_to_one_link_isolated_test.py`: PASS. Source whitelist confirms Makeup + DOWN Threshold only; 100,000 source→Output randomized clamp cases and 100,000 Output→Threshold randomized clamp cases preserved equal-and-opposite dB deltas.
- `limiter_mode_continuity_isolated_test.py --sanitize`: PASS. 864 limiter mode scenarios, 100 Normal Single/Dual memory toggles, restore/Undo suppression test doubles, headless callback path, and 252,021 boundary-policy transfers.
- These are not a Windows JUCE/VST3 build. The real integration gate remains `BUILD_WINDOWS.cmd`, which runs `revision1211`, `continuity`, and `dual`.
