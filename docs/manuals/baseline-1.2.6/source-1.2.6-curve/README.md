# Super Compression 中文说明书：曲线图阶段草稿

2026-09-27：按用户要求，将超级分段压缩手册中的 Classic / Super 静态曲线对比图移入 Super Compression 中文手册第 5 页。沿用原手册青蓝、橙色、微软雅黑 / Segoe UI 和矢量绘图。

**用户随后明确：改完先保留，后续还要加入 Limiter 模式，因此这不是最终说明书。**

当前完成：中文第 5 页曲线图与相关例子。英文版及公开 Release 保持原样。未执行发布、Plan B、桌面交付或插件修改。不得将此草稿直接作为最终发布说明书；Limiter 功能和说明需在后续明确任务中处理。

PDF：`D:/Codex/Outputs/QQ Super Compression/1.2.6 中文说明书曲线更新/QQ Super Compression 用户手册 中文版_v1.2.6.pdf`

- 28 页、28 个书签；仅第 5 页内容改变。
- 其余 27 页提取文字及解码后的绘图流均与原发布版一致。
- 第 5 页已渲染并目视检查，未见裁切、溢出或文字重叠。
- 6,844,304 字节；SHA-256：`687b23a16f2d928088776e63f4355810238397f6789dd5148354eb06127b57f3`。
- 原发布中英文 PDF 和原生成脚本哈希均保持不变。

使用本目录 `build_manual_1_2_6_zh.py` 重建，截图位于相邻 `source-1.2.6/assets`，字体位于 `C:/Windows/Fonts`。它将新第 5 页合入原版 PDF，保留其他页面和书签。`verify_curve_zh.py` 检查页数、内容差异、文字边界与基线文件，并将第 5 页渲染到 `D:/Codex/Temp/QQSCManual126Curve`。

`prepare_curve_zh.py` 是初稿组装记录；最终 builder 已额外加入精确保留其余页面的步骤，不应再次运行组装脚本覆盖后续修改。
