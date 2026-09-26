"""Chinese 1.2.0 manual, retaining the approved typography, palette and user-facing structure.
English remains unchanged pending Chinese approval.
"""
from pathlib import Path
import argparse
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor, white
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, Table, TableStyle
from PIL import Image
from pypdf import PdfReader

arg=argparse.ArgumentParser()
arg.add_argument('--output',type=Path,required=True)
arg.add_argument('--captures',type=Path,required=True)
arg.add_argument('--theme-images',type=Path,required=True)
arg.add_argument('--fonts',type=Path,required=True)
arg.add_argument('--language',choices=('zh',),default='zh',
                 help='Chinese review first; English requires approval of the Chinese edition.')
a=arg.parse_args();a.output.mkdir(parents=True,exist_ok=True)
for name,file in [('Body','segoeui.ttf'),('Bold','seguisb.ttf'),('CN','msyh.ttc'),('CNBold','msyhbd.ttc')]:
    pdfmetrics.registerFont(TTFont(name,str(a.fonts/file),subfontIndex=0))
W,H=595.276,841.89;M=48;CW=W-2*M
TEAL='#009db5';INK='#293940';MUTED='#70848e';PALE='#eaf6f8'

for lang in ('zh',):
    zh=lang=='zh';font='CN' if zh else 'Body';bold='CNBold' if zh else 'Bold'
    pdfmetrics.registerFontFamily(font,normal=font,bold=bold,italic=font,boldItalic=bold)
    name='QQ Super Compression 用户手册 中文版_v1.2.0.pdf'
    c=canvas.Canvas(str(a.output/name),pagesize=(W,H),pageCompression=1)
    c.setTitle('QQ Super Compression 1.2.0 用户手册')
    c.setAuthor('Qing Audio');c.setSubject('操作、混音与侧链使用指南')
    page=0;y=0;mins=[]
    def p_obj(text,size=10,colour=INK,boldface=False):
        return Paragraph(text,ParagraphStyle('p',fontName=bold if boldface else font,
            fontSize=size,leading=size*1.52,textColor=HexColor(colour),
            wordWrap='CJK' if zh else None))
    def para(text,size=10,after=10):
        global y
        o=p_obj(text,size);_,h=o.wrap(CW,800);o.drawOn(c,M,y-h);y-=h+after
    def heading(text):
        global y
        y-=5;o=p_obj(text,12,TEAL,True);_,h=o.wrap(CW,800);o.drawOn(c,M,y-h);y-=h+7
    def note(title,text):
        global y
        o=p_obj('<b>'+title+'</b><br/>'+text,9.6);_,h=o.wrap(CW-26,800)
        c.setFillColor(HexColor(PALE));c.rect(M,y-h-22,CW,h+22,fill=1,stroke=0)
        c.setFillColor(HexColor(TEAL));c.rect(M,y-h-22,3,h+22,fill=1,stroke=0)
        o.drawOn(c,M+13,y-h-10);y-=h+34
    def step(n,title,text):
        global y
        o=p_obj('<b>'+title+'</b><br/>'+text,10);_,h=o.wrap(CW-31,800)
        c.setFillColor(HexColor(TEAL));c.circle(M+9,y-9,9,fill=1,stroke=0)
        c.setFillColor(white);c.setFont('Bold',9);c.drawCentredString(M+9,y-12,str(n))
        o.drawOn(c,M+31,y-h);y-=max(h,20)+13
    def table(headers,rows,widths,size=9.2):
        global y
        cells=[[p_obj(escape(str(v)),size,TEAL,True) for v in headers]]
        cells += [[p_obj(escape(str(v)),size) for v in row] for row in rows]
        tb=Table(cells,colWidths=[CW*w for w in widths])
        tb.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor(PALE)),
          ('ROWBACKGROUNDS',(0,1),(-1,-1),[white,HexColor('#f6f9fa')]),
          ('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),
          ('TOPPADDING',(0,0),(-1,-1),6),('BOTTOMPADDING',(0,0),(-1,-1),6)]))
        _,h=tb.wrap(CW,800);tb.drawOn(c,M,y-h);y-=h+14
    def image_path(name):return (a.captures/name) if name.startswith('manual-') else (a.theme_images/name)
    def picture(name,caption='',maxh=390,crop=None,width=None):
        global y
        path=image_path(name);im=Image.open(path);iw,ih=im.size;im.close()
        x0,t0,sw,sh=crop or (0,0,iw,ih)
        scale=min((width or CW)/sw,maxh/sh);dw,dh=sw*scale,sh*scale;left=(W-dw)/2
        c.saveState();clip=c.beginPath();clip.rect(left,y-dh,dw,dh);c.clipPath(clip,stroke=0)
        c.drawImage(str(path),left-x0*scale,y+(t0-ih)*scale,iw*scale,ih*scale,mask='auto');c.restoreState()
        y-=dh+7
        if caption:
            o=p_obj(caption,8.3,MUTED);_,h=o.wrap(CW,800);o.drawOn(c,M,y-h);y-=h+9
        else:y-=6
    def regions(labels, boundaries, colours):
        global y
        top=y; gap=5; cell=(CW-2*gap)/3; height=58
        for i,(title,body) in enumerate(labels):
            x=M+i*(cell+gap)
            c.setFillColor(HexColor(colours[i]));c.roundRect(x,top-height,cell,height,4,fill=1,stroke=0)
            o=p_obj('<b>'+title+'</b><br/>'+body,9.1)
            _,h=o.wrap(cell-18,height);o.drawOn(c,x+9,top-10-h)
        c.setStrokeColor(HexColor(MUTED));c.setLineWidth(.6)
        c.line(M,top-height-18,W-M,top-height-18)
        for i,label in enumerate(boundaries):
            x=M+(i+1)*cell+(i+.5)*gap
            c.line(x,top-height-14,x,top-height-22)
            c.setFont(font,8);c.setFillColor(HexColor(MUTED));c.drawCentredString(x,top-height-35,label)
        c.setFont(font,8);c.drawString(M,top-height-50,'检测电平由低到高（作用区间示意）')
        y=top-height-65
    def finish():
        if not page:return
        assert y>=53,(lang,page,round(y,2))
        mins.append(round(y,1))
        c.setStrokeColor(HexColor('#d8e6e9'));c.setLineWidth(.5);c.line(M,40,W-M,40)
        c.setFillColor(HexColor(TEAL));c.setFont('Body',8);c.drawString(M,26,'QING AUDIO')
        c.setFillColor(HexColor(MUTED));c.setFont(font,8)
        c.drawRightString(W-M,26,'QQ Super Compression 1.2.0  |  '+str(page))
        c.showPage()
    def start(title,subtitle=''):
        global page,y
        finish();page+=1;c.bookmarkPage('p'+str(page));c.addOutlineEntry(title,'p'+str(page),0)
        c.setFillColor(HexColor(MUTED));c.setFont(font,8)
        c.drawRightString(W-M,H-34,'QQ Super Compression  ·  用户手册')
        y=H-59;o=p_obj(title,22,TEAL,True);_,h=o.wrap(CW,150);o.drawOn(c,M,y-h);y-=h+11
        if subtitle:
            o=p_obj(subtitle,10,MUTED);_,h=o.wrap(CW,150);o.drawOn(c,M,y-h);y-=h+14

    # 1 - one current edition, not a revision appendix.
    start('QQ Super Compression','用户手册  /  1.2.0')
    heading('控制动态，尽量保留原本的起音。')
    para('用于人声、乐器、总线和母带。没有传统 Attack / Release 控件；使用 Lookahead、Ratio、Threshold 和 Mix 调整素材的动态表现。')
    para('单压可以向下控制动态，也可以在指定范围内向上提升；双压则在同一界面中分别设置上压与下压。',9.6)
    picture('manual-light-ST.png',maxh=402)
    note('从听感出发','先决定需要多稳的动态、要保留多少起音，再调整参数。图表帮助理解变化，但不代替听音。')

    # 2 - contents and a useful first session.
    start('快速上手与阅读导航','第一次使用，先按下面六步走。')
    for i,(title,text) in enumerate([
      ('插入需要处理的轨道','先用 SINGLE、ST 和 SC: INT，让插件根据这条轨道自身的声音工作。'),
      ('选择 Lookahead','可从 26 ms 开始。它是一个实用起点，不是每种素材都必须使用的设置。'),
      ('从 1:1 开始调整','所有 Ratio 默认 1:1，此时不改变动态。先试向下压缩时，把 Ratio 调高，并让 Range 保持 OFF；上压与双压见第 7-9 页。'),
      ('把 Mix 当作处理量工具','如果处理过重，降低 Mix，逐渐混回原始动态。提升与衰减的显示都会随 Mix 改变。'),
      ('补偿音量后比较','播放一段有代表性的素材，使用 Match，再用 Makeup / Output Gain 微调，避免被响度差误导。'),
      ('回到完整混音中听','切换 Bypass 或 A/B，检查起音、稳定度和律动，而不只是盯着 GR 数字。')],1):step(i,title,text)
    table(['想查什么','页码'],[
      ['界面、主题与操作','3-4'],['Ratio、单压范围与向上压缩','5-7'],
      ['双压与 Ratio LINK','8-9'],['响度比较、Display 与 Lookahead','10-12'],
      ['ST / LR / MS 与声道 LINK','13-15'],['侧链面板、内部 HPF、外部路由','16-18'],
      ['混音实例、参数速查与排错','19-23']], [.78,.22],9)

    # 3
    start('认识界面','主要音频控件保持在同一位置，换主题不改变功能。')
    picture('manual-light-ST.png','Light / ST。图中数值用于说明界面，不是固定推荐预设。',maxh=361)
    table(['区域','用来做什么'],[
      ['Display','对照原始动态、含 Mix 的提升 / 衰减和最终输出。'],
      ['两根边界推子','单压为 Threshold / Range；双压为 UP / DOWN 阈值。'],
      ['Input / Output / Gain +/-','检查输入、输出电平与有效动态增益。'],
      ['底部旋钮与 SINGLE / DUAL','调整音量、Ratio 和 Mix；在 Ratio 旁切换单压 / 双压。'],
      ['Mode / LINK / Monitor','选择通道方式，联动参数或单独监听。'],
      ['右上角','打开侧链、切换主题、操作 A/B 与 Bypass。']], [.32,.68],9)
    para('界面采用 3:2 横向比例。拖动右下角可以调整大小，下次打开时会记住上一次的界面尺寸。',9.5)

    # 4
    start('主题切换与基本操作')
    para('点击界面右上角显示 LIGHT、DARK 或 CLASSIC 的主题按钮，即可循环切换。插件下次打开时会恢复上一次选择。')
    picture('manual-theme-button.png','主题按钮位于 SC 按钮右侧、A/B 按钮左侧。',maxh=72,width=280)
    table(['主题','外观选择'],[['Light','浅色界面。'],['Dark','低亮度深色界面。'],['Classic','保留原经典深色界面。']], [.25,.75])
    para('三种主题的布局、参数和声音相同。选择最适合当前环境的外观即可。')
    # Two visual choices, not implementation descriptions.
    yy=y;gap=14;cell=(CW-gap)/2
    for n,label,x in [('manual-dark.png','Dark',M),('manual-classic.png','Classic',M+cell+gap)]:
        im=Image.open(a.captures/n);ww,hh=im.size;im.close();ht=cell*hh/ww
        c.drawImage(str(a.captures/n),x,yy-ht,cell,ht,mask='auto');c.setFont('Body',9);c.setFillColor(HexColor(MUTED));c.drawString(x,yy-ht-14,label)
    y=yy-ht-30
    table(['操作','结果'],[
      ['拖动旋钮 / Shift 拖动','普通调整 / 精细调整。'],
      ['双击数值','直接输入精确数值。'],
      ['Alt + 左键','恢复默认值；联动开启时，相关控件也会跟随并受范围限制。'],
      ['Ctrl/Cmd + Z','撤销；加 Shift 则重做。']], [.39,.61],9)

    # 5
    start('Ratio、Input、Mix：三种调法','它们都能影响结果，但不能互相精确替代。')
    picture('manual-light-controls.png',maxh=92)
    heading('Ratio')
    para('单压以 1:1 为中点：高于 1:1 时向下压缩，低于 1:1 时向上提升。在符合阈值范围的部分，向下数值越大通常压得越多；向上数值越小通常提得越多。双压将两种方向分成 UP RATIO 和 DOWN RATIO，见第 8 页。')
    para('这里的 Ratio 不是传统压缩器超过阈值后的固定 dB 斜率，不必照搬其他压缩器的数值。所有 Ratio 默认 1:1。',9.6)
    heading('Input Gain')
    para('Input Gain 调整进入处理路径的电平，也会改变 Dry 路径的音量。使用 INT 时，它同时改变检测信号相对 Threshold 的位置。使用 EXT 时，外部检测电平由发送电平与 Key Gain 决定，不能用 Input Gain 代替。')
    heading('Mix')
    para('Mix 在 Dry 和处理后的 Wet 之间混合。它不只是一个附加效果比例，也是重要的处理量控制：先把提升或衰减调得明显，再降低 Mix，把需要的原始动态混回来。')
    note('动态增益包含 Mix','Mix = 0% 时有效动态增益为 0 dB；100% 时保留完整处理量。中间值不是简单的“增益 dB 数字乘以 Mix”。Makeup 和 Output Gain 不计入这项动态增益。')
    para('实用顺序：先调 Ratio 找到处理感，再用 Mix 控制最终力度；需要改变整体输入电平时才调 Input Gain，最后重新比较响度。')

    # 6
    start('单压：Threshold 与 Range','先决定哪些电平参与处理，再用 Ratio 选择方向与力度。')
    picture('manual-light-single-up.png','SINGLE：下方 Threshold 与上方 Range 共同限定范围，推子与图中的虚线对应。',maxh=245)
    table(['检测电平','动态处理'],[
      ['低于或等于 Threshold','不作动态提升或衰减。'],
      ['高于 Threshold，且低于 Range','按 Ratio 向上或向下处理。'],
      ['达到或超过有限 Range','完全恢复为 0 dB 动态增益。']], [.46,.54],9.3)
    heading('Range OFF：没有上截止')
    para('Range 位于顶端 OFF 时不设置上界，保留原来的向下处理方式。将它从上往下拉，才启用有限上截止。有限的 0 dB 与 OFF 是两种不同设置。Threshold 的 OFF 则表示没有下限门槛。',9.8)
    note('两根推子相碰时','Range 始终在 Threshold 上方或与其重合。拖动一根撞到另一根时，另一根会一起移动；两者重合时不作动态处理。反向拖动即可重新分开。')
    para('这里的“0 dB 动态增益”只表示不提升、不衰减；Input、Makeup、Mix 和 Output Gain 仍按各自设置工作。EXT 模式下，判断的是外部检测信号。',9.3)

    # 7
    start('向上压缩：提升门槛以上的部分','沿用 Super Compression 的动态处理方式，让选中的细节更靠前。')
    para('在 SINGLE 中，将 Ratio 从 1:1 向左调，例如 1:2、1:4、1:8，即进入向上压缩。界面上的 1:8 对应数值 1/8；数值越小，符合条件的部分通常获得越多提升。')
    regions([('保持原音量','低于或等于 Threshold'),('向上提升','Threshold 与 Range 之间'),('保持原音量','达到或超过有限 Range')],['Threshold','Range'],['#f3f6f7','#eaf6f8','#f3f6f7'])
    note('门槛决定是否提升，不把声音关掉','Threshold 在这里像一个允许提升的门槛。门槛以下仍保留原音量，并不是把更小的声音静音，也不是专门提升阈值以下的声音。')
    step(1,'先圈出想抬起来的内容','切到 SINGLE，以 Threshold 设下限、Range 设上限。例如先用 -50 dB / -10 dB，听需要的细节是否落在两者之间。数值只是示例。')
    step(2,'再调 Ratio 与 Mix','从 1:1 逐渐向 1:2、1:4 调整；觉得提升过多时，减小力度或降低 Mix。先不要用 Makeup 代替动态提升。')
    step(3,'同时听安静处和较响处','作用范围内较轻的部分通常获得更多提升；越接近上界，提升越趋近 0 dB。Range 为 OFF 时没有有限上截止。')
    para('想保留很轻的呼吸、底噪或残响原有音量时，将下限门槛放在它们上方，再听需要的主体是否仍能进入提升区。这里按检测电平选择，不识别声音种类。',9.5)

    # 8
    start('双压：较轻处提升，较响处压低','点击 Ratio 旁的小按钮，将 SINGLE 切换为 DUAL。')
    picture('manual-light-dual.png','DUAL：UP RATIO 与 DOWN RATIO 分别控制两种方向；UP / DOWN 两个阈值替代 Threshold / Range。',maxh=260)
    regions([('保持原音量','低于或等于 UP 阈值'),('由 UP RATIO 提升','高于 UP、低于 DOWN'),('由 DOWN RATIO 压低','高于 DOWN 阈值')],['UP 阈值','DOWN 阈值'],['#f3f6f7','#eaf6f8','#eaf6f8'])
    para('达到 DOWN 阈值时，动态增益回到 0 dB；超过后停止上压，只由 Down Ratio 处理。两种增益不会在同一检测电平上叠加。Dual 不再设置 Range。',9.7)
    table(['新实例的起点','怎么理解'],[
      ['UP = -inf；DOWN = 0 dB','UP 的下限门槛完全打开；不是关闭上压。'],
      ['Up Ratio = 1:1；Down Ratio = 1:1','两种动态处理都处于中性状态，需调整 Ratio 才能听到效果。']], [.48,.52],9.2)
    note('阈值顺序与碰撞','DOWN 必须在 UP 上方或与其重合。拖动相撞会推着另一根一起移动；两阈值重合时，整个动态处理停止。')

    # 9
    start('Ratio LINK：按相反倍数联动','两个 Ratio 之间偏上的小 LINK，只在 DUAL 中显示。')
    picture('manual-light-dual-controls.png','Up 与 Down 各自保留原来的相对关系；打开 LINK 不会使数值跳变。',maxh=100)
    para('首次使用时 Ratio LINK 默认开启。之后会记住你上次的选择；重新打开已保存的工程，则以工程保存的 LINK 状态为准。')
    table(['原来的 Down / Up','调整 Down','联动后的 Up'],[
      ['1:1 / 1:1','1:1 → 2:1','1:2'],
      ['4:1 / 1:2','4:1 → 8:1','1:4'],
      ['8:1 / 1:4','8:1 → 4:1','1:2']], [.36,.32,.32],9.6)
    heading('保留原有关系，而不是重新配对')
    para('例如 Down 加倍，Up 的数值就减半；调 Up 时也是同样的反向关系。从 4:1 与 1:2 出发，并不会被强行改成一对倒数。拖动、精细调整和直接输入都遵守这条规则。')
    note('到达边界，会一起停下','Up 的范围为 1:32 至 1:1，Down 为 1:1 至 32:1。任意一方将超出范围时，联动在共同允许的位置停止；反向调整即可继续。只开关 LINK 不会改变当前 Ratio。')
    heading('想单独听上压时')
    para('先关闭 Ratio 中间的小 LINK，让 Down Ratio 保持 1:1，再单独调整 Up Ratio。否则在 LINK 开启时，调整 Up 也会带动 Down，听到的是两种设置一起改变。')
    heading('LR / MS 中仍然适用')
    para('小 LINK 会随紧凑布局缩小，仍位于 Up / Down Ratio 之间。它联动每个声道内部的上、下 Ratio；Mode 旁的声道 LINK 则联动 L/R 或 M/S，见第 15 页。')

    # 7
    start('补偿响度，再做 A/B 比较')
    table(['控件','作用'],[
      ['Makeup','补偿 Wet 的音量，位于 Mix 之前。Mix 较低时，不是整段输出的统一音量旋钮。'],
      ['Output Gain','调整整段最终输出，不改变动态增益显示。'],
      ['Match','根据已播放素材做一次响度补偿，并写入 Makeup；不是持续自动音量。']], [.24,.76])
    step(1,'播放有代表性的素材','等 Match 可用，再点击它。过短、静音或无有效读数的片段不能提供可靠比较。')
    step(2,'复查最终输出响度','Match 对齐 Dry 与补偿前 Wet；最终 Mix 不一定自动等响。必要时用 Makeup 和 Output Gain 小幅修正。')
    step(3,'比较声音，而不是比较大小','切换 Bypass，留意起音、尾部细节和段落稳定度。')
    heading('使用 A / B 保存两种想法')
    para('先在 A 调好一组设置，用 A→B 复制，再切换到 B 做另一种调整。之后点 A 或 B 比较。反向复制使用 B→A；复制会替换目标一侧的声音参数。')
    para('A/B 包含侧链的 Source、Key Gain 和 HPF。主题不属于声音比较；Bypass、LINK 与 SC LISTEN 也不随 A/B 声音快照切换。')
    note('Mix = 0% 不等于完整 Bypass','Mix 为 0% 时仍经过 Input Gain 与 Output Gain。Bypass 则用于比较未经这些音量与压缩处理的原始信号。')
    heading('等响度比较：上压与下压可以有相同的动态')
    para('在相同输入、声道模式与检测设置下，若两种 Single 设置的 <b>Threshold 均为 OFF（-inf）、Range 为 OFF、Mix 为 100%</b>，分别使用 <b>8:1 下压</b>与 <b>1:8 上压</b>，再将两者的输出音量匹配，理论上会得到相同的动态结果：声音起伏之间的相对关系相同，只是整体音量不同。',9.6)
    para('Dual 默认阈值为 <b>UP = -inf、DOWN = 0 dB</b>。保留这组阈值，并从默认状态用 LINK 联动两个 Ratio 时，等响度比较的效果仍可与对应的 Single 相同。切到 Dual 并不自动带来不同的动态。请仔细观察 <b>Display</b>，按素材需要调整 <b>上下阈值和上下 Ratio</b>，明确哪些部分要抬升、哪些部分要压低。上述比较以 Mix 100% 为前提；改变作用区间或混合比例后，应重新判断。',9.6)


    # 8
    start('看懂 Display 与 Meter')
    picture('manual-light-dual.png','同一段历史中，绿色表示提升，衰减用 Cut / Mix 表示；橙色是最终输出。',maxh=239,crop=(32,152,2336,1072))
    table(['看到什么','含义'],[
      ['Dry / Input','Input Gain 之前的原始动态参考；所以调 Input 时灰线不会整体移动。'],
      ['Cut / Mix','包含 Mix 的动态衰减；与 Dry 的距离表示实际压低了多少。'],
      ['Boost / Mix','包含 Mix 的动态提升，以绿色表示；与 Dry 的距离表示提升了多少。'],
      ['Output','包含 Makeup、Mix 与 Output Gain 的最终输出。'],
      ['External key','EXT 且外部通道可用时，以较淡的曲线显示侧链参考，用来核对触发。'],
      ['Gain +/-','0 dB 位于中央，提升向上、衰减向下。正值表示提升，负值表示衰减；Hold 保留近期较明显的变化。']], [.25,.75],8.9)
    para('动态增益不包含 Makeup / Output Gain；Mix = 0% 时为 0 dB。三主题均可查看提升与衰减，具体颜色可对照图下方的同名图例。',9.3)
    note('历史也会跟着参数变化','调整 Ratio、阈值、Range、Mix 等参数，会更新可见历史。EXT 的 Key Gain 实时更新；HPF 松手后才刷新，期间可能显示 HPF UPDATING。图形是当前设置下的历史参考，不是重新播放音频。')

    # 9
    start('Lookahead 与 0 ms 模式')
    para('Lookahead 是提前观察窗口，不是 Attack。较长窗口通常更适合干净、稳定的动态控制。<br/>窗口越长，插件延迟也越大；宿主通常会补偿这段延迟。')
    picture('manual-light-0ms.png','0 ms 时，Lookahead 下方出现 Oversampling。',maxh=327)
    table(['选择','用途与取舍'],[
      ['10 ms','较短窗口，延迟较低。'],
      ['26 / 40 ms','常用起点；按低频稳定度与整体听感选择。'],
      ['80 / 100 ms','更长窗口；听是否更稳定，同时留意额外延迟。'],
      ['0 ms','更明显的非线性色彩，不是最透明模式。']], [.24,.76],9)
    para('0 ms 的 Oversampling 可选 1x / 8x / 16x。可从 8x 开始；更高倍率会增加负荷和延迟。10 ms 及以上时该控件隐藏。0 ms 搭配 8x / 16x 仍有额外延迟。',9.5)

    # 10
    start('ST 与 LR：一起处理，或分别处理')
    para('点击右下方 Mode 按钮，按 ST → MS → LR 循环切换。通常先用 ST；只有确实需要左右不同处理时，再选择 LR。')
    picture('manual-light-LR.png','LR：上方为 L，下方为 R，各有独立控件。',maxh=373)
    table(['模式','怎么用'],[
      ['ST','对立体声使用共同压缩增益，适合作为人声、乐器和总线的起点。'],
      ['LR','左右各有独立的 Ratio、边界、Makeup 和 Mix；DUAL 时每侧各有 UP / DOWN。处理不对称素材时，同时检查声像是否偏移。']], [.17,.83],9.4)
    note('MONITOR','选择 L 或 R 可单独监听对应一侧，并居中呈现，便于比较。结束后回到 ALL，检查正常立体声结果；不要把单侧监听留作最终输出。')

    # 11
    start('MS：分别控制中心与两侧')
    picture('manual-light-dual-MS.png','MS / DUAL：上方为 Mid，下方为 Side；每个声道各有 UP / DOWN Ratio 和阈值。',maxh=340)
    para('Mid 主要反映左右共有的内容，Side 反映左右差异。它们不是“中间一条轨道”和“剩下所有轨道”的精确分离。')
    step(1,'先听 M 与 S 各包含什么','用 MONITOR 的 M / S 检查素材，再回到 ALL。')
    step(2,'按目的分别调整','例如需要收住中心动态时，先调 M 的 Ratio / Mix；需要保留环境与宽度时，对 S 谨慎处理。')
    note('用完整混音复查','M/S 的压缩或音量变化可能改变空间感。回到 ALL 后检查中心、宽度和单声道兼容性，不只听独奏。')

    # 12
    start('声道 LINK：保留差异，一起调整')
    para('在 LR 或 MS 模式下，Mode 旁的 LINK 用于声道之间的同类参数。先分别调好两侧，再打开它，就能一起调整而不把两侧强制拉成一样。')
    picture('manual-light-LR-link.png','SINGLE 示例：Mode 旁的声道 LINK 已开启，左右 Ratio 仍分别为 3:1 与 5:1。',maxh=110)
    table(['参数','原设置','调整后示例'],[
      ['Ratio','3:1 / 5:1','4:1 / 6:1'],['Threshold','-20 / -10 dB','-18 / -8 dB'],
      ['Makeup','-3 / +1 dB','-2 / +2 dB'],['Mix','100% / 70%','90% / 60%']], [.26,.36,.38])
    step(1,'关闭 LINK，建立两侧关系','先确定哪一侧需要更强的压缩或更少的混合。')
    step(2,'打开 LINK，一起微调','拖动任意一侧同类参数，另一侧跟随同样的变化量。精调和直接输入也适用。')
    note('到达边界会一起停下','任意一侧将超出范围时，两侧会停在能保留差值的位置。一侧 Threshold 为 OFF 时，不按有限 dB 差值联动；先把两侧设为有效阈值再建立关系。')
    heading('DUAL 中的两个 LINK')
    para('Ratio 中间的小 LINK 管每个声道内的 Up / Down 反向联动；Mode 旁的 LINK 管 L/R 或 M/S 的同类参数。两者都开启时，声道间一起变化，各声道内的 Up / Down 仍保持原来的相对关系。')

    # 13
    start('侧链：先认识面板','点击右上角 SC: INT / SC: EXT 打开，按钮文字表示当前来源。')
    picture('manual-sidechain-panel.png',maxh=185,width=415)
    table(['控件','使用方法'],[
      ['INT / EXT','INT 根据当前轨道本身判断；EXT 根据另一条轨道送来的信号控制当前轨道的提升或衰减。'],
      ['KEY GAIN','只在 EXT 可用。调整外部 Key 的检测电平，改变触发强度，不直接调主轨道音量。'],
      ['HPF','减少检测信号中的低频；作用于 INT 或 EXT。OFF 为全频，开启后范围 20-500 Hz。'],
      ['KEY LEVEL','查看送到检测器的信号是否有电平。EXT 显示 N/A 时，先检查宿主侧链通道是否启用。'],
      ['SC LISTEN','临时听检测信号，帮助检查路由与 HPF；听完关闭，再判断正常输出。']], [.24,.76],9.4)
    note('两个常用任务','低频一来整段都被压：用 INT + HPF，见第 17 页。<br/>希望鼓声出现时贝斯让位：用 EXT，见第 18 页。')

    # 14
    start('内部侧链：让低频少主导压缩')
    para('适用场景：处理鼓组、Mix Bus 或母带时，底鼓或贝斯每次出现都会把其他内容明显拉低。你想减少这种触发，而不是把声音本身的低频切掉。')
    for i,(tt,body) in enumerate([
      ('先用 INT 听清问题','循环一段低频与其他内容同时出现的素材，保持 HPF 为 OFF，建立对比。'),
      ('打开侧链面板','点击 SC: INT，确认 SOURCE 仍选 INT。Key Gain 此时灰显是正常的。'),
      ('缓慢提高 HPF','从较低截止频率开始，听低频到来时整段是否还被过度压低。一次只改一点。'),
      ('必要时试听检测信号','打开 SC LISTEN，听 HPF 去掉了哪些低频；随后关闭，回到完整声音。'),
      ('松开旋钮后看历史','HPF 的历史图在松手后刷新；如果显示 HPF UPDATING，等它消失再比较。'),
      ('重新找合适的压缩量','滤波后 GR 可能变少或形状变化。根据目的用 Ratio、Threshold 或 Mix 重新调整，并做等响度比较。')],1):step(i,tt,body)
    note('听起来“没有切低频”是正常的','HPF 只过滤用于判断压缩的 Key，不直接滤波正常输出。它也不是 Key Gain 的替代：Key Gain 整体调检测电平，HPF 改变不同频率的触发权重。截止频率过高可能让需要控制的低频被忽略。')

    # 15
    start('外部侧链：用底鼓让贝斯让位')
    para('插件插在“要被压低”的贝斯轨道上；底鼓只作为触发来源送入它的侧链。不要把插件插反，也不要把普通音频发送误当作侧链发送。')
    # Compact routing makes the two signals unambiguous.
    x=M;boxw=(CW-38)/3;bh=47
    labels=['底鼓轨道','贝斯上的侧链输入','贝斯产生 GR']
    for i,label in enumerate(labels):
        xx=x+i*(boxw+19);c.setFillColor(HexColor(PALE));c.roundRect(xx,y-bh,boxw,bh,5,fill=1,stroke=0)
        ob=p_obj(label,9,TEAL,True);_,hh=ob.wrap(boxw-16,bh);ob.drawOn(c,xx+8,y-(bh+hh)/2)
        if i<2:c.setFillColor(HexColor(TEAL));c.setFont('Body',13);c.drawString(xx+boxw+4,y-29,'→')
    y-=bh+20
    for i,(tt,body) in enumerate([
      ('在宿主中启用侧链','启用贝斯轨上这个插件实例的侧链输入；在底鼓轨道添加指向它的侧链发送，并打开发送。不同宿主的按钮名称和位置可能不同。'),
      ('插件选择 EXT','点击 SC 按钮，把 SOURCE 切到 EXT。播放底鼓并检查 KEY LEVEL；无读数时先查路由，不要直接把 Ratio 拧满。'),
      ('听一听送来的到底是什么','短暂打开 SC LISTEN，应听到底鼓而不是贝斯。确认后关闭试听。'),
      ('调整触发与压缩力度','为向下避让，可先用 SINGLE、Ratio 高于 1:1、Range OFF；再用 Key Gain 或发送电平调检测强度，用 Threshold 调下限。HPF 先保持 OFF。'),
      ('用 Mix 决定让出多少空间','先找到清楚的避让，再减小 Mix，保留需要的贝斯持续感。边听两条轨道边调，不只听贝斯独奏。'),
      ('最后再决定要不要 HPF','提高 HPF 会减少底鼓低频的触发，可能反而减弱你想要的避让。只在需要更偏向敲击部分、或排除多余低频触发时使用。')],1):step(i,tt,body)
    para('没有传统 Attack / Release 可调；避让形状会受触发素材、Lookahead 和压缩设置共同影响，不保证在所有素材上都比传统侧链更顺滑。',9.3)

    # 16
    start('把操作放回真实混音')
    heading('人声、吉他、钢琴：稳住动态，保留表达')
    para('先试 SINGLE / ST / INT 和 26 ms，把 Ratio 调到 1:1 以上，用 Mix 找到字头、拨弦或琴槌仍自然的力度。需要只控制较响部分时，启用 Threshold，并让 Range 保持 OFF。使用 Match 后回到伴奏中比较。')
    heading('尾音与轻声：让选中的细节靠前')
    para('想抬起较轻的字、拨弦尾音或残响时，试单压向上；用 Threshold 排除不想提升的微小声音，再用 Range 限定上界。想同时收住较响部分时，切到 Dual，分别调整 Up 与 Down；初次听两种作用时可先关闭 Ratio LINK。')
    heading('Mix Bus / 母带：追求整体凝聚感')
    para('当你想要整体更稳、更有 Glue，又不希望传统 Attack / Release 明显重塑鼓的音头时，可以把它作为 G Bus 类压缩的另一种选择。先用轻度处理，观察前后段落的平衡；需要时降低 Mix。')
    para('底鼓一来就压低全曲时，试 INT + HPF；不要为了让 GR 更小而把 HPF 调到忽略所有低频。最后用相近响度检查鼓的起音、低频、空间感和整首歌的起伏。')
    heading('人声触发伴奏：清出唱词空间')
    para('把插件插在伴奏总线上，将人声送到它的外部侧链，选 EXT。先用 SC LISTEN 确认来源；选择 SINGLE、Ratio 高于 1:1、Range OFF，再调 Key Gain、Threshold 和 Mix，让有人声时伴奏适当退后。')
    para('如果爆破音触发过重，试着提高侧链 HPF。如果呼吸或底噪也让伴奏退让，先检查发送信号和 Threshold；不要期待 HPF 单独解决所有误触发。')
    note('每次比较都做这三件事','关闭 SC LISTEN；MONITOR 回到 ALL；确认前后响度接近。需要限制最终峰值时，另用合适的限幅工具；本插件不是砖墙限幅器。')

    # 21
    start('参数速查：动态与范围')
    table(['参数','范围 / 选项','说明'],[
      ['SINGLE / DUAL','单压 / 双压','Ratio 旁的小按钮；切换后使用各自的参数。'],
      ['Single Ratio','1:32 至 32:1','默认 1:1；低于 1 向上，高于 1 向下。'],
      ['Up Ratio','1:32 至 1:1','默认 1:1；仅负责向上提升。'],
      ['Down Ratio','1:1 至 32:1','默认 1:1；仅负责向下压缩。'],
      ['Threshold','OFF / -119.99 至 0 dB','单压下限，默认 OFF。'],
      ['Range','OFF / -inf 至 0 dB','单压上限，默认 OFF；有限 0 dB 不等于 OFF。'],
      ['UP THR','-inf 至 0 dB','双压下限，默认 -inf。'],
      ['DOWN THR','-inf 至 0 dB','双压交接阈值，默认 0 dB。'],
      ['Ratio LINK','OFF / ON','首次默认 ON；之后记住选择，工程状态优先。'],
      ['Mode','ST / MS / LR','声道处理方式，与 SINGLE / DUAL 分开选择。'],
      ['声道 LINK','OFF / ON','Mode 旁；LR / MS 同类参数按相对差值联动。']], [.23,.32,.45],9.2)
    note('范围顺序与默认值','单压 Range 不低于 Threshold；双压 DOWN 不低于 UP。两边界重合时不作动态处理。所有声道的三类 Ratio 均默认 1:1。已有工程会保留已保存的数值。')
    para('1:8 表示 1/8，8:1 表示 8。Ratio 数值用于选择动态处理强度，不是固定的提升 dB 数，也不等同传统压缩器的固定 dB 斜率。',9.5)

    # 22
    start('参数速查：音量、监听与侧链')
    table(['参数','范围 / 选项','说明'],[
      ['Input Gain','-24 至 +24 dB','进入处理的电平。'],
      ['Makeup','-36 至 +36 dB','默认 0 dB；Wet 音量，位于 Mix 前。'],
      ['Mix','0 至 100%','默认 100%；包含在有效动态增益中。'],
      ['Output Gain','-24 至 +24 dB','默认 0 dB；最终输出。'],
      ['MONITOR','ALL / L / R 或 M / S','LR / MS 可用；单域居中监听。'],
      ['Lookahead','0 / 10 / 26 / 40 / 80 / 100 ms','初始起点 26 ms；新实例会记住上次选择。'],
      ['Oversampling','1x / 8x / 16x','只在 0 ms 可见；初始 8x。'],
      ['SOURCE','INT / EXT','默认 INT；EXT 需宿主路由。'],
      ['KEY GAIN','-24 至 +24 dB','默认 0 dB；只在 EXT 可调。'],
      ['HPF','OFF / 20 至 500 Hz','默认 OFF；只过滤检测信号。'],
      ['SC LISTEN','OFF / ON','临时试听；关闭面板或编辑器即关闭。']], [.22,.34,.44],9)
    note('不要混淆几个音量入口','Input 调主信号输入；Key Gain 只调外部检测信号；Makeup 调 Mix 之前的 Wet；Output Gain 调最终输出。比较动态前，先把前后响度调到接近。')

    # 23
    start('上压与双压：没有变化时检查什么')
    table(['现象','先检查什么'],[
      ['调整后没有听到提升','先确认 Single Ratio 或 Up Ratio 已低于 1:1、Mix 大于 0，且 Bypass 已关闭。'],
      ['Dual 的 Up 没有效果','检测电平要高于 UP、低于 DOWN。UP 以下原样输出，高于 DOWN 时只做下压。'],
      ['推子调好却完全不工作','两根边界是否重合？重合会使整个动态处理停止；分开后再比较。'],
      ['较响的部分突然不再处理','SINGLE 是否设置了有限 Range？达到或超过它时动态增益回到 0 dB；需要无上界时选 OFF。'],
      ['调 Up，Down 也在变化','两个 Ratio 中间的 LINK 默认开启。需要独立调整时，先关闭它。'],
      ['LINK 不能继续往同一方向调','另一侧可能已到范围边界；联动会共同停止。反向调整即可继续。'],
      ['看着主轨道电平却不按预期触发','检查 SOURCE 是否为 EXT，或 HPF 是否改变了检测信号。阈值判断依据是检测信号。']], [.33,.67],9.2)
    heading('单独确认双压的向上作用')
    step(1,'把其他变化先固定','选择 DUAL，关闭 Ratio LINK，让 Down Ratio 为 1:1。保持 Mix 100%，各音量增益为 0 dB，选择合适的 Lookahead。')
    step(2,'确保素材在两个阈值之间','例如 UP 为 -50 dB、DOWN 为 -20 dB，确认一段轻声的检测电平主要落在两者之间。此时 Ratio 仍为 1:1，不会出现提升。')
    step(3,'从 1:1 逐渐增加上压力度','将 Up Ratio 依次调向 1:2、1:4、1:8，同时听音，观察 Boost / Mix 与正向动态增益。确认后再加入 Down 的处理，重新比较整体响度。')

    # 24
    start('遇到问题，先检查这里')
    table(['现象','先检查什么'],[
      ['EXT 没有效果','宿主侧链输入和发送是否启用？SOURCE 是否为 EXT？KEY LEVEL 是否有读数？N/A 表示当前未提供外部通道；-inf 常见于静音或未收到信号。'],
      ['Key Gain 不能调整','当前为 INT 时这是正常行为；它只调整 EXT。'],
      ['HPF 调了没有明显变化','它不直接切主声音低频。检查 Key 是否确有低频、当前是否有 GR；降低 Mix 也会减弱听到的差异。'],
      ['HPF 历史没有边拖边变','松开后才刷新历史；等待 HPF UPDATING 消失。Key Gain 在 EXT 下实时更新。'],
      ['声音变成侧链来源了','检查 SC LISTEN 是否开启；关闭后回到正常输出。'],
      ['只有一侧或声音变窄','先把 MONITOR 切回 ALL，再检查 LR / MS 设置。'],
      ['Match 灰显','播放有代表性且非静音的素材，让有效响度读数建立。'],
      ['换 Lookahead 后宿主短暂调整','延迟发生变化，宿主可能重新做延迟补偿。录音监听时也要考虑延迟。'],
      ['图中没有 -90 dB 以下','这是 Display 的绘制范围，不表示音频被截断。']], [.30,.70],8.9)
    para('安装、更新和插件扫描问题，请参阅随附的 Windows / macOS 安装说明。',9.3)
    para('使用许可：Qing Audio 非商业源码共享许可证 1.0。禁止商业使用；完整条款与对应源码见项目仓库。',8.7)
    para('<link href="https://github.com/Ziqing-Gu/QQ-Super-Compression" color="'+TEAL+'">github.com/Ziqing-Gu/QQ-Super-Compression</link>',9)
    finish();c.save()
    doc=PdfReader(a.output/name);assert len(doc.pages)==23
    txt='\n'.join(p.extract_text() for p in doc.pages)
    for banned in ('Revision','Rev 2','Plan A','Plan B','Plan C','JUCE','offscreen','离屏','2.2x','归一化','缓存','v1.0.1','附页','addendum','schema'):
        assert banned not in txt,(lang,banned)
    print(lang, len(doc.pages),'pages; lowest content baselines:',mins)
