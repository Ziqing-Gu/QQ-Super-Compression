# 1.2.7 release checklist

1. Preserve accepted Windows binary SHA-256 292F588A9E91FF8EC5BBD609BDFB3E9CD6165DDA1D2B22C2C28427AE2B2DAAC6. Production Source/ and Assets/ must match the accepted Stable source exactly.
2. Include matching 35-page Chinese and English manuals, their sources/screenshots, bilingual release notes and install guides. Retain author preface, license and third-party notices.
3. Freeze and hash a new full Plan B snapshot under D:/备份文件/Vibe Coding/QQ Super Compression/源代码, including exact JUCE 8.0.15 and intended GitHub source.
4. Fetch and check remote main before a normal push. Preserve history. Verify remote commit content, then build three Mac jobs from that exact commit.
5. Verify native Limiter/Ceiling/headphone/MATCH tests on both architectures, AU validation, ZIP integrity, versions, architecture, minimum OS and signature state.
6. Tag the verified source v1.2.7 and publish eight Release assets: two 35-page PDF manuals, two install TXT guides, Windows x64 ZIP, Apple Silicon VST3 ZIP, Intel VST3 ZIP, Universal 2 AU ZIP. Verify asset hashes and Latest status.
7. Put exactly those eight files in the desktop delivery folder: four documents at root, one ZIP in Win/, three ZIPs in Mac/. Deliver real files, not shortcuts; keep proof and logs in D:/Codex/Outputs outside this folder.
8. Refresh Plan B if release-bound source/docs change. Keep previous snapshots immutable.
