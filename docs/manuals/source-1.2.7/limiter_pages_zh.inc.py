    start('Limiter：在动态处理后控制峰值','点击顶部 LIMITER；Classic 与 Super 均可使用。')
    picture('manual-limiter-light.png','Limiter / Classic / ST，TP 开启。示例数值用于解释界面，不是通用预设。',maxh=318)
    para('Limiter 在原有动态处理后加入最终 Ceiling，控制送出的峰值。你仍可使用 Single / Dual、向上处理和 ST / LR / MS；ALGO 继续决定前面的动态曲线。')
    table(['位置','作用'],[
      ['顶部 LIMITER','开启最终峰值保护及 Limiter 专用参数。'],
      ['Output 旁 LINK','让下压阈值与输出相互联动，见下一页。'],
      ['Ceiling / TP','设置上限，并选择真峰值或采样峰值保护。'],
      ['Output 左侧耳机','以补偿后的监听电平比较完整限幅结果。']], [.30,.70],9.3)
    note('先处理动态，再判断最终输出','先调 Threshold、Ratio 与 Mix，再设 Ceiling。最后用耳机 + Match + Bypass 做等响比较；只看压缩量或把 Ratio 调大，不能替代最终峰值检查。')


    start('Limiter 的 Ratio、阈值与 Output LINK')
    picture('manual-limiter-controls.png','Limiter 启用后，Input 旁的普通 LINK 隐藏，Output 旁出现 Limiter LINK。',maxh=97)
    heading('下压从 20:1 开始，上压仍可使用')
    para('Limiter 的下压 Ratio 范围为 20:1 至 1000:1。Single 的 1:1000 至 1:1 部分仍用于上压或不做动态变化，向下区域从 20:1 开始；Dual 的 Up 保留原范围，Down 为 20:1 至 1000:1。')
    heading('用阈值与 Output 配合调整力度')
    para('Output 旁的 LINK 默认开启。降低下压阈值时，Output 随之补偿；提高 Output 时，下压阈值相应降低。它按当前算法、Ratio、Makeup 和 Mix 的关系工作，不应把每一次阈值移动都理解成固定 1 dB 的互换。')
    para('Single 下压时关联 Threshold；Dual 关联 DOWN THR。UP THR 用来选择哪些声音可以提升，不受这个 LINK 带动。修改 Makeup、Mix 或曲线设置后，联动会更新 Output；需要各自调整时可关闭 LINK。',9.6)
    heading('进入与退出 Limiter')
    para('进入时以普通设置建立 Limiter 参数，下压 Ratio 至少为 20:1。Output 初始继承非负的普通 Output；原值为负时从 0 dB 开始，随后由下压阈值与输出联动。')
    note('关闭 Limiter 会恢复普通参数','普通压缩与 Limiter 分别保留 Ratio、边界和 Output。关闭后回到进入前的普通设置；再次进入时会按当前普通设置建立 Limiter 参数。Makeup、Mix、Input 等共用控件仍保留当前值。')


    start('Ceiling 与 TP：选择最终上限')
    picture('manual-limiter-light.png','TP 点亮：Ceiling 以 dBTP 显示。',maxh=202,width=205,crop=(1700,1236,334,344))
    table(['设置','含义'],[
      ['Ceiling','范围 -24 至 0 dB，默认 0。设置最终输出的上限。'],
      ['TP 点亮','默认状态；将重建波形中的采样间峰值计入保护。'],
      ['TP 熄灭','按采样点峰值保护，数值显示为 dBFS；真峰值仍可能高于该值。']], [.25,.75],9.5)
    para('例如 Ceiling = -1 dB：TP 点亮时按真峰值上限工作；关闭 TP 时按采样峰值上限工作。TP 会保留少量重建余量，因此实际最大读数可以略低于设置值，不必恰好贴线。')
    heading('数值操作')
    para('双击数值输入；上下拖动调整，按住 Shift 更精细。Alt 单击恢复为 0。Ceiling 的修改支持 Ctrl+Z / Ctrl+Shift+Z 撤销与重做。',9.6)
    note('开关的作用范围','TP 与 Ceiling 只在 Limiter 下工作，关闭 Limiter 时一起隐藏并停止生效。TP 切换保留平滑过渡，状态随工程和 A/B 保存；旧工程仍采用它保存的选择。')


    start('TP 读数：看实际输出，保留峰值')
    picture('manual-limiter-tp-meter.png','右侧 TP L/R 读数，监测最终左右立体声的真峰值。',maxh=151,width=280)
    heading('读数来自 Ceiling 之后')
    para('TP 表读取实际输出，包含 Makeup、Mix、Output Gain、Ceiling 和耳机 1:1 补偿的结果。它不是把 Ceiling 设置值直接显示出来，也不会为了贴合设置值而把读数截到上限。')
    heading('Hold 与清零')
    step(1,'播放需要检查的片段','峰值读数会保留约 20 秒；出现新的更高峰值时更新并重新计时。Hold 按音频播放时间更新。')
    step(2,'开始新一轮检查','鼠标双击 TP 读数即可立即归零，再播放检查片段。无需等待旧峰值自动消退。')
    step(3,'修改 Ceiling 后重新测量','保留的旧峰值可能来自修改前的设置。先清零，再判断当前上限是否符合预期。')
    table(['看到的情况','怎样理解'],[
      ['关闭 TP 后读数高于 Ceiling','TP 表仍测量真峰值；采样峰值保护不保证采样间峰值同样受限。'],
      ['打开耳机后 TP 读数变化','监听补偿已计入读数，请结合 MON 换算上限判断。'],
      ['MS 的单域表与 TP 不同','TP 看最终 L/R；单独的 M/S 电平与左右输出上限不是同一个量。']], [.38,.62],9.2)


    start('耳机 1:1：保留限幅，抵消输出增益','Output Gain 左侧的小耳机，只在 Limiter 模式显示。')
    picture('manual-limiter-monitor-controls.png','耳机点亮后，Output 的数值和联动保留；下方出现 MON 监听上限。',maxh=103)
    para('打开耳机，声音仍完整经过动态处理和 Ceiling，然后再抵消可见 Output Gain 的增益。限幅造成的动态变化与波形仍然保留，只把结果换到便于比较的监听音量。')
    heading('Output 数值仍然有意义')
    para('Output 可以继续调整；下压阈值、Makeup 与 Output 的联动仍然有效。耳机只是抵消 Output 的直接音量作用，参数改变带来的动态变化仍能听见。关闭耳机，恢复正常 Output Gain。')
    heading('Ceiling 也换到同一监听标准')
    note('MON = Ceiling - Output Gain','例如 Ceiling = -1 dB、Output = +6 dB，耳机开启时对应的监听上限为 -7 dB。Ceiling 原值仍为 -1 dB；关闭耳机后恢复正常输出标准。TP 表跟随实际输出测量。')
    para('耳机开关默认关闭，随工程保存，独立于 A/B。切换 A/B 时可以一直保持同一监听方式；耳机与 Bypass 使用平滑过渡。它不单独增加一段音频延迟。',9.6)
    note('1:1 不会自动等响','耳机只抵消 Output Gain，并不自动弥补压缩、Ceiling 或 Input 的全部响度变化。等响比较仍需点击 Match；下一页给出完整操作。')


    start('Limiter 等响比较：耳机 + Match + Bypass')
    step(1,'固定本次试听方式','打开 Limiter 与耳机，设置 Ceiling 和 TP。关闭 SC LISTEN，声道 MONITOR 回到 ALL；先听完整立体声结果。')
    step(2,'播放同一段有代表性的素材','让响度读数建立。修改参数后会重新收集数据；持续调参、完全静音或刚开始播放时，不要急着点击 Match。')
    step(3,'点击 Match，补偿到 Makeup','耳机模式比较原始输入与真正经过 Ceiling、耳机补偿后的输出。LR / MS 使用共同补偿量，保留已有声道相对平衡；干湿混合也会计入。')
    step(4,'用插件 Bypass 比较原声','Bypass 返回延迟对齐的原始输入，不经过限幅或耳机音量补偿。听起音、尾音、密度与律动，必要时微调 Makeup。')
    step(5,'结束试听，恢复正常输出','关闭耳机，保留调好的参数。重新检查 TP 和 Ceiling，再判断送往后级的最终输出。')
    heading('Match 做一次补偿，不持续追着音量跑')
    para('素材段落改变、Ceiling 深度改变或继续调参后，可以重新播放并 Match。Limiter LINK 关闭、参数到达边界或 Mix 为完全干声时，Makeup 能做到的补偿可能受限。',9.5)
    note('比较 A/B 时','分别给 A、B 播放相同的素材，再 Match。耳机状态独立于 A/B，Limiter / TP / Ceiling 则随 A/B 恢复；尽量保持相同 Lookahead，避免延迟切换干扰判断。')


    start('Limiter 使用提示与常见疑问')
    heading('恢复速度与低频保留')
    para('最终峰值保护会随信号的周期与重复变化调整恢复：孤立峰值过后可较快恢复，低频或连续调幅时采用更稳的处理。它仍有平滑与缓冲，不是逐点硬削波；复杂瞬态仍需结合听感判断。')
    heading('26 ms 与宿主显示的延迟')
    para('Lookahead 的初始起点仍是 26 ms。最终峰值保护还需要约 7-8.5 ms 的固定缓冲；例如 48 kHz 下约为 8.17 ms。普通压缩与 Limiter、TP 开关之间保留相同总延迟，便于宿主补偿。0 ms 或过采样模式也不应直接理解成总延迟为零。',9.6)
    table(['现象','先检查什么'],[
      ['Ceiling / TP / 耳机不见了','确认顶部 LIMITER 已开启；这些控件只在该模式显示。'],
      ['调 Output，阈值也在动','Limiter 的 Output LINK 开启。它联动下压边界；需要独立调整时关闭 LINK。'],
      ['耳机下听不出 Output 的增益','这是监听补偿的用途。数值及联动仍有效，关闭耳机恢复正常输出。'],
      ['Mix 为 0% 仍有限幅','Mix 只混合动态处理部分；最终 Ceiling 仍在后面工作。完整原声比较用 Bypass。'],
      ['TP 的峰值像是没有更新','旧峰值仍在 Hold。双击清零后播放新片段，结合 TP / Peak 选择判断。'],
      ['MATCH 还不可用','播放非静音片段，并停止连续调参，让当前设置积累有效测量。']], [.34,.66],9.1)
    note('先确认听的是哪一路','最终峰值判断使用完整输出；关闭 SC LISTEN，并把声道 MONITOR 设为 ALL。耳机等响试听完成后，关闭耳机再核对正常输出。')
