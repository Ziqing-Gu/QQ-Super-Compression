# QQ Super Compression 1.2.7 中文说明书

状态：中文已确认；对应英文版已完成。两本手册均为 35 页，用于 1.2.7 Stable 发布。

## 修订来源与风格

- 来源是用户保留的 1.2.6 中文曲线更新稿，原文件存放于 `../baseline-1.2.6/held-curve-Chinese-1.2.6.pdf`，未修改。
- 原稿 SHA-256：`687B23A16F2D928088776E63F4355810238397F6789DD5148354EB06127B57F3`。
- 保留 A4 页面、微软雅黑/Segoe UI 字体、青蓝与橙色、浅青提示框、表格及编号步骤的样式。
- 保留第 5 页 Classic / Super 静态曲线，未加入旋钮灯光方向说明。
- 新增第 25–31 页：Limiter、下压 Ratio 与 Output LINK、Ceiling / TP、20 秒峰值保持、耳机 1:1、MATCH / Bypass 等响比较及使用提示。
- 同步修订 A/B 保存范围、Display、增益范围、固定缓冲与参数速查；普通压缩的参数说明与 Limiter 区分。

## 界面图来源

`assets/` 来自当前 1.2.7 的实际 JUCE 编辑器离屏渲染，使用同一组已验证生产对象代码和确定性音频输入。没有操作用户的 DAW，没有改写用户偏好；截图使用独立的 `documentation-only.settings`。

`manual_capture_1_2_7.cpp` 是仅用于文档的入口；`build_capture.py` 使用已接受构建中的编译/链接参数。生成程序和中间文件放在 `D:\Codex\Temp\QQSCManual127`。生产 `Source/` 与 `Assets/` 没有因文档工作而改变。Ceiling 细节图在 PDF 中对完整实际截图作裁切，未重绘控件。

## 重现 PDF

依赖：Python 3、ReportLab、Pillow、pypdf、pdfplumber；字体读取 Windows 的 `C:\Windows\Fonts`。在本目录运行：

```powershell
python .\revise_manual.py
python .\build_manual_1_2_7_zh.py --output 'D:\Codex\Outputs\QQ Super Compression\1.2.7 中文说明书' --captures '.\assets' --theme-images '.\assets' --fonts 'C:\Windows\Fonts'
```

`revise_manual.py` 从所保留的旧源生成完整的新版本排版脚本，`limiter_pages_zh.inc.py` 保存新增的七页内容。修改时应改这两个输入文件，再生成最终脚本。

## 检查

用 Poppler 将 PDF 以 110 dpi 渲染到 `D:\Codex\Temp\QQSCManual127\page-01.png` 至 `page-35.png`，再运行 `verify_manual.py`。检查包括页数、书签、指定章节页码、必要功能说明、已移除的过期说法、文本越界和旧原稿哈希；生成七张联系表供目视核对。

`QA.json` 对应本次实际 PDF 的哈希与审阅结果。重新运行验证脚本会将视觉审阅状态改回 pending，必须重新查看渲染图后再记录完成。最终 PDF 已检查全 35 页，并复查新增页面、修改页与细节图，无遮挡、文字越界或截断控件。

最终 PDF 同步到 `../QQ-Super-Compression-1.2.7-User-Manual-Chinese.pdf`，并随第二份、独立的 Plan B 文档快照保存。第一份 Stable 快照保持不变。

## English edition

Run `prepare_english.py`, then `build_manual_1_2_7_en.py` with the same assets/fonts and an English output directory. `english_translations.json` holds reviewed new/changed strings; unchanged text reuses the preserved matched bilingual 1.2.6 sources. `verify_english.py` checks 35 pages, bookmarks, key content, text bounds and untranslated text. All pages were visually reviewed; see QA-EN.json.
