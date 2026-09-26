from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parent
accepted = Path(r'D:\Codex\Workspaces\QQSuperCompression-1.2.5-ThresholdFloor')
candidate = Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.5 Candidate-Classic90\Verification\CANDIDATE.json')
record = json.loads(candidate.read_text(encoding='utf-8-sig'))
for item in record['SourceFiles']:
    assert hashlib.sha256((root / 'Source' / item['Path']).read_bytes()).hexdigest().upper() == item['SHA256']
for folder in ('Source', 'Assets', 'tests'):
    for p in (accepted / folder).rglob('*'):
        if p.is_file():
            assert p.read_bytes() == (root / p.relative_to(accepted)).read_bytes()
assert (root / 'CMakeLists.txt').read_bytes() == (accepted / 'CMakeLists.txt').read_bytes()

p = root / 'README.md'
s = p.read_text(encoding='utf-8-sig')
start = s.index('# QQ Super Compression 1.2.5 Candidate')
end = s.index('\n## ', start + 2)
s = s[:start] + '''# QQ Super Compression 1.2.5 Stable

2026-09-27 经用户确认，将 Classic −90 dB 修订版设为 Stable。保留 Classic / Super、完整结果 A/B 淡变、记忆的 I/O LINK 和算法选择、取消绝对检测门限且支持 ±120 dB 补偿的 MATCH。声音与已安装、已试听的 Classic90 验证版完全相同。

当前说明和兼容边界见 [1.2.5 Stable 记录](STABLE_1.2.5.md) 与 [发行说明](RELEASE_NOTES_1.2.5.md)。Plan B 保存完整源码及精确 JUCE 依赖。后续前瞻实验在独立工作区进行，不纳入此稳定基线；分段压缩、远程发布和桌面打包尚未执行。

User-approved Stable as of 2026-09-27. Audio is identical to the auditioned Classic90 revision. See [Stable record](STABLE_1.2.5.md) and [release notes](RELEASE_NOTES_1.2.5.md). Plan B preserves full source and exact JUCE dependencies. The lookahead experiment is separate; no multiband changes or new remote release are included. Older documentation below is historical.
''' + s[end:]
p.write_text(s, encoding='utf-8')

release = '''# QQ Super Compression 1.2.5 Stable — 2026-09-27

用户明确批准当前 Classic90 版本为 Stable，并要求先完成 Plan B，再开展独立前瞻实验。此次稳定化只更新记录与复现脚本；生产源码、资源、参数、版本号和二进制均保留已试听版本。

- Classic 有限阈值下限 −90 dB；Super 保留 −inf，Range OFF 保持无上界。旧存储值在 Classic 中按有效下限解释，未编辑值可在 Super 中继续使用。
- Classic / Super 支持工程状态、A/B 和最后一次明确 UI 选择的记忆；首次 Classic。I/O LINK 首次 ON，并记忆最后一次明确操作，工程状态优先。
- 相同 Input、检测和延迟配置下，A/B 对完整结果做 20 ms S 形淡变；直接算法按钮仍为 10 ms。不同输入、侧链或延迟配置仍使用已有重配置路径。
- MATCH / Makeup 支持 ±120 dB，取消 −70 LUFS 绝对门限，保留 K 加权及相对门控；真正零信号和缺失测量不产生补偿。该匹配测量不等同于严格 EBU R128 Integrated LUFS。
- 插件身份、参数 ID 和顺序不变。旧工程的归一化 Makeup 自动化须核对，因为参数范围扩大。

已知设计边界：当前未来窗口峰值检测会在电平突变前产生预影响；比窗口峰值小的局部声音可能被压到阈值以下。Display 是历史输入与检测电平的投影，不能替代实际输出录音。用户知悉此行为后将本版固定为稳定对照；改进方案不属于本版。稳态正弦低谐波结果不构成任意信号下零失真的保证。

## English

User-approved Stable baseline, preserving the auditioned Classic90 audio and binary without DSP changes. Classic has a finite −90 dB minimum; Super retains −inf. Range OFF remains unbounded. Algorithm choice and initially enabled I/O LINK remember explicit user choices while saved project state takes priority.

Complete-result A/B uses a 20 ms smoothstep crossfade when input, detector and timing configuration match; direct algorithm switching remains 10 ms. MATCH removes the absolute loudness gate and supports ±120 dB Makeup, retaining K weighting and relative gating. Existing normalized Makeup automation requires review after the range expansion.

The future-window detector still has pre-influence around level changes and can attenuate smaller local material below the threshold. Display is a historical projection rather than a captured output waveform. This known behavior is retained in the approved baseline. A separate experiment will investigate alternatives; it is not part of this Stable. Low steady-tone harmonics are not an unconditional zero-distortion claim.
'''
(root / 'RELEASE_NOTES_1.2.5.md').write_text(release, encoding='utf-8')
stable = '''# QQ Super Compression 1.2.5 Stable / Plan B

Approval: user explicitly requested Stable and Plan B on 2026-09-27 before any new lookahead experiment.

- Source: `D:\\Codex\\Workspaces\\QQSuperCompression-1.2.5-Stable`
- Verified binary build: `D:\\Codex\\Temp\\QQSC-1.2.5-ThresholdFloor`
- Stable output: `D:\\Codex\\Outputs\\QQ Super Compression\\1.2.5 Stable`
- System: `C:\\Program Files\\Common Files\\VST3\\QQ Super Compression.vst3`
- Binary SHA-256: `A88D8781740058C2E74D3C90E880CF59C48313278D7407011F8D2B9DCC014294`
- Version: 1.2.5; pinned dependency: JUCE 8.0.15; existing Qscp identity retained.

Production Source, Assets, tests and CMakeLists.txt are byte-identical to the accepted Classic90 checkout. Stable designation is a documentation promotion, not a binary rebuild. The installed bundle is checked against the accepted build; no overwrite is needed when hashes already match.

Validation: fresh native DSP suite and actual installed VST3 reference suite (120 cases); the unchanged editor retains the previously passing offscreen UI checks. Test logs, input hashes and completion records live in the output Verification directory. Only COMPLETE in `Verification/PLAN_B_COMPLETION.json` certifies finished Plan B.

Plan B is a new immutable snapshot under `D:\\备份文件\\Vibe Coding\\QQ Super Compression\\源代码`, containing the complete source, exact JUCE 8.0.15 tree, manifests and validation evidence. Use `build-stable.cmd` to reproduce on Windows with VS 2022 Build Tools and CMake. It locates the bundled dependency relative to the snapshot, or accepts QQSC_JUCE_PATH.

See RELEASE_NOTES_1.2.5.md for behavior and compatibility boundaries. The original manuals remain unchanged. No Plan C/D, desktop handoff or multiband changes are included. New detector experiments must use a separate workspace and must not overwrite this baseline or the system Stable.
'''
(root / 'STABLE_1.2.5.md').write_text(stable, encoding='utf-8')
p = root / 'CHANGELOG.md'
p.write_text(release + '\n---\n\n' + p.read_text(encoding='utf-8-sig'), encoding='utf-8')
p = root / 'AI_DEVELOPMENT_HANDOFF.md'
p.write_text('# Current: 1.2.5 Stable — 2026-09-27\n\nFollow [STABLE_1.2.5.md](STABLE_1.2.5.md). User approved Classic90 as the unchanged Stable baseline and Plan B. Subsequent lookahead work must be isolated. No desktop, multiband or publication work is authorized in this stage. The existing installed binary is the Stable binary; verify its hash before any proposed replacement.\n\n---\n\n# Historical candidate checkpoint\n\n' + p.read_text(encoding='utf-8-sig'), encoding='utf-8')
for name in ('TRY_1.2.5.md', 'CANDIDATE_CHECKPOINT_1.2.5.md'):
    p = root / name
    p.write_text('> Historical candidate record, superseded by [STABLE_1.2.5.md](STABLE_1.2.5.md) on 2026-09-27. The auditioned binary was promoted unchanged.\n\n' + p.read_text(encoding='utf-8-sig'), encoding='utf-8')
print('Stable documentation ready; production, resources and test code unchanged.')
