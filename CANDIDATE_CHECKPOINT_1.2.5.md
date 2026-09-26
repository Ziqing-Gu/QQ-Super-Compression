> Historical candidate record, superseded by [STABLE_1.2.5.md](STABLE_1.2.5.md) on 2026-09-27. The auditioned binary was promoted unchanged.

# 1.2.5 Candidate checkpoint

Current source: `D:\Codex\Workspaces\QQSuperCompression-1.2.5-ThresholdFloor`.
Build: `D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor`.
Delivery: `D:\Codex\Outputs\QQ Super Compression\1.2.5 Candidate-Classic90`.

Authorized scope: Classic finite -90 dB floor (2026-09-27 revision), remembered initially enabled Input/Output Link, remembered Classic/Super choice, complete-result A/B fade, deep MATCH. MATCH removes the -70 LUFS absolute gate; relative gating remains. Do not restore the earlier fallback design. New editor instances inherit the last explicit algorithm click, default Classic on first use; project, automation and A/B state take priority and do not overwrite the global preference.

A/B requests retain the original source until consumed by the audio callback. The audio thread uses a nonblocking try-lock for the request and immutable complete bank transfer functions. Two added wet oversampling channels and a separate delayed Dry matrix avoid mixing arbitrary intermediate Ratio/Makeup settings. Both endpoints share input/detector/timing configuration; no additional reported latency. Fade is 20 ms smoothstep; direct algorithm button remains 10 ms. Different input, sidechain or latency configurations retain the previous reconfiguration behavior.

Validation commands: `build-tests.cmd`, followed by `QQSCDynamicsCheck`, `QQSCVisualCheck`, and `QQSCAlgorithmVSTCheck` against released 1.2.3 / 1.2.2 bundles. `package-candidate.ps1` requires three passing logs. `install-candidate.ps1` checks audio hosts are closed, verifies the installed original 1.2.5 SHA, creates and verifies a rollback, installs and verifies each candidate file.

The output `Verification\CANDIDATE.json` is the authority for installation status, hashes and rollback path. Until it says Installed=true, do not claim the system plugin has been replaced.

Publication, Stable designation, Plan B/C/D/E/F and multiband changes remain paused pending user audition. No desktop files unless the user explicitly asks for Plan C. Existing public manuals are unchanged; TRY_1.2.5.md describes this candidate and its compatibility boundaries.
