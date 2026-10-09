"""English 1.3.2 manual matching the approved Chinese edition."""
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
arg = argparse.ArgumentParser()
arg.add_argument('--output', type=Path, required=True)
arg.add_argument('--captures', type=Path, required=True)
arg.add_argument('--theme-images', type=Path, required=True)
arg.add_argument('--fonts', type=Path, required=True)
arg.add_argument('--language', choices=('en',), default='en', help='English edition matching the approved Chinese manual.')
a = arg.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
for name, file in [('Body', 'segoeui.ttf'), ('Bold', 'seguisb.ttf'), ('CN', 'msyh.ttc'), ('CNBold', 'msyhbd.ttc')]:
    pdfmetrics.registerFont(TTFont(name, str(a.fonts / file), subfontIndex=0))
W, H = (595.276, 841.89)
M = 48
CW = W - 2 * M
TEAL = '#009db5'
INK = '#293940'
MUTED = '#70848e'
PALE = '#eaf6f8'
for lang in ('en',):
    zh = lang == 'zh'
    font = 'CN' if zh else 'Body'
    bold = 'CNBold' if zh else 'Bold'
    pdfmetrics.registerFontFamily(font, normal=font, bold=bold, italic=font, boldItalic=bold)
    name = 'QQ Super Compression User Manual English_v1.3.2.pdf'
    c = canvas.Canvas(str(a.output / name), pagesize=(W, H), pageCompression=1)
    c.setTitle('QQ Super Compression 1.3.2 User Manual')
    c.setAuthor('Qing Audio')
    c.setSubject('Dynamics, Limiter, level-matched listening and sidechain guide')
    page = 0
    y = 0
    mins = []

    def p_obj(text, size=10, colour=INK, boldface=False):
        if page in (31, 36): size *= 0.94
        return Paragraph(text, ParagraphStyle('p', fontName=bold if boldface else font, fontSize=size, leading=size * 1.40, textColor=HexColor(colour), wordWrap='CJK' if zh else None))

    def para(text, size=10, after=10):
        global y
        o = p_obj(text, size)
        _, h = o.wrap(CW, 800)
        o.drawOn(c, M, y - h)
        y -= h + after

    def heading(text):
        global y
        y -= 5
        o = p_obj(text, 12, TEAL, True)
        _, h = o.wrap(CW, 800)
        o.drawOn(c, M, y - h)
        y -= h + 7

    def note(title, text):
        global y
        o = p_obj('<b>' + title + '</b><br/>' + text, 9.6)
        _, h = o.wrap(CW - 26, 800)
        c.setFillColor(HexColor(PALE))
        c.rect(M, y - h - 22, CW, h + 22, fill=1, stroke=0)
        c.setFillColor(HexColor(TEAL))
        c.rect(M, y - h - 22, 3, h + 22, fill=1, stroke=0)
        o.drawOn(c, M + 13, y - h - 10)
        y -= h + 34

    def step(n, title, text):
        global y
        o = p_obj('<b>' + title + '</b><br/>' + text, 10)
        _, h = o.wrap(CW - 31, 800)
        c.setFillColor(HexColor(TEAL))
        c.circle(M + 9, y - 9, 9, fill=1, stroke=0)
        c.setFillColor(white)
        c.setFont('Bold', 9)
        c.drawCentredString(M + 9, y - 12, str(n))
        o.drawOn(c, M + 31, y - h)
        y -= max(h, 20) + 13

    def table(headers, rows, widths, size=9.2):
        global y
        cells = [[p_obj(escape(str(v)), size, TEAL, True) for v in headers]]
        for row in rows:
            values = []
            for j, v in enumerate(row):
                text = escape(str(v))
                if page == 2 and j == 1:
                    target = str(v).split('-')[0]
                    text = '<link href="#p' + target + '" color="' + TEAL + '">' + text + '</link>'
                values.append(p_obj(text, size))
            cells.append(values)
        tb = Table(cells, colWidths=[CW * w for w in widths])
        tb.setStyle(TableStyle([('VALIGN', (0, 0), (-1, -1), 'TOP'), ('BACKGROUND', (0, 0), (-1, 0), HexColor(PALE)), ('ROWBACKGROUNDS', (0, 1), (-1, -1), [white, HexColor('#f6f9fa')]), ('LEFTPADDING', (0, 0), (-1, -1), 7), ('RIGHTPADDING', (0, 0), (-1, -1), 7), ('TOPPADDING', (0, 0), (-1, -1), 6), ('BOTTOMPADDING', (0, 0), (-1, -1), 6)]))
        _, h = tb.wrap(CW, 800)
        tb.drawOn(c, M, y - h)
        y -= h + 14

    def image_path(name):
        return a.captures / name if name.startswith('manual-') else a.theme_images / name

    def picture(name, caption='', maxh=390, crop=None, width=None):
        global y
        path = image_path(name)
        im = Image.open(path)
        iw, ih = im.size
        im.close()
        x0, t0, sw, sh = crop or (0, 0, iw, ih)
        scale = min((width or CW) / sw, maxh / sh)
        dw, dh = (sw * scale, sh * scale)
        left = (W - dw) / 2
        c.saveState()
        clip = c.beginPath()
        clip.rect(left, y - dh, dw, dh)
        c.clipPath(clip, stroke=0)
        c.drawImage(str(path), left - x0 * scale, y + (t0 - ih) * scale, iw * scale, ih * scale, mask='auto')
        c.restoreState()
        y -= dh + 7
        if caption:
            o = p_obj(caption, 8.3, MUTED)
            _, h = o.wrap(CW, 800)
            o.drawOn(c, M, y - h)
            y -= h + 9
        else:
            y -= 6

    def regions(labels, boundaries, colours):
        global y
        top = y
        gap = 5
        cell = (CW - 2 * gap) / 3
        height = 58
        for i, (title, body) in enumerate(labels):
            x = M + i * (cell + gap)
            c.setFillColor(HexColor(colours[i]))
            c.roundRect(x, top - height, cell, height, 4, fill=1, stroke=0)
            o = p_obj('<b>' + title + '</b><br/>' + body, 9.1)
            _, h = o.wrap(cell - 18, height)
            o.drawOn(c, x + 9, top - 10 - h)
        c.setStrokeColor(HexColor(MUTED))
        c.setLineWidth(0.6)
        c.line(M, top - height - 18, W - M, top - height - 18)
        for i, label in enumerate(boundaries):
            x = M + (i + 1) * cell + (i + 0.5) * gap
            c.line(x, top - height - 14, x, top - height - 22)
            c.setFont(font, 8)
            c.setFillColor(HexColor(MUTED))
            c.drawCentredString(x, top - height - 35, label)
        c.setFont(font, 8)
        c.drawString(M, top - height - 50, 'Detector level: low to high (active intervals)')
        y = top - height - 65

    def finish():
        if not page:
            return
        assert y >= 53, (lang, page, round(y, 2))
        mins.append(round(y, 1))
        c.setStrokeColor(HexColor('#d8e6e9'))
        c.setLineWidth(0.5)
        c.line(M, 40, W - M, 40)
        c.setFillColor(HexColor(TEAL))
        c.setFont('Body', 8)
        c.drawString(M, 26, 'QING AUDIO')
        c.setFillColor(HexColor(MUTED))
        c.setFont(font, 8)
        c.drawRightString(W - M, 26, 'QQ Super Compression 1.3.2  |  ' + str(page))
        c.showPage()

    def start(title, subtitle=''):
        global page, y
        finish()
        page += 1
        c.bookmarkPage('p' + str(page))
        c.addOutlineEntry(title, 'p' + str(page), 0)
        c.setFillColor(HexColor(MUTED))
        c.setFont(font, 8)
        c.drawRightString(W - M, H - 34, 'QQ Super Compression  ·  User Manual')
        y = H - 59
        o = p_obj(title, 21, TEAL, True)
        _, h = o.wrap(CW, 150)
        o.drawOn(c, M, y - h)
        y -= h + 11
        if subtitle:
            o = p_obj(subtitle, 10, MUTED)
            _, h = o.wrap(CW, 150)
            o.drawOn(c, M, y - h)
            y -= h + 14

    def transfer_plot():
        global y
        x = M + 43
        top = y - 8
        w = CW - 61
        h = 153
        px = lambda v: x + (v + 50) / 50 * w
        py = lambda v: top - h + (v + 50) / 50 * h
        c.setLineWidth(0.5)
        c.setStrokeColor(HexColor('#CBDDE3'))
        for v in [-50, -40, -30, -20, -10, 0]:
            c.line(x, py(v), x + w, py(v))
            c.line(px(v), py(-50), px(v), py(0))
            c.setFillColor(HexColor(MUTED))
            c.setFont('Body', 7.5)
            c.drawRightString(x - 7, py(v) - 2, str(v))
            c.drawCentredString(px(v), py(-50) - 13, str(v))
        c.saveState()
        c.setDash(3, 3)
        c.setStrokeColor(HexColor(MUTED))
        c.line(px(-50), py(-50), px(0), py(0))
        c.restoreState()
        for colour, mode in [(TEAL, 0), ('#ED7C31', 1)]:
            p = c.beginPath()
            for i in range(251):
                level = -50 + i * 0.2
                thr = -24
                r = 4
                if level <= thr:
                    out = level
                elif mode == 0:
                    out = thr + (level - thr) / r
                else:
                    out = level + 20 * math.log10((1 + (r - 1) * 10 ** (thr / 20)) / (1 + (r - 1) * 10 ** (level / 20)))
                if i == 0:
                    p.moveTo(px(level), py(out))
                else:
                    p.lineTo(px(level), py(out))
            c.setStrokeColor(HexColor(colour))
            c.setLineWidth(1.6)
            c.drawPath(p)
        c.setFillColor(HexColor(INK))
        c.setFont(font, 8)
        c.drawString(M, top + 11, 'Output dB')
        c.drawRightString(W - M, py(-50) - 29, 'Detector level dB')
        c.setFont('Bold', 8)
        c.setFillColor(HexColor(TEAL))
        c.drawString(x + 8, py(-7), 'CLASSIC')
        c.setFillColor(HexColor('#ED7C31'))
        c.drawString(x + 86, py(-7), 'SUPER')
        y = top - h - 40
        o = p_obj('Static curves: Threshold=-24 dB, Ratio=4:1, Range=OFF; no compensation, boundary transitions or detector timing.', 8.3, MUTED)
        _, ht = o.wrap(CW, 800)
        o.drawOn(c, M, y - ht)
        y -= ht + 9
    start('QQ Super Compression', 'User Manual  /  1.3.2 Stable · English Edition')
    heading('Control dynamics. Keep the attack natural.')
    para('For vocals, instruments, buses and mastering. The dynamics core has no conventional Attack or Release knobs. Shape the sound with Lookahead, SAFE, Ratio, Threshold and Mix; the Limiter has three separate TP recovery modes.')
    para('Classic uses a fixed compression ratio in dB; Super uses the original QQ curve. Both support Single, upward and Dual compression. One OS setting covers all Lookahead presets; the Limiter adds Hard Clip / TP, recovery modes and 1:1 headphone monitoring.', 9.6)
    picture('manual-light-ST.png', maxh=402)
    note('Start with what you hear', 'Decide how steady the dynamics should be and how much articulation to retain. The Display helps explain changes; it does not replace listening.')
    start('Quick start and navigation', 'Set up your sound first, then explore the advanced controls as needed.')
    for i, (title, text) in enumerate([('Choose a starting point', 'Start with CLASSIC, SINGLE, ST and SC: INT. Use 26 ms Lookahead, SAFE off by default, and Distort 0%.'), ('Adjust Ratio from 1:1', 'Turn right for downward compression or left for upward compression. Leave Range OFF initially, then set Threshold.'), ('Use Mix to set the final amount', 'If processing is too strong, blend the original dynamics back in. See pages 8-11 for upward and Dual compression.'), ('Compare at matched loudness', 'Play representative material, click Match, then fine-tune Makeup / Output Gain.'), ('Use the standard Limiter workflow', 'Set Ratio to 20:1-1000:1, keep Output LINK on, and gradually lower Threshold. Output rises to compensate. See page 25.'), ('Listen in the full mix', 'Switch Bypass or A/B and listen to attacks, tails, density and groove.')], 1):
        step(i, title, text)
    table(['Find a topic', 'Pages'], [['Interface, themes and gestures', '3-4'], ['Classic / Super, Single and upward processing', '5-8'], ['Dual, branch switches and Ratio LINK', '9-11'], ['Input / Output LINK, Match and A/B', '12-14'], ['Display and Lookahead', '15-17'], ['ST / LR / MS and channel LINK', '18-20'], ['Sidechain and mixing examples', '21-24'], ['Limiter, Ceiling, TP and headphone monitoring', '25-31'], ['Parameter reference and troubleshooting', '32-35'], ['Separate mode settings, oversampling and latency', '36-37'], ['TP recovery and output LUFS-I', '38-39'], ['Per-instance FULL / ECO and stop/resume', '40-41'], ['SAFE, Distort / Window and crackles', '42-43']], [0.78, 0.22], 8.5)
    start('Know the interface', 'The same audio controls remain in place across all themes.')
    picture('manual-light-ST.png', 'Light / ST. The values illustrate the interface; they are not a recommended preset.', maxh=361)
    table(['Area', 'Use it to'], [['Display', 'View historical level and gain references under current settings; see page 15.'], ['Two boundary faders', 'Threshold / Range in Single; UP / DOWN thresholds in Dual.'], ['Input / Output / Gain +/-', 'Check input, output and effective dynamic gain.'], ['Bottom knobs and SINGLE / DUAL', 'Adjust gain, Ratio and Mix; switch Single / Dual beside Ratio.'], ['Mode / LINK / Monitor', 'Choose channel processing, link controls or audition domains.'], ['Top buttons', 'Toggle LIMITER and SAFE; select FULL / ECO, sidechain, theme, A/B and Bypass. Algorithm buttons are in the Ratio section at the bottom.']], [0.32, 0.68], 9)
    para('The interface uses a 3:2 landscape aspect ratio. Drag the bottom-right corner to resize it. The plugin remembers the last size when reopened.', 9.5)
    start('Themes and everyday controls')
    para('Click the top-right button labeled LIGHT, DARK or CLASSIC to cycle themes. The last selection is restored when the plugin opens again.')
    picture('manual-theme-button.png', 'The theme button is between SC and A/B.', maxh=52, width=125)
    table(['Theme', 'Appearance'], [['Light', 'Light interface.'], ['Dark', 'Subdued dark interface.'], ['Classic', 'Original classic dark interface.']], [0.25, 0.75])
    para('All three themes change appearance only. The CLASSIC theme and the CLASSIC algorithm in the Ratio section are independent: one changes the colors, the other changes the processing curve.')
    yy = y
    gap = 14
    cell = (CW - gap) / 2
    for n, label, x in [('manual-dark.png', 'Dark', M), ('manual-classic.png', 'Classic', M + cell + gap)]:
        im = Image.open(a.captures / n)
        ww, hh = im.size
        im.close()
        ht = cell * hh / ww
        c.drawImage(str(a.captures / n), x, yy - ht, cell, ht, mask='auto')
        c.setFont('Body', 9)
        c.setFillColor(HexColor(MUTED))
        c.drawString(x, yy - ht - 14, label)
    y = yy - ht - 30
    table(['Adjustment', 'Result'], [['Drag / Shift-drag', 'Normal / fine adjustment; press or release Shift while dragging.'], ['Double-click a value', 'Type an exact value.'], ['Alt + left click', 'Restore the default. When linked, related controls follow within their allowed ranges.'], ['Ctrl/Cmd + Z', 'Undo; add Shift to redo.']], [0.39, 0.61], 9)
    start('Classic and Super: two dynamic curves', 'Select the algorithm in the lower Ratio area. DUAL has separate UP / DOWN choices.')
    table(['Algorithm', 'How it works'], [['Classic', 'A fixed dB ratio above the threshold. For downward compression, divide the excess dB by Ratio to find how much excess remains.'], ['Super', 'The original QQ curve. Ratio sets its strength, but does not describe a fixed dB compression ratio at every detector level.']], [0.18, 0.82], 9.5)
    transfer_plot()
    heading('A worked example')
    para('Classic downward: Threshold=-10 dB, Ratio=5:1, Range=OFF. A steady detector level of 0 dB exceeds the threshold by 10 dB. Compression leaves 2 dB of that excess, giving <b>-8 dB</b>. At -12 dB, the detector stays below threshold and receives no dynamic attenuation.', 9.6, 8)
    para('This example uses internal detection, 0 dB Input, Makeup and Output Gain, and Mix=100%. The formula describes a steady detector level, not a hard limit on individual samples. For changing signals and the Display, see pages 15-16.', 8.8, 8)
    heading('Upward processing still boosts above the lower gate')
    para('In both algorithms, the lower threshold enables boost above it. Classic uses the dB slope corresponding to the reciprocal Down Ratio; Super uses the original upward curve. Both share the same active intervals, Range behavior and Dual handoff rules.', 9.6, 8)
    note('Minimum threshold and remembered choice', 'Classic reaches -90 dB and keeps the same curve at its lowest setting; Super retains -inf. New instances in normal mode recall the last manual algorithm choice. Projects and A/B restore saved choices; Limiter has its own settings. Short fades reduce switching transients.')
    start('Ratio, Input and Mix', 'Three useful controls, not interchangeable settings.')
    picture('manual-light-controls.png', maxh=92)
    heading('Ratio')
    para('Normal Single processing is neutral at 1:1: above it, processing is downward; below it, processing is upward. Within the active threshold range, higher downward ratios usually cut more, while smaller upward ratios usually boost more. Dual separates UP RATIO and DOWN RATIO; see page 9.')
    para('See page 5 for the meaning of Ratio in Classic and Super. Normal Single mode ranges from 1:200 to 200:1, with 1:1 as the default. Final peak protection comes from the Limiter Ceiling; increasing the normal compression Ratio alone does not enable Limiter.', 9.6)
    heading('Input Gain')
    para('Input Gain changes the level entering the processing path, including the Dry path. With INT it also changes the detector level relative to Threshold. With EXT, the external send and Key Gain set the detector level; Input Gain is not a substitute.')
    heading('Mix')
    para('Mix blends Dry with processed Wet. It is also a useful processing amount control: first make the boost or cut clear, then lower Mix to blend back the original dynamics you want to retain.')
    note('Dynamic gain includes Mix', 'At Mix = 0%, effective dynamic gain is 0 dB; at 100%, the full processing amount is retained. Intermediate values are not simply the gain in dB multiplied by Mix. Makeup and Output Gain are excluded from this dynamic gain.')
    para('A practical order: find the compression character with Ratio, set its final strength with Mix, adjust Input Gain when you need a different input level, then compare loudness again.')
    start('Single: Threshold and Range', 'Choose the active levels, then set direction and strength with Ratio.')
    picture('manual-light-single-up.png', 'SINGLE: the lower Threshold and upper Range define the processing interval. The faders align with the dashed lines in the display.', maxh=220)
    table(['Detector level', 'Dynamic processing'], [['At or below Threshold', 'No dynamic boost or attenuation.'], ['Above Threshold and below Range', 'Upward or downward processing, according to Ratio.'], ['At or above a finite Range', 'Returns fully to 0 dB dynamic gain.']], [0.46, 0.54], 9.3)
    heading('Range OFF: no upper cutoff')
    para('At its top OFF position, Range imposes no upper limit on the selected algorithm. Pull it down to set a cutoff. A finite 0 dB setting differs from OFF. Classic Threshold stops at -90 dB; Super retains -inf, meaning no lower gate.', 9.8)
    para('Near a finite Range, dynamic gain returns continuously to 0 dB from inside the active interval, avoiding a sudden jump across the boundary. This transition follows detector level; it adds no Attack time or extra latency.', 9.3)
    note('When the faders meet', 'Range stays at or above Threshold. When one fader reaches the other, it moves the other along; coincident boundaries disable dynamic processing. Drag back to separate them.')
    para("Here, 0 dB dynamic gain means the dynamic branch applies no boost or cut. Input, Makeup, Mix and Output Gain still work, and Limiter's final Ceiling remains independent. With EXT, the external detector signal determines the active range.", 9.3)
    start('Upward: boost above the gate', 'Classic and Super both use this gate rule to bring selected detail forward.')
    para('In SINGLE, move Ratio below 1:1, for example to 1:2, 1:4 or 1:8, to enter upward processing. The displayed 1:8 means 1/8. Smaller values generally give eligible material more boost.')
    regions([('Original level', 'At or below Threshold'), ('Upward boost', 'Between Threshold and Range'), ('Original level', 'At or above a finite Range')], ['Threshold', 'Range'], ['#f3f6f7', '#eaf6f8', '#f3f6f7'])
    note('The gate enables boost; it does not mute audio', 'Threshold acts as a gate that allows boost above it. Below the gate, the original level is retained: quieter sounds are neither muted nor specifically boosted.')
    step(1, 'Select the detail to bring forward', 'Select SINGLE. Set a lower Threshold and upper Range, for example -50 dB / -10 dB. Listen for the detail you want between them. These values are examples only.')
    step(2, 'Then adjust Ratio and Mix', 'Move gradually from 1:1 toward 1:2 and 1:4. If the boost is too strong, reduce its strength or lower Mix. Do not substitute Makeup for dynamic boost.')
    step(3, 'Listen to quiet and loud passages', 'Boost enters smoothly near the lower gate. Beyond that transition, quieter parts in the active interval generally receive more boost. Boost approaches 0 dB near the upper boundary. Range OFF removes the finite upper cutoff.')
    para('To leave very quiet breaths, noise or reverb at their original level, place the lower gate above them. Check that the desired material still enters the boost interval. Selection follows detector level, not sound type.', 9.5)
    start('Dual: lift quieter parts, control louder ones', 'Click the small button beside Ratio to change SINGLE to DUAL.')
    picture('manual-light-dual.png', 'DUAL: UP RATIO and DOWN RATIO control the two directions separately. UP / DOWN thresholds replace Threshold / Range.', maxh=245)
    regions([('Original level', 'At or below UP'), ('Boosted by UP RATIO', 'Above UP, below DOWN'), ('Cut by DOWN RATIO', 'Above DOWN')], ['UP threshold', 'DOWN threshold'], ['#f3f6f7', '#eaf6f8', '#eaf6f8'])
    para('At DOWN, dynamic gain returns to 0 dB. Above it, upward processing stops and only Down Ratio applies. The two gains never stack at the same detector level. Dual has no Range control.', 9.7)
    table(['Starting a new instance', 'How to read it'], [['Normal UP: -90 dB in Classic, -inf in Super. DOWN: 0 dB.', 'The thresholds start at opposite ends; this does not disable upward processing.'], ['Up Ratio = 1:1；Down Ratio = 1:1', 'Both directions are neutral. Adjust a Ratio to hear processing.']], [0.48, 0.52], 9.2)
    note('Threshold order and collision', 'DOWN must stay at or above UP. When one fader reaches the other, it pushes the other along. Dynamic processing stops when the thresholds coincide.')
    start('Dual switches: audition each direction', 'UP / DOWN each have an on/off switch and an algorithm button for separate auditioning and curve selection.')
    picture('manual-light-dual-controls.png', 'Both branches are on. Each algorithm is shown in its Ratio area. The switches preserve Ratio values.', maxh=148, width=245)
    table(['UP / DOWN', 'Processing you hear'], [['ON / ON', 'Boost between UP and DOWN; attenuation above DOWN.'], ['ON / OFF', 'Upward only. Boost still stops at DOWN.'], ['OFF / ON', 'Downward only. No dynamic processing below DOWN.'], ['OFF / OFF', 'No dynamic boost or attenuation. Other gain and Mix settings still apply.']], [0.27, 0.73], 9.5)
    step(1, 'Set up the full Dual sound', 'Set both thresholds and Ratios. Play material with quiet and loud passages.')
    step(2, 'Hear what each direction contributes', 'Turn DOWN off to hear upward compression, then UP off to hear downward compression, then enable both. Each branch can use Classic or Super; changing the UP algorithm does not change DOWN.')
    step(3, 'Check the display and compare at similar loudness', 'The switches update dynamic gain and the Display history reference. Compensate overall level, then compare detail, articulation and consistency.')
    note('Crossfaded switching; boundaries still apply', 'Each branch uses a short crossfade to reduce ON / OFF transients. Switching a branch off neither moves the thresholds nor extends the other branch into a different interval. In LR / MS, each channel has its own switches.')
    start('Ratio LINK: opposite proportional changes', 'The small LINK above and between the two Ratios appears only in DUAL.')
    picture('manual-light-dual-controls.png', 'Up and Down retain their existing relative relationship. Enabling LINK does not make values jump.', maxh=100)
    para('Ratio LINK defaults to on at first use, then remembers your last choice. When reopening a saved project, the LINK state saved in that project takes priority.')
    table(['Initial Down / Up', 'Adjust Down', 'Linked Up'], [['1:1 / 1:1', '1:1 → 2:1', '1:2'], ['4:1 / 1:2', '4:1 → 8:1', '1:4'], ['8:1 / 1:4', '8:1 → 4:1', '1:2']], [0.36, 0.32, 0.32], 9.6)
    heading('Preserve the relationship, without re-pairing')
    para('For example, doubling Down halves the Up value; adjusting Up uses the same inverse relationship. Starting at 4:1 and 1:2 will not force them into a reciprocal pair. Dragging, fine adjustment and direct entry all follow this rule.')
    note('At a limit, both stop together', 'Normal Up ranges from 1:200 to 1:1 and Down from 1:1 to 200:1. Limiter Up ranges from 1:8 to 1:1 and Down from 1:1 to 1000:1. If either would exceed its range, both stop at the shared limit. Reverse direction to continue. Toggling LINK alone does not change either Ratio.')
    heading('To hear upward processing alone')
    para('Switch DOWN off to preserve its Ratio while auditioning Up. To adjust Up Ratio independently as well, turn off the small Ratio LINK; otherwise the Down value still follows your adjustment.')
    heading('Also available in LR / MS')
    para('The small LINK scales down with compact layouts and stays between Up / Down Ratio. It links those Ratios within each channel. Channel LINK beside Mode links L/R or M/S; see page 20.')
    start('Input / Output LINK: opposite gain changes', 'Above and to the right of INPUT GAIN. It is separate from Ratio LINK.')
    picture('manual-input-output-link.png', 'Example: Input +3 dB, Output -3 dB, with LINK enabled.', maxh=105)
    para('With LINK on, raising Input Gain in the interface lowers Output Gain by the same number of dB. Adjusting Output moves Input in the opposite direction too. This helps reduce simple level changes while you explore a different level entering the processor.')
    table(['Initial Input / Output', 'Adjustment', 'Result'], [['0 / 0 dB', 'Raise Input by 3 dB', '+3 / -3 dB'], ['+2 / -1 dB', 'Raise Input by 3 dB', '+5 / -4 dB'], ['+5 / -4 dB', 'Raise Output by 2 dB', '+3 / -2 dB']], [0.35, 0.33, 0.32], 9.5)
    heading('Keep an existing offset')
    para('The link follows relative changes; it does not force the two values to be exact opposites. Toggling LINK alone leaves the gains unchanged. If either control reaches its allowed limit, both stop together.')
    heading('Remember the last choice')
    para('LINK is on initially. New instances then use your last choice; a saved project restores its own state. Projects and A/B save the current LINK setting. Check the link switches after recalling a snapshot before making further adjustments.')
    note('Limiter uses a different link', 'Normal Input / Output LINK moves the two gains in opposite directions; it does not replace Match. It is hidden and inactive in Limiter. The LINK beside Output links the downward threshold and output in opposite directions; see page 25.')
    start('Match and Makeup: compare at equal loudness')
    table(['Control', 'Use'], [['Makeup', 'Adjusts Wet level before Mix: normal mode ±30 dB; Limiter ±120 dB.'], ['Output Gain', 'Normal: -24 to +24 dB; Limiter: -120 to +120 dB. See pages 29-30 for headphone listening.'], ['Match', 'Uses played material for a one-time loudness adjustment to Makeup. It is not continuous auto gain.']], [0.24, 0.76], 9.6)
    step(1, 'Play a representative passage', 'Allow a valid loudness reading to build before clicking Match. Use material that represents the passage, not just a very brief attack.')
    step(2, 'Compare in the current listening mode', 'Normal mode compares Dry with Wet before compensation. Limiter headphone mode compares original input with the actual limited listening output. Makeup changes Wet; see page 30.')
    step(3, 'Trim the final level', 'Use Makeup or Output Gain for any small correction. Compare with Bypass or A/B and listen to attacks, tails and consistency across the passage.')
    heading('Deep compression and very low levels')
    para('Deep compression no longer prevents matching simply because Wet falls below a fixed loudness gate. The larger Limiter Makeup range allows substantial compensation. Match still needs valid, nonzero measurements; total silence or too little material cannot provide a reliable result.')
    note('Judge loudness and peak protection separately', "Match helps compare loudness; Limiter's Ceiling controls final peaks. Check output and TP after matching, especially after upward processing or further parameter changes.")
    heading('Three distinctions to keep in mind')
    para('Dynamic gain excludes Makeup and Output Gain. Input / Output LINK only makes equal and opposite gain changes; it cannot replace loudness matching. At Mix = 0%, Input and Output Gain still apply, so this is not full Bypass.')
    start('A/B: compare algorithms and directions')
    picture('manual-top-controls.png', 'A and B hold two sound settings. A→B and B→A copy between them.', maxh=62)
    step(1, 'Save the first idea', 'Set the algorithm and processing amount in A. Copy A→B, then switch to B. Copying replaces the destination sound settings.')
    step(2, 'Change the part you want to compare', 'For example, use Classic in A and Super in B, or compare downward and upward processing. Keep Lookahead, detector source and monitoring the same where possible.')
    step(3, 'Match each, then switch', 'Play the same material for each setting and use Match; trim Makeup if needed. Click A / B and compare the sound, without mistaking louder for better.')
    table(['Recalled by A/B', 'Outside the A/B sound snapshot'], [['Algorithms, Single / Dual, thresholds, ratios, branch switches, gains, Mix, Lookahead, SAFE, Distort / Window, overall OS, sidechain, Limiter, TP, Ceiling, TP recovery, LINK states and both sets of processing parameters.', 'Theme, FULL / ECO, 1:1 headphone monitoring, channel solo, Bypass and SC LISTEN.']], [0.6, 0.4], 9.3)
    heading('Why can reciprocal Ratios sound the same?')
    para('The upward and downward curves retain a symmetrical relationship. In a shared active interval, some reciprocal settings differ only by a constant gain, so they can sound very close after loudness matching. One example is 8:1 versus 1:8 in Super / SINGLE with Threshold = -inf, Range = OFF and Mix = 100%.')
    para('Classic also has a corresponding relationship within a shared active interval, away from the lower gate transition. Crossing a threshold, entering a finite Range transition, changing Mix or changing the Dual handoff can break equality across a whole passage.', 9.5)
    note('Switching and latency', 'A/B uses a transition to reduce switching artifacts. Matching Lookahead and oversampling makes sound comparisons easier; changing modes or rates may require the host to update delay compensation. A/B recalls each saved window value, and the Display recalculates with the corresponding timing alignment.')
    start('Read the Display and meters', 'History is recalculated with the parameters; live meters show the current processing.')
    picture('manual-light-dual.png', 'Gray: input reference; blue: Cut / Mix; green: Boost / Mix; orange: output reference.', maxh=195, crop=(32, 152, 2336, 1072))
    table(['What you see', 'Meaning'], [['Dry / Input', 'Historical level reference before Input Gain. Adjusting Input does not shift the whole gray trace.'], ['Cut / Mix and Boost / Mix', 'Dynamic references recalculated from the current algorithm, detection and Mix. In Limiter, the blue trace also includes an estimate of current Ceiling attenuation.'], ['Output', 'Projected output level at the current settings, including gains, Mix and the current Hard Clip / TP Ceiling.'], ['External key', 'A faint sidechain reference appears when an external channel is available, helping identify the trigger source.'], ['Right-hand GAIN +/-', 'Live gain changes: positive for boost, negative for attenuation. Hold retains recent prominent changes.']], [0.25, 0.75], 9.1)
    note('Eight seconds of history follow your settings', 'Changing Ratio, thresholds, Mix, gains, Limiter or TP reprojects existing history using the current settings. HPF refreshes after the control is released or settles; HPF UPDATING indicates work in progress. History advances in audio time. Lookahead and A/B changes retain the delay associated with captured audio, preventing a horizontal offset.')
    para('Display is neither a recording of the final waveform nor a sample-by-sample true-peak measurement. INT / EXT detection can give the blue and gray traces different references, so their spacing alone does not reveal actual GR for every sample. Read the TP meter for final output peaks and the right-hand meters for live dynamics.', 9.4)
    para('At Ratio 1:1 with all gains at 0 dB, internal full-band detection and no Ceiling or other additional processing, input and output history should overlap. At 0 ms both detectors are identical; toggling SAFE does not create a different dynamics curve.', 9.3)
    start('Lookahead and SAFE: choose a detector', 'Lookahead sets how far ahead you can see; the window sets how much signal you use.')
    para('Lookahead lets the processor inspect a short stretch of future signal before outputting the current sound. The window moves sample by sample, and gain is recalculated for every sample, not once per Lookahead interval. It does not use conventional Attack / Release smoothing for dynamics gain.')
    picture('manual-safe-button.png', 'SAFE sits to the right of LIMITER. Lit yellow means Safe; unlit means Normal. New instances default to OFF.', maxh=42, width=105)
    table(['Detector', 'How past/future peaks are used'], [['Normal / SAFE off', "Take the peak of each past/future window, then use the smaller one. Typically retains more local dynamics and reduces a loud event's advance influence on nearby quiet audio."], ['Safe / SAFE lit', 'Use the larger of the two peaks. A longer window stabilizes the detector level and helps suppress crackles caused by rapid gain changes.']], [0.25, 0.75], 9.6)
    note('The cost of Safe: pre- and post-attenuation', 'An upcoming loud event can attenuate nearby quiet audio before it arrives. After it ends, the past window may still contain that peak and attenuate the following quiet audio. Dips can therefore appear around level changes. They follow the explicit Window setting; no hidden Attack / Release stage is added.')
    heading('Same window length, different tradeoffs')
    para("In Normal, a quieter window on one side can avoid using the larger peak on the other. Complex layers, delay or reverb may nevertheless cause faster gain changes. Safe tends to stabilize detection while extending a loud event's influence over nearby audio. Classic / Super determines a separate choice: which dynamics curve to apply to the detected level.", 9.6)
    heading('Choose Lookahead, window and OS together')
    para('Lookahead defaults to 26 ms. Normal uses Distort to shorten the window; Safe uses Window in milliseconds. See page 42. Shorter windows can increase harmonics or modulation coloration, while overall OS can reduce aliasing. None of these settings guarantees zero distortion for every signal.', 9.6)
    para('SAFE and Distort / Window are saved with the project and A/B. At 0 ms Lookahead, or with a zero-length window, both detectors are identical; lighting SAFE then provides no additional window protection.', 9.5)
    start('Lookahead and the 0 ms option', 'Choose 0, 26, 40, 80 or 100 ms. The separate 10 ms preset has been removed.')
    para('Lookahead sets the base audio delay and the maximum detector-window length. Distort / Window sets the actual window; shortening it does not shorten the selected Lookahead delay. The host normally compensates for the total latency reported by the plug-in.')
    picture('manual-light-0ms.png', 'At 0 ms the window is zero and cannot be edited. Overall OS still offers 1x / 4x / 8x / 16x below it.', maxh=248)
    table(['Choice', 'Use and trade-off'], [['26 / 40 ms', 'Default: 26 ms. Use Distort / Window to fine-tune the detection range.'], ['80 / 100 ms', 'Allows longer windows; check stability, Safe pre-attenuation and extra latency.'], ['0 ms', 'Zero-window detection, with more local dynamics and coloration. SAFE and Normal are identical.']], [0.25, 0.75], 9.3)
    para('All Lookahead presets use the same overall OS control. Oversampling filters and Limiter Ceiling protection may add latency, so 0 ms Lookahead does not mean zero total latency in every configuration. See page 37.', 9.5)
    note("The Limiter's defining feature: 0 ms Lookahead", "Make the chorus loud while retaining the level of quiet sections, without raising the entire song as much. Compare overall OS at 4x / 8x / 16x while listening to loud and quiet sections. This is a defining feature of Super Compression's Limiter and is worth trying; see page 31.")
    para('Old sessions set to 10 ms migrate to 26 ms. For a 10 ms detection range, use Safe at 26 ms Lookahead and set Window to 10 ms. Changing Lookahead manually resets Normal to Distort 0% and Safe to the full Window.', 9.2)
    start('ST and LR: together or independently')
    para('Click Mode at the lower right to cycle ST → MS → LR. Start with ST; choose LR when the two sides genuinely need different treatment.')
    picture('manual-light-LR.png', 'LR: L above, R below, with independent controls.', maxh=373)
    table(['Mode', 'How to use it'], [['ST', 'Uses common compression gain for stereo; a useful starting point for tracks and buses.'], ['LR', 'Left and right have independent Ratio, boundary, Makeup and Mix controls. In DUAL, each side has its own UP / DOWN. Check for image shifts when processing asymmetric material.']], [0.17, 0.83], 9.4)
    note('MONITOR', 'Choose L or R to audition that side centered. Return to ALL to check normal stereo, and do not leave one-sided audition active for the final output.')
    start('MS: control center and sides')
    picture('manual-light-dual-MS.png', 'MS / DUAL: Mid is above Side. Each channel has its own UP / DOWN Ratios and thresholds.', maxh=340)
    para('Mid mainly represents content common to both sides; Side represents their differences. They are not a precise separation of a center track from every other track.')
    step(1, 'Listen to M and S first', 'Use M / S under MONITOR, then return to ALL.')
    step(2, 'Adjust each for a reason', 'For steadier center dynamics, start with M Ratio / Mix. Treat S cautiously when preserving ambience and width.')
    note('Recheck the full mix', 'Compression or level changes in M/S can alter the sense of space. In ALL, check center, width and mono compatibility, not only soloed domains.')
    start('Channel LINK: adjust together, keep differences')
    para('In LR or MS, LINK beside Mode links matching parameters across channels. Set the two sides separately, then enable it to adjust them together without forcing identical values.')
    picture('manual-light-LR-link.png', 'SINGLE example: channel LINK beside Mode is on, while the left and right Ratios remain at 3:1 and 5:1.', maxh=110)
    table(['Parameter', 'Before', 'After an equal change'], [['Ratio', '3:1 / 5:1', '4:1 / 6:1'], ['Threshold', '-20 / -10 dB', '-18 / -8 dB'], ['Makeup', '-3 / +1 dB', '-2 / +2 dB'], ['Mix', '100% / 70%', '90% / 60%']], [0.26, 0.36, 0.38])
    step(1, 'Set the relationship with LINK off', 'Decide which domain needs more compression or a different blend.')
    step(2, 'Enable LINK for joint adjustments', 'Move either parameter in a pair; the other follows by the same change. Fine adjustment and direct entry work too.')
    note('Both stop at a limit', 'If either side would exceed its range, both stop where the difference can be retained. In Super, one Threshold at -inf cannot form a finite dB offset; set both thresholds to finite values first if you want that relationship.')
    heading('Three LINK controls, three roles')
    para('The small LINK between the Ratios links Up / Down within each channel. LINK beside Mode links matching L/R or M/S parameters. With both enabled, channels move together while each channel retains its existing Up / Down relationship. LINK beside Input only controls opposite input/output gain changes; see page 12.')
    start('Sidechain: start with the panel', 'Click SC: INT / SC: EXT at the top right. Its label shows the current source.')
    picture('manual-sidechain-panel.png', maxh=185, width=415)
    table(['Control', 'How to use it'], [['INT / EXT', 'INT follows the current track itself. EXT uses a signal sent from another track to control boost or attenuation on the current track.'], ['KEY GAIN', 'Available only with EXT. Adjusts the external detector level and trigger strength, not the main track level.'], ['HPF', 'Reduces low frequencies in the INT or EXT detector key. OFF is full-band; active range is 20-500 Hz.'], ['KEY LEVEL', 'Check that the detector receives a signal. With EXT showing N/A, first check that the host sidechain bus is enabled.'], ['SC LISTEN', 'Temporarily audition the detector key to check routing and HPF. Switch it off before judging normal output.']], [0.24, 0.76], 9.4)
    note('Two common tasks', 'Low notes pull the whole part down: try INT + HPF, page 22.<br/>Want bass to duck when the kick plays? Use EXT, page 23.')
    start('Internal sidechain: reduce bass dominance')
    para('Use this when kick or bass repeatedly pulls down the rest of a drum bus, mix bus or master. The goal is less low-frequency triggering, not less bass in the audible signal.')
    for i, (tt, body) in enumerate([('Start with INT and hear the problem', 'Loop a passage with bass and other content together. Leave HPF OFF to establish a comparison.'), ('Open the sidechain panel', 'Click SC: INT and keep SOURCE on INT. A disabled Key Gain is normal here.'), ('Raise HPF gradually', 'Start at a low cutoff and listen for less whole-mix ducking on bass events. Make small changes.'), ('Audition the key if needed', 'Use SC LISTEN to hear which bass content HPF removes, then turn it off and return to the full sound.'), ('Inspect history after release', 'HPF history refreshes after you release the knob. If HPF UPDATING appears, wait for it to clear before comparing.'), ('Rebalance the compression amount', 'Filtering may reduce GR or change its shape. Revisit Ratio, Threshold or Mix as needed, then compare at similar loudness.')], 1):
        step(i, tt, body)
    note('It is normal not to hear bass being cut', 'HPF filters only the key used to decide compression, not normal output. It is not equivalent to Key Gain: Key Gain scales the detector level; HPF changes frequency weighting. Too high a cutoff can make the compressor ignore bass that still needs control.')
    start('External sidechain: kick-controlled bass')
    para('Insert the plugin on the bass - the track to turn down. Send the kick to its sidechain as the trigger. Do not reverse the tracks or confuse a normal audio send with a sidechain send.')
    x = M
    boxw = (CW - 38) / 3
    bh = 47
    labels = ['KICK TRACK', 'BASS PLUG-IN KEY', 'BASS IS REDUCED']
    for i, label in enumerate(labels):
        xx = x + i * (boxw + 19)
        c.setFillColor(HexColor(PALE))
        c.roundRect(xx, y - bh, boxw, bh, 5, fill=1, stroke=0)
        ob = p_obj(label, 9, TEAL, True)
        _, hh = ob.wrap(boxw - 16, bh)
        ob.drawOn(c, xx + 8, y - (bh + hh) / 2)
        if i < 2:
            c.setFillColor(HexColor(TEAL))
            c.setFont('Body', 13)
            c.drawString(xx + boxw + 4, y - 29, '→')
    y -= bh + 20
    for i, (tt, body) in enumerate([('Enable the host sidechain input', 'Enable sidechain input for this plugin instance on the bass. Add and enable a kick send to that sidechain. Names and locations vary by host.'), ('Select EXT in the plugin', 'Open SC, choose EXT and play the kick. Check KEY LEVEL first; if it has no reading, fix routing before increasing Ratio.'), ('Confirm the actual trigger', 'Briefly enable SC LISTEN: you should hear the kick, not the bass. Turn audition off afterward.'), ('Set trigger level and reduction', 'For downward ducking, start with SINGLE, Ratio above 1:1 and Range OFF. Set detector strength with Key Gain or the send level, and set the lower boundary with Threshold. Leave HPF OFF initially.'), ('Use Mix to decide how much room to make', 'Find an audible duck, then reduce Mix to retain the desired bass sustain. Listen to both tracks together, not only solo bass.'), ('Decide whether HPF helps', 'Raising HPF reduces low-frequency kick triggering and may weaken the duck you want. Use it when you want more emphasis on the attack or less unwanted low-bass triggering.')], 1):
        step(i, tt, body)
    para('There are no conventional Attack / Release controls. Ducking depends on the trigger signal, Lookahead, SAFE, window and compression settings; it is not necessarily smoother than conventional sidechain compression on every source.', 9.3)
    start('Apply it in a real mix')
    heading('Vocals, guitar, piano: consistency with articulation')
    para('Try SINGLE / ST / INT and 26 ms. Raise Ratio above 1:1, then use Mix to keep consonants, plucks or hammer attacks natural. To process only louder parts, raise Threshold and leave Range OFF. Use Match and compare in the mix.')
    para('If clean guitar or vocals containing delay / reverb produce continuous crackles, enable SAFE with the full Window, then shorten it gradually and compare. See page 43 for the procedure.', 9.3, 8)
    heading('Tails and quiet phrases: bring detail forward')
    para('Try Single upward processing to lift quieter words, pluck tails or reverb. Use Threshold to exclude unwanted very quiet sounds and Range to set the upper limit. To also control louder parts, switch to Dual and adjust Up and Down separately. Use the branch ON / OFF switches to audition each direction. Turn Ratio LINK off if you also want to adjust each Ratio independently.')
    heading('Mix bus / mastering: a more cohesive balance')
    para('When you want a steadier, more cohesive mix without deliberately reshaping drum attacks through conventional Attack / Release, try it as an alternative to G Bus-style compression. Start gently, compare balance across sections and lower Mix when needed.')
    para('If every kick pulls the whole mix down, try INT + HPF. Do not raise the cutoff merely to make the GR number smaller. At similar loudness, check drum attack, low end, space and the song’s larger dynamic movement.')
    heading('Vocal-keyed accompaniment: make space for words')
    para('Insert the plugin on the backing bus, send the vocal to its external sidechain and choose EXT. Confirm the source with SC LISTEN. Select SINGLE, Ratio above 1:1 and Range OFF, then adjust Key Gain, Threshold and Mix so the backing steps back while the vocal plays.')
    para('If plosives trigger too much ducking, try raising sidechain HPF. If breaths or noise also make the accompaniment dip, check the sent signal and Threshold; HPF alone will not solve every unwanted trigger.')
    note('For every comparison', 'Turn off SC LISTEN, return channel MONITOR to ALL, and bring the before/after loudness close. For final peak control, enable Limiter and set Ceiling. Combine the headphone button with Match for level-matched listening; see pages 25-31.')
    start('Limiter: high ratios and Output LINK', 'For most material, use a Ratio between 20:1 and 1000:1.')
    picture('manual-limiter-light.png', 'Example: 100:1, Threshold -18 dB, Output +18 dB, LINK on. Adjust the amount to suit your material.', maxh=224)
    step(1, 'Enable Limiter and set the downward Ratio', 'Start in SINGLE with Ratio at 20:1 or higher, up to 1000:1. Leave Range OFF and Mix at 100% initially. Set the required Ceiling and TP.')
    step(2, 'Keep Output LINK on', 'Use the LINK beside Output. As Threshold moves down, Output rises by the same number of dB to compensate.')
    step(3, 'Gradually lower Threshold', 'Drag down slowly during playback, listening to compression, density and attacks. For example, starting at Threshold 0 dB / Output 0 dB, lowering Threshold to -3 dB raises Output automatically to +3 dB.')
    step(4, 'Finish downward compression before adding more', 'Judge the amount by ear and by the final peaks. Then consider upward compensation, Mix or a matched-loudness headphone comparison. Start with this downward-compression workflow before using Mix.')
    note('Check LINK compensation and final peaks together', 'Output follows Threshold automatically; Ceiling provides final peak protection. LINK is equal-and-opposite dB movement, not a guarantee of constant perceived loudness. Compare with headphones + Match + Bypass; see pages 29-30.')
    note('Once downward compression is set, try 0 ms', "To make the chorus loud while retaining quiet sections, try the Limiter's 0 ms mode with overall OS at 4x / 8x / 16x. This is a key feature of Super Compression's Limiter; see page 31 for the listening procedure.")
    start('Limiter Dual: add gentle upward compression', 'Use Dual to add compensation after setting the standard downward compression.')
    picture('manual-limiter-dual-gentle.png', 'Example: DOWN stays at 100:1; UP is 1:1.2. Output LINK is on; Ratio LINK between UP / DOWN is off.', maxh=205)
    step(1, "Complete the previous page's downward setup", 'Use a Ratio of 20:1-1000:1, Threshold and Output LINK to set the desired Limiter compression. Note the algorithm and Ratio.')
    step(2, 'Switch to DUAL and retain the downward setup', 'Single Threshold carries over to the DOWN threshold. DOWN RATIO and Single Ratio are stored separately: check the DOWN algorithm and Ratio, and set them to your Single values if needed to retain the compression you established.')
    step(3, 'Add only a little UP', 'Keep Output LINK on and turn Ratio LINK between UP / DOWN off to avoid changing the downward Ratio. With UP on, move gently from 1:1 toward 1:1.2; this small range is usually enough. Use the UP threshold to select what gets lifted.')
    para('The UP threshold is independent of Output Gain and Makeup. Keeping Output LINK on will not pull UP along. The minimum is -inf for Super upward compression and -90 dB for Classic.', 9.2, 8)
    note('Listen closely to kick and snare sustain', 'Upward compression can raise kick and snare tails, making them more prominent. Listen beyond attacks and loudness: check whether sustain fills the spaces between hits or makes the rhythm feel sluggish.')
    heading('If Dual does not work well, try segmented compression')
    para('Use gentle upward compression to add detail to the established downward setup. Keep the original DOWN RATIO, then fine-tune if needed. If adding UP does not improve the result, reduce or disable it and try segmented compression to control dynamics; this can work better. Decide whether to use Mix after settling these choices.', 9.6)
    start('Ceiling and TP: choose the final limit')
    picture('manual-limiter-light.png', 'Ceiling, recovery and TP sit below Output. The unit is dBTP with TP enabled.', maxh=174, width=160, crop=(1700, 1230, 336, 356))
    table(['Setting', 'Meaning'], [['Ceiling', '-24 to 0 dB; default 0. Sets the reference upper limit for final peak protection.'], ['TP on', 'Controls true peaks with reconstruction detection and gain recovery. All four overall OS rates are available.'], ['TP off', 'Hard Clip: native-rate clipping at 1x, or clipping at the selected 4x / 8x / 16x audio rate.'], ['Overall OS', 'Dynamics and Ceiling share one audio rate; there is no separate Ceiling OS setting.']], [0.25, 0.75], 9.5)
    para('TP retains a small reconstruction margin, so readings need not sit exactly on the line. Hard Clip has different recovery and distortion behavior. With TP off, inter-sample peaks can still exceed the dBFS ceiling; check the actual TP reading.', 9.6)
    heading('Editing the value')
    para('Double-click Ceiling to type a value; drag vertically to adjust, use Shift for fine control or Alt-click to reset to 0. Undo / redo is supported. Recovery modes affect TP only; see page 38.', 9.5)
    note('TP is available at 1x', 'At overall OS 1x, audio stays at its native sample rate while TP uses an independent reconstruction detector. The audio is not silently switched to 8x. Final peak protection and latency still follow the active path; see page 37.')
    start('TP meter: measure output and hold peaks')
    picture('manual-limiter-tp-meter.png', 'The TP L/R readout measures true peaks in the final left/right stereo output.', maxh=151, width=280)
    heading('Measured after Ceiling')
    para('The TP meter reads actual output, including Makeup, Mix, Output Gain, Ceiling and headphone 1:1 compensation. It does not simply display the Ceiling setting, or clamp its reading to that setting.')
    heading('Hold and reset')
    step(1, 'Play the passage to check', 'The peak reading holds for about 20 seconds. A new higher peak updates the value and restarts the timer. Hold advances with audio playback time.')
    step(2, 'Start a fresh measurement', 'Double-click the TP readout to clear it immediately, then play the passage again. There is no need to wait for the previous held peak to expire.')
    step(3, 'Measure again after changing Ceiling', 'A held peak may come from earlier settings. Clear it before deciding whether the current limit behaves as expected.')
    table(['What you see', 'How it works'], [['TP exceeds Ceiling with TP off', 'The meter still measures true peaks. Sample-peak protection does not also guarantee the intersample peak limit.'], ['TP changes with headphones on', 'The readout includes listening compensation. Judge it against the converted MON limit.'], ['MS domain meters differ from TP', 'TP measures final L/R. Individual M/S levels and left/right output limits are different quantities.']], [0.38, 0.62], 9.2)
    start('Headphone 1:1: keep limiting, offset gain', 'The small headphone button left of Output Gain appears only in Limiter.')
    picture('manual-limiter-monitor-controls.png', 'With headphones on, Output values and links stay active. MON shows the listening limit below.', maxh=103)
    para("With headphones on, audio still passes through the complete dynamics and Ceiling stages. The visible Output Gain is then cancelled. Limiting's dynamic and waveform changes remain; only the listening level is adjusted for comparison.")
    heading('The Output value still matters')
    para("Output remains adjustable. Links between the downward threshold, Makeup and Output remain active; the Dual UP threshold stays independent. The headphone function cancels only Output's direct level change, so changes to dynamics remain audible. Switch it off to restore normal Output Gain.")
    heading('Ceiling follows the same listening reference')
    note('MON = Ceiling - Output Gain', 'For example, Ceiling = -1 dB and Output = +6 dB give a listening limit of -7 dB with headphones on. The Ceiling setting stays at -1 dB. Switching headphones off restores the normal output reference. TP measures the actual output.')
    para('Headphones default to off, are saved with the project, and remain independent of A/B. Keep the same listening mode while comparing A/B. Headphone and Bypass switches use smooth transitions; headphones add no separate audio delay.', 9.6)
    note('1:1 does not automatically mean equal loudness', 'Headphones cancel Output Gain; they do not automatically compensate all loudness changes from compression, Ceiling or Input. Use Match for a level-matched comparison. The next page gives the full procedure.')
    start('Limiter comparison: headphone + Match + Bypass')
    step(1, 'Compare after setting up the Limiter', 'Complete the downward setup on page 25, or Dual compensation on page 26, then enable headphones. Retain Ceiling and TP, turn SC LISTEN off and return MONITOR to ALL.')
    step(2, 'Play the same representative passage', 'Allow loudness measurements to build. Parameter changes start fresh measurement collection. Do not rush to Match while adjusting continuously, during silence or immediately after starting playback.')
    step(3, 'Click Match to compensate Makeup', 'Headphone mode compares original input with actual output after Ceiling and headphone compensation. LR / MS use a common correction, preserving relative channel balance. The dry/wet blend is included.')
    step(4, 'Compare the original with plugin Bypass', 'Bypass returns delay-aligned original input without limiting or headphone gain compensation. Listen for attack, tails, density and groove; trim Makeup if needed.')
    step(5, 'Finish listening and restore normal output', 'Switch headphones off and keep the adjusted settings. Check TP and Ceiling again before judging the final signal sent downstream.')
    heading('Match applies one correction, not continuous gain riding')
    para('After changing the passage, limiting depth or other parameters, play again and Match. With Limiter LINK off, controls at their limits, or Mix fully dry, the compensation available through Makeup may be restricted.', 9.5)
    note('When comparing A/B', 'Play the same material for A and B, then Match each. Headphone state is independent of A/B; Limiter / TP / Ceiling, rates and recovery are recalled with A/B. Keep Lookahead and rates alike where possible to reduce latency changes.')
    start('Limiter: the 0 ms feature and practical tips')
    note('0 ms Lookahead: a loud chorus, quiet sections retained', "This is QQ Super Compression Limiter's standout feature and is well worth trying. In Limiter mode, 0 ms Lookahead can make the chorus very loud while retaining the level of quiet sections, without raising the whole song by a large amount. Loud passages become louder while quiet passages keep their sense of quiet, preserving a stronger dynamic contrast between sections.")
    heading('Try 4x / 8x / 16x overall oversampling')
    para('First set up high-ratio downward compression as on page 25, keep Output LINK enabled, then select 0 ms Lookahead. Compare overall OS at 4x, 8x and 16x across loud and quiet sections for level, dynamics and tone. One rate covers dynamics and Ceiling; see page 37. At 0 ms no detector window is available, so SAFE and Normal give the same result.', 9.6)
    heading('Listen to recovery after peaks')
    para('With TP on, TIGHT recovers faster, AUTO adapts to the signal, and SMOOTH recovers more gradually. AUTO is the default. All retain the same protection setting and latency, but dynamics and sound can differ; see page 38.')
    heading('Host latency reflects the entire active path')
    para('The Lookahead setting of 26 ms need not equal host-reported total latency. Normal mode adds no Limiter Ceiling latency. Entering Limiter or changing oversampling can change that total. Even 0 ms does not guarantee zero total latency in every configuration; see page 37.')
    table(['Symptom', 'First check'], [['Ceiling / TP / headphones are missing', 'Enable LIMITER at the top; these controls appear only in that mode.'], ['Output also moves the threshold', 'Output LINK is on; it inversely links the downward threshold. The Dual UP threshold does not follow.'], ['No direct Output gain heard with headphones on', 'That is the listening compensation. Values and links remain active; switch headphones off for normal output.'], ['Limiting remains at Mix 0%', 'The final Ceiling remains after the mix. Use Bypass to compare with the complete original signal.'], ['The TP peak seems unchanged', 'Double-click the TP reading to reset, then play again. ECO pauses analysis while the editor is closed.'], ['MATCH is still unavailable', 'Play a non-silent passage, stop continuous adjustments and let the current settings accumulate valid measurements.'], ['ASIO-Guard remains high after stopping', 'Check ECO selection and host Stop status, then other load and host scheduling. See page 41.']], [0.34, 0.66], 9.3)
    note('Confirm which signal you are hearing', 'Turn SC LISTEN off and return channel MONITOR to ALL. After matched-loudness headphone listening, turn headphones off and check the actual output sent downstream. LUFS-I also follows the current monitoring result; see page 39.')
    start('Quick reference: algorithms and dynamics')
    table(['Parameter', 'Range / choices', 'Notes'], [['Algorithm', 'Classic / Super', 'One choice in Single; separate UP / DOWN choices in Dual.'], ['SINGLE / DUAL', 'Single / Dual', 'Limiter preserves downward-threshold continuity when switching; see page 36.'], ['Single Ratio', 'Normal 1:200 to 200:1; Limiter 1:8 to 1000:1', 'Default / reset: 1:1. Standard Limiter use: 20:1 to 1000:1.'], ['Up Ratio', 'Normal 1:200 to 1:1; Limiter 1:8 to 1:1', 'Default 1:1. Start gentle Limiter upward compensation between 1:1 and 1:1.2.'], ['Down Ratio', 'Normal 1:1 to 200:1; Limiter 1:1 to 1000:1', 'Default 1:1. Check the established downward Ratio when entering Limiter Dual.'], ['UP / DOWN switches', 'ON / OFF', 'Both ON by default; audition independently while retaining Ratios.'], ['Threshold / UP THR', 'Classic -90 to 0 dB; Super -inf to 0 dB', 'The branch algorithm determines the lowest setting.'], ['Range', 'OFF or a set upper boundary', 'Single defaults to OFF; the algorithm limits finite upper bounds.'], ['DOWN THR', 'Classic -90 to 0 dB; Super -inf to 0 dB', 'Dual crossover threshold between upward and downward processing.'], ['Ratio LINK', 'OFF / ON', 'Links UP / DOWN by inverse factors; initially ON.'], ['Mode / channel LINK', 'ST / MS / LR；OFF / ON', 'Links corresponding parameters in LR / MS.']], [0.23, 0.36, 0.41], 9.1)
    note('Boundaries and defaults', 'Range cannot fall below Threshold, nor DOWN below UP. Colliding boundaries move together; coincident boundaries stop dynamics processing. A new Limiter starts at Single Threshold / Dual DOWN 0 dB and Ratio 1:1. Existing projects restore saved settings.')
    para("Super's lowest finite threshold is -119.99 dB, followed by -inf; the Single lower limit may read OFF. Display extends down to -90 dB. Range OFF differs from a finite 0 dB upper bound.", 9.3)
    start('Quick reference: gain, monitoring and SC')
    table(['Parameter', 'Range / choices', 'Notes'], [['Input Gain', 'Normal / Limiter: ±24 dB', 'Each mode retains its own input setting.'], ['Makeup', 'Normal ±30 dB; Limiter ±120 dB', 'Default 0; between Wet and Mix.'], ['Mix', '0 to 100%', 'Default 100%; blends the dynamics processing.'], ['Output Gain', 'Normal +/-24 dB; Limiter +/-120 dB', 'Headphones cancel the direct monitoring gain from Limiter Output.'], ['Input / Output LINK', 'OFF / ON', 'Equal-and-opposite gain linking in normal mode.'], ['MONITOR', 'ALL / L / R or M / S', 'Available in LR / MS; auditions one channel component in the center.'], ['Lookahead', '0 / 26 / 40 / 80 / 100 ms', 'Initially 26 ms; manual changes restore the full window.'], ['SAFE / window control', 'Default OFF; Distort / Window', 'Normal: 0-100%; Safe: milliseconds. See page 42.'], ['Overall OS', '1x / 4x / 8x / 16x', 'Active at every Lookahead setting; initially 8x.'], ['SOURCE / KEY GAIN', 'INT / EXT；±24 dB', 'KEY GAIN is adjustable only in EXT.'], ['HPF / SC LISTEN', 'OFF / 20-500 Hz；OFF / ON', 'Filters detection only. Listening ends when the panel or editor closes.'], ['Limiter / TP', 'Defaults: OFF / ON', 'Uses Hard Clip when TP is off.'], ['Ceiling', '-24 to 0 dB', 'Default 0 dB. Uses overall OS; TP is also available at 1x.'], ['TP recovery', 'TIGHT / AUTO / SMOOTH', 'Default AUTO; inactive with TP off.'], ['Headphone 1:1', 'Default OFF', 'MON = Ceiling - Output; saved in projects, independent of A/B.'], ['FULL / ECO', 'New instances recall the last manual choice', 'Per instance; saved in projects, independent of A/B.']], [0.23, 0.35, 0.42], 8.6)
    para('Normal and Limiter retain separate gain, detection and timing settings; see page 36. See pages 40-41 for ECO transport suspension, hidden-editor analysis and FULL low-residual sleep.', 9.3)
    start('Upward / Dual troubleshooting')
    table(['Symptom', 'First check'], [['No audible boost after adjustment', 'Check that the relevant Ratio is below 1:1, Mix is above 0% and Bypass is off. In Dual, that branch must be ON.'], ['No upward effect in Dual', 'The detector must be above UP and below DOWN. Below UP, the original level is retained; above DOWN, only downward processing applies.'], ['Faders set, but no processing', 'Do the boundaries coincide? This stops all dynamic processing. Separate them before comparing.'], ['Loud parts suddenly stop being processed', 'Is a finite Range set in SINGLE? At or above it, dynamic gain returns to 0 dB. Select OFF if you want no upper cutoff.'], ['Adjusting Up also changes Down', 'LINK between the two Ratios is on by default. Turn it off to adjust them independently.'], ['LINK will not move further', 'The other Ratio may have reached its limit, stopping both controls. Reverse the adjustment to continue.'], ['Main track level does not explain triggering', 'Check whether SOURCE is EXT or HPF is changing the detector signal. Threshold decisions follow the detector signal.']], [0.33, 0.67], 9.2)
    heading('Check Dual upward processing alone')
    step(1, 'Keep other variables fixed', 'Select DUAL, switch DOWN off and turn Ratio LINK off. Keep Mix at 100% and all gain controls at 0 dB. Choose a suitable Lookahead.')
    step(2, 'Place the signal between the thresholds', 'For example, set UP to -50 dB and DOWN to -20 dB. Check that the detector level of a quiet passage lies mainly between them. With Ratio still at 1:1, there is no boost yet.')
    step(3, 'Increase upward strength from 1:1', 'Move Up Ratio through 1:2, 1:4 and 1:8 while listening and watching Boost / Mix and positive dynamic gain. Once confirmed, enable DOWN to add attenuation and compare overall loudness again.')
    start('Troubleshooting')
    heading('Why can Classic output fall below the threshold?')
    para('Threshold defines the gain curve for the detector level, not a minimum output level. Quieter sounds within a window can receive the gain set by its peak. Shared ST gain, external sidechain or HPF can also make the detector differ from the signal you see. Makeup and Output Gain then change the output further.', 9.5)
    para('With Range OFF, internal detection, all gains at 0 dB and Mix at 100%, a steady level above threshold stays at or above it under the Classic formula. If a dip appears only in the Display, remember that it is a historical projection and check the actual audio. Not every dip is advance compression.', 9.5)
    table(['Symptom', 'First check'], [['EXT has no effect', 'Enable the host sidechain input and send. Select EXT and check KEY LEVEL. N/A means no external bus is available.'], ['Key Gain is disabled', 'Normal with INT: it adjusts only the external key.'], ['HPF does not cut output bass', 'It filters the detector only. History refreshes after release; wait for HPF UPDATING to clear.'], ['Sidechain sound / narrow image', 'Turn off SC LISTEN, return MONITOR to ALL, then check LR / MS.'], ['Match is unavailable', 'Play enough non-silent material for valid readings. Falling below the old gate no longer disables matching.'], ['Pause after changing Lookahead', 'Latency changes, so the host may readjust compensation. Also consider latency when monitoring a live recording.'], ['Nothing below -90 dB', 'This is the Display range, not an audio cutoff.']], [0.3, 0.7], 8.9)
    para('For Limiter TP, Ceiling, headphone and output issues, see page 31. Advanced operation is covered on pages 36-43. For installation, updates and scanning, refer to the installation guide for your platform.', 9)
    para('License: Qing Audio Non-Commercial Source-Share License 1.0. Commercial use is prohibited; see the project repository for full terms and corresponding source.', 8.7)
    para('<link href="https://github.com/Ziqing-Gu/QQ-Super-Compression" color="' + TEAL + '">github.com/Ziqing-Gu/QQ-Super-Compression</link>', 9)
    start('Normal and Limiter: separate settings', "Keep each mode's established sound when switching between them.")
    para('Normal compression and Limiter retain their own Input, Makeup, Mix, Output, algorithms, Single / Dual, ratios, boundaries, ST / LR / MS, sidechain, Lookahead and overall OS settings. The first entry into Limiter starts from its own defaults rather than copying normal compression.')
    table(['Adjustment', 'Result'], [['Set normal Input to +3 dB, then enter Limiter', 'Limiter recalls its own Input; normal mode retains +3 dB.'], ['Select Dual and set Ceiling in Limiter', 'Leaving restores normal mode. Re-entering recalls your Limiter setup.'], ['Switch A/B', 'Recalls both mode parameter sets and the active mode saved in that snapshot.']], [0.47, 0.53], 9.7)
    heading('After downward compression, add UP in Dual')
    para('Complete the standard setup on page 25, then switch to Dual. Retain the downward algorithm and amount; add a little UP between 1:1 and 1:1.2. Listen to kick and snare sustain. If Dual is unsuitable, try segmented compression first; see page 26.', 9.6)
    heading('Switching Single / Dual within Limiter')
    para('Switching to Dual carries Single Threshold over to the DOWN threshold. Switching back to Single carries the current DOWN threshold over to Threshold.')
    para('The other boundary in the destination mode is retained where possible; if the boundaries cross, the usual pushing rule applies. Ratios are stored separately in each mode. Input, Makeup, Mix and Output retain the current Limiter settings.', 9.7)
    note('Example: preserve a manually set threshold', 'Switch from Single Threshold = -18 dB to Dual: DOWN becomes -18 dB. This threshold is not recalculated from Output, even with Output LINK off or with Output and Threshold not equal and opposite.')
    note('Additional Output LINK rules', 'Increasing Makeup by 1 dB reduces Output by 1 dB. Ratio, Mix and algorithm changes do not directly move Output. With downward processing active, Output moves inversely with the Single downward threshold or Dual DOWN threshold. Dual UP does not follow Output / Makeup; its minimum is -inf in Super and -90 dB in Classic. Threshold ordering and range limits still apply.')
    heading('Instance operating settings')
    para('FULL / ECO, 1:1 headphone monitoring and channel solo belong to the current instance and are independent of A/B. Changing FULL / ECO in another instance does not switch this one.', 9.8)
    para('Reopening a project restores its saved instance state. Older projects undergo compatible restoration; check the mode, values and host latency before continuing.', 9.6)
    start('Overall oversampling and host latency', 'One OS control covers dynamics processing and Limiter Ceiling.')
    picture('manual-limiter-os.png', 'Overall OS is below the Lookahead / window controls; 4x is shown.', maxh=139, width=155)
    table(['Overall OS', 'Use and trade-off'], [['1x', 'Audio stays at its native sample rate. Lower CPU cost, with potentially more nonlinear aliasing.'], ['4x', 'A lower-cost oversampling option; compare with 1x first.'], ['8x / 16x', 'Higher processing rates. Choose by sound and CPU headroom; processing cost increases.']], [0.25, 0.75], 9.5)
    para('The initial rate is 8x. It applies at every Lookahead setting, including Distort / Window adjustments at nonzero Lookahead. Normal compression and Limiter save their own OS choices; within Limiter, dynamics and Ceiling share the active audio rate.', 9.5)
    heading('Less aliasing does not mean no coloration')
    para('Shortening the detector window can increase harmonics and gain modulation. Oversampling mainly reduces aliasing from nonlinear processing; it does not remove all harmonics or replace Safe / Window control of the detector level. For crackles, compare windows first and OS second, as on page 43.', 9.5)
    heading('Fixed base delay; total latency comes from the host')
    para('At 26 ms Lookahead, changing Window from 26 to 0 ms retains a 26 ms base delay. Additional latency from oversampling filters and Limiter protection is included separately. After changing Lookahead, OS or processing mode, let the host finish compensating before comparing.', 9.5)
    note('Restoring older sessions', "OS previously active only at 0 ms now applies to other Lookahead settings too. An older session's stored rate may therefore affect tone and CPU load; check overall OS after reopening. The former independent Ceiling OS no longer controls a second audio processing rate.")
    start('TP recovery: TIGHT / AUTO / SMOOTH', 'Click the small button between Ceiling and TP to choose how gain recovers after peaks.')
    picture('manual-recovery-auto.png', 'AUTO is shown. All three modes belong to final TP protection.', maxh=90, width=270)
    table(['Mode', 'Character and listening comparison'], [['TIGHT', 'Releases peak attenuation faster, allowing short-term loudness to recover readily. Listen for stronger fluctuations on bass or sustained amplitude modulation.'], ['AUTO', 'The default. Adjusts recovery to signal cycles and recurring changes, balancing isolated peaks and continuous variation.'], ['SMOOTH', 'Slower, more gradual recovery. Compare bass stability and movement after deep limiting; attenuation may persist longer.']], [0.2, 0.8], 10)
    step(1, 'Keep the same passage and ceiling', 'Keep Ceiling, Ratio, Output, Mix and overall OS fixed; change only the recovery mode.')
    step(2, 'Listen to short peaks and sustained bass', 'Short drum hits, sustained bass and repeated level changes can reveal different trade-offs. Do not judge from a single peak.')
    step(3, 'Compare at matched loudness again', 'Recovery affects average loudness. Replay and Match if needed, then compare tails, bass and attacks.')
    note('Same protection setting, different recovery shape', 'These choices do not change host latency at the current rate. They work only with TP on. With TP off, Hard Clip does not use these three recovery envelopes. Recovery mode is saved in projects and A/B.')
    para('FULL must drain tails and TP recovery before low-residual sleep; SMOOTH can extend this wait. ECO cuts the tail immediately when the host explicitly stops; see page 41.', 9.6)
    start('Output LUFS-I: loudness over a passage', 'The Limiter Display includes OUTPUT / LUFS-I.')
    picture('manual-limiter-lufs.png', 'Accumulated output loudness, with measurement duration at the lower right. This example is not a target value.', maxh=184, width=390)
    para('LUFS-I measures integrated loudness over a passage; TP measures peaks. LUFS-I reads the actual monitoring path, including Ceiling, headphone compensation and the current audition settings.')
    table(['State', 'How it works'], [['READY', 'No measurement accumulated yet. Play the passage you want to assess.'], ['MEASURING', 'Accumulating output loudness from the playback signal.'], ['HOLD', 'Retains the last measurement after stopping.'], ['--.-', 'No valid loudness reading yet. A signal that is too brief or too quiet may not produce one.']], [0.26, 0.74], 9.8)
    para('A new playback start, seek or loop restarts accumulation; stopping holds the result. If the host supplies no transport state, measurement runs continuously while Limiter is enabled.', 9.5)
    heading('Use the same conditions for every comparison')
    para('Play the same passage with identical channel MONITOR, SC LISTEN and headphone states. To judge the normal output sent downstream, turn headphones and SC LISTEN off and set MONITOR to ALL.')
    note('It does not automatically set a target loudness', 'LUFS-I is a reading; Match is a one-time loudness adjustment written to Makeup. Their measurement roles differ: a LUFS-I reading is not a Match completion indicator. ECO pauses analysis while the editor is hidden and restarts accumulation when reopened.')
    para('Loudness measurement uses gating, so very low levels may not produce valid LUFS-I. Also listen to attacks, density, tails and placement in the full mix when judging changes.', 9.6)
    start('FULL / ECO: choose for each instance', 'Clicking FULL / ECO at the top changes only the current plugin instance.')
    picture('manual-performance-eco.png', 'The ECO button. Keep an instance in FULL when it needs continuous analysis.', maxh=64, width=156)
    para('The table compares editor states during playback, recording and offline rendering; see below and page 41 for host Stop.', 9.4)
    table(['State', 'Editor visible', 'Editor closed / hidden'], [['FULL', 'Normal processing and analysis', 'Keeps background analysis and live monitoring active.'], ['ECO', 'Normal processing and analysis', 'Pauses editor analysis; audio retains its settings.']], [0.19, 0.34, 0.47], 9.8)
    heading('Ten instances: one FULL, nine ECO')
    para('For example, choose FULL for an instance that must monitor live input while stopped, and ECO for the other nine. Instances are independent, project-saved and unaffected by A/B. New instances recall the last manual choice; saved projects keep their own choices.')
    heading('When you reopen an ECO editor')
    para('Display and measurement resume and accumulate fresh data. Play representative material and wait for valid Match readings before using it. Applied Makeup, Output and other sound settings remain in place.')
    note('ECO preserves quality and latency while processing', 'During playback, recording and offline rendering, ECO processes at the current settings. Closing the editor pauses analysis without lowering overall OS or removing compensation. On a known host Stop, it mutes and suspends processing regardless of input noise.')
    heading('At Stop, the modes differ')
    para('With a known host Stop, ECO mutes immediately and skips compression, oversampling and Ceiling whether the editor is open or closed. FULL preserves live input monitoring; it may sleep after zero or extremely low input and internal tails have settled.', 9.8)
    start('Suspension and recovery after Stop', 'ECO follows the DAW transport; FULL retains live monitoring.')
    heading('ECO: Stop suspends processing')
    para('When the host explicitly reports Stop, with neither recording nor offline rendering active, ECO zeros the main output in that block and suspends dynamics, overall oversampling and Ceiling processing. Hardware noise or an active sidechain does not prevent suspension. Remaining delay and TP tails are cut off. Editor visibility does not change this behavior.')
    para('Use FULL on an instance that must monitor live input while stopped. ECO keeps processing when the host provides no reliable transport state.', 9.7)
    heading('When processing resumes')
    table(['Host state / action', 'Response'], [['Playback or recording starts', 'Full processing resumes in the first callback block.'], ['Offline rendering', 'Full processing continues, even if Play is not flagged.'], ['Switch to FULL while stopped', 'Live processing and monitoring resume in that block.'], ['Silent passage during playback', 'Full processing continues; zero samples do not trigger ECO Stop.']], [0.43, 0.57], 9.6)
    note('No extra wake delay or transport fade', 'Before the resumed block, old audio, filter and Ceiling histories are cleared and current settings applied. Existing Lookahead, oversampling latency and host PDC remain. The plugin adds no Stop/Start crossfade to its main output; leave monitoring fades to the DAW.')
    heading('FULL low-residual sleep and ASIO-Guard')
    para('FULL still processes live input and ordinary noise after Stop. It can suspend main audio work only on a known Stop without recording or offline rendering, when raw main and selected-sidechain peaks are at or below −180 dBFS, the actual gain-aware output has stayed below −160 dBFS for at least one second, and filter, delay and TP recovery are settled. Extremely low residue such as −200 dBFS becomes zero during sleep. Live input or selected-sidechain activity wakes processing in the current block. Extreme gain may conservatively keep FULL awake.', 9.5)
    para("ASIO-Guard reflects the host's overall scheduling; per-plugin CPU time cannot be converted directly to its percentage. To check ECO at Stop, first confirm the host reports Stop. Export alignment still depends on host rendering and latency compensation.", 9.5)
    start('Distort / Window: one detection range', 'The field sits to the right of Lookahead; SAFE changes its name and units.')
    table(['SAFE state', 'Field', 'Range, default and Alt-click reset'], [['Off: Normal', 'DISTORT', '0-100%; default 0%, using the full Lookahead window.'], ['Lit: Safe', 'WINDOW', '0 ms to the current Lookahead; defaults to the full Lookahead duration.']], [0.24, 0.22, 0.54], 9.5)
    heading('Example: 26 ms Lookahead')
    table(['Normal display', 'Display after enabling Safe', 'Actual window'], [['Distort 0%', 'Window 26 ms', 'Full 26 ms'], ['Distort 50%', 'Window 13 ms', '13 ms'], ['Distort 100%', 'Window 0 ms', 'Zero window']], [0.33, 0.34, 0.33], 9.8)
    para('Toggling SAFE keeps the actual window length and converts its display. Window = Lookahead x (1 - Distort / 100). At 0 ms Lookahead, the window is fixed at zero and the field is disabled.', 9.5)
    note('Distort is a control amount, not a THD reading', 'Higher values shorten the window and can increase harmonics, coloration and rapid gain changes. 0% does not mean zero distortion; 50% does not mean 50% harmonic distortion. The sound depends on the source, Ratio, threshold and OS.')
    heading('Type, drag and reset')
    table(['Adjustment', 'Result'], [['Double-click / vertical drag / Shift-drag', 'Type a value / normal adjustment / fine adjustment.'], ['Alt-click', 'Normal resets to 0%; Safe resets to the current Lookahead duration.'], ['Change Lookahead manually', 'Restores the full new window: Normal 0%, Safe equal to the new Lookahead.'], ['A/B, project restore, reopen editor', 'Retains the saved value rather than resetting as a manual Lookahead change would.']], [0.42, 0.58], 9.2)
    note('26 ms / zero window versus 0 ms / zero window', 'With matching gains, algorithm, detection source and OS, both use zero-window detection and can have similar core dynamics and coloration. The former still retains 26 ms of base delay. Align rendered files before comparing, rather than comparing the same timeline position directly.')
    start('Crackles and pre-attenuation: using SAFE', 'Start with the same clean source containing delay / reverb, then return to the full mix.')
    step(1, 'Establish a full-window comparison', "Start at 26 ms Lookahead, SAFE off and Distort 0%. Keep Ratio, threshold, Makeup, Mix and Output fixed, and listen for crackles and the source's dynamics.")
    step(2, 'Enable SAFE with the full Window', 'Safe uses the larger past/future peak. A longer window helps suppress crackles caused by rapid gain changes. Compare attacks, sustain and repeated echoes with Normal, not just loudness.')
    step(3, 'Shorten Window gradually', 'If audio around loud events is attenuated too much, reduce Window slowly to balance crackle control against pre-attenuation. Lookahead and base delay stay unchanged. Alt-click restores the full window.')
    step(4, 'Compare overall OS last', 'At your chosen window, compare 1x, 4x, 8x and 16x. Higher OS can reduce aliasing but does not replace window selection. Finish with a level-matched A/B comparison and check final output peaks.')
    table(['Goal', 'Try first'], [['Retain more local dynamics', 'SAFE off; start with Distort 0%.'], ['Add coloration deliberately', 'Increase Distort in Normal while listening for crackles and roughness.'], ['Reduce window-related crackles', 'Enable SAFE with the full Window, then shorten it to suit the source.'], ["Shorten Safe's surrounding dips", 'Reduce Window; protection also decreases. At zero, both detectors are identical.']], [0.38, 0.62], 9.3)
    note('Listen to both sides of the Safe tradeoff', 'A longer Window can attenuate quiet audio before a loud event arrives and keep attenuating briefly after it ends. This is pre- and post-attenuation, not an added Attack / Release stage. Safe targets this type of rapid gain modulation; it cannot repair clipped input or host dropouts.')
    para('No visible dip does not mean identical distortion: gain may already vary differently within steady cycles. Listen to actual audio and examine harmonics and level steps together. The plug-in Display helps explain settings, but does not replace those checks.', 9.5)
    finish()
    c.save()
    doc = PdfReader(a.output / name)
    assert len(doc.pages) == 43
    txt = '\n'.join((p.extract_text() for p in doc.pages))
    for banned in ('Revision', 'Rev 2', 'Plan A', 'Plan B', 'Plan C', 'JUCE', 'offscreen', 'offscreen', '2.2x', 'normalized', 'cache', 'v1.0.1', 'appendix', 'addendum', 'schema'):
        assert banned not in txt, (lang, banned)
    import json
    (Path(__file__).parent / 'English-layout-audit.json').write_text(json.dumps({'version': '1.3.2', 'language': 'en', 'pages': len(doc.pages), 'minimum_content_baselines_pt': mins, 'status': 'awaiting_visual_review'}, indent=2), encoding='utf-8')
    print(lang, len(doc.pages), 'pages; lowest content baselines:', mins)
