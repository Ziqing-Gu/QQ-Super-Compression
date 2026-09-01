# QQ Super Compression 1.0.4 — Codex build / validation brief

**Stable baseline:** v1.0.4 Light / Classic UI switch (Plan A/B complete; user-promoted Stable on 2026-09-01)  
**Previous stable rollback reference:** v1.0.3 Centered Domain Monitor (tag v1.0.3 -> 0ef89e19)

Read first:

1. `AI_DEVELOPMENT_HANDOFF.md`
2. `PRODUCT_DESIGN_NOTES.md`
3. `README.md`
4. `CHANGELOG.md`
5. `DEVELOPMENT_HISTORY.md`
6. `docs/TEST_CHECKLIST.md`

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
- Plan C reuses the verified Windows x64 VST3 from the formal Plan A output under `D:\Codex\Outputs\QQ Super Compression\1.0.4\Windows`; it does not rebuild Windows in GitHub Actions by default.
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
