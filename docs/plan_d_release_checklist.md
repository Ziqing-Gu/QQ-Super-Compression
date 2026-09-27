# 1.2.8 release checklist

1. Preserve the accepted Windows VST3 main binary SHA-256 `9A5E39848AB2691AFF7F70AAF3781D6FDBE7004B90D62C3CF57B9A0327F8B7E3`. Production `Source/`, `Assets/` and tests must match the Plan A source exactly.
2. Retain the 1.2.7 Chinese and English 35-page manuals unchanged and label both as historical feature references. They do not document the 1.2.8 Single / Dual threshold-continuity fix; the 1.2.8 README, release notes and install guides are authoritative for that change.
3. Freeze and hash a full Plan B snapshot under `D:/备份文件/Vibe Coding/QQ Super Compression/源代码`, including exact JUCE 8.0.15 and intended GitHub source. If release-bound files change later, create a new snapshot and leave earlier snapshots immutable.
4. Fetch and check remote `main` before a normal push. Preserve Git history, verify remote commit content, and build the Mac artifacts from that exact commit.
5. On Apple Silicon and Intel, run native Limiter, Ceiling, headphone, MATCH, Unity, Dual-algorithm and Single / Dual continuity checks. Validate the Universal 2 AU, ZIP integrity, version, architecture, minimum macOS and signature state.
6. Tag the verified source `v1.2.8` and publish eight Release assets: two clearly named 1.2.7 reference PDF manuals, two 1.2.8 install TXT guides, Windows x64 VST3 ZIP, Apple Silicon VST3 ZIP, Intel VST3 ZIP and Universal 2 AU ZIP. Verify remote sizes, digests, stable download URLs and Latest status.
7. Put exactly those eight user-facing files in the final package: the four documents at root, one ZIP in `Win/` and three ZIPs in `Mac/`. Keep manifests, proof, logs and checksums outside the handoff package.
8. Preserve `LICENSE`, the prominent README non-commercial notice, `LICENSE_POLICY_CHANGE.md`, author text and third-party notices.
