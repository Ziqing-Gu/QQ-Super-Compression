# English manual 1.2.0

2026-09-10. Matches the user-approved 23-page Chinese edition, including page
order, cross-references, actual Rev3 screenshots, palette and layout. Unchanged
sections retain the previous English phrasing. No Ratio lighting-direction text.

All 23 pages were rendered and visually inspected. A full-width semicolon in the
Dual defaults table was replaced with English punctuation and that page checked
again. Both PDFs have 23 pages and 23 bookmarks. Content baselines remain above
the 53pt limit (English minimum 60.5pt). No text/image/footer overlap was found.

- English SHA-256: `5dabb3992976052dcd11241aa3265a6a777268f95a0699e5e291b63441e7e3c6`
- Approved Chinese SHA-256: `b7b40b18cf3e7536263f0fb4a930d0ac337f648e5cf595f0828ea92e78ff784f`

Reproduce with `build_manual_1_2_0_en.py`, Python reportlab/Pillow/pypdf and the
Windows Segoe UI / Microsoft YaHei fonts. Use `--language en`, `--output` for
the output directory, `--captures ../images/1.2.0`, `--theme-images ../images/1.2.0`
and `--fonts C:/Windows/Fonts`. The builder is standalone. Older manuals remain.
