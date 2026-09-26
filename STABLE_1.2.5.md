# QQ Super Compression 1.2.5 Stable / Plan B

Approval: user explicitly requested Stable and Plan B on 2026-09-27 before any new lookahead experiment.

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.2.5-Stable`
- Verified binary build: `D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor`
- Stable output: `D:\Codex\Outputs\QQ Super Compression\1.2.5 Stable`
- System: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`
- Binary SHA-256: `A88D8781740058C2E74D3C90E880CF59C48313278D7407011F8D2B9DCC014294`
- Version: 1.2.5; pinned dependency: JUCE 8.0.15; existing Qscp identity retained.

Production Source, Assets, tests and CMakeLists.txt are byte-identical to the accepted Classic90 checkout. Stable designation is a documentation promotion, not a binary rebuild. The installed bundle is checked against the accepted build; no overwrite is needed when hashes already match.

Validation: fresh native DSP suite and actual installed VST3 reference suite (120 cases); the unchanged editor retains the previously passing offscreen UI checks. Test logs, input hashes and completion records live in the output Verification directory. Only COMPLETE in `Verification/PLAN_B_COMPLETION.json` certifies finished Plan B.

Plan B is a new immutable snapshot under `D:\备份文件\Vibe Coding\QQ Super Compression\源代码`, containing the complete source, exact JUCE 8.0.15 tree, manifests and validation evidence. Use `build-stable.cmd` to reproduce on Windows with VS 2022 Build Tools and CMake. It locates the bundled dependency relative to the snapshot, or accepts QQSC_JUCE_PATH.

See RELEASE_NOTES_1.2.5.md for behavior and compatibility boundaries. The original manuals remain unchanged. No Plan C/D, desktop handoff or multiband changes are included. New detector experiments must use a separate workspace and must not overwrite this baseline or the system Stable.
