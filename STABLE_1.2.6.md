# QQ Super Compression 1.2.6 Stable / Plan B

用户于 2026-09-27 确认接受 Preview 的调制残差取舍，并要求正式替换、删除 Preview VST3、执行 Plan B。

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.2.6-Stable`
- Build: `D:\Codex\Temp\QQSC-1.2.6-Stable`
- Output: `D:\Codex\Outputs\QQ Super Compression\1.2.6 Stable`
- Installed: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`
- Formal identity: Qscp; version 1.2.6; exact dependency JUCE 8.0.15.
- Approved audio baseline: `D:\Codex\Workspaces\QQSuperCompression-1.2.6-Preview`.

正式版本只恢复名称、身份和正式偏好文件，检测与压缩处理不再修改。`verify-promotion.py` 验证生产源码与 Preview 的白名单差异，并冻结待测源码哈希；`QQSCStablePromotionCheck` 对比实际 VST3 输出并检查旧工程状态兼容。

使用 `build-stable.cmd` 构建，`plan-b-126.py stage` 整理验证后的产物，`install-stable.ps1` 安装及清理 Preview，`plan-b-126.py snapshot` 创建并验证正式备份。这些发布操作脚本会核对前置证据并拒绝不安全的重复执行。其他旧构建、安装、发布脚本仅为历史记录。

备份保存在 `D:\备份文件\Vibe Coding\QQ Super Compression\源代码\QQ Super Compression 1.2.6-PlanB-Stable-<时间戳>`，包含完整 Source、精确 JUCE 依赖、测试证据、清单和复现说明。最终路径、数量、字节和 SHA-256 以输出目录 `Verification/PLAN_B_COMPLETION.json` 为准，仅 COMPLETE 表示完成。

详见 [发行说明](RELEASE_NOTES_1.2.6.md)。1.2.5 Stable 源码及其正式备份保持原样；本阶段不涉及多段压缩、桌面、GitHub 或 Mac 构建。PREVIEW_1.2.6.md 以及其他旧版本文档是历史记录。
