QQ Super Compression 1.2.36 中英文说明书制作源文件
状态：中文版已由用户确认；对应英文版已完成并通过内容与排版检查。两种语言均为 41 页。

中文入口：build_manual_1_2_36_zh.py；保留用户确认后的原始制作文件。
英文准备：prepare_english_1_2_36.py；读取同目录中文源文件、english_base_translations.json、english_pending.json、english_new_translations.json，生成英文源与完整对照表。
英文入口：build_manual_1_2_36_en.py。
截图入口：manual_capture_1_2_36.cpp；build_capture.py 链接已验证 1.2.36 对象生成截图。

内容同步：Limiter 20:1-1000:1 + Output LINK；先下压再少量 UP 1:1-1:1.2；底鼓/军鼓延音；分段压缩建议；0 ms Lookahead 可让副歌很响、保留安静部分音量，结合核心 4x/8x/16x；CEILING OS 独立设置；FULL/ECO 每个实例独立；停止后的静音休眠与当前块恢复。
核心 1x/4x/8x/16x，Ceiling Hard Clip 1x/4x/8x/16x，TP 4x/8x/16x。两处初始 8x，保留 TP+存储 1x 临时提升 8x 的兼容行为。

生成英文源：python prepare_english_1_2_36.py
生成 PDF：python build_manual_1_2_36_en.py --output "D:\Codex\Outputs\QQ Super Compression\1.2.36 Manuals" --captures assets --theme-images assets --fonts C:\Windows\Fonts
中文生成使用对应的 _zh.py 入口。Python 运行时需要 ReportLab、Pillow、pypdf，校验还需 pdfplumber；视觉核验使用 Poppler。Windows 系统字体不随包重分发。
校验：verify_english.py / verify_chinese.py；涵盖内容、目录、边界及产品源码/插件哈希。重新运行自动校验后仍需视觉复核。

保存的 QA 只代表此次说明书制作，不声称新完成音频、Cubase GUI、离线导出或 ASIO-Guard 实测。
原有中文审阅备份和旧中英文手册保留。产品文档集成、代码 Plan B 更新、GitHub 与版本发布仍属于后续阶段。
