# 1.3.2 manuals

The Chinese edition was approved before this English translation. Both have 43 pages and describe the same SAFE, Distort/Window, unified oversampling and Limiter behavior. The approved Chinese PDF retains its review-edition cover label; its content was not changed after approval.

Install Python dependencies: `reportlab`, `pypdf`, `Pillow`. On Windows, supply the installed Segoe UI / Microsoft YaHei font directory; do not redistribute proprietary fonts.

```powershell
python docs/manuals/source-1.3.2-en/build_manual_1_3_2_en.py --output ./manual-output --captures ./docs/manuals/source-1.3.2-zh/assets --theme-images ./docs/manuals/source-1.3.2-zh/assets --fonts C:/Windows/Fonts
python docs/manuals/source-1.3.2-zh/build_manual_1_3_2_zh.py --output ./manual-output --captures ./docs/manuals/source-1.3.2-zh/assets --theme-images ./docs/manuals/source-1.3.2-zh/assets --fonts C:/Windows/Fonts
```

The shared assets are offscreen captures of the production 1.3.2 editor. No private test audio is distributed.
