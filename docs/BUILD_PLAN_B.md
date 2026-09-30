# Plan B restore: QQ Super Compression 1.2.36 Stable

The formal snapshot root preserves the complete product source, Assets, docs/manuals, tests, build scripts, workflows, licenses, release records and Verification. Dependencies/JUCE-8.0.15 contains the pinned source and original third-party licenses. Win/ preserves the verified Windows bundle when included in the snapshot manifest.

Restore prerequisites: Windows x64, VS 2022 C++ Build Tools, Windows SDK and CMake 3.22+. In an x64 Native Tools environment, run BUILD_WINDOWS.cmd or BUILD_WINDOWS.ps1 at the snapshot root. The script detects Dependencies/JUCE-8.0.15. Supply -BuildDirectory with a NEW path under D:\Codex\Temp; do not reuse a cache tied to another source tree. -SkipTests skips regression checks only.

正式快照保存完整源码、资源、当前双语手册与制作文件、测试、构建脚本、工作流和许可，以及固定 JUCE 依赖。恢复时需要系统 VS / SDK / CMake 工具，使用新的 D 盘构建目录；脚本不会安装插件。覆盖已安装插件前须关闭宿主。

PLAN_B_MANIFEST.sha256, PLAN_B_SHA256.csv and PLAN_B_VERIFICATION.md prove the copied payload counts, byte sizes and hashes. They do not assert that a cloud sync client has completed its upload. Historical validation records retain their original scope; current version authority is STABLE_1.2.36.json.
