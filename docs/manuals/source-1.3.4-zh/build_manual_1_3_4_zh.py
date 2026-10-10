"""Chinese 1.3.4 review manual; updated from the approved 1.2.42 layout. English awaits user approval."""
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
    name='QQ Super Compression 用户手册 中文版_v1.3.4.pdf'
    c=canvas.Canvas(str(a.output/name),pagesize=(W,H),pageCompression=1)
    c.setTitle('QQ Super Compression 1.3.4 用户手册')
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
        for row in rows:
            values=[]
            for j,v in enumerate(row):
                text=escape(str(v))
                if page==2 and j==1:
                    target=str(v).split('-')[0]
                    text='<link href="#p'+target+'" color="'+TEAL+'">'+text+'</link>'
                values.append(p_obj(text,size))
            cells.append(values)
        tb=Table(cells,colWidths=[CW*w for w in widths])
        tb.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor(PALE)),
          ('ROWBACKGROUNDS',(0,1),(-1,-1),[white,HexColor('#f6f9fa')]),
          ('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),
          ('TOPPADDING',(0,0),(-1,-1),4 if page==2 else 6),('BOTTOMPADDING',(0,0),(-1,-1),4 if page==2 else 6)]))
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
        c.drawRightString(W-M,26,'QQ Super Compression 1.3.4  |  '+str(page))
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
        o=p_obj('静态曲线示意：Threshold=-24 dB、Ratio=4:1，阈值以上继续压缩；未加补偿，未计电平过渡与检测时序。',8.3,MUTED)
        _,ht=o.wrap(CW,800);o.drawOn(c,M,y-ht);y-=ht+9

    start('QQ Super Compression','用户手册  /  1.3.4 Candidate · 中文审阅版')
    heading('控制动态，尽量保留原本的起音。')
    para('用于人声、乐器、总线和母带。动态核心没有传统 Attack / Release 旋钮；使用 Lookahead、SAFE、Ratio、Threshold 和 Mix 调整素材，Limiter 的 TP 另有三种恢复方式。')
    para('Classic 按固定 dB 压缩比控制动态，Super 使用原有 QQ 曲线。两种算法都支持单压、上压与双压；所有 Lookahead 共用一处 OS；Limiter 提供 Hard Clip / TP、恢复方式与耳机 1:1 试听。',9.6)
    picture('manual-light-ST.png',maxh=402)
    note('从听感出发','先决定需要多稳的动态、要保留多少起音，再调整参数。图表帮助理解变化，但不代替听音。')


    start('快速上手与阅读导航','先完成声音设置，再按需要阅读进阶操作。')
    for i,(title,text) in enumerate([
      ('选择起点','先用 CLASSIC、SINGLE、ST、SC: INT；Lookahead 从 26 ms 开始，SAFE 默认关闭、Window 为 26 ms。'),
      ('从 1:1 调整 Ratio','Ratio 决定处理方向与力度：向右压低，向左提升。Threshold 决定从多大的声音开始处理，详见第 7 页。'),
      ('用 Mix 控制最终处理量','处理过重时，逐渐混回原始动态。上压与双压见第 8-11 页。'),
      ('等响度比较','播放有代表性的素材，点击 Match，再用 Makeup / Output Gain 微调。'),
      ('Limiter 按常规下压流程使用','Ratio 设为 20:1 至 1000:1，保持 Output LINK，逐渐下拉 Threshold，让 Output 自动补偿。详见第 28 页。'),
      ('回到完整混音中听','切换 Bypass 或 A/B，检查起音、尾音、密度与律动。')],1):step(i,title,text)
    table(['查找什么','页码'],[
      ['界面、主题与基本操作','3-4'],['Classic / Super、单压与上压','5-8'],
      ['双压、独立开关与 Ratio LINK','9-11'],['Input / Output LINK、Match 与 A/B','12-14'],
      ['Display','15'],['Lookahead 与 Window：关系及操作','16-17'],['SAFE：检测方式与试听步骤','18-19'],
      ['ST / LR / MS 与声道 LINK','20-22'],['侧链与混音实践','23-26'],
      ['Limiter、L/R Link、Ceiling 与等响试听','27-34'],['参数速查与排查','35-38'],
      ['独立模式设置、过采样与延迟','39-40'],['TP 恢复与输出 LUFS-I','41-42'],
      ['FULL / ECO 与停播恢复','43-44']], [.78,.22],8.5)

    start('认识界面','主要音频控件保持在同一位置，换主题不改变功能。')
    picture('manual-light-ST.png','Light / ST。图中数值用于说明界面，不是固定推荐预设。',maxh=361)
    table(['区域','用来做什么'],[
      ['Display','查看当前设置下的历史电平与增益参考，见第 15 页。'],
      ['Threshold / Range 推子','Threshold：超过它才开始压缩。Range：达到或超过它就不压缩。普通单压用这两根推子选择处理的部分，详见第 7 页。'],
      ['Input / Output / Gain +/-','检查输入、输出电平与有效动态增益。'],
      ['底部旋钮与 SINGLE / DUAL','调整音量、Ratio 和 Mix；在 Ratio 旁切换单压 / 双压。'],
      ['右下四排','Mode（Limiter 为 L/R Link）、SAFE、Lookahead + Window、Monitor + OS；Limiter 不显示 Monitor。'],
      ['顶部按钮','开关 LIMITER，选择 FULL / ECO、侧链、主题、A/B 与 Bypass。算法按钮位于底部 Ratio 区。']], [.32,.68],9)
    para('界面采用 3:2 横向比例。拖动右下角可以调整大小，下次打开时会记住上一次的界面尺寸。',9.5)


    start('主题切换与基本操作')
    para('点击界面右上角显示 LIGHT、DARK 或 CLASSIC 的主题按钮，即可循环切换。下次打开会恢复上次选择。')
    picture('manual-theme-button.png','主题按钮位于 SC 按钮右侧、A/B 按钮左侧。',maxh=52,width=125)
    table(['主题','外观选择'],[['Light','浅色界面。'],['Dark','低亮度深色界面。'],['Classic','保留原经典深色界面。']], [.25,.75])
    para('三种主题只改变外观。CLASSIC 主题与 Ratio 区的 CLASSIC 算法是两个独立选择；前者改变配色，后者改变处理算法。')
    # Two visual choices, not implementation descriptions.
    yy=y;gap=14;cell=(CW-gap)/2
    for n,label,x in [('manual-dark.png','Dark',M),('manual-classic.png','Classic',M+cell+gap)]:
        im=Image.open(a.captures/n);ww,hh=im.size;im.close();ht=cell*hh/ww
        c.drawImage(str(a.captures/n),x,yy-ht,cell,ht,mask='auto');c.setFont('Body',9);c.setFillColor(HexColor(MUTED));c.drawString(x,yy-ht-14,label)
    y=yy-ht-30
    table(['操作','结果'],[
      ['拖动旋钮 / Shift 拖动','普通调整 / 精细调整；拖动中可按下或松开 Shift。'],
      ['双击数值','直接输入精确数值。'],
      ['Alt + 左键','恢复默认值；联动开启时，相关控件也会跟随并受范围限制。'],
      ['Ctrl/Cmd + Z','撤销；加 Shift 则重做。']], [.39,.61],9)


    start('Classic 与 Super：两种动态曲线','在底部 Ratio 区选择算法；DUAL 的 UP / DOWN 可分别选择。')
    table(['算法','怎样理解'],[
      ['Classic','阈值以上按固定 dB 比例压缩。下压时，超出阈值的 dB 数除以 Ratio，决定超出部分保留多少。'],
      ['Super','使用原有 QQ 曲线。Ratio 决定曲线力度，但不表示每一个电平位置都具有固定 dB 压缩比例。']], [.18,.82],9.5)
    transfer_plot()
    heading('一个具体例子')
    para('Classic 下压：Threshold=-10 dB、Ratio=5:1。本例让阈值以上的声音继续参与压缩。稳定检测电平为 0 dB 时，超出阈值 10 dB，压后保留 2 dB，得到 <b>-8 dB</b>。若电平为 -12 dB，没有超过阈值，就不压低。',9.6,8)
    para('示例采用内部检测，Input、Makeup、Output Gain 均为 0 dB，Mix=100%。公式描述稳定检测电平，不是逐个采样点的硬限幅；动态素材与 Display 见第 15-16 页。',8.8,8)
    heading('向上压缩：提升超过阈值的部分')
    para('Classic 与 Super 都支持向上压缩。将 Ratio 调到 1:1 以下，超过 Threshold 的部分会得到提升。两种算法的提升力度不同，但都用 Threshold 与 Range 选择要处理的声音；具体操作见第 7-8 页。',9.6,8)
    note('最低阈值与记忆','普通模式 Classic 最低 -90 dB，Super 保留 -inf；Limiter 全部阈值统一为 -45 至 0 dB。普通模式的新实例记住上次手动选择，工程和 A/B 恢复保存的算法；Limiter 使用独立设置。切换采用短淡变，减少瞬态。')


    start('Ratio、Input、Mix：三种调法','它们都能影响结果，但不能互相精确替代。')
    picture('manual-light-controls.png',maxh=92)
    heading('Ratio')
    para('普通压缩的单压以 1:1 为中点：高于 1:1 时向下压缩，低于 1:1 时向上提升。在符合阈值范围的部分，向下数值越大通常压得越多；向上数值越小通常提得越多。双压将两种方向分成 UP RATIO 和 DOWN RATIO，见第 9 页。')
    para('Classic 与 Super 的 Ratio 含义见第 5 页。普通单压范围为 1:200 至 200:1，Ratio 默认 1:1。最终峰值保护由 Limiter 的 Ceiling 负责；仅把普通压缩的 Ratio 调大，不等于启用 Limiter。',9.6)
    heading('Input Gain')
    para('Input Gain 调整进入处理路径的电平，也会改变 Dry 路径的音量。使用 INT 时，它同时改变检测信号相对 Threshold 的位置。使用 EXT 时，外部检测电平由发送电平与 Key Gain 决定，不能用 Input Gain 代替。')
    heading('Mix')
    para('Mix 在 Dry 和处理后的 Wet 之间混合。它不只是一个附加效果比例，也是重要的处理量控制：先把提升或衰减调得明显，再降低 Mix，把需要的原始动态混回来。')
    note('动态增益包含 Mix','Mix = 0% 时有效动态增益为 0 dB；100% 时保留完整处理量。中间值不是简单的“增益 dB 数字乘以 Mix”。Makeup 和 Output Gain 不计入这项动态增益。')
    para('实用顺序：先调 Ratio 找到处理感，再用 Mix 控制最终力度；需要改变整体输入电平时才调 Input Gain，最后重新比较响度。')


    start('单压：Threshold 与 Range','先决定哪些声音要处理，再调整处理的力度。')
    heading('Threshold：超过阈值才开始压缩')
    para('Threshold 决定从多大的声音开始处理。低于或等于它的部分不进行压缩；超过它，才按 Ratio 向下压低或向上提升。')
    heading('Range：超过它的部分不进行压缩')
    para('Range 让较响的部分保留原来的动态。达到或超过 Range 时，不再向下压低，也不再向上提升。因此，只有 Threshold 与 Range 之间的部分参与压缩。普通单压界面中，Range 推子位于 Threshold 上方。')
    heading('怎样使用？')
    para('例如 Threshold = -30 dB、Range = -10 dB：',9.6,6)
    table(['声音的电平','会发生什么'],[
      ['-30 dB 及以下','不进行压缩。'],
      ['高于 -30 dB，低于 -10 dB','按 Ratio 压低或提升。'],
      ['-10 dB 及以上','不进行压缩，保留这部分的动态。']], [.42,.58],9.3)
    para('调低 Range，会让更多较响的部分不参与压缩；调高 Range，则允许更响的部分继续参与。可先固定 Threshold，再拖动 Range，听哪些声音开始恢复原来的动态。',9.5)
    heading('Range OFF：关闭 Range 的作用')
    para('把 Range 推到顶部 OFF，Range 推子就不再限制哪些较响的声音要停止压缩。超过 Threshold 的部分继续按所选算法处理；OFF 关闭的是 Range 的作用，压缩本身仍然工作。',9.5)
    note('最后了解这几种情况','Range = 0 dB：达到 0 dB 就停止压缩；Range OFF：不再由 Range 决定何时停止压缩。<br/>Range 与 Threshold 重合：没有可处理的区间，动态处理停止。推子相碰会一起移动，反向拖动即可分开。<br/>接近 Range 时，处理量会逐渐减小，达到它时完全停止；这是电平变化带来的过渡，没有另加 Attack / Release。')
    para('<b>如果出现噼啪声：</b>启用 Range 后，部分素材可能出现噼啪声。可以尝试点亮 SAFE，让检测更稳定，再比较噼啪声是否减少。SAFE 也可能压低强音前后的声音，需要一起试听；详细用法见第 18-19 页。',9.3)
    para('本页说的“不进行压缩”仅指不再压低或提升动态；Input、Makeup、Mix 与 Output Gain 仍然有效。使用 EXT 时，按外部侧链的电平判断。Limiter 不使用 Range，见第 28 页。',9.1)


    start('向下压缩与向上压缩','先看压缩怎样处理，再比较补偿音量后的结果。')
    heading('向下压缩：压低超过阈值的部分')
    para('在 SINGLE 中，把 Ratio 从 1:1 向右调，例如 2:1、4:1、8:1，就进入向下压缩。超过 Threshold 的部分被压低，较响与较轻声音之间的差距缩小；低于或等于 Threshold 的部分不被压低。')
    para('下压后，主体可能听起来变小。再提高 Makeup 或 Output，让主体回到便于比较的音量时，阈值以下、原本没有压缩的声音也会跟着变大。',9.6)
    heading('向上压缩：提升超过阈值的部分')
    para('把 Ratio 从 1:1 向左调，例如 1:2、1:4、1:8，就进入向上压缩。在允许处理的部分中，较轻的声音通常提升得更多，较响的声音提升得较少，因此也能缩小动态差距。低于或等于 Threshold 的部分不会被提升。')
    para('Classic 与 Super 都支持这两种处理方式。两者的力度曲线不同，但这里的阈值规则相同；启用 Range 后，达到或超过 Range 的部分也不再压低或提升，见第 7 页。',9.6)
    heading('先不加音量补偿，看两者做了什么')
    table(['声音的位置','向下压缩','向上压缩'],[
      ['低于或等于 Threshold','不压低','不提升'],
      ['Threshold 以上的处理区域','压低声音，缩小动态差距','提升声音，也能缩小动态差距'],
      ['启用 Range 后，达到或超过 Range','不压低','不提升']], [.37,.30,.33],9.3)
    heading('上压与下压的对称关系')
    para('在同一种算法内，把下压 8:1 与上压 1:8 的阈值以上声音匹配到相同音量，主要处理区域内的结果可以相同。Classic 与 Super 各自都有这种对称关系。区别在于补偿音量也会改变阈值以下的声音，因此不能只比较主体；第 14 页有具体数值。',9.6)
    note('怎样选择','想连同阈值以下的轻声一起抬高，可尝试下压后补回音量。想提升选中的细节，同时保留更轻的底噪、呼吸或残响，可尝试向上压缩，并将 Threshold 设在不想提升的声音上方。最后都应回到完整混音里比较。')
    para('接近 Threshold 时，上压的提升会逐渐进入；接近 Range 时，处理量会逐渐减小。因此阈值附近、Range 附近不应理解为两种处理必然完全相同。调整 Mix 后也要重新试听。',9.3)


    start('双压：较轻处提升，较响处压低','点击 Ratio 旁的小按钮，将 SINGLE 切换为 DUAL。')
    picture('manual-light-dual.png','DUAL：UP RATIO 与 DOWN RATIO 分别控制两种方向；UP / DOWN 两个阈值替代 Threshold / Range。',maxh=245)
    regions([('保持原音量','低于或等于 UP 阈值'),('由 UP RATIO 提升','高于 UP、低于 DOWN'),('由 DOWN RATIO 压低','高于 DOWN 阈值')],['UP 阈值','DOWN 阈值'],['#f3f6f7','#eaf6f8','#eaf6f8'])
    para('达到 DOWN 阈值时，动态增益回到 0 dB；超过后停止上压，只由 Down Ratio 处理。两种增益不会在同一检测电平上叠加。Dual 不再设置 Range。',9.7)
    table(['新实例的起点','怎么理解'],[
      ['普通 UP：Classic 为 -90 dB，Super 为 -inf。DOWN：0 dB。','低阈值与高阈值从两端开始；不是关闭上压。'],
      ['Up Ratio = 1:1；Down Ratio = 1:1','上压与下压都不改变动态；调整 Ratio 后才能听到效果。']], [.48,.52],9.2)
    note('阈值顺序与碰撞','DOWN 必须在 UP 上方或与其重合。拖动相撞会推着另一根一起移动；两阈值重合时，整个动态处理停止。')


    start('Dual 独立开关：直接试听上压与下压','UP / DOWN 分别有开关和算法按钮，可独立试听与选择曲线。')
    picture('manual-light-dual-controls.png','两路开启；上、下算法分别显示在对应 Ratio 区。开关保留 Ratio 数值。',maxh=148,width=245)
    table(['UP / DOWN','听到的处理'],[
      ['ON / ON','UP 与 DOWN 之间提升，超过 DOWN 后压低。'],
      ['ON / OFF','只保留上压；达到 DOWN 后仍停止提升。'],
      ['OFF / ON','只保留下压；低于 DOWN 时不作动态处理。'],
      ['OFF / OFF','不作动态提升或衰减；其余音量与 Mix 设置仍有效。']], [.27,.73],9.5)
    step(1,'先建立完整的 Dual 设置','设好两个阈值和 Ratio，播放包含轻声与较响片段的素材。')
    step(2,'用开关听清各自贡献','关闭 DOWN 听上压，关闭 UP 听下压，再同时开启。两路可各选 Classic / Super；改变 UP 算法不会同时改变 DOWN 算法。')
    step(3,'看显示，再做等响度比较','开关会改变对应的动态增益与 Display 历史参考。最后补偿整体音量，比较细节、起音和稳定度。')
    note('开关只影响对应的上压或下压','每一路 ON / OFF 使用短交叉淡变，减少切换瞬态。关闭某一路不会移动阈值，也不会把另一方向扩展到原本不属于它的区间。普通 LR / MS 中各声道可分别开关；Mode 的 LINK 开启时，L/R 或 M/S 的对应 UP ON 同步，DOWN ON 也同步，上压和下压的开关仍然独立。')


    start('Ratio LINK：按相反倍数联动','普通压缩 DUAL 的两个 Ratio 之间显示小 LINK；Limiter 不提供该联动。')
    picture('manual-light-dual-controls.png','Up 与 Down 各自保留原来的相对关系；打开 LINK 不会使数值跳变。',maxh=100)
    para('首次使用时 Ratio LINK 默认开启。之后会记住你上次的选择；重新打开已保存的工程，则以工程保存的 LINK 状态为准。')
    table(['原来的 Down / Up','调整 Down','联动后的 Up'],[
      ['1:1 / 1:1','1:1 → 2:1','1:2'],
      ['4:1 / 1:2','4:1 → 8:1','1:4'],
      ['8:1 / 1:4','8:1 → 4:1','1:2']], [.36,.32,.32],9.6)
    heading('保留原有关系，而不是重新配对')
    para('例如 Down 加倍，Up 的数值就减半；调 Up 时也是同样的反向关系。从 4:1 与 1:2 出发，并不会被强行改成一对倒数。拖动、精细调整和直接输入都遵守这条规则。')
    note('达到最小值或最大值时，会一起停下','普通压缩 Up 为 1:200 至 1:1，Down 为 1:1 至 200:1。任意一方将超出范围时，联动在共同允许的位置停止；反向调整即可继续。只开关 LINK 不会改变当前 Ratio。')
    heading('想单独听上压时')
    para('可直接关闭 DOWN 旁的开关，保留 Down Ratio 数值。若还要单独调整 Up Ratio，再关闭两个 Ratio 中间的 LINK；否则调整 Up 时，Down 数值也会跟着改变。')
    heading('LR / MS 中仍然适用')
    para('普通 LR / MS 的小 LINK 联动每个声道内部的上、下 Ratio；Mode 旁的 LINK 联动声道，见第 22 页。Limiter 的 UP / DOWN Ratio 始终独立，先调下压，再补少量上压，见第 29 页。')


    start('Input / Output LINK：反向补偿增益','位于 INPUT GAIN 右上方，与 Ratio LINK 分开工作。')
    picture('manual-input-output-link.png','示例：Input 为 +3 dB，Output 为 -3 dB，LINK 开启。',maxh=105)
    para('打开 LINK 后，在界面上增加 Input Gain，Output Gain 会减少同样的 dB 数；调整 Output 时，Input 也会反向跟随。这样可以在改变进入处理器的电平时，减少纯音量变化对判断的干扰。')
    table(['调整前 Input / Output','操作','调整后'],[
      ['0 / 0 dB','Input 增加 3 dB','+3 / -3 dB'],
      ['+2 / -1 dB','Input 增加 3 dB','+5 / -4 dB'],
      ['+5 / -4 dB','Output 增加 2 dB','+3 / -2 dB']], [.35,.33,.32],9.5)
    heading('保留已有偏移')
    para('它按变化量反向联动，不会强行把两个数值变成互为相反数。只开启或关闭 LINK，当前增益不会跳变；任意一侧达到最小值或最大值时，联动共同停止。')
    heading('记住上一次选择')
    para('首次使用默认开启。之后新实例沿用你上次选择的状态；打开已保存工程时，以工程保存的状态为准。工程与 A/B 会保存当前 LINK 状态；调用快照后先核对联动开关，再继续调整。')
    note('Limiter 使用另一组联动','普通 Input / Output LINK 只做增益的反向调整，不能替代 Match。开启 Limiter 后它隐藏并停止联动；Output 旁的 LINK 让下压阈值与输出反向联动，常规用法见第 28 页。')


    start('Match 与 Makeup：累计播放，再做补偿','播放中可点击；每次都使用本次播放开始至点击时的累计内容。')
    table(['控件','作用'],[
      ['Makeup','补偿 Wet 的音量，位于 Mix 之前；普通模式 ±30 dB，Limiter ±120 dB。'],
      ['Output Gain','普通压缩 ±24 dB；Limiter ±120 dB。耳机试听见第 32-33 页。'],
      ['Match','点击后按累计响度写入 Makeup；不是持续自动音量。']], [.24,.76],9.5)
    step(1,'开始播放，建立有效数据','先选好普通压缩 / Limiter 与耳机状态，再播放有代表性的非静音素材。建立有效响度数据后，播放中就可以点击 Match。')
    step(2,'按需要再次点击','第一次点击不清空记录。假设在第 10 秒和第 20 秒点击，分别使用开始至第 10 秒、开始至第 20 秒的内容；不会把上次补偿再叠加一遍。')
    step(3,'重新播放，才开始新一段','停止后保留本次结果；下次开始播放时重新累计。播放没有停止时，循环和跳转仍属于同一次 Match 统计。')
    note('FULL 与 ECO','FULL 在关闭界面后继续统计。ECO 要做完整累计 Match，须在播放前打开界面并一直保持打开；中途缺失分析时，本次 Match 不可用，重新开始播放后恢复。')
    heading('固定比较条件，再判断声音')
    para('普通压缩比较 Dry 与补偿前 Wet；Limiter 耳机模式比较原始输入与实际限幅后的监听结果，并写入唯一一组 Makeup。点击 Match 或调整 Makeup 都不重设累计起点。',9.5)
    para('切换 Limiter 或耳机状态会改变测量基准并清空统计；请在开始测量前选好。完全静音或数据不足时不能匹配。需要只比较新设置时，停止后重新播放同一段素材。',9.5)
    note('响度与峰值分开看','Match 补偿响度，Ceiling 控制最终峰值。完成后仍需查看输出与 TP，并用 Bypass / A/B 比较起音、尾音和动态。')


    start('A/B：比较算法，也比较处理方向')
    picture('manual-top-controls.png','A、B 为两组声音设置；A→B 与 B→A 用来复制。',maxh=62)
    step(1,'保存第一种想法','在 A 调好算法与处理量。用 A→B 将它复制到 B，再切换到 B。复制会替换目标一侧的声音设置。')
    step(2,'只改变想比较的部分','例如 A 使用 Classic，B 使用 Super；或 A 使用向下压缩，B 使用向上压缩。两边尽量保持同一 Lookahead、检测来源与监听方式。')
    step(3,'分别补偿，再切换','对两组设置分别播放相同素材并 Match，必要时微调 Makeup。点 A / B 比较；不要把更大声误认为动态更好。')
    table(['会随 A/B 恢复','不属于 A/B 声音快照'],[
      ['算法、单压 / 双压、阈值、Ratio、上压 / 下压开关、音量、Mix、Lookahead、SAFE、Window、总 OS、侧链，以及 Limiter、TP、Ceiling、TP 恢复、各类 LINK 状态与两套处理参数。','主题、FULL / ECO、耳机 1:1、声道独奏、Bypass 与 SC LISTEN。']], [.60,.40],9.3)
    heading('上压与下压的对称关系：8:1 与 1:8')
    para('分别在 Classic 内、Super 内比较：保持同一 Threshold、Lookahead、Window 与 SAFE 设置，Range 为 OFF、Mix 为 100%，不使用 Ceiling 等额外处理。以阈值以上的声音为参考补偿音量，主要处理区域内的结果可以相同。',9.4)
    para('例：Classic，Threshold = -24 dB。A 为下压 8:1，再补偿 +21 dB；B 为上压 1:8，不加补偿。下表为稳定电平的结果：',9.3,6)
    table(['输入电平','A：下压后补偿','B：上压'],[
      ['-12 dB（阈值以上）','-1.5 dB','-1.5 dB'],
      ['-6 dB（阈值以上）','-0.75 dB','-0.75 dB'],
      ['-36 dB（阈值以下）','-15 dB','-36 dB']], [.42,.29,.29],9.1)
    para('阈值以下原本都没有压缩，但 A 的整体补偿也把它抬高了，所以两者出现区别。Super 也有这种对称关系，只是需要的补偿量不同；这不表示 Super 与 Classic 的声音相同。',9.3)
    para('阈值附近的上压过渡、启用 Range、改变 Mix 或极端提升量都可能打破这种对应关系。Match 按整段播放内容计算响度，包含阈值以下的声音；要看清这个例子，应以阈值以上的主体对齐音量。',9.1)
    note('切换与延迟','A/B 使用过渡处理来减少瞬态。比较时保持 Lookahead 与过采样一致，更便于只判断声音；模式或倍率改变时，宿主还可能重新调整延迟补偿。A/B 恢复各自保存的 Window 值，Display 按相应设置对齐重算。')


    start('看懂 Display 与 Meter','Limiter 图表为 0 至 -45 dB；普通压缩为 0 至 -90 dB。')
    picture('manual-light-dual.png','灰：输入参考；蓝：Cut / Mix；绿：Boost / Mix；橙：输出参考。',maxh=220,crop=(32,152,2336,1072))
    table(['看到什么','含义'],[
      ['Dry / Input','Input Gain 之前的历史电平参考；调 Input 不会把灰线整体移动。'],
      ['Cut / Mix、Boost / Mix','按当前算法、检测与 Mix 重新计算的动态参考。Limiter 的蓝线还包含当前 Ceiling 的衰减估计。'],
      ['Output','根据当前参数计算的输出电平参考，包含音量、Mix 和当前 Hard Clip / TP Ceiling 的影响。'],
      ['External key','外部通道可用时显示较淡的侧链参考，用来核对触发来源。'],
      ['右侧 GAIN +/-','实时处理的增益变化；提升为正、衰减为负。Hold 保留近期较明显的变化。']], [.25,.75],9.1)
    note('八秒历史会随参数变化','调整 Ratio、阈值、Mix、音量、Limiter 或 TP，已有历史会按当前设置重新计算。HPF 在松手或设置稳定后刷新；HPF UPDATING 表示正在更新。横轴按音频时间推进；Lookahead 与 A/B 切换后，历史仍按采集时的音频延迟对齐，避免整体错位。')
    para('Display 不是录下的最终波形，也不是逐采样真峰值测量。蓝线与灰线使用的参考可因 INT / EXT 检测而不同，不能仅凭曲线间距离推断每个采样的实际 GR。最终输出峰值看 TP 表，实时动态看右侧表头。',9.4)
    para('Ratio 1:1、各增益 0 dB、内部全频检测且未启用 Ceiling 等额外处理时，输入与输出历史应重合。0 ms 时两种检测方式相同，切换 SAFE 不会产生另一套动态曲线。',9.3)


    start('Lookahead 与 Window：延迟和声音分开调','保持延迟不变，连续试听不同 Lookahead 长度的声音。')
    para('Lookahead 表示为提前预读预留多少毫秒。插件将音频延后输出，才能提前分析后面的信号；它设定基础延迟，并限定 Window 的最大值。Window 决定每次计算压缩增益时，实际参考前后多长范围内的峰值。')
    table(['控制','决定什么'],[
      ['Lookahead','设定基础延迟。可选 0、26、40、80、100 ms；默认 26 ms。改变它时，宿主可能需要重新调整延迟补偿。'],
      ['Window','在 0 至当前 Lookahead 之间调整实际检测长度。改变声音，但保持已设定的延迟不变。']], [.24,.76],10)
    note('Window 就是在固定延迟下降低实际 Lookahead','例如 Lookahead 26 ms / Window 10 ms，与 Lookahead 10 ms / Window 10 ms 相比：其余参数相同、设置稳定并对齐延迟后，动态核心的压缩结果与谐波染色相同。前者只多保留 16 ms 基础延迟。因此无需反复切换插件延迟，也能比较更短 Lookahead 的声音。')
    heading('例：Lookahead 一直保持 26 ms')
    table(['Window','实际检测长度','基础延迟'],[
      ['26 ms','使用完整 26 ms','26 ms，不变'],
      ['10 ms','缩短为 10 ms','26 ms，不变'],
      ['0 ms','按当前采样检测','26 ms，不变']], [.24,.45,.31],9.8)
    para('这里的基础延迟不含 OS 滤波和 Limiter Ceiling 的额外延迟；保持其余设置不变时，只调整 Window，插件报告的总延迟也不会改变。',9.5)
    heading('为什么适合连续比较声音？')
    para('直接切换 Lookahead，宿主可能重新调整延迟补偿，从而出现短暂停顿。改调 Window 则保留同一延迟，让你在避免这类卡顿的情况下，连续比较检测长度对音头、尾音、动态和染色的影响。')
    heading('Window 是参考时长，不是压缩间隔')
    para('Window = 10 ms，不表示每隔 10 ms 才压缩一次。它表示每次计算参考当前采样前后各 10 ms 的峰值，再由 Normal / Safe 决定取哪一边。检测范围逐采样移动，增益也逐采样更新。缩短 Window 改变参考范围，不改变压缩更新频率。',9.6)

    start('Lookahead 与 Window：操作及 0 ms','先用 Window 选声音，再决定是否缩短 Lookahead 延迟。')
    picture('manual-safe-timing.png','Lookahead 与 Window 位于右下控制区的同一排；Window 的单位始终为 ms。',maxh=115,width=140)
    table(['Lookahead','Window 可调范围','默认 / Alt 单击'],[
      ['0 ms','固定 0 ms，不可调','0 ms'],
      ['26 ms','0-26 ms','26 ms'],
      ['40 / 80 / 100 ms','0 至所选 Lookahead','等于所选 Lookahead。']], [.25,.36,.39],9.3)
    table(['操作','结果'],[
      ['双击 / 上下拖动 / Shift 拖动','输入毫秒 / 连续调整 / 精细调整。'],
      ['Alt 单击 Window','恢复当前 Lookahead 允许的最大值。'],
      ['手动切换 Lookahead','Window 跟随新的 Lookahead，恢复最大值。'],
      ['A/B、工程恢复、重新打开界面','恢复已存 Window，不当作手动切换复位。']], [.43,.57],9.1)
    note('26 ms / Window 0 ms，与 0 ms / Window 0 ms','在相同算法、参数和 OS 下，两者都按当前采样进行动态检测；前者保留 26 ms 基础延迟，后者移除这部分延迟。若 Window = 0 ms 的声音最合适，可再切到 0 ms Lookahead；切换时让宿主完成延迟补偿。Limiter 的具体试听见第 34 页。')
    para('多数普通压缩可保持完整 Window。缩短后，局部动态、谐波染色和噼啪声可能变得更明显；数值较低，尤其低于 10 ms 时，建议开启过采样，见第 40 页。Window 调节避免的是延迟补偿变化造成的卡顿，不代表处理音色不变。',9.5)
    para('旧工程的 10 ms Lookahead 会迁移为 26 ms；仍可将 Window 设为 10 ms，使用相同长度的检测。0 ms Lookahead 也不代表整条路径零延迟：OS 滤波和 Limiter Ceiling 的额外延迟仍可能存在。',9.2)

    start('SAFE：检测方式与提前压','独立选择检测策略；不改变已设定的 Lookahead 延迟。')
    picture('manual-safe-button.png','SAFE 位于右下区域第二排；黄色点亮为 Safe，未点亮为 Normal。新实例默认关闭。',maxh=42,width=105)
    para('先按前两页确定 Lookahead 与 Window，再选择是否开启 SAFE。Window 决定检测长度；SAFE 决定怎样使用这一长度内的前后峰值。两者分别控制检测的范围和取舍。')
    table(['检测方式','怎样处理前后峰值'],[
      ['Normal / SAFE 关闭','分别取过去、未来 Window 的峰值，再选较小的一边。通常保留更多局部动态，减少单个强音对邻近弱音的影响。'],
      ['Safe / SAFE 点亮','取两边峰值中较大的一边。较长 Window 可使检测更稳定，有助于抑制快速增益变化产生的连续噼啪声。']], [.25,.75],9.6)
    heading('什么时候考虑开启？')
    para('带 Delay 或混响的干净吉他、人声等素材，叠加后的电平变化可能让增益快速来回变化。普通单压启用 Range 时，有些素材也会出现类似噼啪。此时可以开启 SAFE，先保留完整 Window，再比较主体、回声和延音。',9.6)
    note('代价：提前压与峰后压','未来强音可能在到达前就压低邻近弱音；强音结束后，过去 Window 仍含这个峰值，也可能继续压低后面的弱音。因此强音前后可能出现凹陷。减小 Window 可缩短影响，但也会减弱对噼啪声的抑制。')
    heading('与 Classic / Super 的区别')
    para('SAFE 改变检测电平的取法；Classic / Super 决定检测到该电平后使用哪条动态曲线。这里的提前压和峰后压来自所选 Window，没有在动态核心中另加隐含 Attack / Release。',9.5)
    heading('默认、切换与 0 ms')
    para('SAFE 默认关闭，随工程和 A/B 保存。切换 SAFE 保持当前 Window 数值及延迟；Lookahead 为 0 ms 时按钮变灰禁用，原选择仍保留。只要实际 Window 为 0 ms，Normal 与 Safe 的检测结果就相同。',9.5)
    para('下一页给出固定参数比较 SAFE、再调整 Window 的试听步骤。',9.5)

    start('SAFE：噼啪声与提前压的试听步骤','优先听同一段带 Delay / 混响的干净素材，再回到完整混音。')
    step(1,'从完整 Window 建立比较','Lookahead 先选 26 ms，SAFE 关闭、Window 26 ms。固定 Ratio、阈值、Makeup、Mix 与 Output，听清噼啪声和主体动态。')
    step(2,'点亮 SAFE，先保留完整 Window','Safe 取前后较大的峰值，较长 Window 有助于抑制增益快速变化引起的噼啪。与 Normal 比较音头、延音和连续回声，不只比较响度。')
    step(3,'逐渐缩短 Window','如果强音前后被压低得太明显，缓慢减小 Window，寻找噼啪声控制与提前压之间的平衡。Lookahead 和基础延迟保持不变；Alt 单击可回到完整 Window。')
    step(4,'最后比较总 OS','在选好的 Window 下比较 1x、4x、8x、16x。较高 OS 可减少混叠，但不会替代 Window 选择。最后用 A/B 等响比较，并检查最终输出峰值。')
    para('普通单压启用 Range 时，有些素材也会出现噼啪声。此时同样可以尝试 SAFE，先保留完整 Window，再按听感缩短。',9.5)
    table(['希望得到什么','优先尝试'],[
      ['保留更大的局部动态','SAFE 关闭，从完整 Window 开始。'],
      ['主动增加染色','Normal 中逐渐缩短 Window，同时留意噼啪与粗糙感，并开启过采样。'],
      ['减少 Window 相关噼啪声','SAFE 点亮，先用完整 Window，再按素材缩短。'],
      ['缩短 Safe 的前后凹陷','减小 Window；保护也随之减弱，Window = 0 ms 时两种检测相同。']], [.38,.62],9.3)
    note('Safe 的取舍要一起听','较长 Window 可能在强音到来前就压低弱音，并在强音结束后维持一小段衰减。这是提前压与峰后压的结果，不是新增的 Attack / Release。Safe 主要针对这类快速增益调制，不能修复输入已削波或宿主掉帧。')
    para('没有可见凹陷，也不代表两个 Window 的失真相同：稳态周期内的增益变化可能已经不同。可以同时听实际音频、看谐波与动态阶跃；插件 Display 用于理解设置，不能代替这些检查。',9.5)



    start('ST 与 LR：一起处理，或分别处理')
    para('普通压缩的右下方 Mode 按 ST → MS → LR 循环切换。通常先用 ST；需要两侧分别调参时选择 LR。Limiter 不显示此选择，使用单张 ST 界面和数值 L/R Link，见第 27 页。')
    picture('manual-light-LR.png','LR：上方为 L，下方为 R，各有独立控件。',maxh=373)
    table(['模式','怎么用'],[
      ['ST','对立体声使用共同压缩增益，适合作为人声、乐器和总线的起点。'],
      ['LR','左右各有独立的 Ratio、阈值、Makeup 和 Mix；DUAL 时每侧各有 UP / DOWN。处理不对称素材时，同时检查声像是否偏移。']], [.17,.83],9.4)
    note('MONITOR','选择 L 或 R 可单独监听对应一侧，并居中呈现，便于比较。结束后回到 ALL，检查正常立体声结果；不要把单侧监听留作最终输出。')


    start('MS：分别控制中心与两侧')
    picture('manual-light-dual-MS.png','MS / DUAL：上方为 Mid，下方为 Side；每个声道各有 UP / DOWN Ratio 和阈值。',maxh=340)
    para('Mid 主要反映左右共有的内容，Side 反映左右差异。它们并不精确对应“中间一条轨道”和“其余所有轨道”。')
    step(1,'先听 M 与 S 各包含什么','用 MONITOR 的 M / S 检查素材，再回到 ALL。')
    step(2,'按目的分别调整','例如需要收住中心动态时，先调 M 的 Ratio / Mix；需要保留环境与宽度时，对 S 谨慎处理。')
    note('用完整混音复查','M/S 的压缩或音量变化可能改变空间感。回到 ALL 后检查中心、宽度和单声道兼容性，不只听独奏。')


    start('声道 LINK：保留差异，一起调整')
    para('在普通压缩 LR 或 MS 模式下，Mode 旁的 LINK 用于声道之间的同类参数。先分别调好两侧，再打开它，就能一起调整而不把两侧强制拉成一样。')
    picture('manual-light-LR-link.png','SINGLE 示例：Mode 旁的声道 LINK 已开启，左右 Ratio 仍分别为 3:1 与 5:1。',maxh=110)
    table(['参数','原设置','调整后示例'],[
      ['Ratio','3:1 / 5:1','4:1 / 6:1'],['Threshold','-20 / -10 dB','-18 / -8 dB'],
      ['Makeup','-3 / +1 dB','-2 / +2 dB'],['Mix','100% / 70%','90% / 60%']], [.26,.36,.38])
    step(1,'关闭 LINK，建立两侧关系','先确定哪一侧需要更强的压缩或更少的混合。')
    step(2,'打开 LINK，一起微调','拖动任意一侧同类参数，另一侧跟随同样变化量。Dual 中，切换一侧 UP ON 会同步另一侧 UP ON；DOWN ON 同理，上压和下压仍独立。')
    note('达到最小值或最大值时，会一起停下','任意一侧将超出范围时，两侧会停在能保留差值的位置。Super 中一侧 Threshold 为 -inf 时，不按有限 dB 差值联动；先把两侧设为有效阈值再建立关系。')
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
    note('两个常用任务','低频一来整段都被压：用 INT + HPF，见第 24 页。<br/>希望鼓声出现时贝斯让位：用 EXT，见第 25 页。')


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
    para('没有传统 Attack / Release 可调；避让形状会受触发素材、Lookahead、SAFE、Window 和压缩设置共同影响，不保证在所有素材上都比传统侧链更顺滑。',9.3)


    start('把操作放回真实混音')
    heading('人声、吉他、钢琴：稳住动态，保留表达')
    para('先试 SINGLE / ST / INT 和 26 ms，把 Ratio 调到 1:1 以上，用 Mix 找到字头、拨弦或琴槌仍自然的力度。需要只控制较响部分时，提高 Threshold，并让 Range 保持 OFF。使用 Match 后回到伴奏中比较。')
    para('带 Delay / 混响的干净吉他或人声若出现连续噼啪，可开启 SAFE、保持完整 Window，再逐步缩短 Window 比较；具体步骤见第 19 页。',9.3,8)
    heading('尾音与轻声：让选中的细节靠前')
    para('想抬起较轻的字、拨弦尾音或残响时，试单压向上；用 Threshold 排除不想提升的微小声音，再用 Range 排除不想提升的较响部分。想同时收住较响部分时，切到 Dual，分别调整 Up 与 Down；可用两个独立 ON / OFF 分别试听；需要单独调 Ratio 时先关闭 Ratio LINK。')
    heading('Mix Bus / 母带：追求整体凝聚感')
    para('当你想要整体更稳、更有 Glue，又不希望传统 Attack / Release 明显重塑鼓的音头时，可以把它作为 G Bus 类压缩的另一种选择。先用轻度处理，观察前后段落的平衡；需要时降低 Mix。')
    para('底鼓一来就压低全曲时，试 INT + HPF；不要为了让 GR 更小而把 HPF 调到忽略所有低频。最后用相近响度检查鼓的起音、低频、空间感和整首歌的起伏。')
    heading('人声触发伴奏：清出唱词空间')
    para('把插件插在伴奏总线上，将人声送到它的外部侧链，选 EXT。先用 SC LISTEN 确认来源；选择 SINGLE、Ratio 高于 1:1、Range OFF，再调 Key Gain、Threshold 和 Mix，让有人声时伴奏适当退后。')
    para('如果爆破音触发过重，试着提高侧链 HPF。如果呼吸或底噪也让伴奏退让，先检查发送信号和 Threshold；不要期待 HPF 单独解决所有误触发。')
    note('每次比较都做这三件事','关闭 SC LISTEN；普通 LR / MS 的 MONITOR 回到 ALL；确认前后响度接近。需要控制最终峰值时，开启 Limiter 并设置 Ceiling；等响试听可配合耳机按钮与 Match，见第 28-34 页。')


    start('Limiter L/R Link：控制左右的相互影响','一张 ST Display、一组参数，保留左右独立限幅的能力。')
    picture('manual-limiter-light.png','右下第一排为 L/R Link 数值，不再提供 ST / LR / MS 选择或声道 Monitor。',maxh=200)
    table(['L/R Link','处理方式'],[
      ['0%（默认 / Alt 复位）','左右各自检测并控制自己的衰减；一侧的强音不会仅因链接而压低另一侧。'],
      ['0%-100% 之间','逐渐增加左右衰减的相互影响，用听感选择独立动态与声像稳定之间的平衡。'],
      ['100%','把两侧中较强的衰减应用到两侧，减少左右增益变化不一致造成的声像移动。']], [.30,.70],9.5)
    para('L/R Link 同时用于动态处理和最终 Ceiling。上压产生的提升不会仅因链接而抬高另一侧。双击输入百分比，上下拖动调整，Shift 微调；Alt 单击回到 0%。',9.4)
    heading('响度优先，还是细节与声像优先')
    para('追求更大响度，建议从 0% 开始。更重视立体声细节与声像稳定时，逐渐调大 L/R Link；它让左右更多地共同衰减，不等于自动恢复被压缩的细节。用相近响度判断取舍。',9.4)
    heading('共用参数，不代表共用全部检测结果')
    para('Ratio、Makeup、Mix 及阈值只显示一组，左右使用同样的设置。Dual 的 UP / DOWN Ratio 各自独立，没有 Ratio LINK。Output 旁的 LINK 仍负责下压阈值、Makeup 与输出补偿，和这里的音频关联量不同。',9.5)
    note('打开旧 Limiter 工程时','原 ST 设置迁移后采用 100% L/R Link；原 LR / MS 采用 0%。低于 -45 dB 的阈值收至 -45 dB，旧 Range 不再参与处理。旧双组参数以原 L（MS 为原 M）组作为共同值，旧 MS 改为左右处理。两组原来不同的工程可能改变声音，请核对并试听。普通压缩的 LR / MS 保持原功能。')

    start('Limiter 常规用法：下压、关联与 Window','先建立下压，再平衡响度、动态与立体声细节。')
    picture('manual-limiter-light.png','单张 ST 图、共用参数；图表与所有阈值均为 -45 至 0 dB，Limiter 不使用 Range。',maxh=205)
    step(1,'从 SINGLE 建立下压','Ratio 先用 20:1 或以上，最高 1000:1；Mix 100%，设置 Ceiling 与 TP，保持 Output 旁的 LINK 开启。慢慢降低 Threshold，Output 会按相同 dB 数反向补偿。')
    step(2,'用 L/R Link 选择左右关联量','追求更大响度，建议从默认 0% 开始，让左右独立限幅。更重视立体声细节与声像稳定时，逐渐调大；两侧会更多地共同衰减，响度也可能受影响。')
    step(3,'用 Window 寻找响度与动态的平衡','普通压缩多数情况下可保持完整 Window；Limiter 中它对响度与动态尤其重要。固定 Lookahead，逐渐减小 Window 比较；较低数值尤其低于 10 ms 时，建议开启过采样。')
    step(4,'Window 为 0 最好时，再试 0 ms Lookahead','在其他参数和 OS 一致时，两者都使用 Window = 0 ms 检测。先在原延迟下选声音，再切到 0 ms 减少基础延迟；切换 Lookahead 时仍需让宿主完成补偿。详见第 34 页。')
    note('区分两个 Link','L/R Link 决定左右衰减的相互影响；Output LINK 负责下压阈值、Makeup 与输出补偿。完成这些调整后，再考虑补少量 UP，并用耳机 + Match + Bypass 等响比较。')

    start('Limiter 双压：下压调好，再补少量上压','双压是常规下压之后的补偿用法。')
    picture('manual-limiter-dual-gentle.png','示例：DOWN 保持 100:1，UP 为 1:1.2；Output LINK 开启；上、下 Ratio 独立，没有 Ratio LINK。',maxh=205)
    step(1,'先完成上一页的下压','先用 20:1 至 1000:1 的 Ratio、Threshold 与 Output LINK 调出合适的 Limiter 下压。记住这时的算法和 Ratio。')
    step(2,'切换 DUAL，保留原来的下压设置','Single 的 Threshold 会延续到 DOWN 阈值。DOWN RATIO 与 Single Ratio 分别保存：核对 DOWN 的算法和 Ratio，必要时设成刚才单压的值，保持已经调好的下压。')
    step(3,'只补少量 UP','保留 Output LINK。Limiter 已取消 Ratio LINK，调整 UP 不会联动 DOWN 的比例。UP 开启后，从 1:1 轻轻调向 1:1.2，通常这一小段已经足够；用 UP 阈值选择参与提升的部分。')
    para('UP 阈值独立于 Output Gain 和 Makeup；保留 Output LINK 也不会把 UP 阈值一起拉动。无论 Super 或 Classic，UP 与 DOWN 阈值都为 -45 至 0 dB，UP 不高于 DOWN。',9.2,8)
    note('特别留意底鼓与军鼓的延音','补上压后，底鼓和军鼓的延音可能被抬高，听起来更突出。不要只听起音与响度，也要听鼓点之间是否被延音填满、节奏是否变得拖沓。')
    heading('双压效果不好时，优先尝试分段压缩')
    para('双压在已调好的下压基础上，用少量上压补充细节，DOWN RATIO 保留原来的下压力度，需要时再微调。若加入上压后整体效果不理想，先减小或关闭上压，再改用分段压缩控制动态，往往更容易得到合适的结果。Mix 是否使用，留到这些处理确定以后再判断。',9.6)


    start('Ceiling 与 TP：选择最终上限')
    picture('manual-limiter-light.png','Output 下方的 Ceiling、恢复方式与 TP。TP 开启时显示 dBTP。',maxh=174,width=160,crop=(1700,1230,336,356))
    table(['设置','含义'],[
      ['Ceiling','-24 至 0 dB，默认 0；设置最终峰值保护的参考上限。'],
      ['TP 开启','通过重建峰值检测与增益恢复控制真峰值。总 OS 的四个倍率均可使用。'],
      ['TP 关闭','使用 Hard Clip；1x 在原采样率削波，4x / 8x / 16x 在对应音频倍率中削波。'],
      ['总 OS','压缩与 Ceiling 共用同一音频倍率；不再单独设置 Ceiling OS。']], [.25,.75],9.5)
    para('TP 开启时保留少量重建余量，读数不必恰好贴线。Hard Clip 的恢复与失真特征不同；关闭 TP 后，采样间真峰值仍可能超过 dBFS 上限，因此要看实际 TP 读数。',9.6)
    heading('数值操作')
    para('双击 Ceiling 数值输入；上下拖动调整，Shift 精调，Alt 单击恢复 0。支持撤销与重做。恢复方式只影响 TP，详见第 41 页。',9.5)
    note('1x 也能使用 TP','总 OS 选 1x 时，音频保持原采样率，TP 仍使用独立的重建峰值检测；不会暗中把整条音频切到 8x。最终峰值保护与延迟仍按当前路径工作，见第 40 页。')


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
      ['普通 MS 的单域表与 TP 不同','TP 看最终 L/R；单独的 M/S 电平与左右输出上限不是同一个量。']], [.38,.62],9.2)


    start('耳机 1:1：保留限幅，抵消输出增益','Output Gain 左侧的小耳机，只在 Limiter 模式显示。')
    picture('manual-limiter-monitor-controls.png','耳机点亮后，Output 的数值和联动保留；下方出现 MON 监听上限。',maxh=103)
    para('打开耳机，声音仍完整经过动态处理和 Ceiling，然后再抵消可见 Output Gain 的增益。限幅造成的动态变化与波形仍然保留，只把结果换到便于比较的监听音量。')
    heading('Output 数值仍然有意义')
    para('Output 可以继续调整；下压阈值、Makeup 与 Output 的联动仍然有效，Dual 的 UP 阈值保持独立。耳机只是抵消 Output 的直接音量作用，参数改变带来的动态变化仍能听见。关闭耳机，恢复正常 Output Gain。')
    heading('Ceiling 也换到同一监听标准')
    note('MON = Ceiling - Output Gain','例如 Ceiling = -1 dB、Output = +6 dB，耳机开启时对应的监听上限为 -7 dB。Ceiling 原值仍为 -1 dB；关闭耳机后恢复正常输出标准。TP 表跟随实际输出测量。')
    para('耳机开关默认关闭，随工程保存，独立于 A/B。切换 A/B 时可以一直保持同一监听方式；耳机与 Bypass 使用平滑过渡。它不单独增加一段音频延迟。',9.6)
    note('1:1 不会自动等响','耳机只抵消 Output Gain，并不自动弥补压缩、Ceiling 或 Input 的全部响度变化。等响比较仍需点击 Match；下一页给出完整操作。')


    start('Limiter 等响比较：耳机 + Match + Bypass')
    step(1,'在已经调好的 Limiter 上比较','先完成第 28 页的下压，或第 29 页的双压补偿，再打开耳机。保持当前 Ceiling 与 TP，关闭 SC LISTEN；Limiter 始终输出完整立体声。')
    step(2,'播放同一段有代表性的素材','先开启耳机，再开始播放，等待有效测量建立。播放中可以点击 Match；需要只统计新的设置时，停止后重新播放。')
    step(3,'点击 Match，补偿到 Makeup','耳机模式比较原始输入与真正经过 Ceiling、耳机补偿后的输出。按实际左右处理结果计算共同补偿量，写入唯一一组 Makeup；干湿混合也会计入。')
    step(4,'用插件 Bypass 比较原声','Bypass 返回延迟对齐的原始输入，不经过限幅或耳机音量补偿。听起音、尾音、密度与律动，必要时微调 Makeup。')
    step(5,'结束试听，恢复正常输出','关闭耳机，保留调好的参数。重新检查 TP 和 Ceiling，再判断送往后级的最终输出。')
    heading('Match 做一次补偿，不持续追着音量跑')
    para('再次点击仍从本次播放开始累计，不清空、不重复叠加补偿。若只想比较调整后的设置，可以停止后重新播放并 Match。Limiter LINK 关闭、参数已达到最小值或最大值或 Mix 为完全干声时，Makeup 能做到的补偿可能受限。',9.5)
    note('比较 A/B 时','分别给 A、B 播放相同的素材，再 Match。耳机状态独立于 A/B，Limiter / TP / Ceiling、倍率与恢复方式随 A/B 恢复；尽量保持相同 Lookahead 与倍率，减少延迟切换的影响。')


    start('Limiter：用 Window 比较，再选择 0 ms','Window 改变检测范围，Lookahead 决定基础延迟。')
    heading('先固定 Lookahead，连续比较不同 Window')
    para('调好下压和 L/R Link 后，先保持 Lookahead 26 ms，让 Window 从 26 ms 逐渐减小。插件不会因此改变报告给宿主的延迟，可避免因延迟补偿重调造成的卡顿，直接比较响度、音头、尾音与段落动态。')
    para('SAFE 关闭时，较短 Window 会增加局部动态与染色，也更容易产生粗糙感或噼啪；SAFE 开启时，缩短 Window 可减少强音前后的衰减，但保护也会减弱。大多数普通压缩不必频繁调整，Limiter 则值得仔细比较。',9.6)
    note('如果 Window = 0 ms 最合适','可以把 Lookahead 也切到 0 ms。在其他参数与 OS 相同、延迟对齐并排除切换过渡后，两者本质上都是 Window = 0 ms 的逐采样检测；原设置保留主 Lookahead 延迟，0 ms 则移除这部分延迟。不要把未对齐的两份音频直接相减。')
    heading('0 ms 的响度与动态取舍')
    para('Limiter 的 0 ms 设置可用于追求更大响度，同时保留段落之间的对比。结果取决于素材、算法和力度：同时试听副歌与安静段落，避免只以响度判断。Window 为 0 时 SAFE 与 Normal 的检测结果相同。',9.6)
    heading('配合过采样与 TP 恢复')
    para('Window 较低，尤其低于 10 ms，以及使用 Limiter Ceiling 时，建议比较 4x / 8x / 16x。过采样减少混叠，不能消除全部谐波。TP 开启时再比较 TIGHT、AUTO、SMOOTH 的峰后恢复，见第 41 页。',9.6)
    note('Lookahead 数字不等于插件总延迟','Ceiling 另有两级约 3 ms 的预读及重建检测对齐，OS 滤波也可能增加延迟。普通模式不承担 Ceiling 的额外延迟；切换 Limiter 即使 Lookahead 相同，宿主也可能重新调整补偿。Window 调节不会增加这类延迟切换。')

    start('参数速查：算法、动态与范围')
    table(['参数','范围 / 选项','说明'],[
      ['算法','Classic / Super','Single 一组；Dual 的 UP / DOWN 可分别选择。'],
      ['SINGLE / DUAL','单压 / 双压','Limiter 切换时保留下压阈值衔接，见第 39 页。'],
      ['Single Ratio','普通 1:200 至 200:1；Limiter 1:8 至 1000:1','1:1 为默认 / 重置值；Limiter 常规使用 20:1 至 1000:1。'],
      ['Up Ratio','普通 1:200 至 1:1；Limiter 1:8 至 1:1','默认 1:1；Limiter 补上压通常先在 1:1 至 1:1.2 间尝试。'],
      ['Down Ratio','普通 1:1 至 200:1；Limiter 1:1 至 1000:1','默认 1:1；切入 Limiter 双压后，核对已调好的下压比例。'],
      ['UP / DOWN 开关','ON / OFF','默认均 ON；独立试听并保留 Ratio。'],
      ['Threshold / UP THR','Limiter -45 至 0 dB','普通：Classic -90 至 0；Super -inf 至 0 dB。'],
      ['Range','电平数值或 OFF','普通 Single 中，达到或超过它就不压缩；OFF 关闭这项作用。Limiter 不使用。'],
      ['DOWN THR','Limiter -45 至 0 dB','普通按算法定下限；DOWN 不低于 UP。'],
      ['Ratio LINK','普通 DUAL：OFF / ON','反向联动 UP / DOWN；首次 ON。Limiter 无此联动。'],
      ['Mode / 声道 LINK','普通：ST / MS / LR；OFF / ON','Limiter 改为 L/R Link 0-100%，见第 27 页。']], [.23,.36,.41],9.1)
    note('推子的顺序与默认值','Range 不低于 Threshold；DOWN 不低于 UP。相撞时一起移动，重合时停止动态处理。新建 Limiter 的 Single Threshold / Dual DOWN 为 0 dB，Ratio 为 1:1；UP 默认 -45 dB；旧工程中更低的 Limiter 阈值收至 -45 dB。')
    para('普通压缩中，Super 的阈值数值最低为 -119.99 dB，再往下为 -inf（Single 的下限可能显示 OFF）。普通 Display 最低 -90 dB；Limiter 最低 -45 dB。Range = 0 dB 时，达到 0 dB 就不压缩；OFF 则关闭 Range 的这项作用。',9.3)


    start('参数速查：音量、监听与侧链')
    table(['参数','范围 / 选项','说明'],[
      ['Input Gain','普通 / Limiter 均 ±24 dB','分别保存各自的输入设置。'],
      ['Makeup','普通 ±30 dB；Limiter ±120 dB','默认 0；位于 Wet 与 Mix 之间。'],
      ['Mix','0 至 100%','默认 100%；控制动态处理混合量。'],
      ['Output Gain','普通 ±24 dB；Limiter ±120 dB','耳机抵消 Limiter Output 的直接监听增益。'],
      ['Input / Output LINK','OFF / ON','普通模式的等量反向增益联动。'],
      ['MONITOR','ALL / L / R 或 M / S','仅普通 LR / MS 可用；Limiter 无声道监听。'],
      ['Lookahead','0 / 26 / 40 / 80 / 100 ms','初始 26 ms；手动切换后恢复完整 Window。'],
      ['Window','0 至当前 Lookahead ms','默认 / Alt 为最大值；固定延迟调整检测长度。'],
      ['SAFE','默认 OFF','选择检测策略；0 ms Lookahead 时禁用。'],
      ['总 OS','1x / 4x / 8x / 16x','所有 Lookahead 生效；初始 8x。'],
      ['SOURCE / KEY GAIN','INT / EXT；±24 dB','KEY GAIN 只在 EXT 可调。'],
      ['HPF / SC LISTEN','OFF / 20-500 Hz；OFF / ON','只滤检测信号；试听在关闭面板或编辑器时结束。'],
      ['Limiter / TP','默认 OFF / ON','TP 关闭时使用 Hard Clip。'],
      ['Ceiling','-24 至 0 dB','默认 0 dB；使用总 OS，TP 在 1x 也可开启。'],
      ['TP 恢复','TIGHT / AUTO / SMOOTH','默认 AUTO；TP 关闭时不作用。'],
      ['耳机 1:1','默认 OFF','MON = Ceiling - Output；工程保存，独立于 A/B。'],
      ['FULL / ECO','新实例沿用上次手动选择','每个实例独立；工程保存，独立于 A/B。']], [.23,.35,.42],8.6)
    para('普通模式与 Limiter 的音量、检测与时间设置分别保留，见第 39 页。ECO 停播暂停、关窗分析暂停与 FULL 极低残留休眠见第 43-44 页。',9.3)


    start('上压与双压：没有变化时检查什么')
    table(['现象','先检查什么'],[
      ['调整后没有听到提升','确认对应 Ratio 低于 1:1，Mix 大于 0，Bypass 关闭；Dual 中，对应的上压或下压开关需为 ON。'],
      ['Dual 的 Up 没有效果','检测电平要高于 UP、低于 DOWN。UP 以下原样输出，高于 DOWN 时只做下压。'],
      ['推子调好却完全不工作','Threshold 与 Range（Dual 中为 UP 与 DOWN）是否重合？重合会使整个动态处理停止；分开后再比较。'],
      ['较响的部分突然不再处理','普通 SINGLE 是否启用了 Range？达到或超过它就不压缩；希望较响的部分也继续处理时，将 Range 设为 OFF。'],
      ['调 Up，Down 也在变化','普通压缩的 Ratio LINK 首次默认开启；需独立调整时关闭。Limiter 的两个 Ratio 始终独立。'],
      ['LINK 不能继续往同一方向调','另一侧可能已达到最小值或最大值；联动会共同停止。反向调整即可继续。'],
      ['看着主轨道电平却不按预期触发','检查 SOURCE 是否为 EXT，或 HPF 是否改变了检测信号。阈值判断依据是检测信号。']], [.33,.67],9.2)
    heading('单独确认双压的向上作用')
    step(1,'把其他变化先固定','选择 DUAL，关闭 DOWN 旁的开关；普通压缩还需关闭 Ratio LINK。保持 Mix 100%，各音量增益为 0 dB，选择合适的 Lookahead。')
    step(2,'确保素材在两个阈值之间','例如 UP 为 -50 dB、DOWN 为 -20 dB，确认一段轻声的检测电平主要落在两者之间。此时 Ratio 仍为 1:1，不会出现提升。')
    step(3,'从 1:1 逐渐增加上压力度','将 Up Ratio 依次调向 1:2、1:4、1:8，同时听音，观察 Boost / Mix 与正向动态增益。确认后再开启 DOWN，加入下压，重新比较整体响度。')


    start('遇到问题，先检查这里')
    heading('Classic 压缩后，为什么有时低于阈值？')
    para('阈值约束的是检测电平上的增益曲线，不是最终输出的最低线。一个 Window 内比检测峰值更轻的声音，也会受到这段增益影响；ST 共用增益、外部侧链或 HPF 也会使检测电平与眼前的声音不同。Makeup 与 Output Gain 还会继续改变输出。',9.5)
    para('Range OFF、内部检测、各音量为 0 dB、Mix 100% 时，稳定且高于阈值的电平按 Classic 公式压缩后仍不低于阈值。若只在 Display 看见下探，请先记住它是根据当前设置计算的参考曲线，再用实际音频核对；不必把每次下探都归因于提前压缩。',9.5)
    table(['现象','先检查什么'],[
      ['EXT 没有效果','确认宿主侧链输入和发送已启用，SOURCE 为 EXT，KEY LEVEL 有读数。N/A 表示未提供外部通道。'],
      ['Key Gain 不能调整','当前为 INT 时正常；它只调整外部 Key。'],
      ['HPF 不切输出低频','它只过滤检测信号。历史在松手后刷新，等 HPF UPDATING 消失再看。'],
      ['声音变成侧链 / 变窄','关闭 SC LISTEN；普通 LR / MS 的 MONITOR 回到 ALL；Limiter 无此声道监听。'],
      ['Match 灰显','播放足够的非静音素材，建立有效读数；ECO 中若有缺失分析，打开界面后重新播放。'],
      ['切换 Lookahead 有停顿','插件延迟改变，宿主可能重新调整补偿；录音监听也要考虑延迟。'],
      ['图表底部以下看不到曲线','普通图表最低 -90 dB；Limiter 最低 -45 dB，只限制显示，不截断音频。']], [.30,.70],8.9)
    para('Limiter、TP、Ceiling 与耳机操作见第 27-34 页；其他进阶设置见第 39-44 页。安装、更新和扫描请参阅对应平台的安装说明。',9)
    para('使用许可：Qing Audio 非商业源码共享许可证 1.0。禁止商业使用；完整条款与对应源码见项目仓库。',8.7)
    para('<link href="https://github.com/Ziqing-Gu/QQ-Super-Compression" color="'+TEAL+'">github.com/Ziqing-Gu/QQ-Super-Compression</link>',9)

    start('普通压缩与 Limiter：两套独立设置','切换工作模式时，保留各自已调好的声音。')
    para('普通压缩和 Limiter 各自保留 Input、Makeup、Mix、Output、算法、Single / Dual、Ratio、阈值、侧链、Lookahead、Window、SAFE 与总过采样。普通压缩另存 ST / LR / MS；Limiter 另存数值 L/R Link。首次进入 Limiter 从它自己的默认设置开始，不复制普通模式。')
    table(['操作','结果'],[
      ['普通模式调 Input +3 dB，再进入 Limiter','Limiter 恢复自己的 Input；普通模式的 +3 dB 被保留。'],
      ['在 Limiter 改为 Dual、设置 Ceiling','退出后恢复普通模式；再进入时回到已设好的 Limiter。'],
      ['切换 A/B','恢复该声音快照中的两套模式参数及当时启用的模式。']], [.47,.53],9.7)
    heading('下压调好后，用 Dual 补上压')
    para('按第 28 页调好常规下压，再切到 Dual；保持下压算法与力度，UP 从 1:1 到 1:1.2 之间少量补充。留意底鼓、军鼓延音；双压不合适时，优先用分段压缩控制动态，详见第 29 页。',9.6)
    heading('Limiter 内部的 Single / Dual 切换')
    para('在 Limiter 内切换到 Dual 时，Single 的 Threshold 延续到 DOWN 阈值；切回 Single 后，Threshold 接续当前 DOWN 阈值。')
    para('切入 Dual 时保留 UP；如果两个阈值顺序冲突，会将它们调整到相同位置，避免 UP 高于 DOWN。切回 Single 时仍不使用 Range。两种模式中的 Ratio 各自保留，Input、Makeup、Mix 和 Output 维持当前 Limiter 设置。',9.7)
    note('例：保持手动阈值关系','Single Threshold = -18 dB 时切换 Dual，DOWN 接续 -18 dB。即使关闭 Output LINK，或 Output 与阈值并非互为相反数，也不会用 Output 重新推算这根阈值。')
    note('Output LINK 的补充规则','Makeup 增加 1 dB，Output 反向减少 1 dB；Ratio、Mix、算法本身不直接推动 Output。下压活动时，Output 与 Single 下压阈值或 Dual DOWN 阈值反向联动。Dual UP 阈值不参与 Output / Makeup 联动；所有 Limiter 阈值均为 -45 至 0 dB，Super 与 Classic 相同。阈值顺序约束与范围限制仍有效。')
    heading('哪些属于工作方式')
    para('FULL / ECO、耳机 1:1 与普通 LR / MS 的声道独奏属于当前实例的工作设置，独立于 A/B。FULL / ECO 不会因为另一个插件实例改变而跟随切换。',9.8)
    para('保存工程后再打开，应以工程中保存的实例状态为准。旧版本工程会经过兼容恢复；继续工作前，核对模式、数值与宿主延迟。',9.6)

    start('什么时候开过采样？','一个 OS 控制，同时覆盖动态处理与 Limiter Ceiling。')
    heading('以下三种情况，建议开启过采样')
    table(['情况','为什么建议开启'],[
      ['1  Lookahead 为 0 ms','Window = 0 ms 下的快速增益变化更容易产生谐波和混叠；先比较 4x / 8x，再按需要提高。'],
      ['2  Window 较低，尤其低于 10 ms','无论 SAFE 是否开启，缩短 Window 都可能增加谐波与调制染色。优先使用过采样，再按音色和负载选择倍率。'],
      ['3  使用 Limiter','Ceiling 发生削峰或限幅时会产生谐波；过采样可减轻其中的混叠。TP 开启或关闭都可使用总 OS。']], [.34,.66],9.8)
    picture('manual-limiter-os.png','OS 位于右下区域第四排；普通压缩与 Limiter 各自保存倍率。',maxh=103,width=114)
    table(['倍率','如何选择'],[
      ['1x','不开音频过采样，运算较少；可作为听音对比基准。'],
      ['4x','较低负荷的起点，先比较粗糙感、高频与持续尾音。'],
      ['8x / 16x','进一步提高处理采样率；初始为 8x，按听感与 CPU 余量选择。']], [.25,.75],9.4)
    para('这是使用建议，不是到 10 ms 就突然“无失真”的边界。过采样主要减少混叠，不会抹掉所有谐波，也不代替 SAFE / Window 对噼啪声与提前压的控制。',9.5)
    note('Window 不改变已选的基础延迟','Lookahead 为 26 ms 时，Window 从 26 调至 0 ms，基础延迟仍为 26 ms。过采样滤波和 Limiter 保护的额外延迟由宿主计入；切换后让宿主完成补偿再比较。')
    para('旧工程中原来仅作用于 0 ms 的 OS 现在覆盖所有 Lookahead；原 Ceiling OS 不再单独控制。1x 仍可使用独立 TP 重建检测，但整条音频不因此切换到高倍率。',9.1)


    start('TP 恢复：TIGHT / AUTO / SMOOTH','点击 Ceiling 与 TP 之间的小按钮，选择峰值过后的恢复取向。')
    picture('manual-recovery-auto.png','当前显示 AUTO。三种方式都属于 TP 最终保护。',maxh=90,width=270)
    table(['方式','听感取向与比较方法'],[
      ['TIGHT','较快释放峰值后的衰减，短期响度较容易恢复。留意低频与连续调幅素材是否出现更明显的波动。'],
      ['AUTO','默认方式。结合信号周期及反复变化调整恢复，在孤立峰值与连续变化之间取得平衡。'],
      ['SMOOTH','更平缓、较慢地恢复，适合比较低频稳定感与较深限幅后的起伏；可能保留更长的衰减。']], [.20,.80],10)
    step(1,'先保持同一段素材与同一上限','固定 Ceiling、Ratio、Output、Mix 与总 OS，只切换恢复方式。')
    step(2,'分别听短峰值和持续低频','短鼓点、持续贝斯与反复起伏的片段，可能体现不同取舍。不要仅凭某一个峰值判断。')
    step(3,'重新做等响度比较','恢复方式会影响平均响度。必要时重新播放并 Match，再比较尾音、低频与起音。')
    note('同一保护档位，不同恢复形状','这三个选择不改变当前倍率的宿主延迟。它们只在 TP 开启时起作用；关闭 TP 使用 Hard Clip，不套用这三种恢复包络。恢复方式随工程与 A/B 保存。')
    para('FULL 的极低残留休眠须等尾音和 TP 恢复结束，SMOOTH 可能延长这段等待。ECO 在宿主明确停播时立即暂停并切断尾音；见第 44 页。',9.6)

    start('输出 LUFS-I：衡量一段声音的响度','Limiter 的 Display 内显示 OUTPUT / LUFS-I。')
    picture('manual-limiter-lufs.png','数字为本次累计输出响度；右下角为已测量时长。示例读数不是目标值。',maxh=184,width=390)
    para('LUFS-I 衡量一段声音的综合响度，TP 衡量峰值；二者解决不同问题。LUFS-I 读取实际监听路径，包含 Ceiling、耳机补偿和当前试听方式的影响。')
    table(['状态','怎样理解'],[
      ['READY','尚未累计测量；先播放需要评估的片段。'],
      ['MEASURING','正在根据播放信号累计输出响度。'],
      ['HOLD','停止后保留上一段测量结果。'],
      ['--.-','尚无有效响度读数；过短或过低的信号可能不足以形成结果。']], [.26,.74],9.8)
    para('开始新的播放、跳转或循环时会重新累计，停止后保留结果。若宿主没有提供播放状态，则在 Limiter 开启时连续测量。',9.5)
    heading('每次比较保持同一条件')
    para('播放同一段素材，保持 SC LISTEN 与耳机状态一致。Limiter 输出完整立体声，不提供声道 MONITOR；判断送往后级的正常输出时，关闭耳机与 SC LISTEN。')
    note('它不会自动把声音调到某个数值','LUFS-I 是读数；Match 是一次响度补偿操作，写入 Makeup。两者各有测量用途，不能把 LUFS-I 读数当作 Match 的完成提示。ECO 隐藏界面时暂停分析，重新打开后 LUFS-I 重新累计；Match 须有本次播放的完整分析，缺失时需重新播放，见第 13 页。')
    para('响度读数包含测量门限，因此极低电平不一定产生有效 LUFS-I。判断前后变化时，也要听起音、密度、尾音与完整混音中的位置。',9.6)

    start('FULL / ECO：每个实例各自选择','点击顶部 FULL / ECO，只改变当前这一个插件实例。')
    picture('manual-performance-eco.png','ECO 按钮。可以让需要持续分析的实例保持 FULL。',maxh=64,width=156)
    para('下表比较播放、录音与离线渲染时的界面状态；宿主停播时的行为见下方及第 44 页。',9.4)
    table(['状态','界面可见时','界面关闭 / 隐藏时'],[
      ['FULL','正常处理与分析','继续后台分析与实时监听。'],
      ['ECO','正常处理与分析','暂停界面分析；音频仍按当前参数处理。']], [.19,.34,.47],9.8)
    heading('十个实例，可以一个 FULL、九个 ECO')
    para('例如需要停播时监听硬件输入的实例选 FULL，其余九个选 ECO。各实例独立、随工程保存，A/B 切换不改变模式。新实例沿用上次手动选择；已有工程仍以保存的模式为准。')
    heading('重新打开 ECO 界面后')
    para('重新打开后恢复显示与 LUFS-I 测量。若 ECO 播放中曾关闭界面，Match 不会把缺失的一段当成完整记录；请保持界面打开，停止后重新播放。已应用的 Makeup、Output 等设置继续保留。')
    note('ECO 在处理期间不降低声音质量或延迟','播放、录音及离线渲染期间，ECO 仍按当前参数完整处理；关窗只暂停界面分析，不降低所选总 OS，也不撤销补偿。宿主明确停播时则静音并暂停处理，与输入是否有底噪无关。')
    heading('停播时按模式决定')
    para('ECO 在宿主明确报告停止时立即静音并跳过压缩、过采样与 Ceiling，界面开关不影响。FULL 保留实时输入监听；全零或极低残留输入且内部尾音稳定后才可能休眠。',9.8)

    start('停播时暂停处理与恢复','ECO 依据 DAW 的停播状态；FULL 保留实时监听。')
    heading('ECO：停播即停止处理')
    para('宿主明确报告停止、且没有录音或离线渲染时，ECO 在该音频块把主输出置零，暂停动态核心、总过采样与 Ceiling。硬件底噪或活动侧链不会阻止暂停；未完的延迟与 TP 尾音也会被切断。界面开关不改变这一行为。')
    para('需要停播时听实时输入，请为该实例选 FULL。宿主不提供可靠播放状态时，ECO 继续正常处理。',9.7)
    heading('哪些情况恢复处理')
    table(['宿主状态 / 操作','处理方式'],[
      ['开始播放或录音','首个回调块恢复完整处理。'],
      ['离线渲染','保持完整处理，即使宿主未同时标记播放。'],
      ['停播时切到 FULL','当前块恢复实时处理与监听。'],
      ['播放中的空白片段','仍完整处理；不会因全零样本触发 ECO 停播。']], [.43,.57],9.6)
    note('恢复不另加等待，也不叠加启停渐变','恢复前会清除停播前的音频、滤波和 Ceiling 历史，按当前参数从首块开始处理；原有 Lookahead、过采样延迟及宿主 PDC 仍存在。插件主输出没有额外的启停 Crossfade；监听平滑请交给 DAW。')
    heading('FULL 的极低残留休眠与 ASIO-Guard')
    para('FULL 在停播时仍处理实时输入与通常的底噪。只有宿主明确停播，且既未录音也未离线渲染，主输入及已选侧链的原始峰值不超过 −180 dBFS，考虑当前增益后至少一秒的实际输出低于 −160 dBFS，并且滤波与延迟尾音及 TP 恢复稳定，才会暂停主要音频运算。−200 dBFS 这类极低残留可在休眠期间归零；真实输入或已选侧链活动会在当前音频块恢复处理。极端增益可能让 FULL 保持工作。',9.5)
    para('ASIO-Guard 是宿主整体调度读数，不能由单插件 CPU 耗时直接换算。检查 ECO 停播效果时先确认宿主已报告停止；实际导出对齐仍由宿主渲染与延迟补偿决定。',9.5)

    finish();c.save()
    doc=PdfReader(a.output/name);assert len(doc.pages)==44
    txt='\n'.join(p.extract_text() for p in doc.pages)
    for banned in ('Revision','Rev 2','Plan A','Plan B','Plan C','JUCE','offscreen','离屏','2.2x','归一化','缓存','v1.0.1','附页','addendum','schema'):
        assert banned not in txt,(lang,banned)
    import json
    (Path(__file__).parent/'Chinese-layout-audit.json').write_text(json.dumps({'version':'1.3.4','language':'zh','pages':len(doc.pages),'minimum_content_baselines_pt':mins,'status':'awaiting_visual_review'},indent=2),encoding='utf-8')
    print(lang, len(doc.pages),'pages; lowest content baselines:',mins)
