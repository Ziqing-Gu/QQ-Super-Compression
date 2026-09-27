"""Chinese 1.2.7 manual, retaining the approved typography, palette and user-facing structure.
English remains unchanged pending Chinese approval.
"""
from pathlib import Path
import argparse
import math
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor, white
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, Table, TableStyle
from PIL import Image
from pypdf import PdfReader, PdfWriter
from pypdf.generic import NameObject
from io import BytesIO

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
    name='QQ Super Compression 用户手册 中文版_v1.2.7.pdf'
    c=canvas.Canvas(str(a.output/name),pagesize=(W,H),pageCompression=1)
    c.setTitle('QQ Super Compression 1.2.7 用户手册')
    c.setAuthor('Qing Audio');c.setSubject('动态处理、Limiter、等响试听与侧链使用指南')
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
        c.drawRightString(W-M,26,'QQ Super Compression 1.2.7  |  '+str(page))
        c.showPage()
    def start(title,subtitle=''):
        global page,y
        finish();page+=1;c.bookmarkPage('p'+str(page));c.addOutlineEntry(title,'p'+str(page),0)
        c.setFillColor(HexColor(MUTED));c.setFont(font,8)
        c.drawRightString(W-M,H-34,'QQ Super Compression  ·  用户手册')
        y=H-59;o=p_obj(title,22,TEAL,True);_,h=o.wrap(CW,150);o.drawOn(c,M,y-h);y-=h+11
        if subtitle:
            o=p_obj(subtitle,10,MUTED);_,h=o.wrap(CW,150);o.drawOn(c,M,y-h);y-=h+14

    def transfer_plot():
        global y
        x=M+43;top=y-8;w=CW-61;h=153
        px=lambda v:x+(v+50)/50*w
        py=lambda v:top-h+(v+50)/50*h
        c.setLineWidth(.5);c.setStrokeColor(HexColor('#CBDDE3'))
        for v in [-50,-40,-30,-20,-10,0]:
            c.line(x,py(v),x+w,py(v));c.line(px(v),py(-50),px(v),py(0))
            c.setFillColor(HexColor(MUTED));c.setFont('Body',7.5)
            c.drawRightString(x-7,py(v)-2,str(v));c.drawCentredString(px(v),py(-50)-13,str(v))
        c.saveState();c.setDash(3,3);c.setStrokeColor(HexColor(MUTED));c.line(px(-50),py(-50),px(0),py(0));c.restoreState()
        for colour,mode in [(TEAL,0),('#ED7C31',1)]:
            p=c.beginPath()
            for i in range(251):
                level=-50+i*.2;thr=-24;r=4
                if level<=thr:out=level
                elif mode==0:out=thr+(level-thr)/r
                else:out=level+20*math.log10((1+(r-1)*10**(thr/20))/(1+(r-1)*10**(level/20)))
                if i==0:p.moveTo(px(level),py(out))
                else:p.lineTo(px(level),py(out))
            c.setStrokeColor(HexColor(colour));c.setLineWidth(1.6);c.drawPath(p)
        c.setFillColor(HexColor(INK));c.setFont(font,8);c.drawString(M,top+2,'输出 dB');c.drawRightString(W-M,py(-50)-29,'检测电平 dB')
        c.setFont('Bold',8);c.setFillColor(HexColor(TEAL));c.drawString(x+8,py(-7),'CLASSIC');c.setFillColor(HexColor('#ED7C31'));c.drawString(x+86,py(-7),'SUPER')
        y=top-h-40
        o=p_obj('静态曲线示意：Threshold=-24 dB、Ratio=4:1、Range=OFF；未加补偿，未计边界过渡与检测时序。',8.3,MUTED)
        _,ht=o.wrap(CW,800);o.drawOn(c,M,y-ht);y-=ht+9

    start('QQ Super Compression','用户手册  /  1.2.7')
    heading('控制动态，尽量保留原本的起音。')
    para('用于人声、乐器、总线和母带。没有传统 Attack / Release 控件；使用 Lookahead、Ratio、Threshold 和 Mix 调整素材的动态表现。')
    para('Classic 按固定 dB 压缩比控制动态，Super 保留原有曲线。两种算法都支持单压、上压与双压；Limiter 加入最终峰值保护，并提供 TP / Peak 选择与耳机 1:1 等响试听。',9.6)
    picture('manual-light-ST.png',maxh=402)
    note('从听感出发','先决定需要多稳的动态、要保留多少起音，再调整参数。图表帮助理解变化，但不代替听音。')


    start('快速上手与阅读导航','第一次使用，先按下面六步走。')
    for i,(title,text) in enumerate([
      ('插入需要处理的轨道','先用 CLASSIC 算法、SINGLE、ST 和 SC: INT，让插件根据当前轨道工作。'),
      ('选择 Lookahead','可从 26 ms 开始。它是一个实用起点，不是每种素材都必须使用的设置。'),
      ('从 1:1 开始调整','普通压缩的 Ratio 默认 1:1。先试向下压缩时，把 Ratio 调高，并让 Range 保持 OFF；上压与双压见第 8-11 页，Limiter 见第 25-31 页。'),
      ('把 Mix 当作处理量工具','如果处理过重，降低 Mix，逐渐混回原始动态。提升与衰减的显示都会随 Mix 改变。'),
      ('补偿音量后比较','播放一段有代表性的素材，使用 Match，再用 Makeup / Output Gain 微调，避免被响度差误导。'),
      ('回到完整混音中听','切换 Bypass 或 A/B，检查起音、稳定度和律动，而不只是盯着 GR 数字。')],1):step(i,title,text)
    table(['想查什么','页码'],[
      ['界面、主题与基本操作','3-4'],['Classic / Super、单压与上压','5-8'],
      ['双压、独立开关与 Ratio LINK','9-11'],['Input / Output LINK、Match 与 A/B','12-14'],
      ['Display 与 Lookahead','15-17'],['ST / LR / MS 与声道 LINK','18-20'],
      ['侧链面板、内部 HPF 与外部路由','21-23'],['混音实例','24'],['Limiter、TP / Ceiling 与耳机试听','25-31'],['参数速查与排错','32-35']], [.78,.22],9)


    start('认识界面','主要音频控件保持在同一位置，换主题不改变功能。')
    picture('manual-light-ST.png','Light / ST。图中数值用于说明界面，不是固定推荐预设。',maxh=361)
    table(['区域','用来做什么'],[
      ['Display','查看当前设置下的历史电平与增益参考，见第 15 页。'],
      ['两根边界推子','单压为 Threshold / Range；双压为 UP / DOWN 阈值。'],
      ['Input / Output / Gain +/-','检查输入、输出电平与有效动态增益。'],
      ['底部旋钮与 SINGLE / DUAL','调整音量、Ratio 和 Mix；在 Ratio 旁切换单压 / 双压。'],
      ['Mode / LINK / Monitor','选择通道方式，联动参数或单独监听。'],
      ['顶部按钮','开关 Limiter，选择 Classic / Super、侧链、主题、A/B 与 Bypass。']], [.32,.68],9)
    para('界面采用 3:2 横向比例。拖动右下角可以调整大小，下次打开时会记住上一次的界面尺寸。',9.5)


    start('主题切换与基本操作')
    para('点击界面右上角显示 LIGHT、DARK 或 CLASSIC 的主题按钮，即可循环切换。下次打开会恢复上次选择。')
    picture('manual-theme-button.png','主题按钮位于 SC 按钮右侧、A/B 按钮左侧。',maxh=72,width=280,crop=(12,0,168,92))
    table(['主题','外观选择'],[['Light','浅色界面。'],['Dark','低亮度深色界面。'],['Classic','保留原经典深色界面。']], [.25,.75])
    para('三种主题只改变外观。CLASSIC 主题与 ALGO: CLASSIC 是两个独立选择；前者改变配色，后者改变处理算法。')
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


    start('Classic 与 Super：两种动态曲线','点击上方 ALGO 按钮切换；SINGLE 与 DUAL 均可使用。')
    table(['算法','怎样理解'],[
      ['Classic','阈值以上按固定 dB 比例压缩。下压时，超出阈值的 dB 数除以 Ratio，决定超出部分保留多少。'],
      ['Super','使用原有 QQ 曲线。Ratio 决定曲线力度，但不表示每一个电平位置都具有固定 dB 压缩比例。']], [.18,.82],9.5)
    transfer_plot()
    heading('一个具体例子')
    para('Classic 下压：Threshold=-10 dB、Ratio=5:1、Range=OFF。稳定检测电平为 0 dB 时，超出阈值 10 dB，压后保留 2 dB，得到 <b>-8 dB</b>。若检测电平为 -12 dB，未越过阈值，不施加动态衰减。',9.6,8)
    para('示例采用内部检测，Input、Makeup、Output Gain 均为 0 dB，Mix=100%。公式描述稳定检测电平，不是逐个采样点的硬限幅；动态素材与 Display 见第 15-16 页。',8.8,8)
    heading('向上分支仍然提升门槛以上的部分')
    para('两种算法的 Up 都以低阈值作为允许提升的门槛。Classic 使用与倒数 Down Ratio 对应的 dB 斜率；Super 使用原有向上曲线。两者的作用区间、Range 与 Dual 交接规则相同。',9.6,8)
    note('最低阈值与记忆','Classic 最低为 -90 dB，拖到底仍延续同一曲线；Super 保留 -inf。新实例记住上次手动选择，工程和 A/B 恢复各自保存的算法。切换采用短淡变，减少瞬态。')


    start('Ratio、Input、Mix：三种调法','它们都能影响结果，但不能互相精确替代。')
    picture('manual-light-controls.png',maxh=92)
    heading('Ratio')
    para('普通压缩的单压以 1:1 为中点：高于 1:1 时向下压缩，低于 1:1 时向上提升。在符合阈值范围的部分，向下数值越大通常压得越多；向上数值越小通常提得越多。双压将两种方向分成 UP RATIO 和 DOWN RATIO，见第 9 页。')
    para('Classic 与 Super 的 Ratio 含义见第 5 页。普通单压范围为 1:1000 至 1000:1，Ratio 默认 1:1。最终峰值保护由 Limiter 的 Ceiling 负责；仅把普通压缩的 Ratio 调大，不等于启用 Limiter。',9.6)
    heading('Input Gain')
    para('Input Gain 调整进入处理路径的电平，也会改变 Dry 路径的音量。使用 INT 时，它同时改变检测信号相对 Threshold 的位置。使用 EXT 时，外部检测电平由发送电平与 Key Gain 决定，不能用 Input Gain 代替。')
    heading('Mix')
    para('Mix 在 Dry 和处理后的 Wet 之间混合。它不只是一个附加效果比例，也是重要的处理量控制：先把提升或衰减调得明显，再降低 Mix，把需要的原始动态混回来。')
    note('动态增益包含 Mix','Mix = 0% 时有效动态增益为 0 dB；100% 时保留完整处理量。中间值不是简单的“增益 dB 数字乘以 Mix”。Makeup 和 Output Gain 不计入这项动态增益。')
    para('实用顺序：先调 Ratio 找到处理感，再用 Mix 控制最终力度；需要改变整体输入电平时才调 Input Gain，最后重新比较响度。')


    start('单压：Threshold 与 Range','先决定哪些电平参与处理，再用 Ratio 选择方向与力度。')
    picture('manual-light-single-up.png','SINGLE：下方 Threshold 与上方 Range 共同限定范围，推子与图中的虚线对应。',maxh=220)
    table(['检测电平','动态处理'],[
      ['低于或等于 Threshold','不作动态提升或衰减。'],
      ['高于 Threshold，且低于 Range','按 Ratio 向上或向下处理。'],
      ['达到或超过有限 Range','完全恢复为 0 dB 动态增益。']], [.46,.54],9.3)
    heading('Range OFF：没有上截止')
    para('Range 位于顶端 OFF 时不设置上界，按所选算法继续处理。将它从上往下拉，才启用有限上截止。有限的 0 dB 与 OFF 是两种不同设置。Classic 的 Threshold 最低为 -90 dB；Super 的最低位置 -inf 表示没有下限门槛。',9.8)
    para('接近有限 Range 时，动态增益会在范围内连续回到 0 dB，避免跨越上界时突然跳变。这是随检测电平变化的过渡，不额外加入 Attack 时间或延迟。',9.3)
    note('两根推子相碰时','Range 始终在 Threshold 上方或与其重合。拖动一根撞到另一根时，另一根会一起移动；两者重合时不作动态处理。反向拖动即可重新分开。')
    para('这里的“0 dB 动态增益”只指动态分支不提升、不衰减；Input、Makeup、Mix 和 Output Gain 仍按设置工作。Limiter 的最终 Ceiling 仍独立生效。EXT 模式下，范围判断使用外部检测信号。',9.3)


    start('向上压缩：提升门槛以上的部分','Classic 与 Super 都使用这项门槛规则，让选中的细节更靠前。')
    para('在 SINGLE 中，将 Ratio 从 1:1 向左调，例如 1:2、1:4、1:8，即进入向上压缩。界面上的 1:8 对应数值 1/8；数值越小，符合条件的部分通常获得越多提升。')
    regions([('保持原音量','低于或等于 Threshold'),('向上提升','Threshold 与 Range 之间'),('保持原音量','达到或超过有限 Range')],['Threshold','Range'],['#f3f6f7','#eaf6f8','#f3f6f7'])
    note('门槛决定是否提升，不把声音关掉','Threshold 在这里像一个允许提升的门槛。门槛以下仍保留原音量，并不是把更小的声音静音，也不是专门提升阈值以下的声音。')
    step(1,'先圈出想抬起来的内容','切到 SINGLE，以 Threshold 设下限、Range 设上限。例如先用 -50 dB / -10 dB，听需要的细节是否落在两者之间。数值只是示例。')
    step(2,'再调 Ratio 与 Mix','从 1:1 逐渐向 1:2、1:4 调整；觉得提升过多时，减小力度或降低 Mix。先不要用 Makeup 代替动态提升。')
    step(3,'同时听安静处和较响处','越接近下限门槛，提升会平滑进入；离开这段过渡后，作用范围内较轻的部分通常获得更多提升。接近上界时，提升趋近 0 dB。Range 为 OFF 时没有有限上截止。')
    para('想保留很轻的呼吸、底噪或残响原有音量时，将下限门槛放在它们上方，再听需要的主体是否仍能进入提升区。这里按检测电平选择，不识别声音种类。',9.5)


    start('双压：较轻处提升，较响处压低','点击 Ratio 旁的小按钮，将 SINGLE 切换为 DUAL。')
    picture('manual-light-dual.png','DUAL：UP RATIO 与 DOWN RATIO 分别控制两种方向；UP / DOWN 两个阈值替代 Threshold / Range。',maxh=245)
    regions([('保持原音量','低于或等于 UP 阈值'),('由 UP RATIO 提升','高于 UP、低于 DOWN'),('由 DOWN RATIO 压低','高于 DOWN 阈值')],['UP 阈值','DOWN 阈值'],['#f3f6f7','#eaf6f8','#eaf6f8'])
    para('达到 DOWN 阈值时，动态增益回到 0 dB；超过后停止上压，只由 Down Ratio 处理。两种增益不会在同一检测电平上叠加。Dual 不再设置 Range。',9.7)
    table(['新实例的起点','怎么理解'],[
      ['UP：Classic 为 -90 dB；Super 为 -inf。DOWN：0 dB。','低阈值与高阈值从两端开始；不是关闭上压。'],
      ['Up Ratio = 1:1；Down Ratio = 1:1','两种动态处理都处于中性状态，需调整 Ratio 才能听到效果。']], [.48,.52],9.2)
    note('阈值顺序与碰撞','DOWN 必须在 UP 上方或与其重合。拖动相撞会推着另一根一起移动；两阈值重合时，整个动态处理停止。')


    start('Dual 独立开关：直接试听上压与下压','UP RATIO 和 DOWN RATIO 旁，各有一个小 ON / OFF。')
    picture('manual-light-dual-controls.png','两路开启。开关保留各自的 Ratio 数值，便于反复比较。',maxh=148,width=245)
    table(['UP / DOWN','听到的处理'],[
      ['ON / ON','UP 与 DOWN 之间提升，超过 DOWN 后压低。'],
      ['ON / OFF','只保留上压；达到 DOWN 后仍停止提升。'],
      ['OFF / ON','只保留下压；低于 DOWN 时不作动态处理。'],
      ['OFF / OFF','不作动态提升或衰减；其余音量与 Mix 设置仍有效。']], [.27,.73],9.5)
    step(1,'先建立完整的 Dual 设置','设好两个阈值和 Ratio，播放包含轻声与较响片段的素材。')
    step(2,'用开关听清各自贡献','关闭 DOWN 听上压，关闭 UP 听下压，再同时开启。无需切回 SINGLE，也不必重设 Ratio。')
    step(3,'看显示，再做等响度比较','开关会改变对应的动态增益与 Display 历史参考。最后补偿整体音量，比较细节、起音和稳定度。')
    note('切换有淡变，边界仍然有效','每一路 ON / OFF 使用短交叉淡变，减少切换瞬态。关闭某一路不会移动阈值，也不会把另一方向扩展到原本不属于它的区间。LR / MS 中各声道可分别开关。')


    start('Ratio LINK：按相反倍数联动','两个 Ratio 之间偏上的小 LINK，只在 DUAL 中显示。')
    picture('manual-light-dual-controls.png','Up 与 Down 各自保留原来的相对关系；打开 LINK 不会使数值跳变。',maxh=100)
    para('首次使用时 Ratio LINK 默认开启。之后会记住你上次的选择；重新打开已保存的工程，则以工程保存的 LINK 状态为准。')
    table(['原来的 Down / Up','调整 Down','联动后的 Up'],[
      ['1:1 / 1:1','1:1 → 2:1','1:2'],
      ['4:1 / 1:2','4:1 → 8:1','1:4'],
      ['8:1 / 1:4','8:1 → 4:1','1:2']], [.36,.32,.32],9.6)
    heading('保留原有关系，而不是重新配对')
    para('例如 Down 加倍，Up 的数值就减半；调 Up 时也是同样的反向关系。从 4:1 与 1:2 出发，并不会被强行改成一对倒数。拖动、精细调整和直接输入都遵守这条规则。')
    note('到达边界，会一起停下','Up 的范围为 1:1000 至 1:1；普通压缩的 Down 为 1:1 至 1000:1，Limiter 的 Down 从 20:1 起。任意一方将超出范围时，联动在共同允许的位置停止；反向调整即可继续。只开关 LINK 不会改变当前 Ratio。')
    heading('想单独听上压时')
    para('可直接关闭 DOWN 旁的开关，保留 Down Ratio 数值。若还要单独调整 Up Ratio，再关闭两个 Ratio 中间的 LINK；否则调整 Up 时，Down 数值也会跟着改变。')
    heading('LR / MS 中仍然适用')
    para('小 LINK 会随紧凑布局缩小，仍位于 Up / Down Ratio 之间。它联动每个声道内部的上、下 Ratio；Mode 旁的声道 LINK 则联动 L/R 或 M/S，见第 20 页。')


    start('Input / Output LINK：反向补偿增益','位于 INPUT GAIN 右上方，与 Ratio LINK 分开工作。')
    picture('manual-input-output-link.png','示例：Input 为 +3 dB，Output 为 -3 dB，LINK 开启。',maxh=105)
    para('打开 LINK 后，在界面上增加 Input Gain，Output Gain 会减少同样的 dB 数；调整 Output 时，Input 也会反向跟随。这样可以在改变进入处理器的电平时，减少纯音量变化对判断的干扰。')
    table(['调整前 Input / Output','操作','调整后'],[
      ['0 / 0 dB','Input 增加 3 dB','+3 / -3 dB'],
      ['+2 / -1 dB','Input 增加 3 dB','+5 / -4 dB'],
      ['+5 / -4 dB','Output 增加 2 dB','+3 / -2 dB']], [.35,.33,.32],9.5)
    heading('保留已有偏移')
    para('它按变化量反向联动，不会强行把两个数值变成互为相反数。只开启或关闭 LINK，当前增益不会跳变；任意一侧到达允许边界时，联动共同停止。')
    heading('记住上一次选择')
    para('首次使用默认开启。之后新实例沿用你上次选择的状态；打开已保存工程时，以工程保存的状态为准。它属于操作习惯，不随 A/B 声音快照切换。')
    note('Limiter 使用另一组联动','普通 Input / Output LINK 只做增益的反向调整，不能替代 Match。开启 Limiter 后它隐藏并停止联动；Output 旁的 LINK 改为控制下压阈值与输出，见第 26 页。')


    start('Match 与 Makeup：补偿后再判断动态')
    table(['控件','作用'],[
      ['Makeup','补偿 Wet 的音量，位于 Mix 之前；范围 -120 至 +120 dB。'],
      ['Output Gain','普通压缩：-24 至 +24 dB；Limiter：-120 至 +120 dB。耳机监听见第 29-30 页。'],
      ['Match','根据已播放素材做一次响度补偿，并写入 Makeup；不是持续自动音量。']], [.24,.76],9.6)
    step(1,'播放有代表性的素材','让有效响度读数建立，再点击 Match。选择能代表实际段落的声音，避免只播放极短音头。')
    step(2,'按当前监听方式比较','普通模式比较 Dry 与补偿前 Wet；Limiter 耳机模式比较原始输入与实际限幅后的监听输出。提高 Makeup 会改变 Wet，见第 30 页。')
    step(3,'微调最终输出','必要时用 Makeup 或 Output Gain 小幅修正，再切换 Bypass 或 A/B，比较起音、尾音和段落稳定度。')
    heading('大压缩量与很低的电平')
    para('较深的压缩不再因为 Wet 低于固定响度门限而直接失去匹配。Makeup 的扩大范围也为较大的补偿量留出空间。Match 仍需要有效、非零的测量；完全静音或尚未积累足够素材时，不能产生可靠结果。')
    note('补偿响度与限制峰值分开判断','Match 用来比较响度；Limiter 的 Ceiling 控制最终峰值。完成 Match 后，仍需查看输出与 TP，尤其是在向上提升或继续修改参数以后。')
    heading('三个容易混淆的地方')
    para('动态增益显示不包含 Makeup 与 Output Gain。Input / Output LINK 只做等量反向调整，不能替代响度匹配。Mix = 0% 仍经过 Input 与 Output Gain，也不等于完整 Bypass。')


    start('A/B：比较算法，也比较处理方向')
    picture('manual-top-controls.png','A、B 为两组声音设置；A→B 与 B→A 用来复制。',maxh=62)
    step(1,'保存第一种想法','在 A 调好算法与处理量。用 A→B 将它复制到 B，再切换到 B。复制会替换目标一侧的声音设置。')
    step(2,'只改变想比较的部分','例如 A 使用 Classic，B 使用 Super；或 A 使用向下压缩，B 使用向上压缩。两边尽量保持同一 Lookahead、检测来源与监听方式。')
    step(3,'分别补偿，再切换','对两组设置分别播放相同素材并 Match，必要时微调 Makeup。点 A / B 比较；不要把更大声误认为动态更好。')
    table(['会随 A/B 恢复','不属于 A/B 声音快照'],[
      ['算法、单压 / 双压、阈值、Ratio、分支开关、音量、Mix、Lookahead、侧链，以及 Limiter、TP、Ceiling、Limiter Output LINK 与两套处理参数。','主题、普通 Input / Output LINK、Ratio / 声道 LINK、耳机 1:1 开关、Bypass 与 SC LISTEN。']], [.60,.40],9.3)
    heading('为什么倒数 Ratio 有时听起来一样？')
    para('上下曲线保留对称关系。在共同作用区内，一些倒数 Ratio 的结果只相差固定增益，等响度后会非常接近。例如 Super / SINGLE、Threshold = -inf、Range = OFF、Mix = 100% 时，8:1 与 1:8 的曲线存在这种关系。')
    para('Classic 在离开下限过渡的共同作用区内也有对应关系。跨越阈值、有限 Range 的过渡、改变 Mix 或 Dual 交接后，不应期待全段都完全相同。',9.5)
    note('切换与延迟','A/B 使用过渡处理来减少瞬态。比较时保持 Lookahead 一致，更便于只判断声音；若两边 Lookahead 不同，宿主还可能重新调整延迟补偿。')


    start('看懂 Display 与 Meter')
    picture('manual-light-dual.png','同一段历史中，绿色表示提升，衰减用 Cut / Mix 表示；橙色是输出电平参考。',maxh=239,crop=(32,152,2336,1072))
    table(['看到什么','含义'],[
      ['Dry / Input','Input Gain 之前的原始动态参考；所以调 Input 时灰线不会整体移动。'],
      ['Cut / Mix','包含 Mix 的动态衰减；与 Dry 的距离表示实际压低了多少。'],
      ['Boost / Mix','包含 Mix 的动态提升，以绿色表示；与 Dry 的距离表示提升了多少。'],
      ['Output','普通压缩为当前参数下的电平投影；Limiter 为当时采集的最终输出电平，含 Ceiling 与耳机补偿。'],
      ['External key','EXT 且外部通道可用时，以较淡的曲线显示侧链参考，用来核对触发。'],
      ['Gain +/-','0 dB 位于中央，提升向上、衰减向下。正值表示提升，负值表示衰减；Hold 保留近期较明显的变化。']], [.25,.75],8.9)
    para('动态增益不包含 Makeup / Output Gain；Mix = 0% 时为 0 dB。三主题均可查看提升与衰减，具体颜色可对照图下方的同名图例。',9.3)
    note('历史也会跟着参数变化','调整 Ratio、阈值、Range、Mix 等参数，会更新可见历史。EXT 的 Key Gain 实时更新；HPF 松手后才刷新，期间可能显示 HPF UPDATING。普通压缩按当前参数重算历史电平参考。Limiter 的橙线保留当时实际输出，调参后请观察新进入的片段；TP 以右侧表头读数为准。')


    start('Lookahead：预读，但对齐正在处理的声音','Classic 与 Super 使用同一套电平检测时序。')
    para('Lookahead 让插件在输出当前声音前，先看到后面一小段信号。这样可以用较稳定的电平估计决定增益，减少增益追着每个正弦周期变化而产生的谐波。它不是传统压缩器的 Attack。')
    heading('怎样减少大声到来前的凹陷')
    para('检测同时参考当前声音之前与之后的窗口，并采用两边峰值中较低的一边。未来的大声不再单独决定前面安静片段的衰减；当大声真正到来时，再按对齐后的电平计算处理量。')
    # Same palette as the existing range diagrams; schematic, not a measured trace.
    top=y;mid=M+CW/2
    c.setStrokeColor(HexColor(MUTED));c.setLineWidth(.7);c.line(M+14,top-25,W-M-14,top-25)
    for xx,ww,tt in [(M+14,CW/2-14,'过去窗口'),(mid,CW/2-14,'未来窗口')]:
        c.setFillColor(HexColor(PALE));c.roundRect(xx,top-56,ww,24,3,fill=1,stroke=0)
        c.setFillColor(HexColor(TEAL));c.setFont(font,9);c.drawCentredString(xx+ww/2,top-48,tt)
    c.setStrokeColor(HexColor(TEAL));c.line(mid,top-14,mid,top-62)
    c.setFillColor(HexColor(INK));c.setFont(font,9);c.drawCentredString(mid,top-5,'当前正在处理的声音')
    c.setFillColor(HexColor(MUTED));c.setFont(font,8);c.drawCentredString(mid,top-80,'示意：两个窗口都包含当前时刻；实际输出仍按 Lookahead 延后。')
    y=top-101
    note('默认起点仍为 26 ms','压缩检测仍按选定窗口工作。1.2.7 的最终峰值保护另需固定缓冲，因此宿主报告的总延迟会略大于 Lookahead；切换 Limiter / TP 时保留这段延迟，见第 31 页。')
    heading('保留低失真目标，也理解动态信号的取舍')
    para('稳定正弦可以在合适的窗口中得到稳定的增益，尽量保留波形形状。音乐不是恒定的正弦：快速调幅、短促变化和复杂叠加仍可能产生调制残差；不能把 Lookahead 理解为对所有信号都保证零谐波失真。')
    para('新版优先减少不必要的提前衰减，让处理更贴近当前声音。若某段素材仍有细小凹陷或波形变化，应结合检测来源、窗口长度和音频实测判断，不能只凭一条 Display 曲线下结论。',9.5)


    start('Lookahead 与 0 ms 模式')
    para('Lookahead 影响电平估计与延迟，不是 Attack。窗口加长可能改善部分低频或快速变化素材的稳定度，但不能保证每一种瞬态都更好。<br/>窗口越长，插件延迟也越大；宿主通常会补偿这段延迟。')
    picture('manual-light-0ms.png','0 ms 时，Lookahead 下方出现 Oversampling。',maxh=327)
    table(['选择','用途与取舍'],[
      ['10 ms','较短窗口，延迟较低。'],
      ['26 / 40 ms','默认 26 ms；需要时与 40 ms 对照，按听感选择。'],
      ['80 / 100 ms','更长窗口；听是否更稳定，同时留意额外延迟。'],
      ['0 ms','更明显的非线性色彩，不是最透明模式。']], [.24,.76],9)
    para('0 ms 的 Oversampling 可选 1x / 8x / 16x。可从 8x 开始；更高倍率会增加负荷和延迟。10 ms 及以上时隐藏。0 ms 搭配 8x / 16x 仍有额外延迟，最终峰值保护的固定缓冲也计入宿主报告。',9.5)


    start('ST 与 LR：一起处理，或分别处理')
    para('点击右下方 Mode 按钮，按 ST → MS → LR 循环切换。通常先用 ST；只有确实需要左右不同处理时，再选择 LR。')
    picture('manual-light-LR.png','LR：上方为 L，下方为 R，各有独立控件。',maxh=373)
    table(['模式','怎么用'],[
      ['ST','对立体声使用共同压缩增益，适合作为人声、乐器和总线的起点。'],
      ['LR','左右各有独立的 Ratio、边界、Makeup 和 Mix；DUAL 时每侧各有 UP / DOWN。处理不对称素材时，同时检查声像是否偏移。']], [.17,.83],9.4)
    note('MONITOR','选择 L 或 R 可单独监听对应一侧，并居中呈现，便于比较。结束后回到 ALL，检查正常立体声结果；不要把单侧监听留作最终输出。')


    start('MS：分别控制中心与两侧')
    picture('manual-light-dual-MS.png','MS / DUAL：上方为 Mid，下方为 Side；每个声道各有 UP / DOWN Ratio 和阈值。',maxh=340)
    para('Mid 主要反映左右共有的内容，Side 反映左右差异。它们并不精确对应“中间一条轨道”和“其余所有轨道”。')
    step(1,'先听 M 与 S 各包含什么','用 MONITOR 的 M / S 检查素材，再回到 ALL。')
    step(2,'按目的分别调整','例如需要收住中心动态时，先调 M 的 Ratio / Mix；需要保留环境与宽度时，对 S 谨慎处理。')
    note('用完整混音复查','M/S 的压缩或音量变化可能改变空间感。回到 ALL 后检查中心、宽度和单声道兼容性，不只听独奏。')


    start('声道 LINK：保留差异，一起调整')
    para('在 LR 或 MS 模式下，Mode 旁的 LINK 用于声道之间的同类参数。先分别调好两侧，再打开它，就能一起调整而不把两侧强制拉成一样。')
    picture('manual-light-LR-link.png','SINGLE 示例：Mode 旁的声道 LINK 已开启，左右 Ratio 仍分别为 3:1 与 5:1。',maxh=110)
    table(['参数','原设置','调整后示例'],[
      ['Ratio','3:1 / 5:1','4:1 / 6:1'],['Threshold','-20 / -10 dB','-18 / -8 dB'],
      ['Makeup','-3 / +1 dB','-2 / +2 dB'],['Mix','100% / 70%','90% / 60%']], [.26,.36,.38])
    step(1,'关闭 LINK，建立两侧关系','先确定哪一侧需要更强的压缩或更少的混合。')
    step(2,'打开 LINK，一起微调','拖动任意一侧同类参数，另一侧跟随同样的变化量。精调和直接输入也适用。')
    note('到达边界会一起停下','任意一侧将超出范围时，两侧会停在能保留差值的位置。Super 中一侧 Threshold 为 -inf 时，不按有限 dB 差值联动；先把两侧设为有效阈值再建立关系。')
    heading('三种 LINK，各有分工')
    para('Ratio 中间的小 LINK 管每个声道内的 Up / Down 反向联动；Mode 旁的 LINK 管 L/R 或 M/S 的同类参数。两者都开启时，声道间一起变化，各声道内的 Up / Down 仍保持原来的相对关系。Input 旁的 LINK 只管输入与输出增益的反向补偿，见第 12 页。')


    start('侧链：先认识面板','点击右上角 SC: INT / SC: EXT 打开，按钮文字表示当前来源。')
    picture('manual-sidechain-panel.png',maxh=185,width=415)
    table(['控件','使用方法'],[
      ['INT / EXT','INT 根据当前轨道本身判断；EXT 根据另一条轨道送来的信号控制当前轨道的提升或衰减。'],
      ['KEY GAIN','只在 EXT 可用。调整外部 Key 的检测电平，改变触发强度，不直接调主轨道音量。'],
      ['HPF','减少检测信号中的低频；作用于 INT 或 EXT。OFF 为全频，开启后范围 20-500 Hz。'],
      ['KEY LEVEL','查看送到检测器的信号是否有电平。EXT 显示 N/A 时，先检查宿主侧链通道是否启用。'],
      ['SC LISTEN','临时听检测信号，帮助检查路由与 HPF；听完关闭，再判断正常输出。']], [.24,.76],9.4)
    note('两个常用任务','低频一来整段都被压：用 INT + HPF，见第 22 页。<br/>希望鼓声出现时贝斯让位：用 EXT，见第 23 页。')


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


    start('把操作放回真实混音')
    heading('人声、吉他、钢琴：稳住动态，保留表达')
    para('先试 SINGLE / ST / INT 和 26 ms，把 Ratio 调到 1:1 以上，用 Mix 找到字头、拨弦或琴槌仍自然的力度。需要只控制较响部分时，提高 Threshold，并让 Range 保持 OFF。使用 Match 后回到伴奏中比较。')
    heading('尾音与轻声：让选中的细节靠前')
    para('想抬起较轻的字、拨弦尾音或残响时，试单压向上；用 Threshold 排除不想提升的微小声音，再用 Range 限定上界。想同时收住较响部分时，切到 Dual，分别调整 Up 与 Down；可用两个独立 ON / OFF 分别试听；需要单独调 Ratio 时先关闭 Ratio LINK。')
    heading('Mix Bus / 母带：追求整体凝聚感')
    para('当你想要整体更稳、更有 Glue，又不希望传统 Attack / Release 明显重塑鼓的音头时，可以把它作为 G Bus 类压缩的另一种选择。先用轻度处理，观察前后段落的平衡；需要时降低 Mix。')
    para('底鼓一来就压低全曲时，试 INT + HPF；不要为了让 GR 更小而把 HPF 调到忽略所有低频。最后用相近响度检查鼓的起音、低频、空间感和整首歌的起伏。')
    heading('人声触发伴奏：清出唱词空间')
    para('把插件插在伴奏总线上，将人声送到它的外部侧链，选 EXT。先用 SC LISTEN 确认来源；选择 SINGLE、Ratio 高于 1:1、Range OFF，再调 Key Gain、Threshold 和 Mix，让有人声时伴奏适当退后。')
    para('如果爆破音触发过重，试着提高侧链 HPF。如果呼吸或底噪也让伴奏退让，先检查发送信号和 Threshold；不要期待 HPF 单独解决所有误触发。')
    note('每次比较都做这三件事','关闭 SC LISTEN；声道 MONITOR 回到 ALL；确认前后响度接近。需要控制最终峰值时，开启 Limiter 并设置 Ceiling；等响试听可配合耳机按钮与 Match，见第 25-31 页。')


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


    start('参数速查：算法、动态与范围')
    table(['参数','范围 / 选项','说明'],[
      ['ALGO','Classic / Super','固定 dB 比例 / 原有 QQ 曲线；记住上次选择，工程与 A/B 优先。'],
      ['SINGLE / DUAL','单压 / 双压','Ratio 旁的小按钮；使用各自的参数。'],
      ['Single Ratio','普通：1:1000 至 1000:1','普通默认 1:1。Limiter 上压至 1:1，下压区为 20:1 至 1000:1。'],
      ['Up Ratio','1:1000 至 1:1','默认 1:1；负责向上提升。'],
      ['Down Ratio','普通：1:1 至 1000:1；Limiter：20:1 至 1000:1','普通默认 1:1；进入 Limiter 下压从 20:1 起。'],
      ['UP / DOWN 开关','ON / OFF','默认均 ON；独立试听，保留 Ratio。'],
      ['Threshold / UP THR','Classic：-90 至 0 dB；Super：-inf 至 0 dB','下限门槛；默认在所选算法的最低位置。'],
      ['Range','OFF 或有限上界','单压默认 OFF；有限上界为 Classic -90 至 0 dB，Super -inf 至 0 dB。'],
      ['DOWN THR','Classic：-90 至 0 dB；Super：-inf 至 0 dB','双压交接阈值，默认 0 dB。'],
      ['Ratio LINK','OFF / ON','首次默认 ON；记住选择，工程优先。'],
      ['Mode / 声道 LINK','ST / MS / LR；OFF / ON','LINK 联动 LR / MS 的同类参数。']], [.23,.34,.43],8.9)
    note('边界与默认值','Range 不低于 Threshold；DOWN 不低于 UP。相撞时一起移动，重合时停止动态处理。已有工程保留保存的设置；普通压缩各声道的三类 Ratio 默认均为 1:1；Limiter 的下压范围单独限制。')
    para('Super 的有限阈值最低为 -119.99 dB，再往下为 -inf；Display 可见范围最低为 -90 dB。Range OFF 与有限 0 dB 不是同一状态。',9.2)


    start('参数速查：音量、监听与侧链')
    table(['参数','范围 / 选项','说明'],[
      ['Input Gain','-24 至 +24 dB','进入处理的电平。'],
      ['Makeup','-120 至 +120 dB','默认 0 dB；Wet 音量，位于 Mix 前。'],
      ['Mix','0 至 100%','默认 100%；包含在有效动态增益中。'],
      ['Output Gain','普通 ±24 dB；Limiter ±120 dB','Limiter 的耳机模式抵消其监听增益，数值和联动仍有效。'],
      ['Input / Output LINK','OFF / ON','首次默认 ON；等量反向联动，记住选择，工程优先。'],
      ['MONITOR','ALL / L / R 或 M / S','LR / MS 可用；单域居中监听。'],
      ['Lookahead','0 / 10 / 26 / 40 / 80 / 100 ms','初始起点 26 ms；新实例会记住上次选择。'],
      ['Oversampling','1x / 8x / 16x','只在 0 ms 可见；初始 8x。'],
      ['SOURCE','INT / EXT','默认 INT；EXT 需宿主路由。'],
      ['KEY GAIN','-24 至 +24 dB','默认 0 dB；只在 EXT 可调。'],
      ['HPF','OFF / 20 至 500 Hz','默认 OFF；只过滤检测信号。'],
      ['SC LISTEN','OFF / ON','临时试听；关闭面板或编辑器即关闭。'],
      ['Limiter / TP','默认 OFF / ON','TP 只在 Limiter 下生效；关闭 TP 使用采样峰值。'],
      ['Ceiling','-24 至 0 dB','默认 0；Alt 单击恢复 0。'],
      ['耳机 1:1','默认 OFF','工程保存，独立于 A/B；MON = Ceiling - Output。']], [.22,.34,.44],8.3)
    note('不要混淆几个音量入口','Input 调主信号输入；Key Gain 只调外部检测信号；Makeup 调 Mix 之前的 Wet；Output Gain 调最终输出。比较动态前，先把前后响度调到接近。')


    start('上压与双压：没有变化时检查什么')
    table(['现象','先检查什么'],[
      ['调整后没有听到提升','确认对应 Ratio 低于 1:1，Mix 大于 0，Bypass 关闭；Dual 的对应分支需为 ON。'],
      ['Dual 的 Up 没有效果','检测电平要高于 UP、低于 DOWN。UP 以下原样输出，高于 DOWN 时只做下压。'],
      ['推子调好却完全不工作','两根边界是否重合？重合会使整个动态处理停止；分开后再比较。'],
      ['较响的部分突然不再处理','SINGLE 是否设置了有限 Range？达到或超过它时动态增益回到 0 dB；需要无上界时选 OFF。'],
      ['调 Up，Down 也在变化','两个 Ratio 中间的 LINK 默认开启。需要独立调整时，先关闭它。'],
      ['LINK 不能继续往同一方向调','另一侧可能已到范围边界；联动会共同停止。反向调整即可继续。'],
      ['看着主轨道电平却不按预期触发','检查 SOURCE 是否为 EXT，或 HPF 是否改变了检测信号。阈值判断依据是检测信号。']], [.33,.67],9.2)
    heading('单独确认双压的向上作用')
    step(1,'把其他变化先固定','选择 DUAL，关闭 DOWN 旁的开关，并关闭 Ratio LINK。保持 Mix 100%，各音量增益为 0 dB，选择合适的 Lookahead。')
    step(2,'确保素材在两个阈值之间','例如 UP 为 -50 dB、DOWN 为 -20 dB，确认一段轻声的检测电平主要落在两者之间。此时 Ratio 仍为 1:1，不会出现提升。')
    step(3,'从 1:1 逐渐增加上压力度','将 Up Ratio 依次调向 1:2、1:4、1:8，同时听音，观察 Boost / Mix 与正向动态增益。确认后再开启 DOWN，加入下压，重新比较整体响度。')


    start('遇到问题，先检查这里')
    heading('Classic 压缩后，为什么有时低于阈值？')
    para('阈值约束的是检测电平上的增益曲线，不是最终输出的最低线。一个窗口内比检测峰值更轻的声音，也会受到这段增益影响；ST 共用增益、外部侧链或 HPF 也会使检测电平与眼前的声音不同。Makeup 与 Output Gain 还会继续改变输出。',9.5)
    para('Range OFF、内部检测、各音量为 0 dB、Mix 100% 时，稳定且高于阈值的电平按 Classic 公式压缩后仍不低于阈值。若只在 Display 看见下探，请先记住它是历史投影，再用实际音频核对；不必把每次下探都归因于提前压缩。',9.5)
    table(['现象','先检查什么'],[
      ['EXT 没有效果','确认宿主侧链输入和发送已启用，SOURCE 为 EXT，KEY LEVEL 有读数。N/A 表示未提供外部通道。'],
      ['Key Gain 不能调整','当前为 INT 时正常；它只调整外部 Key。'],
      ['HPF 不切输出低频','它只过滤检测信号。历史在松手后刷新，等 HPF UPDATING 消失再看。'],
      ['声音变成侧链 / 变窄','关闭 SC LISTEN；MONITOR 回到 ALL，再检查 LR / MS。'],
      ['Match 灰显','播放足够的非静音素材，建立有效读数；低于旧门限不再自动禁用。'],
      ['切换 Lookahead 有停顿','插件延迟改变，宿主可能重新调整补偿；录音监听也要考虑延迟。'],
      ['图中没有 -90 dB 以下','这是 Display 绘制范围，不表示声音被截断。']], [.30,.70],8.9)
    para('Limiter 的 TP、Ceiling、耳机与输出问题见第 31 页。安装、更新和扫描请参阅对应平台的安装说明。',9)
    para('使用许可：Qing Audio 非商业源码共享许可证 1.0。禁止商业使用；完整条款与对应源码见项目仓库。',8.7)
    para('<link href="https://github.com/Ziqing-Gu/QQ-Super-Compression" color="'+TEAL+'">github.com/Ziqing-Gu/QQ-Super-Compression</link>',9)

    finish();c.save()
    doc=PdfReader(a.output/name);assert len(doc.pages)==35
    txt='\n'.join(p.extract_text() for p in doc.pages)
    for banned in ('Revision','Rev 2','Plan A','Plan B','Plan C','JUCE','offscreen','离屏','2.2x','归一化','缓存','v1.0.1','附页','addendum','schema'):
        assert banned not in txt,(lang,banned)
    print(lang, len(doc.pages),'pages; lowest content baselines:',mins)
