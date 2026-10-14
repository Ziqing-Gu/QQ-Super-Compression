# QQ Super Compression 1.4.1 manual (en)

Current 49-page bilingual manual for the user-approved 1.4.1 Stable version.
The threshold table separates new-instance defaults from Alt/Option reset; legacy recall is a separate note. Screenshots show the larger threshold labels and compact lower-left LUFS meter. SAFE layout and the early ST Link chapters remain as approved.

Run `build_manual_1_4_1_en.py --output <output-directory> --captures ../source-1.4.1-zh/assets --theme-images ../source-1.4.1-zh/assets --fonts <font-directory>` with Python, reportlab, Pillow and pypdf. Fonts: Segoe UI and Microsoft YaHei. English source is regenerated with `build_english_source.py` and its checked-in translation maps. Existing captures are included; the capture C++ source documents how the production editor was rendered.

Only manual content and publishing documentation changed for this release preparation. The accepted plug-in binary remains 1.4.1.
