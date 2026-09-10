# QQ Super Compression 1.1.8 Revision 2 — readable Light pointer

2026-09-07. User stopped Stable / Plan B before the source copy, then asked for a thicker/brighter pointer with more inward reach. No Plan B was performed; this revision remains a candidate. Last completed Stable/backup baseline is 1.1.7.

## Narrow change

Only the Light pointer changes: warm-white core width 10 -> 22, orange edge 14 -> 30, core colour #FFF4DC -> #FFFBED; inner radius 92 -> 60 and outer radius remains 184. The lit bar is approximately 2.2x wider and 35% longer than Revision 1. Halo width/intensity and outer arc/ticks remain unchanged. The unlit zero index is wider/darker for visibility but emits no light. No Dark, Classic, layout, parameters, DSP or history changes. The user's own README introduction is retained.

## Checks

- Incremental Windows x64 Release VST3 and actual-editor renderer built successfully; final build log contains no warning/error lines.
- Light material/cache contract and protected DSP/state/history/layout checks against active 1.1.7 passed. Approved Dark renderer hash remains F60F55665A1C37B3EC9C916A56ED7B2F70FEFADB81C8D5493F2035A26487415C.
- Actual 1x/minimum-size and 2x Light screenshots inspected: pointer has a clear wider warm-white body, remains inside the cap, and extends inward without reaching the centre. ST/LR/MS and sidechain layouts retained.
- Compared with Revision 1, Light image changes remain within rotary draw areas; Light meters/background and all five Classic reference images match exactly. Dark ST/LR/MS main Display/meters match. All three themes retain equal bounds/visibility.
- Runtime zero-emission, fixed material/shadow, 200 cached paints, stationary bottom finish, preference migration/round-trip and unchanged APVTS checks passed. Tests did not write user preferences or open a DAW.
- Steinberg module validator exit 0, Fx/Dynamics loaded; this is not full DAW certification.

## Delivery

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.1.8-DarkMaterial`
- Build: `D:\Codex\Temp\QQSuperCompression-1.1.8-DarkMaterial`
- Actual-render evidence: build `ValidationRev2` and `visual-check-rev2.log`.
- Output: `D:\Codex\Outputs\QQ Super Compression 1.1.8 Rev2 Pointer Plan A`
- Main VST3 SHA-256: `7509DB02BADAAF10CDF72DDB54215C76EBF483B8A469AA5B1C4DCAB01F08F5EA`
- moduleinfo.json SHA-256: `29BA287C0B1E207146D147FA24F1958A3451D52B447A32310E98A9DAB0605D2A`
- Previous installed Revision 1 SHA-256: `7301F20BD7CDD68CB33444E71A9FB9CB5D4ED43784EC7B311A93F2E9756E1731`

Delivery and installation completed. Both built/output/installed bundle files have matching SHA-256 hashes. No matching host was running at either pre-install check. The previous installed Revision 1 was copied to `Rollback - 1.1.8 Revision 1` and both files verified before overwrite. Installed path: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`. ZIP SHA-256: `0322EE34DCB4811B8C662E6890FB934D4B78394421F7B9C0D2B54F5BB7EE5084`.

Do not execute Plan B or promote this revision until the user confirms it. Actual Cubase acceptance remains the user's check. No GitHub/Actions/Release in this task.
