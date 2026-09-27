# Current: 1.2.8 Plan C correction — 2026-09-28

Desktop delivery is now verified: eight files, 43,518,833 bytes, comprising Windows x64 VST3, Apple Silicon VST3, Intel VST3, Universal 2 AU, bilingual installation guides and two clearly labelled historical 1.2.7 manuals. Apple Silicon job 108719043229 passed in run 36354377015 at f5891559f24d54abe272d37d2941cdb1e068ba89. Intel job 108717571154 passed in run 36353856702 at 9ca6e6ff284dc133d7c7ecd3137861c6fc314093; the run's aggregate failure belongs to its earlier ARM attempt and is not reported as a successful run. AU job 108714126279 passed at a281cd9700b57d29378820b8fdb1fd9080e72767. All three successful jobs have identical production inputs; the 36 production source/resource/build files match the accepted Plan A workspace. The ARM Ceiling test now accounts for sub-step floating-point rounding and requires reset/redo to restore the exact stored default. The transient matrix-selection failure in run 36354310433 was corrected before the successful ARM build. Windows was reused without rebuilding. Plan D remains deferred.

Product: QQ Super Compression, not QQ Super Multiband Compression. Public repository: Ziqing-Gu/QQ-Super-Compression. Input: the user's 1.2.8 Limiter Mode Continuity source, based on 1.2.7 Strict Output Link. Plan A is complete, with Windows binary SHA-256 9A5E39848AB2691AFF7F70AAF3781D6FDBE7004B90D62C3CF57B9A0327F8B7E3. Plan B was already confirmed complete by the user; do not inspect, refresh or change frozen backups. This instruction supersedes historical checkpoint advice below about refreshing backups.

The governing Plan definitions changed on 2026-09-01: Plan C includes source synchronization, native Apple Silicon/Intel VST3, Universal 2 AU and the actual desktop user package. Plan D means the formal GitHub Release and is deferred. Previous claims that source push alone completed Plan C were incorrect. AU passed in macOS run 36352181159 attempt 2, from a281cd9700b57d29378820b8fdb1fd9080e72767. The VST3 checks exposed two portability issues: comparison against the requested decimal rather than the stored float on arm64, and Ctrl instead of Command for undo on macOS. Commit 9ca6e6ff284dc133d7c7ecd3137861c6fc314093 uses exact pre/post stored-value equality and the platform command modifier; production source is unchanged. Run 36353856702 rebuilds the two VST3s and reuses the verified AU. Windows reuses Plan A; no Windows Actions build is required.

The first Mac run used the historical QQSCDynamicsCheck and failed on its obsolete 26ms total-latency assumption. Attempts to adapt old tests were fully reverted. The current workflow uses the supplied 1.2.8 Limiter, Unity, Dual algorithm and Continuity suites, plus architecture/version/signature checks and AU validation. Do not claim the obsolete suite passed. Historical tag v1.2.8 remains at 365ee5c; the exact Mac build commit above is authoritative for this package, with identical production inputs.

Only the verified, physically present desktop package establishes Plan C completion. Keep internal evidence separate from the user package. The existing 35-page bilingual 1.2.7 manuals may be supplied only under their original version as historical references; they are not complete 1.2.8 manuals. DAW listening and user acceptance are not inferred from automation.

---

# Historical: 1.2.6 Plan C/D release checkpoint — 2026-09-27

User explicitly requested Plan C then D for QQ Super Compression (not multiband), with the approved new Chinese and English manuals. Release source: D:/Codex/Workspaces/QQSuperCompression-1.2.6-PlanCD. Output/evidence: D:/Codex/Outputs/QQ Super Compression/1.2.6. Public repository: Ziqing-Gu/QQ-Super-Compression.

Reuse the validated installed Windows Stable 1.2.6 bundle: B7AB486AE66BB57997FA88CF44255C886602F913ACAAE3FC26B856CDFD885493. Production Source/Assets/CMake are byte-identical to Stable. Both approved 28-page PDFs are integrated without alteration, with their source builders/assets. Only docs and workflow labels differ from Stable. Source/version and native cross-platform tests must match before publication. Historical workflows and binary identities remain.

Create a NEW complete source + exact JUCE 8.0.15 snapshot under the existing D:/备份文件/Vibe Coding/QQ Super Compression/源代码 hierarchy. The external Verification/PLAN_B_COMPLETION.json is authoritative; COMPLETE freezes that snapshot. Never reopen or refresh old completed snapshots. Later source changes require another new snapshot, not mutation.

Final package: actual desktop directory QQ Super Compression 1.2.6, root bilingual installation guides and new PDF manuals, Win/ and Mac/ ZIPs. Keep proof files out of it. Publish the exact same eight files on v1.2.6 and verify asset digests/unauthenticated availability. Verification/RELEASE_COMPLETION.json records final success. This checkpoint does not claim pending remote jobs, backup or packaging have already completed. No multiband modification or new Windows install/rebuild is required.

---

# Current: 1.2.6 Stable — 2026-09-27

Follow [STABLE_1.2.6.md](STABLE_1.2.6.md) and RELEASE_NOTES_1.2.6.md. The user accepted the Preview tradeoff and requested formal replacement, Preview VST3 removal and Plan B. Both algorithms use the accepted 26ms aligned detector. Restore the formal Qscp identity and QQSuperCompression preferences, preserve accepted audio, verify old formal state compatibility, and complete the formal source/dependency snapshot. Completion authority: output Verification/PLAN_B_COMPLETION.json. Older instructions below apply only to their dated baselines.

---

# Historical: 1.2.5 Stable — 2026-09-27

Follow [STABLE_1.2.5.md](STABLE_1.2.5.md). User approved Classic90 as the unchanged Stable baseline and Plan B. Subsequent lookahead work must be isolated. No desktop, multiband or publication work is authorized in this stage. The existing installed binary is the Stable binary; verify its hash before any proposed replacement.

---

# Historical candidate checkpoint

# Current: 1.2.5 audition candidate — 2026-09-26

This checkout is the 1.2.5 candidate, not the historical 1.2.3 release below. Follow [CANDIDATE_CHECKPOINT_1.2.5.md](CANDIDATE_CHECKPOINT_1.2.5.md) and [TRY_1.2.5.md](TRY_1.2.5.md). Stable, publication and multiband work are paused pending user audition. No desktop output unless the user requests Plan C. The final MATCH design removes the absolute detection gate; do not restore the earlier fallback.

# Historical: 1.2.3 Plan C/D release checkpoint — 2026-09-26

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

## Implementation pointers

Finite Down gain=pow(T/p,1-1/R). Finite Up gain=pow(A/p,1-r) followed by the retained lower gate blend. A is Single finite Range/0dB or Dual Down threshold. -inf maps to0linear and uses the original rational branch. Single finite Range returns to unity internally; Dual ends Up at DOWN before Down takes over. Existing future-window peak and latency are unchanged.

Use tests/db_dynamics_test.cpp via QQSCDynamicsCheck, and QQSCVisualCheck for current expectations. The legacy included test main is historical helper code and is not the current curve oracle. Lower than-100dB thresholds remain finite until the-120dB sentinel. Weighted Mix/crossfade sums avoid cancellation at deep Down. Display and meter calculations match the audio.
