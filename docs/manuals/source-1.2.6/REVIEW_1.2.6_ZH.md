# QQ Super Compression 1.2.6 Chinese manual review

Status: Chinese review edition completed; awaiting the user's content approval.
English translation, release publication, and plugin installation are outside this task.

## Deliverable

`D:\Codex\Outputs\QQ Super Compression\1.2.6 中文说明书待确认\QQ Super Compression 用户手册 中文版_v1.2.6.pdf`

- 28 A4 pages, 28 navigation bookmarks, 6,325,915 bytes.
- SHA-256: `51BED92CFE91305FE46100EF162A910C8EEF7205BC492D7296ED34F49DD4951B`.
- Retains the approved teal #009db5, pale #eaf6f8, Microsoft YaHei / Segoe UI, 48-point margins, numbered steps and callout style.
- All 28 pages rendered and visually reviewed. Revised pages 4, 5, 15, 17, 19 and 27 reviewed again.
- Text extraction, content bounds, required parameter values, stale-value checks and page numbers passed.
- Existing Chinese and English 1.2.0 PDF hashes remain unchanged.

## Main changes

- Page 5: Classic fixed dB ratio vs original Super curve; Classic -90 dB floor and Super -inf; algorithm memory.
- Pages 6-9: 1:1000 to 1000:1 ratios, neutral defaults, gate-above upward action, continuous Range transition and Dual handoff.
- Pages 10-12: branch ON/OFF audition and fades; relative Ratio LINK; relative inverse Input/Output LINK with remembered state.
- Pages 13-14: Makeup ±120 dB, no fixed absolute loudness cutoff for Match; A/B algorithm/branch states and conditional reciprocal symmetry.
- Pages 15-17: Display historical projection, aligned past/future detection, 26 ms default, low distortion goal without universal zero-distortion claims.
- Pages 25-28: current references and troubleshooting, including output below threshold.
- No Ratio lighting-direction instructions added.

## Reproducible assets

`manual_capture_1_2_6.cpp` renders the real current editor offscreen with deterministic demonstration audio and isolated settings. It links the verified production shared code and existing JUCE objects read-only; it does not build or install another plugin.

The 16 PNG files actually used by the PDF are retained in `assets/`. Intermediate compiler and page-render files are under `D:\Codex\Temp\QQSCManual126`. Capture provenance and PDF QA results are recorded beside this file.

To rebuild the same layout, run `build_manual_1_2_6_zh.py` with `--captures` and `--theme-images` pointing to this workspace's `assets`, `--fonts C:\Windows\Fonts`, and `--output` set to the D-drive review output folder. The builder requires reportlab, Pillow and pypdf.

`revise_manual.py` documents how the edition was assembled from the old Chinese builder. Future direct edits should be made to the current builder; rerunning the assembly script would replace those direct edits.

The approved Stable source snapshot and its English PDF were not edited. Integration into release documentation should follow the user's approval of this edition.
