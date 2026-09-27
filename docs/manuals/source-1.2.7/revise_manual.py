"""Create the Chinese 1.2.7 revision from the retained 1.2.6 curve draft."""
from pathlib import Path
HERE=Path(__file__).resolve().parent
base=HERE.parent/'baseline-1.2.6/source-1.2.6-curve/build_manual_1_2_6_zh.py'
s=base.read_text(encoding='utf-8').replace('1.2.6','1.2.7')
def replace(old,new):
    global s
    assert old in s,old[:100]
    s=s.replace(old,new)

replace("    # Keep the other 27 published pages exactly, including their font subsets,\n"+s.split("    # Keep the other 27 published pages exactly, including their font subsets,\n",1)[1].split("    doc=PdfReader(a.output/name);assert len(doc.pages)==28",1)[0],"")
replace('assert len(doc.pages)==28','assert len(doc.pages)==35')
replace("c.setAuthor('Qing Audio');c.setSubject('操作、混音与侧链使用指南')", "c.setAuthor('Qing Audio');c.setSubject('动态处理、Limiter、等响试听与侧链使用指南')")
replace("para('Classic 按固定 dB 压缩比控制动态，Super 保留原有曲线。两种算法都支持单压、门槛以上的向上提升，以及分别设置上、下压的双压模式。',9.6)",
        "para('Classic 按固定 dB 压缩比控制动态，Super 保留原有曲线。两种算法都支持单压、上压与双压；Limiter 加入最终峰值保护，并提供 TP / Peak 选择与耳机 1:1 等响试听。',9.6)")
replace("('从 1:1 开始调整','所有 Ratio 默认 1:1，此时不改变动态。先试向下压缩时，把 Ratio 调高，并让 Range 保持 OFF；上压与双压见第 8-11 页。')", "('从 1:1 开始调整','普通压缩的 Ratio 默认 1:1。先试向下压缩时，把 Ratio 调高，并让 Range 保持 OFF；上压与双压见第 8-11 页，Limiter 见第 25-31 页。')")
replace("['混音实例、参数速查与排错','24-28']", "['混音实例','24'],['Limiter、TP / Ceiling 与耳机试听','25-31'],['参数速查与排错','32-35']")
replace("['右上角','切换 Classic / Super 算法、侧链、主题、A/B 与 Bypass。']", "['顶部按钮','开关 Limiter，选择 Classic / Super、侧链、主题、A/B 与 Bypass。']")
replace("para('单压以 1:1 为中点：", "para('普通压缩的单压以 1:1 为中点：")
replace("单压范围为 1:1000 至 1000:1；所有 Ratio 默认 1:1。数值很大也不表示插件成为砖墙限幅器。", "普通单压范围为 1:1000 至 1000:1，Ratio 默认 1:1。最终峰值保护由 Limiter 的 Ceiling 负责；仅把普通压缩的 Ratio 调大，不等于启用 Limiter。")
replace("note('LINK 不等于 Match','Input 可能改变检测电平与压缩量，Output 只调整最终输出。因此等量反向变化不保证处理前后等响。想比较动态，请继续使用 Match 或手动补偿。')", "note('Limiter 使用另一组联动','普通 Input / Output LINK 只做增益的反向调整，不能替代 Match。开启 Limiter 后它隐藏并停止联动；Output 旁的 LINK 改为控制下压阈值与输出，见第 26 页。')")
replace("['Output Gain','调整最终混合输出；范围 -24 至 +24 dB。']", "['Output Gain','普通压缩：-24 至 +24 dB；Limiter：-120 至 +120 dB。耳机监听见第 29-30 页。']")
replace("step(2,'比较 Dry 与 Wet','Match 对齐 Dry 与补偿前 Wet。提高 Makeup 会影响 Wet；Mix 小于 100% 时，最终混合结果仍需听音复查。')", "step(2,'按当前监听方式比较','普通模式比较 Dry 与补偿前 Wet；Limiter 耳机模式比较原始输入与实际限幅后的监听输出。提高 Makeup 会改变 Wet，见第 30 页。')")
replace("note('大幅补偿后，重新检查输出','Match 是比较工具，不是峰值保护。尤其在大压缩量、向上提升或参数继续变化后，检查最终电平，避免后级过载。')", "note('补偿响度与限制峰值分开判断','Match 用来比较响度；Limiter 的 Ceiling 控制最终峰值。完成 Match 后，仍需查看输出与 TP，尤其是在向上提升或继续修改参数以后。')")
replace("['算法、单压 / 双压、阈值、Ratio、Dual 独立开关、音量、Mix、Lookahead、侧链 Source / Key Gain / HPF 等。','主题、各类 LINK、Bypass 与 SC LISTEN。']", "['算法、单压 / 双压、阈值、Ratio、分支开关、音量、Mix、Lookahead、侧链，以及 Limiter、TP、Ceiling、Limiter Output LINK 与两套处理参数。','主题、普通 Input / Output LINK、Ratio / 声道 LINK、耳机 1:1 开关、Bypass 与 SC LISTEN。']")
replace("['Output','按当前设置投影的输出电平参考，含 Makeup、Mix 与 Output Gain。']", "['Output','普通压缩为当前参数下的电平投影；Limiter 为当时采集的最终输出电平，含 Ceiling 与耳机补偿。']")
replace("图形按当前参数重算历史电平参考，不是逐采样录制的输出波形，也不是重新播放音频。", "普通压缩按当前参数重算历史电平参考。Limiter 的橙线保留当时实际输出，调参后请观察新进入的片段；TP 以右侧表头读数为准。")
replace("heading('1.2.7 如何减少大声到来前的凹陷')", "heading('怎样减少大声到来前的凹陷')")
replace("note('默认仍为 26 ms','本次检测调整没有把默认 Lookahead 加长，也没有额外增加一段 Attack / Release 或延迟。需要其他窗口长度时，仍可按素材自行选择。')", "note('默认起点仍为 26 ms','压缩检测仍按选定窗口工作。1.2.7 的最终峰值保护另需固定缓冲，因此宿主报告的总延迟会略大于 Lookahead；切换 Limiter / TP 时保留这段延迟，见第 31 页。')")
replace("0 ms 搭配 8x / 16x 仍有额外延迟。", "0 ms 搭配 8x / 16x 仍有额外延迟，最终峰值保护的固定缓冲也计入宿主报告。")
replace("note('每次比较都做这三件事','关闭 SC LISTEN；MONITOR 回到 ALL；确认前后响度接近。需要限制最终峰值时，另用合适的限幅工具；本插件不是砖墙限幅器。')", "note('每次比较都做这三件事','关闭 SC LISTEN；声道 MONITOR 回到 ALL；确认前后响度接近。需要控制最终峰值时，开启 Limiter 并设置 Ceiling；等响试听可配合耳机按钮与 Match，见第 25-31 页。')")
replace("['Down Ratio','1:1 至 1000:1','默认 1:1；负责向下压缩。']", "['Down Ratio','普通：1:1 至 1000:1；Limiter：20:1 至 1000:1','普通默认 1:1；进入 Limiter 下压从 20:1 起。']")
replace("['Single Ratio','1:1000 至 1000:1','默认 1:1；低于 1 向上，高于 1 向下。']", "['Single Ratio','普通：1:1000 至 1000:1','普通默认 1:1。Limiter 上压至 1:1，下压区为 20:1 至 1000:1。']")
replace("所有声道的三类 Ratio 默认均为 1:1。", "普通压缩各声道的三类 Ratio 默认均为 1:1；Limiter 的下压范围单独限制。")
replace("['Output Gain','-24 至 +24 dB','默认 0 dB；最终输出。']", "['Output Gain','普通 ±24 dB；Limiter ±120 dB','Limiter 的耳机模式抵消其监听增益，数值和联动仍有效。']")
replace("['SC LISTEN','OFF / ON','临时试听；关闭面板或编辑器即关闭。']], [.22,.34,.44],9)", "['SC LISTEN','OFF / ON','临时试听；关闭面板或编辑器即关闭。'],\n      ['Limiter / TP','默认 OFF / ON','TP 只在 Limiter 下生效；关闭 TP 使用采样峰值。'],\n      ['Ceiling','-24 至 0 dB','默认 0；Alt 单击恢复 0。'],\n      ['耳机 1:1','默认 OFF','工程保存，独立于 A/B；MON = Ceiling - Output。']], [.22,.34,.44],8.3)")
replace("para('安装、更新和扫描问题，请参阅随附的 Windows / macOS 安装说明。',9)", "para('Limiter 的 TP、Ceiling、耳机与输出问题见第 31 页。安装、更新和扫描请参阅对应平台的安装说明。',9)")

extra=HERE/'limiter_pages_zh.inc.py'
replace('这里的“0 dB 动态增益”只表示不提升、不衰减；Input、Makeup、Mix 和 Output Gain 仍按各自设置工作。EXT 模式下，判断的是外部检测信号。', '这里的“0 dB 动态增益”只指动态分支不提升、不衰减；Input、Makeup、Mix 和 Output Gain 仍按设置工作。Limiter 的最终 Ceiling 仍独立生效。EXT 模式下，范围判断使用外部检测信号。')
replace('Up 的范围为 1:1000 至 1:1，Down 为 1:1 至 1000:1。任意一方将超出范围时，联动在共同允许的位置停止；反向调整即可继续。只开关 LINK 不会改变当前 Ratio。', 'Up 的范围为 1:1000 至 1:1；普通压缩的 Down 为 1:1 至 1000:1，Limiter 的 Down 从 20:1 起。任意一方将超出范围时，联动在共同允许的位置停止；反向调整即可继续。只开关 LINK 不会改变当前 Ratio。')
insert=extra.read_text(encoding='utf-8')
replace("    start('参数速查：算法、动态与范围')",insert+"\n\n    start('参数速查：算法、动态与范围')")
(HERE/'build_manual_1_2_7_zh.py').write_text(s,encoding='utf-8')
print('Prepared 35-page Chinese manual source; existing curve figure retained.')
