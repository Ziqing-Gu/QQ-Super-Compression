# Current checkpoint: 1.2.0 Rev4 Stable — 2026-09-10

Rev4 supersedes the historical checkpoint below. See [STABLE_1.2.0.md](STABLE_1.2.0.md) and [VERIFICATION_REV4_1.2.0.md](VERIFICATION_REV4_1.2.0.md). The user authorized B, then C and D with the completed 23-page PDFs retained unchanged. Current production schema is 14.

---

# Current Stable — 1.2.0 Rev3 (schema13)

2026-09-10: User-promoted Stable; reuse the verified Rev3 Windows build.
See `STABLE_1.2.0.md` and `PLAN_A_VERIFICATION_1.2.0.md`. Plan B includes the exact
JUCE 8.0.15 source under `_PlanB/Dependencies/JUCE-8.0.15`; reproduce in a fresh
D-drive working copy using that directory as JUCE_PATH and QQSC_FETCH_JUCE=OFF.
Never configure or build inside the frozen snapshot. Manuals are deferred.

2026-09-10. Windows Release build and compiled DSP/editor/LINK/settings tests
pass. See `EXPERIMENT_1.2.0.md` for current semantics and `VERIFICATION_1.2.0.md`
for evidence. Earlier headings below are historical, not the current spec.

- UP is a lower gate; boost only UP<p<DOWN, down-only above DOWN. UP=-inf opens
  the gate. The previous Rev2 below-UP activation was wrong and is superseded.
- All15 Ratio defaults/reset targets are1. Existing stored numbers stay intact.
- Dual Ratio LINK preserves UP*DOWN, defaults ON, remembers last click for new
  instances and respects saved project state. It is independent of domain LINK.
- Three themes share94px primary dials/216px spacing. Ratio arcs start at unity;
  Single origin.5, Dual UP origin1, Dual DOWN origin0. Mode button(380,620,60,21);
  Ratio LINK ST(278,644,36,17), LR/MS(281,636,30,14).
- Source `D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown`; build remains
  `D:\Codex\Temp\QQSuperCompression-1.2.0-UpDown`; output
  `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`.
- Rev2 source checkpoint `D:\Codex\Archives\QQSuperCompression-1.2.0-Rev2-before-Rev3-20260910`.
- Build targets QQSuperCompression_VST3, QQSCVisualCheck, QQSCDynamicsCheck.
  Actual installation status/hashes: Rev3 output `Verification/INSTALLATION.json`.
  Stable promotion and Plan B are the current scope; Plan C/D, remote publication,
  macOS and multiband work remain outside this checkpoint.

## Historical revisions

# Current build brief — 1.2.0 Rev2 Windows experiment

The plug-in version is 1.2.0 and state schema is 12. Rev2 Windows Release build,
compiled DSP/editor/gesture tests and ten inherited checks pass. Installation
completed with hosts closed and build/output/system hash parity. Use
`VERIFICATION_1.2.0.md` and output `Verification/INSTALLATION.json` for evidence.
Older results and hashes below belong to their explicitly labelled revisions.

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown`.
- Build: `D:\Codex\Temp\QQSuperCompression-1.2.0-UpDown`.
- Delivery: `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev2`.
- Retain the existing JUCE 8.0.15 / C++17 / Visual Studio 2022 x64 toolchain.
  Build `QQSuperCompression_VST3`; enable `QQSC_BUILD_DYNAMICS_CHECK=ON` and
  `QQSC_BUILD_VISUAL_CHECK=ON` for `QQSCDynamicsCheck` and `QQSCVisualCheck`.
  Run the latter with its D-drive render-output directory argument.
- Rev2 changes Range's default to an explicit OFF endpoint. The OFF state
  removes only the upper cutoff; finite 0 dB and other finite saved Range
  values retain `detector >= Range => unity`, including schema 11 restores.
  Missing Range parameters migrate to OFF. Dual defaults are UP = -inf and
  DOWN = 0 dB: new instances are neutral until the thresholds are moved.
- Upward gain now uses `g = 1 / (r + (1-r) * p/A)`, with A = Dual UP,
  Single finite Range, or linear 1 for Single Range OFF. Ratio 1/8 at 10 dB
  below A gives theoretical +7.92198 dB independently of A's absolute level.
- Render the bottom mode switch at `(394, 620, 60, 21)` and the full-height
  boundary rails. Check shared thumb/line coordinates for ST/LR/MS and
  internal/external key, including mode changes while audio is stopped.
- Run actual-processor Range OFF/full-scale 599.9 Hz parity against 1.1.9,
  finite-0 cutoff and state migration, neutral Dual defaults, threshold-relative
  lift, collisions, state/A-B, Mix, signed peaks, latency and harmonic checks.
  Measure the retained 480-point Display caches in both modes.
- Finite boundary transitions remain hard. Test their behaviour separately
  from steady-carrier harmonics. Lookahead 0 ms retains the existing colouring
  mode and its 1x/8x/16x oversampling; it is not the transparency reference.
- Scope is the Windows experiment. Do not publish to GitHub, build a macOS
  delivery, promote Stable or modify the multiband project as part of Rev2.

--- Historical 1.2.0 Rev1 record; not Rev2 verification ---

# 1.2.0 Rev1 Single / Dual experiment — historical build and installation

2026-09-10. Candidate only; 1.1.9 remains the Stable rollback. Windows VST3 installed after two host-closure checks, with complete build/output/install SHA-256 parity. The previous installed 1.1.9 bundle is preserved and verified in the output rollback folder.

- Source: D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown
- Build: D:\Codex\Temp\QQSuperCompression-1.2.0-UpDown
- Output: D:\Codex\Outputs\QQ Super Compression 1.2.0 UpDown Experiment
- Installed: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3
- Windows binary SHA-256: 359ACBB47019DC70ED64A9321EA6C2983CA8F4E600D6CC6FFFF15DD9A67B0C09
- Actual processor audio/state tests, actual-editor fader/ratio tests, three-theme ST/MS/LR/minimum screenshots and ten inherited source/math regressions pass. See VERIFICATION_1.2.0.md and output Verification/INSTALLATION.json.
- Single Ratio 1/32..32 default8; Dual Up Ratio1/32..1 default1/8; Dual Down Ratio1..32 default8. Dual defaults UP-24dB / DOWN-12dB. Smaller68x104 dual ST ratio controls and60x21 mode switch.
- Display median Single4.218ms / Dual4.389ms at480 points, cached software rendering.
- Known experiment limits: hard interval boundary jumps (test~12.49dB), changed old normalized Ratio automation mapping, and deferred companion notification for host-driven collisions. Use a fresh instance/session for user audition. User listening/acceptance remains pending.
- No Plan B/C/D, GitHub, macOS, Stable promotion or multiband edits. Prior frozen backups were not accessed or changed. No further rebuild/install is required unless source changes.

--- Historical entries below ---

# QQ Super Compression - Build / validation brief

## Current 1.1.9 — 3:2 Landscape

Completed Plan A/B/C/D: public source/tag c70c50fe90c738ed9079464b7299aa67cb08081d / v1.1.9; macOS run 34209677129 succeeded for three jobs and runner AU auval. Four archives, two manuals and two guides passed version/architecture/hash checks and were published as eight Release assets. Windows Actions was not run, no installation performed, and frozen Plan B untouched. See current handoff for final paths and remote completion proof.

Built with the same JUCE 8.0.15, CMake, C++17 and Visual Studio 2022 x64 toolchain as 1.1.8. No dependency or core DSP changes. Configure normally; add `-DQQSC_BUILD_VISUAL_CHECK=ON` to build the optional real-editor renderer. Run `QQSCVisualCheck <output-directory>` for three-theme/mode/bounds/cache/preference checks and `tests/*selftest.py` for source/math checks. `landscape_contract_selftest.py <active-1.1.8-source>` verifies protected source identity; older skin-only parity modes intentionally expect the old geometry and are historical tests.

Windows formal output is built once in Plan A and reused in Plan C. Windows Actions is manual-only and not run by default. Dispatch `build-macos-vst3-au.yml` at the exact release tag for Apple Silicon VST3, Intel VST3 and Universal 2 AU. Check actual bundle versions/architectures and signing/minimum-OS metadata before delivery. No DAW certification is inferred from compilation or offscreen UI testing.

Previous Stable/rollback: 1.1.8 Revision 2. Keep frozen Plan B snapshots untouched. See PLAN_A_VERIFICATION_1.1.9.md and current handoff for this version's evidence; entries below retain historical results.

--- Historical build briefs below ---
## Current Stable — v1.1.8 Revision 2

2026-09-07: User confirmed the completed Revision 2 as Stable and requested Plan B. Reuse PLAN_A_VERIFICATION_1.1.8_REV2.md; do not rebuild, reinstall or rerun tests for this promotion. Previous Stable/rollback: 1.1.7. See AI_DEVELOPMENT_HANDOFF.md for Plan B completion/freeze. Earlier build/pause notes below are historical.

### Revision 2 development history

2026-09-07: User stopped the Stable / Plan B operation before the source copy and requested a wider, brighter Light pointer. This revision is a candidate; no 1.1.8 Plan B backup exists. Reuse the local 1.1.8 build directory for an incremental build and real-size pointer check. Last completed Stable/backup: v1.1.7. Revision 1 Plan A evidence remains historical in PLAN_A_VERIFICATION_1.1.8.md; do not confuse its hashes with Revision 2. Dark/Classic and all audio behavior remain unchanged.

## Previous Stable — v1.1.7

2026-09-07: User accepted/promoted v1.1.7 and requested Plan B. Inherit the completed Plan A evidence in PLAN_A_VERIFICATION_1.1.7.md; no rebuild, reinstall or test rerun for this promotion. Previous Stable/rollback: 1.1.6 Revision 3. Refer to AI_DEVELOPMENT_HANDOFF.md for Plan B completion. Older candidate/build notes below are historical.

## v1.1.7 Candidate — Three Themes

- Stable/rollback: v1.1.6 Revision 3. New near-black Dark; retain Light and Classic exactly except version readout.
- Same offline JUCE 8.0.15 / MSVC x64 toolchain, target QQSuperCompression_VST3 and optional QQSCVisualCheck.
- Run tests/*selftest.py. dark_skin_contract_selftest.py accepts the **active** v1.1.6 Rev3 source directory to check protected source and geometry. Never pass or revisit a frozen Plan B backup.
- Visual renderer now checks all three themes, disk preference migration/restore using a test-only file, normalized blue emission, bounded caching, and Input meter contrast. User preference files are not written by tests.
- No GitHub, Actions, macOS, Plan B/C/D or Stable promotion in this run.

## v1.1.6 Revision 2 Candidate — Accepted Asset Knobs

- Stable/rollback: v1.1.5. UI-only; no DSP, parameter, state or layout changes.
- Build Windows Release VST3 with the existing JUCE 8.0.15/MSVC toolchain.
- Optional CMake switch: QQSC_BUILD_VISUAL_CHECK=ON. Target QQSCVisualCheck renders the actual editor to an output directory supplied as its sole argument; it never opens a DAW or writes user UI preferences.
- Run tests/*selftest.py. warm_skin_contract_selftest.py optionally accepts the untouched stable source root for byte-identity/layout checks.
- The two approved study textures are embedded by QQSCWarmKnobAssets; no external runtime asset directory is needed. WarmKnobAsset.h is the production compositor, not the rejected WarmMaterial knob shader.
- LIGHT Input/Output/GR colors are shared by meters and Display; Classic remains unchanged.
- Install only after hosts close; see PLAN_A_VERIFICATION_1.1.6.md for actual installation status. No Stable promotion or publishing in this Plan A.

## v1.1.5 Stable - Fluid/Cached Dynamic Display Rendering

- Base: v1.1.4 Candidate; previous Stable rollback: v1.1.2.
- Windows x64 VST3 Release / JUCE 8.0.15 / MSVC 19.44.
- 60 Hz / 480-point approximately eight-second history, fixed projection arrays, retained path caches, sparse bounded GR shade, and opaque child rendering.
- GR math, Key Gain live projection, HPF background replay, audio DSP, parameters/state schema 10, automation IDs, and A/B remain unchanged.
- Twelve Python/source/math checks, standalone BS.1770, Steinberg validator, metadata, and build/output/install parity: PASS.
- Main binary SHA-256: `ABB9CFD1CF7929C6D4C6A7D9F72226535F36169854A1697B44550047F54B64E1`.
- Read `PLAN_A_VERIFICATION_1.1.5.md` first for Plan A evidence.
- Plan A/B/C/D are complete and v1.1.5 is the current Stable/public Release. Public source commit/tag: `952f7691f67c810ba351c28e213d3620d3425b24` / `v1.1.5`; macOS run `33580627982` passed all three jobs and AU `auval`. Windows reused Plan A and Windows Actions was not run.

--- PREVIOUS CANDIDATE BUILD BRIEF BELOW ---

## v1.1.4 Candidate - Reliable/Faster HPF Display Replay

- Base: v1.1.3 Candidate; Stable rollback: v1.1.2.
- Windows x64 VST3 Release / JUCE 8.0.15 / MSVC 19.44.
- Latest-request retry, two-tick non-mouse debounce, visible-window pre-roll, current-domain two-engine replay, and `HPF UPDATING` status.
- Audio DSP, parameter/state schema 10, automation IDs, A/B, and real-time Key Gain behaviour are unchanged.
- Eleven Python/source/math checks, standalone BS.1770, Steinberg validator, metadata, and build/output/install parity: PASS.
- Replay-core benchmark: old about 19-26 ms; optimized about 8-13 ms, typically about 55% lower.
- Main binary SHA-256: `05A41D64AC1CA45A7EF89F34E6BE946A18E83323308A24A496C89EF381504731`.
- Read `PLAN_A_VERIFICATION_1.1.4.md` first for current Candidate evidence.
- No Plan B/C/D, Stable promotion, GitHub, Actions, macOS, or Release work is included.

--- PREVIOUS CANDIDATE BUILD BRIEF BELOW ---

## v1.1.3 Candidate - Sidechain Display History Replay

- Base and rollback: v1.1.2 Stable.
- Build target: Windows x64 VST3 Release, JUCE 8.0.15, MSVC 19.44.
- Plan A output: verified local formal VST3 bundle and ZIP; installed after Cubase was confirmed closed with build/output/install hash parity.
- Eleven Python/source/math checks, standalone BS.1770, and Steinberg vst3effectsvalidator: PASS.
- Bundle: 2 files / 6,742,107 bytes.
- Main binary SHA-256: `9B37B5C756D33D1E32E7E4982695CB5387FD89284C875D9CE99530AD7004AB59`.
- Key Gain history is real-time; HPF history replays after gesture release or a short non-mouse debounce.
- Display capture is editor-only, ten seconds, capped at 48 kHz; audio DSP and state schema 10 are unchanged.
- Read `PLAN_A_VERIFICATION_1.1.3.md` and the top of `AI_DEVELOPMENT_HANDOFF.md` for the current Candidate evidence.
- No Plan B/C/D, GitHub, Actions, macOS, Release, or Stable promotion is included.

--- CURRENT STABLE BUILD BRIEF BELOW ---


## v1.1.2 Stable - Mix-aware Dynamic Display

- Build target: Windows x64 VST3 Release, JUCE 8.0.15, MSVC 19.44.
- Plan A output: verified local formal output; machine-specific path omitted from public source.
- System install: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3.
- Build/output/install parity: 2 files, 6,724,699 bytes, tree SHA-256 2D6CE2509C70C9D697756F4AD264BCB6EC284619DB2B341A5A670AE11BDF061C.
- Main binary SHA-256: 240A9DBEBDE3D88B14A59096218525204CD43B020B486E390A1042330EAA1DE3.
- Ten Python/source/math checks and standalone BS.1770 pass; validator is unavailable.
- v1.1.2 completed Plan A and Plan B and is Stable by the project standing rule. Plan C reuses the verified Windows Plan A output and manually dispatches only Apple Silicon VST3, Intel VST3, and Universal 2 AU from the exact public v1.1.2 tag. Windows Actions is retained for explicit reproduction only and is not run by Plan C; Plan D/Release remains separate.

Read AI_DEVELOPMENT_HANDOFF.md and PLAN_A_VERIFICATION_1.1.2.md first for the current Stable contract and Plan A evidence.

--- CURRENT STABLE BUILD BRIEF BELOW ---
# QQ Super Compression 1.1.1 Stable - Codex build / validation brief

## v1.1.1 Stable - Side Chain HPF

- Build target: Windows x64 VST3 Release, JUCE 8.0.15, MSVC 19.44.
- Plan A output: verified local formal output; machine-specific path omitted from public source.
- System install: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3.
- Build/output/install parity: 2 files, 6,716,507 bytes, tree SHA-256 50FB3109C22DDB55E591941301C81A034CFEC097501C2639BA0E7FF9D273CFB6.
- Nine Python/source/math checks and standalone BS.1770 pass; validator is unavailable.
- Plan B source backup: verified internal formal backup; machine-specific path omitted from public source.
- v1.1.1 is the previous Stable rollback; v1.1.2 is current Stable.
- No Plan C/D, GitHub, Actions, macOS build, packaging, or Release work is part of this run.

**Stable baseline:** v1.1.0 External Key (Plan A/B complete; user-promoted Stable on 2026-09-02)
**Previous stable rollback reference:** v1.0.4 Light / Classic UI switch

Read first:

1. `AI_DEVELOPMENT_HANDOFF.md`
2. `PRODUCT_DESIGN_NOTES.md`
3. `README.md`
4. `CHANGELOG.md`
5. `DEVELOPMENT_HISTORY.md`
6. `docs/TEST_CHECKLIST.md`

## v1.1.0 Stable baseline

**Current Stable target:** `v1.1.0 — External Key` (Plan A and formal Plan B complete)

**Previous Stable rollback baseline:** `v1.0.4 — Light / Classic UI switch`

Build directly from the v1.0.4 Stable code. External Key may replace only the detector source; it must not change the carrier, future-window gain law, Threshold, Lookahead, Oversampling, PDC, Match, Display, Monitor, A/B semantics, or either theme.

Required contract:

- optional mono/stereo input bus named `Sidechain`;
- INT = exact v1.0.4 post-Input-Gain detector; EXT = sidechain after dedicated Key Gain;
- disconnected/silent EXT = zero GR, audible carrier intact;
- ST common key, LR independent key domains, stereo MS matrix, mono EXT common to both M/S;
- Key Source + Key Gain are appended APVTS parameters and A/B members; old states migrate to INT / 0 dB;
- SC Listen is non-automatable/non-persistent/non-A/B, latency-aligned, ignored by true Bypass, and resets OFF on panel/editor close/state restore;
- the same 230x146 floating panel geometry is used in LIGHT and CLASSIC; no main-layout movement.

Verified Plan A evidence inherited by Plan B:

- JUCE 8.0.15 / MSVC Windows x64 Release VST3 build with no new warnings;
- all eight Python/source/math tests plus standalone BS.1770 test pass;
- binary and module metadata show 1.1.0;
- copy the verified bundle to local formal output storage;
- install only after Cubase/DAW processes are confirmed closed;
- hash parity between build, output, and system-installed bundle;
- clearly record Steinberg validator as unavailable if it cannot be found;
- the user explicitly promoted v1.1.0 to Stable on 2026-09-02; detailed Cubase sidechain/audio/UI/PDC/old-project checks remain recorded as manual follow-up.

## Scope

Build from v1.0.4 Stable. The LIGHT / CLASSIC theme choice is visual-only. Do not alter the transparent future-window compressor, Threshold, LINK, independent Mix, Match, Oversampling, PDC, Display or v1.0.3 Monitor behaviour.

Verify the new Monitor exactly:

- LR ALL normal stereo; L/R centered at `1/sqrt(2)`.
- MS ALL normal stereo; M centered at unity; S centered at `1/sqrt(2)`.
- Display/Meters/Match stay pre-monitor and must not fall 3.01 dB when L/R/S Monitor is selected.
- Monitor hidden in ST; LR shows ALL/L/R; MS shows ALL/M/S.
- LR and MS selections restore separately after project save/reopen.
- A/B does not change Monitor; host automation list does not gain Monitor parameters.
- True Bypass remains a true bypass and ignores Monitor.

## Required validation before Stable claim

- Windows Release VST3 compile with JUCE 8.0.15; no new warnings.
- Cubase scan/load and panel shows v1.0.4.
- Listen to LR L/R centered and compare level against the established -3.01 dB centered convention.
- Confirm M is **not** 3.01 dB quieter.
- Confirm S is centered mono and compensated -3.01 dB.
- Toggle Monitor while Display/Meter are active and verify graphs/meters retain normal processing levels.
- Save/reopen project and verify independent LR/MS Monitor memory.
- Regression: all v1.0.2 LINK behaviours, future-window DSP, Threshold, Mix, PDC/Bypass, 0 ms Oversampling and Match.

Current v1.0.4 environment: JUCE/MSVC Windows x64 Release build, installed-bundle parity and seven source/math regression tests passed. Steinberg validator was unavailable and is not claimed for v1.0.4.

## Build

- VST3 / Release.
- Prefer an existing JUCE checkout with `-DJUCE_PATH=...`.
- Otherwise CMake is pinned to JUCE 8.0.15 when network access is available.
- Do not call Python/static checks a successful plug-in build.
- Plan C reuses the verified Windows x64 VST3 from the formal local Plan A output; it does not rebuild Windows in GitHub Actions by default.
- Plan C manually dispatches `.github/workflows/build-macos-vst3-au.yml` for Apple Silicon VST3, Intel x86_64 VST3 and Universal 2 AU from the confirmed public `v1.0.4` source commit/tag.

## Non-negotiable DSP baseline

v1.0.2 must be a workflow-only change from the v1.0.1 Stable audio engine:

```text
input
 -> optional 0 ms-only Oversampling (1x/8x/16x)
 -> future-window peak detector
 -> QQ Ratio + optional Threshold boundary
 -> apply gain to matching delayed sample
 -> Makeup / Mix / Output
```

There is no user Attack/Release envelope. Threshold OFF must execute the exact pre-Threshold QQ law. Lookahead intentionally controls the future-window length; the accepted microscopic pre-influence near abrupt level changes is not a regression.

## Domain parameters

ST: one Ratio / Threshold / Makeup / Mix.

LR: independent L/R Ratio / Threshold / Makeup / Mix.

MS: independent M/S Ratio / Threshold / Makeup / Mix.

ST uses the stronger exact current L/R window peak level, then calculates linked gain with the **ST** Ratio/Threshold. Hidden LR Ratio values must not affect ST.

## v1.0.2 LINK semantics

One Link state covers **Ratio, Threshold, Makeup and Mix** pairs in LR/MS.

It is **relative link**, not equality link:

- preserve the numerical difference captured at edit start;
- both controls move by the same delta;
- when either hits a boundary, both stop;
- never snap values equal merely because Link is enabled;
- Mix uses percentage-point delta (e.g. `100/70 -> -10 -> 90/60`);
- direct numeric entry must use the same shared-delta/boundary law as normal drag and Shift fine drag.

Threshold OFF is conceptual `-inf`; if exactly one threshold starts OFF, keep that OFF member outside finite-dB linking for that edit. If both are OFF, a finite direct entry may bring both out together from the same value.

## Oversampling regression

- 0 ms shows `1x / 8x / 16x`.
- 10/26/40/80/100 ms hide Oversampling and run 1x internally.
- remembered 0 ms choice must survive switching away/back.
- PDC/Dry/Mix/Bypass must include exact Oversampling FIR latency at 0 ms.

## UI regression

- Fixed design space 1020x820, uniformly scaled.
- Display/Meter row = 550 design px; current v1.0.4 lower row = 158 design px (uses former bottom slack).
- LR/MS use two stacked full-width histories.
- Display visible range = 0…-90 dB.
- Mode is a click-cycle button; Lookahead is a ComboBox. Mode and Lookahead are 108x23 and aligned; LINK is 34x23 to the right.
- Threshold Shift fine drag / Alt reset / direct entry remain functional.

## Historical v1.0.2 validation checklist (already promoted Stable)

- build VST3 with JUCE/MSVC;
- Cubase scan/open/save/restore;
- verify panel/binary metadata show v1.0.2;
- LINK Ratio drag + Shift + direct numeric entry in LR and MS;
- LINK Threshold drag + Shift + direct numeric entry, including one-OFF and both-OFF cases;
- LINK Makeup drag + Shift + direct numeric entry;
- LINK Mix drag + Shift + direct numeric entry;
- verify shared-boundary stopping preserves offsets;
- verify LINK OFF leaves all four pairs independent;
- regression: transparent core/Threshold/Lookahead/Oversampling/PDC/Display/A-B/Undo-Redo match v1.0.1 Stable;
- old project state loads without migration changes (v1.0.2 adds no parameter/state schema).

v1.0.4 validation: local JUCE 8.0.15 / MSVC Windows x64 VST3 build, installed-bundle parity and seven source/math regression tests passed. The user explicitly promoted this Plan A/Plan B revision to the current Stable baseline on 2026-09-01. Steinberg validator was unavailable for the v1.0.4 local run and remains an explicit validation gap; v1.0.3 remains the verified cross-platform rollback reference.

---

## v1.0.1 Candidate Revision 2 verification

After building, verify in Cubase/PluginDoctor:

1. Mode button is visible in ST, LR and MS and cycles ST -> MS -> LR -> ST.
2. LINK is visible in LR/MS, hidden in ST, and still links only Ratio/Threshold/Makeup relatively.
3. LR exposes separate L/R Mix; MS exposes separate M/S Mix; ST exposes one Mix.
4. In MS, setting M Mix=100% and S Mix=0% changes only the Mid processed contribution while Side remains Dry before decode.
5. Threshold faders are stacked vertically: L/M top, R/S bottom.
6. Display remains the same enlarged v1.0.1 size.
