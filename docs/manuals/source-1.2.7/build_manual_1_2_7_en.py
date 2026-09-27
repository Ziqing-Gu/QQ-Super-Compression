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
arg.add_argument('--language',choices=('en',),default='en',
                 help='English edition matching the approved Chinese manual.')
a=arg.parse_args();a.output.mkdir(parents=True,exist_ok=True)
for name,file in [('Body','segoeui.ttf'),('Bold','seguisb.ttf'),('CN','msyh.ttc'),('CNBold','msyhbd.ttc')]:
    pdfmetrics.registerFont(TTFont(name,str(a.fonts/file),subfontIndex=0))
W,H=595.276,841.89;M=48;CW=W-2*M
TEAL='#009db5';INK='#293940';MUTED='#70848e';PALE='#eaf6f8'

for lang in ('en',):
    zh=lang=='zh';font='CN' if zh else 'Body';bold='CNBold' if zh else 'Bold'
    pdfmetrics.registerFontFamily(font,normal=font,bold=bold,italic=font,boldItalic=bold)
    name='QQ Super Compression User Manual English_v1.2.7.pdf'
    c=canvas.Canvas(str(a.output/name),pagesize=(W,H),pageCompression=1)
    c.setTitle('QQ Super Compression 1.2.7 User Manual')
    c.setAuthor('Qing Audio');c.setSubject('Dynamics, Limiter, level-matched listening and sidechain guide')
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
        c.setFont(font,8);c.drawString(M,top-height-50,'Detector level: low to high (active intervals)')
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
        c.drawRightString(W-M,H-34,'QQ Super Compression  ·  User Manual')
        y=H-59;o=p_obj(title,21,TEAL,True);_,h=o.wrap(CW,150);o.drawOn(c,M,y-h);y-=h+11
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
        c.setFillColor(HexColor(INK));c.setFont(font,8);c.drawString(M,top+11,'Output dB');c.drawRightString(W-M,py(-50)-29,'Detector level dB')
        c.setFont('Bold',8);c.setFillColor(HexColor(TEAL));c.drawString(x+8,py(-7),'CLASSIC');c.setFillColor(HexColor('#ED7C31'));c.drawString(x+86,py(-7),'SUPER')
        y=top-h-40
        o=p_obj('Static curves: Threshold=-24 dB, Ratio=4:1, Range=OFF; no compensation, boundary transitions or detector timing.',8.3,MUTED)
        _,ht=o.wrap(CW,800);o.drawOn(c,M,y-ht);y-=ht+9

    start('QQ Super Compression','User Manual  /  1.2.7')
    heading('Control dynamics. Keep the attack natural.')
    para('For vocals, instruments, buses and mastering. There are no conventional Attack / Release controls: shape dynamics with Lookahead, Ratio, Threshold and Mix.')
    para('Classic uses a fixed dB ratio; Super retains the original curve. Both support Single, upward and Dual processing. Limiter adds final peak protection, TP / Peak selection and headphone 1:1 listening for level-matched comparisons.',9.6)
    picture('manual-light-ST.png',maxh=402)
    note('Start with what you hear','Decide how steady the dynamics should be and how much articulation to retain. The Display helps explain changes; it does not replace listening.')


    start('Quick start and navigation','A first session in six steps.')
    for i,(title,text) in enumerate([
      ('Insert on the track to process','Start with the CLASSIC algorithm, SINGLE, ST and SC: INT so the plugin follows the track itself.'),
      ('Choose Lookahead','26 ms is a practical starting point, not a rule for every source.'),
      ('Start at 1:1','Normal compression starts at 1:1. To try downward processing, raise Ratio and leave Range OFF. See pages 8-11 for upward and Dual processing, or pages 25-31 for Limiter.'),
      ('Use Mix to set the processing amount','If processing is too strong, lower Mix to blend back the original dynamics. Both boost and cut displays follow Mix.'),
      ('Compensate level and compare','Play a representative passage, use Match, then trim Makeup / Output Gain to avoid loudness bias.'),
      ('Listen in the full mix','Use Bypass or A/B to judge articulation, consistency and groove - not only the GR number.')],1):step(i,title,text)
    table(['Find a topic','Pages'],[
      ['Interface, themes and gestures','3-4'],['Classic / Super, Single and upward processing','5-8'],
      ['Dual, branch switches and Ratio LINK','9-11'],['Input / Output LINK, Match and A/B','12-14'],
      ['Display and Lookahead','15-17'],['ST / LR / MS and channel LINK','18-20'],
      ['Sidechain panel, internal HPF and external routing','21-23'],['Mixing examples','24'],['Limiter, TP / Ceiling and headphone listening','25-31'],['Parameter reference and troubleshooting','32-35']], [.78,.22],9)


    start('Know the interface','The same audio controls remain in place across all themes.')
    picture('manual-light-ST.png','Light / ST. The values illustrate the interface; they are not a recommended preset.',maxh=361)
    table(['Area','Use it to'],[
      ['Display','View historical level and gain references under current settings; see page 15.'],
      ['Two boundary faders','Threshold / Range in Single; UP / DOWN thresholds in Dual.'],
      ['Input / Output / Gain +/-','Check input, output and effective dynamic gain.'],
      ['Bottom knobs and SINGLE / DUAL','Adjust gain, Ratio and Mix; switch Single / Dual beside Ratio.'],
      ['Mode / LINK / Monitor','Choose channel processing, link controls or audition domains.'],
      ['Top buttons','Enable Limiter; select Classic / Super, sidechain, theme, A/B and Bypass.']], [.32,.68],9)
    para('The interface uses a 3:2 landscape aspect ratio. Drag the bottom-right corner to resize it. The plugin remembers the last size when reopened.',9.5)


    start('Themes and everyday controls')
    para('Click the top-right button labelled LIGHT, DARK or CLASSIC to cycle themes. The last selection is restored when the plug-in opens again.')
    picture('manual-theme-button.png','The theme button is between SC and A/B.',maxh=72,width=280,crop=(12,0,168,92))
    table(['Theme','Appearance'],[['Light','Light interface.'],['Dark','Subdued dark interface.'],['Classic','Original classic dark interface.']], [.25,.75])
    para('Themes change appearance only. The CLASSIC theme is separate from ALGO: CLASSIC: one changes the visual style, the other changes the processing algorithm.')
    # Two visual choices, not implementation descriptions.
    yy=y;gap=14;cell=(CW-gap)/2
    for n,label,x in [('manual-dark.png','Dark',M),('manual-classic.png','Classic',M+cell+gap)]:
        im=Image.open(a.captures/n);ww,hh=im.size;im.close();ht=cell*hh/ww
        c.drawImage(str(a.captures/n),x,yy-ht,cell,ht,mask='auto');c.setFont('Body',9);c.setFillColor(HexColor(MUTED));c.drawString(x,yy-ht-14,label)
    y=yy-ht-30
    table(['Adjustment','Result'],[
      ['Drag / Shift-drag','Normal / fine adjustment.'],
      ['Double-click a value','Type an exact value.'],
      ['Alt + left click','Restore the default. When linked, related controls follow within their allowed ranges.'],
      ['Ctrl/Cmd + Z','Undo; add Shift to redo.']], [.39,.61],9)


    start('Classic and Super: two dynamic curves','Switch with ALGO at the top. Both algorithms work in SINGLE and DUAL.')
    table(['Algorithm','How it works'],[
      ['Classic','A fixed dB ratio above the threshold. For downward compression, divide the excess dB by Ratio to find how much excess remains.'],
      ['Super','The original QQ curve. Ratio sets its strength, but does not describe a fixed dB compression ratio at every detector level.']], [.18,.82],9.5)
    transfer_plot()
    heading('A worked example')
    para('Classic downward: Threshold=-10 dB, Ratio=5:1, Range=OFF. A steady detector level of 0 dB exceeds the threshold by 10 dB. Compression leaves 2 dB of that excess, giving <b>-8 dB</b>. At -12 dB, the detector stays below threshold and receives no dynamic attenuation.',9.6,8)
    para('This example uses internal detection, 0 dB Input, Makeup and Output Gain, and Mix=100%. The formula describes a steady detector level, not a hard limit on individual samples. For changing signals and the Display, see pages 15-16.',8.8,8)
    heading('Upward processing still boosts above the lower gate')
    para('In both algorithms, the lower threshold enables boost above it. Classic uses the dB slope corresponding to the reciprocal Down Ratio; Super uses the original upward curve. Both share the same active intervals, Range behavior and Dual handoff rules.',9.6,8)
    note('Minimum threshold and remembered choice','Classic stops at -90 dB and keeps the same curve at its minimum; Super retains -inf. New instances remember the last manual selection. Projects and A/B restore their saved algorithms. A short crossfade reduces switching transients.')


    start('Ratio, Input and Mix','Three useful controls, not interchangeable settings.')
    picture('manual-light-controls.png',maxh=92)
    heading('Ratio')
    para('Normal Single processing is neutral at 1:1: above it, processing is downward; below it, processing is upward. Within the active threshold range, higher downward ratios usually cut more, while smaller upward ratios usually boost more. Dual separates UP RATIO and DOWN RATIO; see page 9.')
    para("See page 5 for the two algorithms' Ratio behavior. Normal Single spans 1:1000 to 1000:1 and defaults to 1:1. Limiter's Ceiling provides final peak protection; a high normal compression ratio alone does not enable Limiter.",9.6)
    heading('Input Gain')
    para('Input Gain changes the level entering the processing path, including the Dry path. With INT it also changes the detector level relative to Threshold. With EXT, the external send and Key Gain set the detector level; Input Gain is not a substitute.')
    heading('Mix')
    para('Mix blends Dry with processed Wet. It is also a useful processing amount control: first make the boost or cut clear, then lower Mix to blend back the original dynamics you want to retain.')
    note('Dynamic gain includes Mix','At Mix = 0%, effective dynamic gain is 0 dB; at 100%, the full processing amount is retained. Intermediate values are not simply the gain in dB multiplied by Mix. Makeup and Output Gain are excluded from this dynamic gain.')
    para('A practical order: find the compression character with Ratio, set its final strength with Mix, adjust Input Gain when you need a different input level, then compare loudness again.')


    start('Single: Threshold and Range','Choose the active levels, then set direction and strength with Ratio.')
    picture('manual-light-single-up.png','SINGLE: the lower Threshold and upper Range define the processing interval. The faders align with the dashed lines in the display.',maxh=220)
    table(['Detector level','Dynamic processing'],[
      ['At or below Threshold','No dynamic boost or attenuation.'],
      ['Above Threshold and below Range','Upward or downward processing, according to Ratio.'],
      ['At or above a finite Range','Returns fully to 0 dB dynamic gain.']], [.46,.54],9.3)
    heading('Range OFF: no upper cutoff')
    para('At its top OFF position, Range imposes no upper limit on the selected algorithm. Pull it down to set a cutoff. A finite 0 dB setting differs from OFF. Classic Threshold stops at -90 dB; Super retains -inf, meaning no lower gate.',9.8)
    para('Near a finite Range, dynamic gain returns continuously to 0 dB from inside the active interval, avoiding a sudden jump across the boundary. This transition follows detector level; it adds no Attack time or extra latency.',9.3)
    note('When the faders meet','Range stays at or above Threshold. When one fader reaches the other, it moves the other along; coincident boundaries disable dynamic processing. Drag back to separate them.')
    para("Here, 0 dB dynamic gain means the dynamic branch applies no boost or cut. Input, Makeup, Mix and Output Gain still work, and Limiter's final Ceiling remains independent. With EXT, the external detector signal determines the active range.",9.3)


    start('Upward: boost above the gate','Classic and Super both use this gate rule to bring selected detail forward.')
    para('In SINGLE, move Ratio below 1:1, for example to 1:2, 1:4 or 1:8, to enter upward processing. The displayed 1:8 means 1/8. Smaller values generally give eligible material more boost.')
    regions([('Original level','At or below Threshold'),('Upward boost','Between Threshold and Range'),('Original level','At or above a finite Range')],['Threshold','Range'],['#f3f6f7','#eaf6f8','#f3f6f7'])
    note('The gate enables boost; it does not mute audio','Threshold acts as a gate that allows boost above it. Below the gate, the original level is retained: quieter sounds are neither muted nor specifically boosted.')
    step(1,'Select the detail to bring forward','Select SINGLE. Set a lower Threshold and upper Range, for example -50 dB / -10 dB. Listen for the detail you want between them. These values are examples only.')
    step(2,'Then adjust Ratio and Mix','Move gradually from 1:1 toward 1:2 and 1:4. If the boost is too strong, reduce its strength or lower Mix. Do not substitute Makeup for dynamic boost.')
    step(3,'Listen to quiet and loud passages','Boost enters smoothly near the lower gate. Beyond that transition, quieter parts in the active interval generally receive more boost. Boost approaches 0 dB near the upper boundary. Range OFF removes the finite upper cutoff.')
    para('To leave very quiet breaths, noise or reverb at their original level, place the lower gate above them. Check that the desired material still enters the boost interval. Selection follows detector level, not sound type.',9.5)


    start('Dual: lift quieter parts, control louder ones','Click the small button beside Ratio to change SINGLE to DUAL.')
    picture('manual-light-dual.png','DUAL: UP RATIO and DOWN RATIO control the two directions separately. UP / DOWN thresholds replace Threshold / Range.',maxh=245)
    regions([('Original level','At or below UP'),('Boosted by UP RATIO','Above UP, below DOWN'),('Cut by DOWN RATIO','Above DOWN')],['UP threshold','DOWN threshold'],['#f3f6f7','#eaf6f8','#eaf6f8'])
    para('At DOWN, dynamic gain returns to 0 dB. Above it, upward processing stops and only Down Ratio applies. The two gains never stack at the same detector level. Dual has no Range control.',9.7)
    table(['Starting a new instance','How to read it'],[
      ['UP: -90 dB in Classic, -inf in Super. DOWN: 0 dB.','The thresholds start at opposite ends; this does not disable upward processing.'],
      ['Up Ratio = 1:1; Down Ratio = 1:1','Both directions are neutral. Adjust a Ratio to hear processing.']], [.48,.52],9.2)
    note('Threshold order and collision','DOWN must stay at or above UP. When one fader reaches the other, it pushes the other along. Dynamic processing stops when the thresholds coincide.')


    start('Dual switches: audition each direction','A small ON / OFF switch sits beside each UP RATIO and DOWN RATIO knob.')
    picture('manual-light-dual-controls.png','Both branches on. Each switch preserves its Ratio value for repeated comparison.',maxh=148,width=245)
    table(['UP / DOWN','Processing you hear'],[
      ['ON / ON','Boost between UP and DOWN; attenuation above DOWN.'],
      ['ON / OFF','Upward only. Boost still stops at DOWN.'],
      ['OFF / ON','Downward only. No dynamic processing below DOWN.'],
      ['OFF / OFF','No dynamic boost or attenuation. Other gain and Mix settings still apply.']], [.27,.73],9.5)
    step(1,'Set up the full Dual sound','Set both thresholds and Ratios. Play material with quiet and loud passages.')
    step(2,'Hear what each direction contributes','Switch DOWN off to hear Up, or UP off to hear Down, then enable both. There is no need to return to SINGLE or reset either Ratio.')
    step(3,'Check the display and compare at similar loudness','The switches update dynamic gain and the Display history reference. Compensate overall level, then compare detail, articulation and consistency.')
    note('Crossfaded switching; boundaries still apply','Each branch uses a short crossfade to reduce ON / OFF transients. Switching a branch off neither moves the thresholds nor extends the other branch into a different interval. In LR / MS, each channel has its own switches.')


    start('Ratio LINK: opposite proportional changes','The small LINK above and between the two Ratios appears only in DUAL.')
    picture('manual-light-dual-controls.png','Up and Down retain their existing relative relationship. Enabling LINK does not make values jump.',maxh=100)
    para('Ratio LINK defaults to on at first use, then remembers your last choice. When reopening a saved project, the LINK state saved in that project takes priority.')
    table(['Initial Down / Up','Adjust Down','Linked Up'],[
      ['1:1 / 1:1','1:1 → 2:1','1:2'],
      ['4:1 / 1:2','4:1 → 8:1','1:4'],
      ['8:1 / 1:4','8:1 → 4:1','1:2']], [.36,.32,.32],9.6)
    heading('Preserve the relationship, without re-pairing')
    para('For example, doubling Down halves the Up value; adjusting Up uses the same inverse relationship. Starting at 4:1 and 1:2 will not force them into a reciprocal pair. Dragging, fine adjustment and direct entry all follow this rule.')
    note('At a limit, both stop together','Up spans 1:1000 to 1:1. Normal Down spans 1:1 to 1000:1; Limiter Down starts at 20:1. If either control would exceed its range, both stop at their shared allowed limit. Reverse direction to continue. Toggling LINK alone does not change Ratio.')
    heading('To hear upward processing alone')
    para('Switch DOWN off to preserve its Ratio while auditioning Up. To adjust Up Ratio independently as well, turn off the small Ratio LINK; otherwise the Down value still follows your adjustment.')
    heading('Also available in LR / MS')
    para('The small LINK scales down with compact layouts and stays between Up / Down Ratio. It links those Ratios within each channel. Channel LINK beside Mode links L/R or M/S; see page 20.')


    start('Input / Output LINK: opposite gain changes','Above and to the right of INPUT GAIN. It is separate from Ratio LINK.')
    picture('manual-input-output-link.png','Example: Input +3 dB, Output -3 dB, with LINK enabled.',maxh=105)
    para('With LINK on, raising Input Gain in the interface lowers Output Gain by the same number of dB. Adjusting Output moves Input in the opposite direction too. This helps reduce simple level changes while you explore a different level entering the processor.')
    table(['Initial Input / Output','Adjustment','Result'],[
      ['0 / 0 dB','Raise Input by 3 dB','+3 / -3 dB'],
      ['+2 / -1 dB','Raise Input by 3 dB','+5 / -4 dB'],
      ['+5 / -4 dB','Raise Output by 2 dB','+3 / -2 dB']], [.35,.33,.32],9.5)
    heading('Keep an existing offset')
    para('The link follows relative changes; it does not force the two values to be exact opposites. Toggling LINK alone leaves the gains unchanged. If either control reaches its allowed limit, both stop together.')
    heading('Remember the last choice')
    para('LINK is on at first use. New instances then use your last choice, while saved projects restore their own state. This is a workflow preference and does not switch with A/B sound snapshots.')
    note('Limiter uses a different link','Normal Input / Output LINK adjusts gains in opposite directions; it does not replace Match. It hides and stops linking in Limiter. The LINK beside Output instead connects the downward threshold and output; see page 26.')


    start('Match and Makeup: compare at equal loudness')
    table(['Control','Use'],[
      ['Makeup','Compensates Wet level before Mix; range -120 to +120 dB.'],
      ['Output Gain','Normal: -24 to +24 dB; Limiter: -120 to +120 dB. See pages 29-30 for headphone listening.'],
      ['Match','Uses played material for a one-time loudness adjustment to Makeup. It is not continuous auto gain.']], [.24,.76],9.6)
    step(1,'Play a representative passage','Allow a valid loudness reading to build before clicking Match. Use material that represents the passage, not just a very brief attack.')
    step(2,'Compare in the current listening mode','Normal mode compares Dry with Wet before compensation. Limiter headphone mode compares original input with the actual limited listening output. Makeup changes Wet; see page 30.')
    step(3,'Trim the final level','Use Makeup or Output Gain for any small correction. Compare with Bypass or A/B and listen to attacks, tails and consistency across the passage.')
    heading('Deep compression and very low levels')
    para('Deeply compressed Wet is no longer excluded just because it falls below a fixed loudness gate. The wider Makeup range also allows larger corrections. Match still needs a valid, nonzero measurement: silence or insufficient material cannot provide a reliable result.')
    note('Judge loudness and peak protection separately',"Match helps compare loudness; Limiter's Ceiling controls final peaks. Check output and TP after matching, especially after upward processing or further parameter changes.")
    heading('Three distinctions to keep in mind')
    para('Dynamic gain excludes Makeup and Output Gain. Input / Output LINK only makes equal and opposite gain changes; it cannot replace loudness matching. At Mix = 0%, Input and Output Gain still apply, so this is not full Bypass.')


    start('A/B: compare algorithms and directions')
    picture('manual-top-controls.png','A and B hold two sound settings. A→B and B→A copy between them.',maxh=62)
    step(1,'Save the first idea','Set the algorithm and processing amount in A. Copy A→B, then switch to B. Copying replaces the destination sound settings.')
    step(2,'Change the part you want to compare','For example, use Classic in A and Super in B, or compare downward and upward processing. Keep Lookahead, detector source and monitoring the same where possible.')
    step(3,'Match each, then switch','Play the same material for each setting and use Match; trim Makeup if needed. Click A / B and compare the sound, without mistaking louder for better.')
    table(['Recalled by A/B','Outside the A/B sound snapshot'],[
      ['Algorithm, Single / Dual, thresholds, ratios, branch switches, gains, Mix, Lookahead, sidechain, plus Limiter, TP, Ceiling, Limiter Output LINK and both parameter banks.','Theme, normal Input / Output LINK, Ratio / channel LINK, headphone 1:1 switch, Bypass and SC LISTEN.']], [.60,.40],9.3)
    heading('Why can reciprocal Ratios sound the same?')
    para('The upward and downward curves retain a symmetrical relationship. In a shared active interval, some reciprocal settings differ only by a constant gain, so they can sound very close after loudness matching. One example is 8:1 versus 1:8 in Super / SINGLE with Threshold = -inf, Range = OFF and Mix = 100%.')
    para('Classic also has a corresponding relationship within a shared active interval, away from the lower gate transition. Crossing a threshold, entering a finite Range transition, changing Mix or changing the Dual handoff can break equality across a whole passage.',9.5)
    note('Switching and latency','A/B uses transitions to reduce switching transients. Keep Lookahead the same for a clearer sound comparison. Different Lookahead settings may also cause the host to readjust delay compensation.')


    start('Read the Display and meters')
    picture('manual-light-dual.png','In the same history view, green shows boost and Cut / Mix shows attenuation. Orange is the output-level reference.',maxh=239,crop=(32,152,2336,1072))
    table(['What you see','Meaning'],[
      ['Dry / Input','Original dynamics before Input Gain; the grey line does not move as a whole when Input changes.'],
      ['Cut / Mix','Dynamic attenuation including Mix. Its distance from Dry indicates the effective cut.'],
      ['Boost / Mix','Dynamic boost including Mix, shown in green. Its distance from Dry indicates the amount of boost.'],
      ['Output','Normal compression projects levels under current settings. Limiter shows final output captured at the time, including Ceiling and headphone compensation.'],
      ['External key','When EXT is selected and the external input is available, a lighter trace shows the sidechain reference to help check triggering.'],
      ['Gain +/-','Zero is at the center: boost above, cut below. Positive values indicate boost and negative values indicate attenuation. Hold retains recent prominent changes.']], [.25,.75],8.9)
    para('Dynamic gain excludes Makeup / Output Gain and is 0 dB at Mix = 0%. All three themes show boost and cut; match the colors to the named legend below the graph.',9.3)
    note('History responds to current settings',"Ratio, threshold, Range and Mix changes update visible history. EXT Key Gain updates live; HPF refreshes on release and may show HPF UPDATING. Normal compression recalculates historical references. Limiter's orange trace retains the actual output captured then: watch newly arriving material after edits. Read TP from the right-hand meter.")


    start('Lookahead: align gain with the sound','Classic and Super share the same detector timing.')
    para('Lookahead lets the plugin see a short section of upcoming signal before it outputs the current sound. A more stable level estimate helps reduce harmonics caused by gain following every sine-wave cycle. Lookahead is not conventional Attack.')
    heading('Reducing the dip before a louder event')
    para('Detection considers windows before and after the current sound, then uses the lower of their two peaks. A future loud event no longer determines the attenuation of the preceding quiet passage on its own. When the loud event arrives, gain follows the aligned level estimate.')
    # Same palette as the existing range diagrams; schematic, not a measured trace.
    top=y;mid=M+CW/2
    c.setStrokeColor(HexColor(MUTED));c.setLineWidth(.7);c.line(M+14,top-25,W-M-14,top-25)
    for xx,ww,tt in [(M+14,CW/2-14,'Past window'),(mid,CW/2-14,'Future window')]:
        c.setFillColor(HexColor(PALE));c.roundRect(xx,top-56,ww,24,3,fill=1,stroke=0)
        c.setFillColor(HexColor(TEAL));c.setFont(font,9);c.drawCentredString(xx+ww/2,top-48,tt)
    c.setStrokeColor(HexColor(TEAL));c.line(mid,top-14,mid,top-62)
    c.setFillColor(HexColor(INK));c.setFont(font,9);c.drawCentredString(mid,top-5,'Sound currently being processed')
    c.setFillColor(HexColor(MUTED));c.setFont(font,8);c.drawCentredString(mid,top-80,'Schematic: both windows include the present; output is still delayed by Lookahead.')
    y=top-101
    note('The initial default remains 26 ms','The compression detector still uses the selected window. Version 1.2.7 adds a fixed buffer for final peak protection, so reported total latency is slightly greater than Lookahead. This buffer stays in place when switching Limiter / TP; see page 31.')
    heading('Low distortion, with a trade-off on changing signals')
    para('A suitable window can give steady sine waves stable gain and preserve their shape. Music is not a constant sine wave: rapid amplitude modulation, short events and complex combinations can still produce modulation residuals. Lookahead does not guarantee zero harmonic distortion for every signal.')
    para('This version prioritizes less unnecessary advance attenuation so processing follows the current sound more closely. If a passage still shows small dips or waveform changes, check the detector source, window length and actual audio. Do not judge the result from a Display trace alone.',9.5)


    start('Lookahead and the 0 ms option')
    para('Lookahead affects level estimation and latency, not Attack. Longer windows may improve stability for some low-frequency or rapidly changing material, but do not improve every transient.<br/>A longer window also increases plugin latency, which the host will usually compensate.')
    picture('manual-light-0ms.png','At 0 ms, Oversampling appears below Lookahead.',maxh=327)
    table(['Choice','Use and trade-off'],[
      ['10 ms','Shorter window and lower latency.'],
      ['26 / 40 ms','Default 26 ms. Compare with 40 ms if needed and choose by ear.'],
      ['80 / 100 ms','Longer windows; listen for greater stability and consider the extra latency.'],
      ['0 ms','More pronounced nonlinear colour, not the most transparent mode.']], [.24,.76],9)
    para('At 0 ms, Oversampling offers 1x / 8x / 16x. Start with 8x; higher factors add load and latency. The control hides at 10 ms and above. At 0 ms, 8x / 16x still add latency, and the final peak-protection buffer is also included in the host report.',9.5)


    start('ST and LR: together or independently')
    para('Click Mode at the lower right to cycle ST → MS → LR. Start with ST; choose LR when the two sides genuinely need different treatment.')
    picture('manual-light-LR.png','LR: L above, R below, with independent controls.',maxh=373)
    table(['Mode','How to use it'],[
      ['ST','Uses common compression gain for stereo; a useful starting point for tracks and buses.'],
      ['LR','Left and right have independent Ratio, boundary, Makeup and Mix controls. In DUAL, each side has its own UP / DOWN. Check for image shifts when processing asymmetric material.']], [.17,.83],9.4)
    note('MONITOR','Choose L or R to audition that side centred. Return to ALL to check normal stereo, and do not leave one-sided audition active for the final output.')


    start('MS: control centre and sides')
    picture('manual-light-dual-MS.png','MS / DUAL: Mid is above Side. Each channel has its own UP / DOWN Ratios and thresholds.',maxh=340)
    para('Mid mainly represents content common to both sides; Side represents their differences. They are not a precise separation of a centre track from every other track.')
    step(1,'Listen to M and S first','Use M / S under MONITOR, then return to ALL.')
    step(2,'Adjust each for a reason','For steadier centre dynamics, start with M Ratio / Mix. Treat S cautiously when preserving ambience and width.')
    note('Recheck the full mix','Compression or level changes in M/S can alter the sense of space. In ALL, check centre, width and mono compatibility, not only soloed domains.')


    start('Channel LINK: adjust together, keep differences')
    para('In LR or MS, LINK beside Mode links matching parameters across channels. Set the two sides separately, then enable it to adjust them together without forcing identical values.')
    picture('manual-light-LR-link.png','SINGLE example: channel LINK beside Mode is on, while the left and right Ratios remain at 3:1 and 5:1.',maxh=110)
    table(['Parameter','Before','After an equal change'],[
      ['Ratio','3:1 / 5:1','4:1 / 6:1'],['Threshold','-20 / -10 dB','-18 / -8 dB'],
      ['Makeup','-3 / +1 dB','-2 / +2 dB'],['Mix','100% / 70%','90% / 60%']], [.26,.36,.38])
    step(1,'Set the relationship with LINK off','Decide which domain needs more compression or a different blend.')
    step(2,'Enable LINK for joint adjustments','Move either parameter in a pair; the other follows by the same change. Fine adjustment and direct entry work too.')
    note('Both stop at a limit','If either side would exceed its range, both stop where the difference can be retained. In Super, one Threshold at -inf cannot form a finite dB offset; set both thresholds to finite values first if you want that relationship.')
    heading('Three LINK controls, three roles')
    para('The small LINK between the Ratios links Up / Down within each channel. LINK beside Mode links matching L/R or M/S parameters. With both enabled, channels move together while each channel retains its existing Up / Down relationship. LINK beside Input only controls opposite input/output gain changes; see page 12.')


    start('Sidechain: start with the panel','Click SC: INT / SC: EXT at the top right. Its label shows the current source.')
    picture('manual-sidechain-panel.png',maxh=185,width=415)
    table(['Control','How to use it'],[
      ['INT / EXT','INT follows the current track itself. EXT uses a signal sent from another track to control boost or attenuation on the current track.'],
      ['KEY GAIN','Available only with EXT. Adjusts the external detector level and trigger strength, not the main track level.'],
      ['HPF','Reduces low frequencies in the INT or EXT detector key. OFF is full-band; active range is 20-500 Hz.'],
      ['KEY LEVEL','Check that the detector receives a signal. With EXT showing N/A, first check that the host sidechain bus is enabled.'],
      ['SC LISTEN','Temporarily audition the detector key to check routing and HPF. Switch it off before judging normal output.']], [.24,.76],9.4)
    note('Two common tasks','Low notes pull the whole part down: try INT + HPF, page 22.<br/>Want bass to duck when the kick plays? Use EXT, page 23.')


    start('Internal sidechain: reduce bass dominance')
    para('Use this when kick or bass repeatedly pulls down the rest of a drum bus, mix bus or master. The goal is less low-frequency triggering, not less bass in the audible signal.')
    for i,(tt,body) in enumerate([
      ('Start with INT and hear the problem','Loop a passage with bass and other content together. Leave HPF OFF to establish a comparison.'),
      ('Open the sidechain panel','Click SC: INT and keep SOURCE on INT. A disabled Key Gain is normal here.'),
      ('Raise HPF gradually','Start at a low cutoff and listen for less whole-mix ducking on bass events. Make small changes.'),
      ('Audition the key if needed','Use SC LISTEN to hear which bass content HPF removes, then turn it off and return to the full sound.'),
      ('Inspect history after release','HPF history refreshes after you release the knob. If HPF UPDATING appears, wait for it to clear before comparing.'),
      ('Rebalance the compression amount','Filtering may reduce GR or change its shape. Revisit Ratio, Threshold or Mix as needed, then compare at similar loudness.')],1):step(i,tt,body)
    note('It is normal not to hear bass being cut','HPF filters only the key used to decide compression, not normal output. It is not equivalent to Key Gain: Key Gain scales the detector level; HPF changes frequency weighting. Too high a cutoff can make the compressor ignore bass that still needs control.')


    start('External sidechain: kick-controlled bass')
    para('Insert the plug-in on the bass - the track to turn down. Send the kick to its sidechain as the trigger. Do not reverse the tracks or confuse a normal audio send with a sidechain send.')
    # Compact routing makes the two signals unambiguous.
    x=M;boxw=(CW-38)/3;bh=47
    labels=['KICK TRACK','BASS PLUG-IN KEY','BASS IS REDUCED']
    for i,label in enumerate(labels):
        xx=x+i*(boxw+19);c.setFillColor(HexColor(PALE));c.roundRect(xx,y-bh,boxw,bh,5,fill=1,stroke=0)
        ob=p_obj(label,9,TEAL,True);_,hh=ob.wrap(boxw-16,bh);ob.drawOn(c,xx+8,y-(bh+hh)/2)
        if i<2:c.setFillColor(HexColor(TEAL));c.setFont('Body',13);c.drawString(xx+boxw+4,y-29,'→')
    y-=bh+20
    for i,(tt,body) in enumerate([
      ('Enable the host sidechain input','Enable sidechain input for this plug-in instance on the bass. Add and enable a kick send to that sidechain. Names and locations vary by host.'),
      ('Select EXT in the plug-in','Open SC, choose EXT and play the kick. Check KEY LEVEL first; if it has no reading, fix routing before increasing Ratio.'),
      ('Confirm the actual trigger','Briefly enable SC LISTEN: you should hear the kick, not the bass. Turn audition off afterward.'),
      ('Set trigger level and reduction','For downward ducking, start with SINGLE, Ratio above 1:1 and Range OFF. Set detector strength with Key Gain or the send level, and set the lower boundary with Threshold. Leave HPF OFF initially.'),
      ('Use Mix to decide how much room to make','Find an audible duck, then reduce Mix to retain the desired bass sustain. Listen to both tracks together, not only solo bass.'),
      ('Decide whether HPF helps','Raising HPF reduces low-frequency kick triggering and may weaken the duck you want. Use it when you want more emphasis on the attack or less unwanted low-bass triggering.')],1):step(i,tt,body)
    para('There are no conventional Attack / Release controls. Ducking shape depends on the trigger, Lookahead and compression settings; it is not guaranteed to be smoother than every conventional sidechain.',9.3)


    start('Apply it in a real mix')
    heading('Vocals, guitar, piano: consistency with articulation')
    para('Try SINGLE / ST / INT and 26 ms. Raise Ratio above 1:1, then use Mix to keep consonants, plucks or hammer attacks natural. To process only louder parts, raise Threshold and leave Range OFF. Use Match and compare in the mix.')
    heading('Tails and quiet phrases: bring detail forward')
    para('Try Single upward processing to lift quieter words, pluck tails or reverb. Use Threshold to exclude unwanted very quiet sounds and Range to set the upper limit. To also control louder parts, switch to Dual and adjust Up and Down separately. Use the branch ON / OFF switches to audition each direction. Turn Ratio LINK off if you also want to adjust each Ratio independently.')
    heading('Mix bus / mastering: a more cohesive balance')
    para('When you want a steadier, more cohesive mix without deliberately reshaping drum attacks through conventional Attack / Release, try it as an alternative to G Bus-style compression. Start gently, compare balance across sections and lower Mix when needed.')
    para('If every kick pulls the whole mix down, try INT + HPF. Do not raise the cutoff merely to make the GR number smaller. At similar loudness, check drum attack, low end, space and the song’s larger dynamic movement.')
    heading('Vocal-keyed accompaniment: make space for words')
    para('Insert the plugin on the backing bus, send the vocal to its external sidechain and choose EXT. Confirm the source with SC LISTEN. Select SINGLE, Ratio above 1:1 and Range OFF, then adjust Key Gain, Threshold and Mix so the backing steps back while the vocal plays.')
    para('If plosives trigger too much ducking, try raising sidechain HPF. If breaths or noise also make the accompaniment dip, check the sent signal and Threshold; HPF alone will not solve every unwanted trigger.')
    note('For every comparison','Turn off SC LISTEN, return channel MONITOR to ALL, and bring the before/after loudness close. For final peak control, enable Limiter and set Ceiling. Combine the headphone button with Match for level-matched listening; see pages 25-31.')


    start('Limiter: control peaks after dynamics','Click LIMITER at the top. Both Classic and Super are supported.')
    picture('manual-limiter-light.png','Limiter / Classic / ST with TP enabled. These values explain the interface; they are not a universal preset.',maxh=318)
    para('Limiter adds a final Ceiling after the existing dynamics to control outgoing peaks. Single / Dual, upward processing and ST / LR / MS remain available. ALGO still selects the dynamic curve before that final stage.')
    table(['Location','Use'],[
      ['LIMITER at the top','Enable final peak protection and the Limiter parameter set.'],
      ['LINK beside Output','Link the downward threshold and output; see the next page.'],
      ['Ceiling / TP','Set the upper limit and select true-peak or sample-peak protection.'],
      ['Headphone beside Output','Compare the full limited result at a compensated listening level.']], [.30,.70],9.3)
    note('Shape dynamics, then assess the final output','Set Threshold, Ratio and Mix first, then Ceiling. Use headphone + Match + Bypass for a level-matched comparison. A large gain-reduction reading or high Ratio does not replace checking final peaks.')


    start('Limiter: Ratio, threshold and Output LINK')
    picture('manual-limiter-controls.png','In Limiter, normal LINK beside Input hides and Limiter LINK appears beside Output.',maxh=97)
    heading('Down starts at 20:1; Up remains available')
    para("Limiter's downward Ratio spans 20:1 to 1000:1. Single retains 1:1000 to 1:1 for upward or neutral dynamics, with the downward region starting at 20:1. Dual retains the original Up range; Down spans 20:1 to 1000:1.")
    heading('Adjust strength with threshold and Output')
    para('LINK beside Output is on by default. Lowering the downward threshold compensates Output; raising Output lowers that threshold. The relationship depends on the algorithm, Ratio, Makeup and Mix. It is not a fixed 1 dB threshold-to-output exchange.')
    para('Single downward processing links Threshold; Dual links DOWN THR. UP THR selects material eligible for boost and is not moved by this LINK. Makeup, Mix or curve edits update linked Output. Turn LINK off for independent adjustments.',9.6)
    heading('Entering and leaving Limiter')
    para('Entering builds a Limiter parameter set from normal settings, with downward Ratio at least 20:1. Output initially inherits a nonnegative normal Output; a negative value starts at 0 dB. Downward threshold and Output then work through their link.')
    note('Leaving Limiter restores normal settings','Normal compression and Limiter retain separate Ratio, boundary and Output settings. Leaving restores the normal settings from before entry. Re-entering builds Limiter settings from the current normal setup. Shared Makeup, Mix and Input controls keep their current values.')


    start('Ceiling and TP: choose the final limit')
    picture('manual-limiter-light.png','TP lit: Ceiling is shown in dBTP.',maxh=202,width=205,crop=(1700,1236,334,344))
    table(['Setting','Meaning'],[
      ['Ceiling','Range: -24 to 0 dB; default 0. Sets the final output limit.'],
      ['TP lit','Default. Protection includes intersample peaks in the reconstructed waveform.'],
      ['TP unlit','Protects sample peaks; the value shows dBFS. True peaks may still exceed it.']], [.25,.75],9.5)
    para('For example, Ceiling = -1 dB sets a true-peak limit with TP on, or a sample-peak limit with TP off. TP keeps a small reconstruction margin, so the measured maximum can sit slightly below the setting rather than exactly on it.')
    heading('Editing the value')
    para('Double-click to type; drag vertically to adjust, or hold Shift for finer movement. Alt-click restores 0. Ceiling edits support Ctrl+Z / Ctrl+Shift+Z undo and redo.',9.6)
    note('Where the switch applies','TP and Ceiling work only in Limiter. Both hide and stop applying when Limiter is off. TP changes are smoothed. Its state is saved with projects and A/B; an existing project keeps its saved choice.')


    start('TP meter: measure output and hold peaks')
    picture('manual-limiter-tp-meter.png','The TP L/R readout measures true peaks in the final left/right stereo output.',maxh=151,width=280)
    heading('Measured after Ceiling')
    para('The TP meter reads actual output, including Makeup, Mix, Output Gain, Ceiling and headphone 1:1 compensation. It does not simply display the Ceiling setting, or clamp its reading to that setting.')
    heading('Hold and reset')
    step(1,'Play the passage to check','The peak reading holds for about 20 seconds. A new higher peak updates the value and restarts the timer. Hold advances with audio playback time.')
    step(2,'Start a fresh measurement','Double-click the TP readout to clear it immediately, then play the passage again. There is no need to wait for the previous held peak to expire.')
    step(3,'Measure again after changing Ceiling','A held peak may come from earlier settings. Clear it before deciding whether the current limit behaves as expected.')
    table(['What you see','How it works'],[
      ['TP exceeds Ceiling with TP off','The meter still measures true peaks. Sample-peak protection does not also guarantee the intersample peak limit.'],
      ['TP changes with headphones on','The readout includes listening compensation. Judge it against the converted MON limit.'],
      ['MS domain meters differ from TP','TP measures final L/R. Individual M/S levels and left/right output limits are different quantities.']], [.38,.62],9.2)


    start('Headphone 1:1: keep limiting, offset gain','The small headphone button left of Output Gain appears only in Limiter.')
    picture('manual-limiter-monitor-controls.png','With headphones on, Output values and links stay active. MON shows the listening limit below.',maxh=103)
    para("With headphones on, audio still passes through the complete dynamics and Ceiling stages. The visible Output Gain is then cancelled. Limiting's dynamic and waveform changes remain; only the listening level is adjusted for comparison.")
    heading('The Output value still matters')
    para("Output remains adjustable, and its links with the downward threshold and Makeup remain active. Headphones cancel Output's direct level contribution, but dynamic changes caused by edits remain audible. Switch headphones off to restore normal Output Gain.")
    heading('Ceiling follows the same listening reference')
    note('MON = Ceiling - Output Gain','For example, Ceiling = -1 dB and Output = +6 dB give a listening limit of -7 dB with headphones on. The Ceiling setting stays at -1 dB. Switching headphones off restores the normal output reference. TP measures the actual output.')
    para('Headphones default to off, are saved with the project, and remain independent of A/B. Keep the same listening mode while comparing A/B. Headphone and Bypass switches use smooth transitions; headphones add no separate audio delay.',9.6)
    note('1:1 does not automatically mean equal loudness','Headphones cancel Output Gain; they do not automatically compensate all loudness changes from compression, Ceiling or Input. Use Match for a level-matched comparison. The next page gives the full procedure.')


    start('Limiter comparison: headphone + Match + Bypass')
    step(1,'Choose the listening setup','Enable Limiter and headphones, then set Ceiling and TP. Turn off SC LISTEN and return channel MONITOR to ALL to hear the full stereo result.')
    step(2,'Play the same representative passage','Allow loudness measurements to build. Parameter changes start fresh measurement collection. Do not rush to Match while adjusting continuously, during silence or immediately after starting playback.')
    step(3,'Click Match to compensate Makeup','Headphone mode compares original input with actual output after Ceiling and headphone compensation. LR / MS use a common correction, preserving relative channel balance. The dry/wet blend is included.')
    step(4,'Compare the original with plugin Bypass','Bypass returns delay-aligned original input without limiting or headphone gain compensation. Listen for attack, tails, density and groove; trim Makeup if needed.')
    step(5,'Finish listening and restore normal output','Switch headphones off and keep the adjusted settings. Check TP and Ceiling again before judging the final signal sent downstream.')
    heading('Match applies one correction, not continuous gain riding')
    para('After changing the passage, limiting depth or other parameters, play again and Match. With Limiter LINK off, controls at their limits, or Mix fully dry, the compensation available through Makeup may be restricted.',9.5)
    note('When comparing A/B','Play the same passage for A and B, then Match each. Headphones stay independent of A/B; Limiter / TP / Ceiling restore with A/B. Prefer the same Lookahead so latency changes do not distract from the comparison.')


    start('Limiter tips and troubleshooting')
    heading('Recovery speed and low-frequency detail')
    para('Final peak protection adapts recovery to signal periods and repetition. It can recover quickly after isolated peaks, while low frequencies or repeated modulation receive steadier treatment. It still uses smoothing and buffering, not sample-by-sample hard clipping. Judge complex transients by listening.')
    heading('26 ms versus the host latency report')
    para('Lookahead initially defaults to 26 ms. Final peak protection needs about 7-8.5 ms of fixed buffering, approximately 8.17 ms at 48 kHz. Normal compression, Limiter and TP switching retain the same total latency for host compensation. Neither 0 ms nor an oversampling choice necessarily means zero total latency.',9.6)
    table(['Symptom','First check'],[
      ['Ceiling / TP / headphones are missing','Enable LIMITER at the top; these controls appear only in that mode.'],
      ['Output also moves the threshold','Limiter Output LINK is on. It links the downward boundary; switch it off for independent edits.'],
      ['Output gain is inaudible with headphones','That is the listening compensation. Values and links remain active; switch headphones off for normal output.'],
      ['Limiting remains at Mix 0%','Mix blends the dynamic section. Final Ceiling still works afterward. Use Bypass for the complete original signal.'],
      ['The TP peak seems unchanged','The old peak is still held. Double-click to reset, play a new passage and consider the TP / Peak selection.'],
      ['MATCH is still unavailable','Play non-silent audio and stop continuous edits so the current settings can accumulate valid measurements.']], [.34,.66],9.1)
    note('Confirm which signal you are hearing','Judge final peaks with the full output: SC LISTEN off and channel MONITOR at ALL. After level-matched listening, turn headphones off before checking normal output.')


    start('Quick reference: algorithms and dynamics')
    table(['Parameter','Range / choices','Notes'],[
      ['ALGO','Classic / Super','Fixed dB ratio / original QQ curve. Remembers the last choice; project and A/B states take priority.'],
      ['SINGLE / DUAL','Single / Dual','Button beside Ratio; each mode retains its own parameters.'],
      ['Single Ratio','Normal: 1:1000 to 1000:1','Normal default 1:1. Limiter Up extends to 1:1; Down spans 20:1 to 1000:1.'],
      ['Up Ratio','1:1000 to 1:1','Default 1:1; upward boost.'],
      ['Down Ratio','Normal: 1:1 to 1000:1; Limiter: 20:1 to 1000:1','Normal default 1:1; Limiter downward ratios start at 20:1.'],
      ['UP / DOWN switches','ON / OFF','Both default ON. Independent audition; Ratio values retained.'],
      ['Threshold / UP THR','Classic: -90 to 0 dB; Super: -inf to 0 dB','Lower gate; defaults to the selected algorithm’s lowest position.'],
      ['Range','OFF or a set upper boundary','Single defaults to OFF. Set range: Classic -90 to 0 dB; Super -inf to 0 dB.'],
      ['DOWN THR','Classic: -90 to 0 dB; Super: -inf to 0 dB','Dual handoff threshold; default 0 dB.'],
      ['Ratio LINK','OFF / ON','Initially ON; remembers the last choice. Project state takes priority.'],
      ['Mode / channel LINK','ST / MS / LR; OFF / ON','LINK joins matching parameters in LR / MS.']], [.23,.34,.43],8.9)
    note('Boundaries and defaults','Range never falls below Threshold; DOWN never falls below UP. Colliding boundaries move together; coincident boundaries stop dynamic processing. Existing projects retain saved values. All three normal Ratio controls start at 1:1 in every domain; Limiter has a separate downward range.')
    para('Super’s lowest finite threshold is -119.99 dB, followed by -inf at the bottom. The Display plots down to -90 dB. Range OFF is different from a finite 0 dB setting.',9.2)


    start('Quick reference: gain, monitoring and SC')
    table(['Parameter','Range / choices','Notes'],[
      ['Input Gain','-24 to +24 dB','Level into the processing path.'],
      ['Makeup','-120 to +120 dB','Default 0 dB; Wet level before Mix.'],
      ['Mix','0 to 100%','Default 100%; included in effective dynamic gain.'],
      ['Output Gain','Normal +/-24 dB; Limiter +/-120 dB','Limiter headphones cancel its listening gain; the value and links remain active.'],
      ['Input / Output LINK','OFF / ON','Initially ON. Equal opposite changes; remembers the last choice, with project state taking priority.'],
      ['MONITOR','ALL / L / R or M / S','LR / MS audition; individual domain centred.'],
      ['Lookahead','0 / 10 / 26 / 40 / 80 / 100 ms','Initial starting point 26 ms; new instances remember the last choice.'],
      ['Oversampling','1x / 8x / 16x','Visible only at 0 ms; initially 8x.'],
      ['SOURCE','INT / EXT','Default INT; EXT needs host routing.'],
      ['KEY GAIN','-24 to +24 dB','Default 0 dB; adjustable only with EXT.'],
      ['HPF','OFF / 20 to 500 Hz','Default OFF; filters only the detector key.'],
      ['SC LISTEN','OFF / ON','Temporary audition; closing panel/editor switches it off.'],
      ['Limiter / TP','Defaults: OFF / ON','TP acts only in Limiter; with TP off, protection uses sample peaks.'],
      ['Ceiling','-24 to 0 dB','Default 0; Alt-click resets to 0.'],
      ['Headphone 1:1','Default OFF','Saved with the project; independent of A/B. MON = Ceiling - Output.']], [.22,.34,.44],8.3)
    note('Keep the gain controls distinct','Input changes the main input; Key Gain changes only the external detector; Makeup changes Wet before Mix; Output Gain changes the final output. Match loudness before comparing dynamics.')


    start('Upward / Dual troubleshooting')
    table(['Symptom','First check'],[
      ['No audible boost after adjustment','Check that the relevant Ratio is below 1:1, Mix is above 0% and Bypass is off. In Dual, that branch must be ON.'],
      ['No upward effect in Dual','The detector must be above UP and below DOWN. Below UP, the original level is retained; above DOWN, only downward processing applies.'],
      ['Faders set, but no processing','Do the boundaries coincide? This stops all dynamic processing. Separate them before comparing.'],
      ['Loud parts suddenly stop being processed','Is a finite Range set in SINGLE? At or above it, dynamic gain returns to 0 dB. Select OFF if you want no upper cutoff.'],
      ['Adjusting Up also changes Down','LINK between the two Ratios is on by default. Turn it off to adjust them independently.'],
      ['LINK will not move further','The other Ratio may have reached its limit, stopping both controls. Reverse the adjustment to continue.'],
      ['Main track level does not explain triggering','Check whether SOURCE is EXT or HPF is changing the detector signal. Threshold decisions follow the detector signal.']], [.33,.67],9.2)
    heading('Check Dual upward processing alone')
    step(1,'Keep other variables fixed','Select DUAL, switch DOWN off and turn Ratio LINK off. Keep Mix at 100% and all gain controls at 0 dB. Choose a suitable Lookahead.')
    step(2,'Place the signal between the thresholds','For example, set UP to -50 dB and DOWN to -20 dB. Check that the detector level of a quiet passage lies mainly between them. With Ratio still at 1:1, there is no boost yet.')
    step(3,'Increase upward strength from 1:1','Move Up Ratio through 1:2, 1:4 and 1:8 while listening and watching Boost / Mix and positive dynamic gain. Once confirmed, enable DOWN to add attenuation and compare overall loudness again.')


    start('Troubleshooting')
    heading('Why can Classic output fall below the threshold?')
    para('Threshold defines the gain curve for the detector level, not a minimum output level. Quieter sounds within a window can receive the gain set by its peak. Shared ST gain, external sidechain or HPF can also make the detector differ from the signal you see. Makeup and Output Gain then change the output further.',9.5)
    para('With Range OFF, internal detection, all gains at 0 dB and Mix at 100%, a steady level above threshold stays at or above it under the Classic formula. If a dip appears only in the Display, remember that it is a historical projection and check the actual audio. Not every dip is advance compression.',9.5)
    table(['Symptom','First check'],[
      ['EXT has no effect','Enable the host sidechain input and send. Select EXT and check KEY LEVEL. N/A means no external bus is available.'],
      ['Key Gain is disabled','Normal with INT: it adjusts only the external key.'],
      ['HPF does not cut output bass','It filters the detector only. History refreshes after release; wait for HPF UPDATING to clear.'],
      ['Sidechain sound / narrow image','Turn off SC LISTEN, return MONITOR to ALL, then check LR / MS.'],
      ['Match is unavailable','Play enough non-silent material for valid readings. Falling below the old gate no longer disables matching.'],
      ['Pause after changing Lookahead','Latency changes, so the host may readjust compensation. Also consider latency when monitoring a live recording.'],
      ['Nothing below -90 dB','This is the Display range, not an audio cutoff.']], [.30,.70],8.9)
    para('For Limiter TP, Ceiling, headphones and output, see page 31. For installation, updates and scanning, read the installation guide for your platform.',9)
    para('License: Qing Audio Non-Commercial Source-Share License 1.0. Commercial use is prohibited; see the project repository for full terms and corresponding source.',8.7)
    para('<link href="https://github.com/Ziqing-Gu/QQ-Super-Compression" color="'+TEAL+'">github.com/Ziqing-Gu/QQ-Super-Compression</link>',9)

    finish();c.save()
    doc=PdfReader(a.output/name);assert len(doc.pages)==35
    txt='\n'.join(p.extract_text() for p in doc.pages)
    for banned in ('Revision','Rev 2','Plan A','Plan B','Plan C','JUCE','offscreen','offscreen','2.2x','normalized','cache','v1.0.1','appendix','addendum','schema'):
        assert banned not in txt,(lang,banned)
    print(lang, len(doc.pages),'pages; lowest content baselines:',mins)
