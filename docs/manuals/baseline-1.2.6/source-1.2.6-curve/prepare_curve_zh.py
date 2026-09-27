"""Create a Chinese documentation revision from the published 1.2.6 source."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'source-1.2.6'
s=(BASE/'build_manual_1_2_6_zh.py').read_text(encoding='utf-8').replace('import argparse','import argparse\nimport math')
plot='''    def transfer_plot():
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

'''
s=s.replace("    start('QQ Super Compression','用户手册  /  1.2.6')",plot+"    start('QQ Super Compression','用户手册  /  1.2.6')",1)
start=s.index("    start('Classic 与 Super：两种动态曲线'")
end=s.index("    start('Ratio、Input、Mix：三种调法'",start)
section='''    start('Classic 与 Super：两种动态曲线','点击上方 ALGO 按钮切换；SINGLE 与 DUAL 均可使用。')
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


'''
s=s[:start]+section+s[end:]
(HERE/'build_manual_1_2_6_zh.py').write_text(s,encoding='utf-8')
files=[BASE/'build_manual_1_2_6_zh.py',BASE/'build_manual_1_2_6_en.py',HERE.parent/'QQ-Super-Compression-1.2.6-User-Manual-Chinese.pdf',HERE.parent/'QQ-Super-Compression-1.2.6-User-Manual-English.pdf']
(HERE/'baseline.json').write_text(json.dumps({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},ensure_ascii=False,indent=2),encoding='utf-8')
print('Revised Chinese page 5; published source and PDFs retained.')
