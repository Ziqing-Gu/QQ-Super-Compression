# QQ Super Compression 1.2.6 English manual

Completed 2026-09-27, following the user's request to produce the English edition of the approved Chinese manual.

## Delivery

- PDF: `D:\Codex\Outputs\QQ Super Compression\1.2.6 英文说明书\QQ Super Compression User Manual English_v1.2.6.pdf`
- 28 pages, 28 navigation bookmarks, 5,838,484 bytes.
- SHA-256: `6169590F6A94F7C36846C26C0F22938AD81C758541200724AEE9B4579D593FB5`.

## Content and style

The English edition follows the approved Chinese manual page by page, including Classic and Super algorithms, threshold semantics, 1:1000 to 1000:1 ratios, upward gating, Single/Range and Dual boundaries, branch switches, relative Ratio LINK, Input/Output LINK, Match, A/B, Display interpretation, and the aligned Lookahead detector. The original English manual's typography, turquoise palette and explanatory style are retained. Corresponding pages use identical screenshot assets to the approved Chinese edition.

No Ratio lighting-direction instructions were added. Low distortion is described with its practical limits rather than as an unconditional zero-distortion guarantee.

## Verification

- All 28 rendered pages visually inspected; page 10 rechecked after final copy correction.
- No text clipping, collisions, broken tables or unreadable glyphs found.
- Page count, bookmarks, version footers, parameter ranges, screenshot parity, text bounds and absence of untranslated Chinese verified by `check_english.py`.
- Completed verification record: `PDF_QA_EN.json`.
- Approved Chinese PDF remains unchanged: SHA-256 `51BED92CFE91305FE46100EF162A910C8EEF7205BC492D7296ED34F49DD4951B`.
- Original English 1.2.0 PDF remains unchanged: SHA-256 `5DABB3992976052DCD11241AA3265A6A777268F95A0699E5E291B63441E7E3C6`.

## Reproduction

`assemble_english.py` creates the self-contained `build_manual_1_2_6_en.py` from the prior English builder and the approved 1.2.6 content. The latter builds the final PDF using the local `assets` directory and Windows Segoe UI fonts. `check_english.py` validates the PDF and renders page previews under `D:\Codex\Temp\QQSCManual126\english`.

This documentation task does not modify the installed plugin, the Stable source tree, existing Plan B snapshots, or desktop packages.
