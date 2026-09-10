from pathlib import Path
import hashlib

root = Path(__file__).resolve().parents[1]
if (root / 'STABLE_1.2.0.md').exists():
    raise SystemExit('Historical candidate-document generator: do not overwrite the Stable release records.')
binary = Path(r'D:\Codex\Temp\QQSuperCompression-1.2.0-UpDown\QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3\Contents\x86_64-win\QQ Super Compression.vst3')
digest = hashlib.sha256(binary.read_bytes()).hexdigest().upper()

documents = {
'EXPERIMENT_1.2.0.md': r'''# QQ Super Compression 1.2.0 Rev3 — current experiment

2026-09-10. Windows x64, JUCE 8.0.15. Plug-in version remains 1.2.0;
state schema is 13. This revision supersedes Rev2's mistaken Dual UP direction.
Source: `D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown`.
Output: `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`.

## Confirmed processing definition

- Single: process only above Threshold and below a finite Range. At/below
  Threshold or at/above finite Range, dynamic gain is unity. Range defaults to
  OFF, an independent unbounded endpoint; saved finite Range0 remains finite.
- Dual: UP is a lower enabling gate. At/below UP, leave the audio unchanged.
  Above UP and below DOWN, raise eligible material. At DOWN, gain is unity.
  Above DOWN, stop upward processing and use Down Ratio only. The two gain
  branches do not overlap or cascade. UP=-inf opens the gate for nonzero audio;
  it does not disable upward processing.
- New Dual thresholds remain UP=-inf, DOWN=0dB. All 15 Ratio defaults (Single,
  UP, DOWN across ST/LR/MS) are now1:1. Existing saved Ratio numbers are retained.
- Single Ratio range1/32..32; Dual UP1/32..1; Dual DOWN1..32. Boundaries push
  their partner on collision. Equal boundaries disable the entire dynamic stage.
- Keep the approved future-window peak/lookahead detector, delayed carrier,
  0ms-only oversampling and accepted downward law. No attack/release was added.

## Gain law shared by audio and Display

Let p be the linear detector peak, T the lower gate and A the upward anchor.
Upward gain inside the eligible region is `1 / (r + (1-r)*p/A)`, with r<=1.
For Single, A is finite Range or1 when Range is OFF. For Dual, A is DOWN;
UP is only the lower gate. Gain tends to unity as p reaches A and is bounded
by1/r. A detector10dB below A at Ratio1:8 receives +7.92198dB, provided p>T.

The retained downward law is `(1+(r-1)*T)/(1+(r-1)*p)` above its threshold.
Single uses Threshold as T; Dual uses DOWN. The existing detector caps p at1.
Do not restore Rev2's `p<UP` activation or use UP as Dual's upper anchor.

## Ratio LINK

- A small LINK between the UP/DOWN columns starts ON on first use. ST uses
  36x17 at(278,644); LR/MS uses30x14 at(281,636). It is hidden in Single.
- Coupling preserves each pair's product UP*DOWN. If DOWN changes D0->D1,
  UP becomes U0*D0/D1, and conversely for UP edits. Starting4 and1/2, changing
  DOWN to8 produces UP1/4, not1/8. Toggling LINK never changes either ratio.
- Shared range limits stop the edit before any linked member exceeds its
  range; reversing resumes travel. Drag, fine drag, Alt reset and numeric
  commit share this rule. A linked Alt reset also respects the shared limits.
- The existing LR/MS domain LINK remains separate: its driven branch retains
  the additive relative delta, while each UP/DOWN pair preserves its own product.
- Explicit editor edits record host gestures for all affected parameters.
  Host automation and state restores load independent values without recoupling.
- The last user click is stored in the existing UI settings for new instances.
  Project state includes dualRatioLink and takes precedence on restoration.
  Reopening an editor keeps its instance state. LINK is excluded from sound A/B.

## Layout and illumination

- In ST/Single, the five primary dials use identical94x94 logical drawing areas
  and centres80,296,512,728,944 (216px spacing), at the same height. All three
  themes use this common geometry; paired LR/MS and Dual ratios remain smaller.
- SINGLE/DUAL moves to(380,620,60,21), closer to Ratio. Full-length boundary
  rails and shared Display coordinates from Rev2 remain unchanged.
- Single Ratio lights outward from1:1 at the centre; upward ratios light left,
  downward ratios right. Dual UP lights from the right endpoint toward the left;
  Dual DOWN retains left-to-right lighting. Neutral has no active arc and keeps
  its position indicator. Renderer caches include the light origin, so switching
  modes refreshes the arc even when the normalised position is unchanged.
- Preserve480-point history, bounded signed shading and one final Output path.

## Validation and scope

See `VERIFICATION_1.2.0.md` for the actual DSP, editor, LINK/settings and rendering
results. Preview images use synthetic history/meter fixtures for layout checks.
Steady-tone low harmonic measurements do not certify hard boundary crossings:
the lower UP gate and finite Single cutoff intentionally remain abrupt. The
existing0ms colouring mode is separate from the lookahead transparency reference.
Old normalised Single Ratio automation has a different mapping from1.1.9's
narrower range. Experiment in new instances and retain the1.1.9 rollback.

The1.1.9 workspace, frozen backups and multiband project are untouched. Rev2
source checkpoint: `D:\Codex\Archives\QQSuperCompression-1.2.0-Rev2-before-Rev3-20260910`.
No Stable promotion, formal Plan B/C/D, GitHub publication or macOS release.
''',
'TRY_1.2.0_zh.md': r'''# QQ Super Compression 1.2.0 Rev3 试听说明

交付：`D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`。
插件版本号仍为1.2.0。请重新打开宿主，用新实例检查新的默认值。

- 所有 Ratio 默认1:1。旧工程已保存的数值继续保留。
- SINGLE：Threshold以上、有限Range以下生效。Range默认OFF，表示没有上截止。
- DUAL：UP是下限门槛；UP以下保持原音量，UP与DOWN之间提升，超过DOWN只做向下压缩。UP=-inf表示打开下限，不是关闭向上处理。两阈值重合时整级停止。
- Ratio亮弧从1:1出发。单压向左右展开；双压UP由右向左、DOWN由左向右。
- Light、Dark、Classic的五个主旋钮已统一大小和中心间距；单双压按钮更靠近Ratio。
- 双压Ratio中间的小LINK首次默认开启，之后记住上次选择；已保存工程优先恢复工程内状态。LR/MS的LINK更小。
- LINK按相反倍数相对联动：Down从1到2，Up从1到1/2；原本Down=4、Up=1/2，Down调到8时Up变为1/4。开关LINK不跳值，到范围边界共同停止。
- Ratio LINK与右侧LR/MS声道LINK是两种独立联动。Alt重置、Shift微调和数值输入继续可用；联动开启时也遵守共同边界。

快速检查向上处理：Dual、UP=-50dB、DOWN=-30dB，输入-40dB正弦，Mix100%、各增益0、Lookahead26ms。Up Ratio从1:1调到1:8、1:32，实测提升约0、7.92、9.43dB；输入降到UP以下应恢复原音量。

有限边界采用已确认的硬截止；跨边界瞬间与稳态正弦是不同测试。0ms保留原有染色/过采样逻辑。
安装前的完整版本在本目录 `Rollback - previous installed version`。替换插件前请完全退出宿主。
本次为Windows本地实验版，多段版本未修改，尚未晋升稳定版或发布GitHub。
''',
'VERIFICATION_1.2.0.md': r'''# QQ Super Compression 1.2.0 Rev3 — verified results

2026-09-10. Windows x64 Release, JUCE8.0.15/VS2022, state schema13.
Current revision only; earlier Rev2 results are retained in its separate output.

## Actual compiled audio processor

- All15 Ratio parameters and decoded host defaults equal1:1. First-use Dual
  Ratio LINK is ON. Existing finite Range0 and saved Ratio/domain banks survive.
- Five-domain activation: at/below UP unity, above UP/below DOWN boost, at DOWN
  unity, above DOWN attenuation independent of Up Ratio. UP=-inf opens the gate.
- Real400Hz input-40dB, UP=-50dB, DOWN=-30dB,48kHz/26ms: actual visible Up knob
  1 ->0dB gain;1/8 ->+7.92198dB;1/32 ->+9.43206dB. The parameter and output agree.
- With the input10dB below DOWN and20dB-wide UP/DOWN intervals at DOWN=-20/-40/
  -60dB, each Ratio1:8 result gives+7.92198dB. Sub-UP audio is not boosted.
- Original599.9Hz full-scale regression:44.1/48/96kHz and10/26/80ms, zero unity
  bursts, maximum sample difference5.96046e-8 versus the retained1.1.9 engine.
- Ten boundary pairs: collision pushing, separation on reversal, equality
  bypass. Actual LR and pure MS processing, signed Mix/Hold, Mix0, absent EXT
  key unity, delayed bypass/PDC,64/511-sample block parity (max error0) pass.
- A/B and host-state roundtrip pass. Schema10 missing fields get explicit new
  defaults on a dirty instance; schema11 finite Range0 and saved banks remain.

Steady float-output H2-H7 THD relative to fundamental, transients excluded:

| Case | Frequency/lookahead | THD |
|---|---|---|
| Single Down |400Hz/26ms|-157.987dB|
| Single Up |400Hz/26ms|-160.312dB|
| Dual Up |400Hz/26ms|-166.398dB|
| Dual Down |400Hz/26ms|-169.571dB|
| Dual Up, strongest Ratio |20Hz/80ms|-168.366dB|

These are specific steady-carrier results, not an unconditional zero-distortion
claim. Confirmed hard gates retain boundary jumps; the Single fixture at
Threshold-40/Range-6 measures12.4926dB downward exit/16.9261dB upward entry.

## Actual editor and settings

- Five domains tested through real control gestures: default reciprocal pair,
  arbitrary product2 pair, both edit directions, actual numeric commits, Alt
  resets, shared limits/reversal, toggle-without-jump, and existing domain LINK.
- Host parameter listeners see balanced gestures for the source and companions;
  switching Single/Dual during a drag closes the old attachment correctly.
- Isolated D-drive settings file verifies first-use ON, last choice across new
  instances, saved project precedence and repeated editor reopen. User settings
  were not changed by validation.
- All three production themes render exact94px ST/Single primary drawing
  areas,216px centre spacing, matching Y positions. ST LINK36x17, LR/MS30x14,
  centred between UP/DOWN columns. Default/minimum1008x672 snapshots inspected.
- Warm and Dark actual material pixels tested across3 origins x5 positions:
  only the span from neutral to current value lights. Classic left/right arcs
  and neutral state inspected in production editor renders. All five domains'
  Single/Dual light origins and mode reattachment pass.
- Full-length fader/Display coordinate alignment, input trim, stopped ST/LR/MS
  changes, real mouse-down without jumps, signed meter and480-point budget pass.
- Existing material cache tests and all ten inherited core/domain/sidechain/
  Display source/mathematical checks pass.

240 changing-boundary LR frames, cached Display projection + software paint:

| Mode | Median | p95 |
|---|---|---|
| Single |4.207ms|5.140ms|
| Dual |4.440ms|5.805ms|

This is a same-machine offscreen measurement, not total DAW UI timing. Preview
PNGs contain synthetic graph/meter fixtures; they are not live audio captures.
JUCE's module helper loads the built VST3 and generates version metadata; no
third-party full VST3 validator or user listening acceptance is claimed.

## Artifact identity

Binary SHA256: `BINARY_HASH`.
Output: `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`.
`Verification/INSTALLATION.json` records host-closure check, installed status,
and full build/output/system bundle parity. The ZIP is separately checked by
reading every bundle entry and hashing its uncompressed contents.
'''
}
for name, text in documents.items():
    (root / name).write_text(text.replace('BINARY_HASH', digest), encoding='utf-8')

current = r'''# Current candidate — 1.2.0 Rev3 (schema13)

2026-09-10. Windows Release build and compiled DSP/editor/LINK/settings tests
pass. See `EXPERIMENT_1.2.0.md` for current semantics and `VERIFICATION_1.2.0.md`
for evidence. Earlier headings below are historical, not the current spec.

- UP is a lower gate; boost only UP<p<DOWN, down-only above DOWN. UP=-inf opens
  the gate. The previous Rev2 below-UP activation was wrong and is superseded.
- All15 Ratio defaults/reset targets are1. Existing stored numbers stay intact.
- Dual Ratio LINK preserves UP*DOWN, defaults ON, remembers last click for new
  instances and respects saved project state. It is independent of domain LINK.
- Three themes share94px primary dials/216px spacing. Ratio arcs start at unity;
  Single origin.5, Dual UP origin1, Dual DOWN origin0. Mode button(380,620,60,21);
  Ratio LINK ST(278,644,36,17), LR/MS(281,636,30,14).
- Source `D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown`; build remains
  `D:\Codex\Temp\QQSuperCompression-1.2.0-UpDown`; output
  `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`.
- Rev2 source checkpoint `D:\Codex\Archives\QQSuperCompression-1.2.0-Rev2-before-Rev3-20260910`.
- Build targets QQSuperCompression_VST3, QQSCVisualCheck, QQSCDynamicsCheck.
  Actual installation status/hashes: Rev3 output `Verification/INSTALLATION.json`.
  No Stable promotion, PlanB/C/D, remote publication, macOS or multiband changes.

## Historical revisions

'''
for name in ['AI_DEVELOPMENT_HANDOFF.md', 'CODEX_BUILD.md', 'CHANGELOG.md']:
    p = root / name
    text = p.read_text(encoding='utf-8-sig')
    if not text.startswith('# Current candidate — 1.2.0 Rev3'):
        p.write_text(current + text, encoding='utf-8')
print('Updated current Rev3 docs. Binary SHA256:', digest)
