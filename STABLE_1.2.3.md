# 1.2.3 Plan C/D release checkpoint — 2026-09-26

User authorized Plan C/D after approving the accepted 1.2.3 Stable algorithm. Production code and assets remain byte-identical to the accepted Stable workspace. Original PDF manuals are unchanged. Current release work updates documentation and Mac workflows, including native DSP regression, macOS 11 minimum and explicit ad-hoc signing.

- Release source: `D:\Codex\Workspaces\QQSuperCompression-1.2.3-PlanCD`.
- New formal source snapshot: `D:\备份文件\Vibe Coding\QQ Super Compression\源代码\QQ Super Compression 1.2.3-PlanB-Release-20260926-205600`.
- External completion authority: `D:\Codex\Outputs\QQ Super Compression\1.2.3\Verification\PlanB-Mac-Validation\PLAN_B_COMPLETION.json`.
- Public source / release: https://github.com/Ziqing-Gu/QQ-Super-Compression/tree/v1.2.3
- Platform test and publication completion authority: `D:\Codex\Outputs\QQ Super Compression\1.2.3\Verification\RELEASE_COMPLETION.json`.

Only COMPLETE in those external records certifies completion. Never reopen, rehash or modify a completed snapshot. The earlier Stable Plan B remains frozen; release documentation requires this new full source + exact dependency snapshot. GitHub publication and platform packaging are verified externally after this source checkpoint. Use current 1.2.3 guides and release checklist; older sections below record their historical scopes.

# Mac native validation follow-up

The first arm64 job passed all fixed-dB, deep-threshold and reciprocal audio checks, then exposed an exact-float assertion in a historical parameter endpoint test. The test now permits less than 0.0001 dB (100 times below its 0.01 dB control step), logs the endpoint, and independently requires finite Range to remain enabled. Production code and the accepted Windows binary are unchanged. A new complete Plan B includes this test correction before final publication.

---

# QQ Super Compression 1.2.3 Stable

User approval:2026-09-26. The user accepted the QQ dB Compression algorithm and requested official Super Compression replacement, removal of the test installation, Stable designation and Plan B.

Version1.2.3, state schema17, JUCE8.0.15. Product QQ Super Compression; pluginQscp; bundle com.qingaudio.qqsupercompression. Original VST3 CIDs ABCDEF019182FAEB51696E6751736370 and ABCDEF011234ABCD51696E6751736370. Preference application QQSuperCompression. No legacy algorithm rollback is introduced for finite old-project thresholds.

- Active source: `D:\Codex\Workspaces\QQSuperCompression-1.2.3-Stable`.
- Build: `D:\Codex\Temp\QQSuperCompression-1.2.3-Stable`.
- Verified local output: `D:\Codex\Outputs\QQ Super Compression 1.2.3 Stable`.
- Formal Plan B snapshot: `D:\备份文件\Vibe Coding\QQ Super Compression\源代码\QQ Super Compression 1.2.3-PlanB-Stable-20260926-200700`.
- External completion authority: `D:\Codex\Outputs\QQ Super Compression 1.2.3 Stable\Verification\PlanB\PLAN_B_COMPLETION.json`.

Only status COMPLETE in the external record certifies verified Plan B completion. The full source, assets, documents, workflows, build scripts, exactJUCE8.0.15 source/licenses, accepted Windows VST3 and evidence are included. Verify all files/bytes/hashes, then freeze. Consult the external record without reopening, rehashing, overwriting, deleting or building in a completed frozen snapshot. Old frozen snapshots are not accessed. Future work belongs in an active workspace and a new snapshot.

DSP, thresholds, Display mathematics and numerical fixes match the user-approved comparison build. Only formal branding/identity/preferences/state variant metadata are changed in production sources. Windows VST3 and processor/editor checks are rebuilt for that identity. Approved original bilingual PDF manuals are unchanged; current supplemental guides describe the new algorithm.

Scope is Stable plus Plan A/B. No GitHub publication, Plan C/D, macOS build or desktop repack is claimed.
