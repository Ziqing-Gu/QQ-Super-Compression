from pathlib import Path
import hashlib,json,shutil

ROOT=Path(__file__).resolve().parent
STABLE=ROOT.parent/'QQSuperCompression-1.2.7-Stable'
DOC=STABLE/'docs/manuals/source-1.2.7'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
qa=json.loads((DOC/'QA-EN.json').read_text(encoding='utf-8-sig'))
assert sha(Path(qa['PDF']))==qa['SHA256']
qa['VisualReview']='passed'
qa['VisualReviewDetails']='All 35 pages reviewed; plot labels, dense pages, controls and Limiter pages checked. No clipping or overlap.'
(DOC/'QA-EN.json').write_text(json.dumps(qa,indent=2),encoding='utf-8')
shutil.copy2(Path(qa['PDF']),STABLE/'docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-English.pdf')
shutil.copytree(STABLE/'docs/manuals',ROOT/'docs/manuals',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.log','documentation-only.settings'))
ss=ROOT/'docs/screenshots/v1.2.7';ss.mkdir(parents=True,exist_ok=True)
for name in ('manual-dark.png','manual-light-dual.png','manual-limiter-light.png','manual-limiter-monitor-controls.png'):
    shutil.copy2(DOC/'assets'/name,ss/name)

for p in (ROOT/'.github/workflows').glob('*.yml'):
    s=p.read_text(encoding='utf-8').replace('1.2.6','1.2.7')
    if 'macos' in p.name:
        s=s.replace('QQSuperCompression_VST3 QQSCDynamicsCheck','QQSuperCompression_VST3 QQSCUnityCheck QQSCLimiterCheck')
        s=s.replace('      - name: Verify native dynamics\n        run: build-macos/QQSCDynamicsCheck_artefacts/Release/QQSCDynamicsCheck',
'''      - name: Verify native Limiter and unity monitor
        shell: bash
        run: |
          build-macos/QQSCLimiterCheck_artefacts/Release/QQSCLimiterCheck "$RUNNER_TEMP/qqsc-limiter-check"
          build-macos/QQSCUnityCheck_artefacts/Release/QQSCUnityCheck "$RUNNER_TEMP/qqsc-unity-check"''')
        s=s.replace('          vst3_binary=$(find', '          test "$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$vst3/Contents/Info.plist")" = "1.2.7"\n          vst3_binary=$(find')
        s=s.replace('          au_binary=$(find', '          test "$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$au/Contents/Info.plist")" = "1.2.7"\n          au_binary=$(find')
    p.write_text(s,encoding='utf-8')

old=(ROOT/'README.md').read_text(encoding='utf-8')
prefix=old.split('# QQ Super Compression 1.2.6 Stable',1)[0]
body='''# QQ Super Compression 1.2.7 Stable

**Classic / Super · Single / Upward / Dual · Limiter · TP / Peak · Headphone 1:1**

1.2.7 在原有动态处理后新增自适应最终 Ceiling，TP 默认开启，也可按采样峰值限幅。耳机 1:1 保留完整限幅结果，抵消 Output Gain 的直接音量变化；配合 MATCH 和 Bypass 可进行等响比较。中英文说明书各 35 页。

Version 1.2.7 adds an adaptive final Ceiling after the existing dynamics. TP is on by default, with sample-peak limiting also available. Headphone 1:1 preserves the complete limited result while cancelling Output Gain's direct level contribution. Use MATCH and Bypass for level-matched comparisons. Both manuals have 35 pages.

[下载 / Release 1.2.7](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/tag/v1.2.7) · [完整更新 / Release notes](RELEASE_NOTES_1.2.7.md) · [历史 / Changelog](CHANGELOG.md)

- [中文安装指南](docs/QQ-Super-Compression-1.2.7-INSTALL-CN.txt)
- [English installation guide](docs/QQ-Super-Compression-1.2.7-INSTALL-EN.txt)
- [中文用户手册 · 35 页](docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-Chinese.pdf)
- [English user manual · 35 pages](docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-English.pdf)

Windows x64 VST3 / macOS Apple Silicon VST3 / Intel VST3 / Universal 2 AU。Mac 最低 macOS 11，临时签名、未公证。Mac requires 11 or newer; bundles are ad-hoc signed, not notarized.

## Limiter、TP 与耳机监听 / Limiter, TP and monitoring

Limiter 支持两种算法、Single / Dual 和 ST / LR / MS。下压 Ratio 为 20:1 至 1000:1；上压仍可使用。Ceiling 为 -24 至 0 dB，默认 0，Alt 单击复位。TP 与 Ceiling 只在 Limiter 下生效。TP 表读取实际最终 L/R 输出，保留约 20 秒峰值，双击清零。

Limiter supports both algorithms, Single / Dual and ST / LR / MS. Downward Ratio spans 20:1 to 1000:1; upward processing remains available. Ceiling spans -24 to 0 dB, defaults to 0 and resets with Alt-click. TP and Ceiling act only in Limiter. The meter reads actual final L/R true peaks, with about 20 seconds of hold and double-click reset.

![Limiter / Light](docs/screenshots/v1.2.7/manual-limiter-light.png)

Output 旁的 LINK 联动下压阈值与输出。耳机开启后，数值及联动仍有效，MON = Ceiling - Output Gain；MATCH 比较原始输入与实际限幅、监听补偿后的输出。耳机不自动保证等响，需先播放有代表性的素材，再使用 MATCH。耳机状态随工程保存，独立于 A/B；Limiter、TP、Ceiling 与 Limiter LINK 则随 A/B 保存。

LINK beside Output connects the downward threshold and output. Headphones preserve those values and links, with MON = Ceiling - Output Gain. MATCH compares original input with actual limited, compensated listening output. Headphones alone do not guarantee equal loudness: play a representative passage before using MATCH. Headphones are saved with the project but independent of A/B; Limiter, TP, Ceiling and Limiter LINK are recalled by A/B.

![Headphone 1:1 / MON](docs/screenshots/v1.2.7/manual-limiter-monitor-controls.png)

## 上压与双压 / Upward and Dual

上压提升下门槛以上、上边界以下的有效部分。Single 用 Threshold / Range；Dual 用 UP / DOWN 阈值及两个 Ratio，达到 DOWN 后结束上压，高于 DOWN 时只做下压。两路分别 ON/OFF，Ratio LINK 按相对反向倍数联动。

Upward processing boosts eligible material above its lower gate and below the upper boundary. Single uses Threshold / Range; Dual uses UP / DOWN thresholds and two ratios. Up ends at DOWN, and only Down acts above it. Each branch has an independent switch; Ratio LINK preserves relative inverse changes.

![Single Up / Dark](docs/screenshots/v1.2.7/manual-dark.png)

![Dual / Light](docs/screenshots/v1.2.7/manual-light-dual.png)

## 延迟与兼容 / Latency and compatibility

Classic / Super 的已接受检测方式和默认 26 ms Lookahead 保持不变。最终峰值保护另需约 7-8.5 ms 固定缓冲，48 kHz 下约 8.17 ms；关闭 Limiter / TP 时仍保留相同总延迟。普通处理与 1.2.6 对齐延迟后的验证结果一致。本版不承诺任意信号零失真；细节及测试边界见说明书与发行说明。

The accepted Classic / Super detector and initial 26 ms Lookahead remain. Final peak protection adds about 7-8.5 ms of fixed buffering, about 8.17 ms at 48 kHz; the same total latency is retained with Limiter / TP off. Validated normal processing matches 1.2.6 after delay alignment. This is not a universal zero-distortion guarantee; see the manuals and release notes for scope.

正式名称、插件身份和已有参数 ID 保留。旧工程缺少新参数时，Limiter 默认关闭；已保存的新工程恢复自身状态。关闭 Limiter 恢复普通 Ratio、边界与 Output；共享 Makeup、Mix、Input 保留当前值。

The formal name, plugin identity and existing parameter IDs remain. Old states without new parameters load with Limiter off; newer projects restore their saved state. Leaving Limiter restores normal Ratio, boundaries and Output, while shared Makeup, Mix and Input keep their current values.

## 构建与验证 / Build and validation

JUCE 8.0.15、CMake 3.22+；Windows 使用 VS 2022 x64。见 [构建方法](REPRODUCE.md)、[Stable 记录](STABLE_1.2.7.md) 和 [Windows 验证](VERIFICATION.md)。Mac 工作流对原生 VST3 执行 Ceiling / 耳机 / MATCH 回归，并验证架构、签名与 AU。

Use JUCE 8.0.15 and CMake 3.22+, with VS 2022 x64 on Windows. See the build, Stable and verification records above. Mac workflows run native Ceiling / headphone / MATCH regression checks, verify architectures and signatures, and validate the AU.

历史版本文档保留供追溯；当前 1.2.7 双语手册优先。非商业源码共享许可保持不变，第三方声明保留。Historical documents remain for reference; the 1.2.7 manuals describe the current release. The non-commercial source-share license and third-party notices are retained.
'''
(ROOT/'README.md').write_text(prefix+body,encoding='utf-8')
notes=(ROOT/'RELEASE_NOTES_1.2.7.md').read_text(encoding='utf-8')
notes=notes.replace('中文手册继续修订并提交确认，英文新版尚未制作。本次仅 Stable / Plan B，没有公开发布。',
'''中英文说明书各 35 页，保留配色、字体与 Classic / Super 曲线，新增完整 Limiter 操作章节。提供 Windows x64 VST3、Mac Apple Silicon / Intel VST3 与 Universal 2 AU；macOS 11+，临时签名、未经公证。

正式插件身份、已有参数 ID 保留。旧工程缺少新参数时 Limiter 默认关闭；现有普通处理经延迟对齐验证与 1.2.6 一致。最终保护额外固定缓冲约 7-8.5 ms，48 kHz 下约 8.17 ms，开关 Limiter / TP 时保持不变。新版监听不是自动持续响度控制；MATCH 仍需有效音频积累。

已接受的 Windows 回归包含 120 组普通处理逐位一致检查、12 组实际 Limiter VST3 对照、18 组耳机增益关系与 12 组 MATCH 对照。对应误差上界和条件见 VERIFICATION.md。TP 保留少量重建余量，不要求每次读数恰好贴住 Ceiling，也不承诺任意音频零失真。''')
notes=notes.replace('The verified Windows binary is promoted unchanged. Chinese manual revision is under review; no matching new English manual or public release is included in this stage.',
'''The verified Windows binary is promoted unchanged. Matching Chinese and English manuals each contain 35 pages, retaining the established design and Classic / Super curve while adding the full Limiter workflow. Packages cover Windows x64 VST3, Mac Apple Silicon / Intel VST3 and Universal 2 AU. Mac requires 11 or newer; bundles are ad-hoc signed, not notarized.

The formal identity and existing parameter IDs are retained. Old states without new parameters load with Limiter off. Validated normal audio matches 1.2.6 after delay alignment. Final protection adds about 7-8.5 ms of fixed buffering, approximately 8.17 ms at 48 kHz, retained across Limiter / TP switching. Monitoring is not continuous automatic gain control; MATCH still requires valid audio measurement.

Accepted Windows checks include 120 bit-identical normal-processing cases, 12 actual Limiter VST3 comparisons, 18 headphone gain-relationship cases and 12 MATCH cases. See VERIFICATION.md for bounds and conditions. TP retains a small reconstruction margin; readings need not exactly touch Ceiling, and arbitrary audio is not guaranteed distortion-free.''')
(ROOT/'RELEASE_NOTES_1.2.7.md').write_text(notes,encoding='utf-8')
changelog=(ROOT/'CHANGELOG.md').read_text(encoding='utf-8')
(ROOT/'CHANGELOG.md').write_text(notes+'\n\n---\n\n'+changelog,encoding='utf-8')

cn='''QQ Super Compression 1.2.7 - 安装与更新

Win/：解压 Windows x64 ZIP，将完整 QQ Super Compression.vst3 文件夹复制到 C:\\Program Files\\Common Files\\VST3。覆盖前保存工程并完全退出所有 DAW / 音频宿主，再打开宿主重新扫描。不要只复制 bundle 里面的二进制文件。

Mac/：选择与宿主进程架构一致的 Apple Silicon 或 Intel VST3；Logic 使用 Universal 2 AU。最低 macOS 11。完整 VST3 bundle 放到 /Library/Audio/Plug-Ins/VST3，AU 放到 /Library/Audio/Plug-Ins/Components；也可使用对应的用户 Library 目录。复制前关闭宿主，随后重新扫描。Mac 包为临时签名、未经 Apple 公证；从官方 Release 下载并按系统提示批准。

1.2.7 新增 Limiter、最终 Ceiling、默认打开且可关闭的 TP 保护、耳机 1:1 监听与对应 MATCH。Limiter 下压 Ratio 20:1 至 1000:1，Ceiling -24 至 0 dB，默认 0，Alt 单击复位。TP 保持约 20 秒，双击清零。耳机保留完整限幅效果，在最后抵消可见 Output Gain，MON 显示换算后的上限。

耳机等响比较：打开 Limiter 和耳机，关闭 SC LISTEN，声道 MONITOR 选 ALL。播放代表性片段建立测量，点击 MATCH，再用插件 Bypass 比较原声。结束后关闭耳机，重新检查正常输出和 TP。耳机状态独立于 A/B，Limiter / TP / Ceiling 随 A/B 保存。

26 ms 仍是初始 Lookahead。最终峰值保护另有约 7-8.5 ms 固定缓冲（48 kHz 下约 8.17 ms）；关闭 Limiter / TP 也保持同样延迟，由宿主补偿。正式插件身份保留；旧工程没有新参数时 Limiter 默认关闭。旧版 Preview / QQ dB Compression 实例仍需手动替换。

两本 35 页 PDF 均对应本版。Classic / Super、上压、Dual、LINK、MATCH 和 Lookahead 详见手册；Limiter 从第 25 页开始。先保存工程再更新，保留旧版安装包供必要时回退。

禁止商业使用。完整对应源代码及许可证：https://github.com/Ziqing-Gu/QQ-Super-Compression/tree/v1.2.7
官方下载：https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/tag/v1.2.7
'''
en='''QQ Super Compression 1.2.7 - Installation and update

Win/: Extract the Windows x64 ZIP. Copy the complete QQ Super Compression.vst3 folder to C:\\Program Files\\Common Files\\VST3. Save your session and close every DAW/audio host before replacing it, then reopen and rescan. Do not copy only the binary inside the bundle.

Mac/: Choose Apple Silicon or Intel VST3 to match the host process architecture; use Universal 2 AU in Logic. Requires macOS 11 or newer. Copy the complete VST3 bundle to /Library/Audio/Plug-Ins/VST3 or the AU to /Library/Audio/Plug-Ins/Components, or their user Library equivalents. Close the host before copying, then rescan. Mac bundles are ad-hoc signed and not Apple-notarized; download from the official Release and follow system approval prompts.

1.2.7 adds Limiter, final Ceiling, switchable TP protection enabled by default, headphone 1:1 monitoring and its MATCH workflow. Limiter downward Ratio spans 20:1 to 1000:1. Ceiling spans -24 to 0 dB, defaults to 0 and resets with Alt-click. TP holds for about 20 seconds and resets on double-click. Headphones preserve the complete limited result and cancel visible Output Gain at the end; MON shows the converted listening limit.

To compare at similar loudness: enable Limiter and headphones, disable SC LISTEN, and select ALL in channel MONITOR. Play a representative passage to establish measurements, click MATCH, then use plugin Bypass to compare the original. Finish by switching headphones off and rechecking normal output and TP. Headphones remain independent of A/B; Limiter / TP / Ceiling are recalled with A/B.

The initial Lookahead remains 26 ms. Final peak protection adds about 7-8.5 ms of fixed buffering (about 8.17 ms at 48 kHz), retained with Limiter / TP off for host compensation. The formal plugin identity is unchanged. Old states without new parameters load with Limiter off. Older Preview / QQ dB Compression instances still need manual replacement.

Both 35-page PDF manuals match this version. They cover Classic / Super, upward, Dual, LINK, MATCH and Lookahead; Limiter starts on page 25. Save sessions before updating and retain an older package if rollback is needed.

Non-commercial use only. Complete corresponding source and license: https://github.com/Ziqing-Gu/QQ-Super-Compression/tree/v1.2.7
Official download: https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/tag/v1.2.7
'''
for lang,s in [('CN',cn),('EN',en)]:
    (ROOT/f'docs/QQ-Super-Compression-1.2.7-INSTALL-{lang}.txt').write_text(s,encoding='utf-8')
st=(ROOT/'STABLE_1.2.7.md').read_text(encoding='utf-8')
st=st.replace('35 页新版中文说明书已完成修订并提交用户审阅','35 页新版中文说明书已获用户确认，并完成对应的 35 页英文版')
st=st.replace('没有与新版内容对应的英文手册','中英文手册均对应本版')
st=st.replace('软件 Stable 不表示尚待用户审阅的说明书已经定稿。','')
st=st.replace('未执行 Plan C/D、公开发布、Mac 构建或桌面交付。','用户已授权 Plan C/D：本源码用于正式 GitHub 发布、Mac 构建与桌面实物交付；最终完成证据保存在 D 盘 Outputs 的发布核验目录。')
(ROOT/'STABLE_1.2.7.md').write_text(st,encoding='utf-8')
readme=ROOT/'docs/manuals/source-1.2.7/README.md'
s=readme.read_text(encoding='utf-8').replace('状态：35 页中文审阅稿；等待用户确认，没有与本稿对应的英文版。软件本体已设为 Stable，文档审阅状态独立于软件状态。','状态：中文已确认；对应英文版已完成。两本手册均为 35 页，用于 1.2.7 Stable 发布。')
s+='\n## English edition\n\nRun `prepare_english.py`, then `build_manual_1_2_7_en.py` with the same assets/fonts and an English output directory. `english_translations.json` holds reviewed new/changed strings; unchanged text reuses the preserved matched bilingual 1.2.6 sources. `verify_english.py` checks 35 pages, bookmarks, key content, text bounds and untranslated text. All pages were visually reviewed; see QA-EN.json.\n'
readme.write_text(s,encoding='utf-8')
(ROOT/'docs/plan_d_release_checklist.md').write_text('''# 1.2.7 release checklist

1. Preserve accepted Windows binary SHA-256 292F588A9E91FF8EC5BBD609BDFB3E9CD6165DDA1D2B22C2C28427AE2B2DAAC6. Production Source/ and Assets/ must match the accepted Stable source exactly.
2. Include matching 35-page Chinese and English manuals, their sources/screenshots, bilingual release notes and install guides. Retain author preface, license and third-party notices.
3. Freeze and hash a new full Plan B snapshot under D:/备份文件/Vibe Coding/QQ Super Compression/源代码, including exact JUCE 8.0.15 and intended GitHub source.
4. Fetch and check remote main before a normal push. Preserve history. Verify remote commit content, then build three Mac jobs from that exact commit.
5. Verify native Limiter/Ceiling/headphone/MATCH tests on both architectures, AU validation, ZIP integrity, versions, architecture, minimum OS and signature state.
6. Tag the verified source v1.2.7 and publish eight Release assets: two 35-page PDF manuals, two install TXT guides, Windows x64 ZIP, Apple Silicon VST3 ZIP, Intel VST3 ZIP, Universal 2 AU ZIP. Verify asset hashes and Latest status.
7. Put exactly those eight files in the desktop delivery folder: four documents at root, one ZIP in Win/, three ZIPs in Mac/. Deliver real files, not shortcuts; keep proof and logs in D:/Codex/Outputs outside this folder.
8. Refresh Plan B if release-bound source/docs change. Keep previous snapshots immutable.
''',encoding='utf-8')
with (ROOT/'.gitignore').open('a',encoding='utf-8') as f:f.write('\n__pycache__/\n*.pyc\n*.log\ndocumentation-only.settings\n')
print('Release source/docs prepared; author preface, license and prior Git history retained.')
