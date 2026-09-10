# Chinese manual 1.2.0 review checkpoint

2026-09-10. The user approved this 23-page Chinese edition and requested the
matching English edition and a cross-platform release. This approval follows
removal of the Ratio lighting-direction chapter. The approved PDF is unchanged.

Output: `D:\Codex\Outputs\QQ Super Compression 1.2.0 中文说明书待确认\QQ Super Compression 用户手册 中文版_v1.2.0.pdf`.
Source copy: `docs/manuals/QQ Super Compression 用户手册 中文版_v1.2.0.pdf`.

The 23-page edition retains the approved 18-page Chinese manual's A4 layout,
48pt margins, Microsoft YaHei/Segoe UI typefaces, teal #009db5 titles, ink #293940,
muted #70848e, pale #eaf6f8 notes, tables and numbered steps. It integrates the
1.2.0 additions into a current manual rather than retaining stale version appendices.

- Pages 6-9: Single Threshold/Range, Range OFF, upward gate, Dual handoff,
  collisions/collapse, independent Ratio controls and relative Ratio LINK.
- Page 11: Cut/Boost and signed Gain +/-; positive/negative display semantics.
- Pages 20-22: three Ratio ranges/unity defaults, parameter reference and
  troubleshooting, including disabling Ratio LINK to audition Up independently.
- Existing theme/resize, Input/Mix/Makeup/Match/A-B, Lookahead, ST/LR/MS/domain
  LINK, internal/external sidechain and mixing guidance are retained and aligned.
- Cover, footers, 23 bookmarks, contents and cross-references now identify 1.2.0.
- User review: remove the Ratio lighting-direction chapter; remaining pages and
  page references are renumbered. Existing image assets are retained for provenance.

Figures are 2x renders of the actual accepted 1.2.0 Rev3 production editor,
fed with varying audio through its processor. Source assets: `../images/1.2.0`.
`CAPTURE_PROVENANCE.json` records the source library, captures and hashes. Figure
values illustrate the controls and are not factory defaults or mandatory presets.

The preceding 24-page draft was visually reviewed. After removing its lighting
chapter, all 23 pages were rerendered and matched to the retained pages; revised
contents and cross-references were checked. Layout asserts a minimum 53pt content baseline (lowest actual
103pt), preserving the 40pt footer rule. Content was independently checked against
the current approved parameter/DSP/editor definition. The exact packaged PDF,
page/bookmark checks and unchanged English hash are recorded under
`D:\Codex\Temp\QQSCManual120\MANUAL_QA.json`.

Reproduce with the bundled Python reportlab/Pillow/pypdf dependencies:

```powershell
python build_manual_1_2_0_zh.py --output 'D:\Codex\Temp\QQSCManual120\Draft' --captures '..\images\1.2.0' --theme-images '..\images\1.2.0' --fonts 'C:\Windows\Fonts' --language zh
```

This is documentation work after the frozen Stable Plan B checkpoint. The frozen
snapshot is neither reopened nor updated. Production code, VST3 and older manuals
remain unchanged. Later backup work must use a new snapshot if requested.
