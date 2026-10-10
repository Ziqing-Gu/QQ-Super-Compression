"""English 1.3.5 manual translated from the approved Chinese edition."""
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
    name = 'QQ Super Compression User Manual English_v1.3.5.pdf'
    c = canvas.Canvas(str(a.output / name), pagesize=(W, H), pageCompression=1)
    c.setTitle('QQ Super Compression 1.3.5 User Manual')
    c.setAuthor('Qing Audio')
    c.setSubject('Dynamics, Limiter, level-matched listening and sidechain guide')
    page = 0
    y = 0
    mins = []

    def p_obj(text, size=10, colour=INK, boldface=False):
        if page == 14: size *= 0.93
        if page in (15, 27, 39): size *= 0.98
        return Paragraph(text, ParagraphStyle('p', fontName=bold if boldface else font, fontSize=size, leading=size * 1.38, textColor=HexColor(colour), wordWrap='CJK' if zh else None))

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
        tb.setStyle(TableStyle([('VALIGN', (0, 0), (-1, -1), 'TOP'), ('BACKGROUND', (0, 0), (-1, 0), HexColor(PALE)), ('ROWBACKGROUNDS', (0, 1), (-1, -1), [white, HexColor('#f6f9fa')]), ('LEFTPADDING', (0, 0), (-1, -1), 7), ('RIGHTPADDING', (0, 0), (-1, -1), 7), ('TOPPADDING', (0, 0), (-1, -1), 4 if page == 2 else 6), ('BOTTOMPADDING', (0, 0), (-1, -1), 4 if page == 2 else 6)]))
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
        # Validate all page baselines after the entire document has been laid out.
        mins.append(round(y, 1))
        c.setStrokeColor(HexColor('#d8e6e9'))
        c.setLineWidth(0.5)
        c.line(M, 40, W - M, 40)
        c.setFillColor(HexColor(TEAL))
        c.setFont('Body', 8)
        c.drawString(M, 26, 'QING AUDIO')
        c.setFillColor(HexColor(MUTED))
        c.setFont(font, 8)
        c.drawRightString(W - M, 26, 'QQ Super Compression 1.3.5  |  ' + str(page))
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
        c.drawString(M, top + 2, 'Output dB')
        c.drawRightString(W - M, py(-50) - 29, 'Detector level dB')
        c.setFont('Bold', 8)
        c.setFillColor(HexColor(TEAL))
        c.drawString(x + 8, py(-7), 'CLASSIC')
        c.setFillColor(HexColor('#ED7C31'))
        c.drawString(x + 86, py(-7), 'SUPER')
        y = top - h - 40
        o = p_obj('Static curves: Threshold = -24 dB, Ratio = 4:1, with compression continuing above Threshold. No makeup gain, level transitions or detector timing are included.', 8.3, MUTED)
        _, ht = o.wrap(CW, 800)
        o.drawOn(c, M, y - ht)
        y -= ht + 9
    start('QQ Super Compression', 'User Manual  /  1.3.5 Stable · English Edition')
    heading('Control dynamics. Keep the attack natural.')
    para('For vocals, instruments, buses and mastering. The dynamics core has no conventional Attack or Release knobs. Shape the sound with Lookahead, SAFE, Ratio, Threshold and Mix; the Limiter has three separate TP recovery modes.')
    para('Classic uses a fixed compression ratio in dB; Super uses the original QQ curve. Both support Single, upward and Dual compression. One OS setting covers all Lookahead presets; the Limiter adds Hard Clip / TP, recovery modes and 1:1 headphone monitoring.', 9.6)
    picture('manual-light-ST.png', maxh=402)
    note('Start with what you hear', 'Decide how steady the dynamics should be and how much articulation to retain. The Display helps explain changes; it does not replace listening.')
    start('Quick start and navigation', 'Set up your sound first, then explore the advanced controls as needed.')
    for i, (title, text) in enumerate([('Choose a starting point', 'Start with CLASSIC, SINGLE, ST and SC: INT; use Lookahead 26 ms, SAFE off by default and Window 26 ms.'), ('Adjust Ratio from 1:1', 'Ratio sets direction and amount: right attenuates, left lifts. Threshold sets the level where processing starts; see page 7.'), ('Use Mix to set the final amount', 'If processing is too strong, blend the original dynamics back in. See pages 8-11 for upward and Dual compression.'), ('Compare at matched loudness', 'Play representative material, click Match, then fine-tune Makeup / Output Gain.'), ('Use the standard Limiter workflow', 'Set Ratio between 20:1 and 1000:1, keep Output LINK on, and lower Threshold gradually for automatic Output compensation; see page 28.'), ('Listen in the full mix', 'Switch Bypass or A/B and listen to attacks, tails, density and groove.')], 1):
        step(i, title, text)
    table(['Find', 'Pages'], [['Interface, themes and gestures', '3-4'], ['Classic / Super, Single and upward processing', '5-8'], ['Dual, upward/downward switches and Ratio LINK', '9-11'], ['Input / Output LINK, Match and A/B', '12-14'], ['Display', '15'], ['Lookahead and Window: relationship and controls', '16-17'], ['SAFE: detection and listening procedure', '18-19'], ['ST / LR / MS and channel LINK', '20-22'], ['Sidechain and practical mixing', '23-26'], ['Limiter, L/R Link, Ceiling and level-matched listening', '27-34'], ['Parameter reference and troubleshooting', '35-38'], ['Separate mode settings, OS and latency', '39-40'], ['TP recovery and output LUFS-I', '41-42'], ['FULL / ECO and transport behavior', '43-44']], [0.78, 0.22], 8.5)
    start('Know the interface', 'The same audio controls remain in place across all themes.')
    picture('manual-light-ST.png', 'Light / ST. The values illustrate the interface; they are not a recommended preset.', maxh=361)
    table(['Area', 'Use it to'], [['Display', 'View historical level and gain references under current settings; see page 15.'], ['Threshold / Range faders', 'Threshold starts compression above its level; Range stops it at or above its level. These two faders select the processing region in normal Single mode; see page 7.'], ['Input / Output / Gain +/-', 'Check input, output and effective dynamic gain.'], ['Bottom knobs and SINGLE / DUAL', 'Adjust gain, Ratio and Mix; switch Single / Dual beside Ratio.'], ['Four rows at the lower right', 'Mode (L/R Link in Limiter), SAFE, Lookahead + Window, Monitor + OS. Limiter has no Monitor.'], ['Top buttons', 'Toggle LIMITER; select FULL / ECO, sidechain, theme, A/B and Bypass. Algorithm buttons are in the bottom Ratio section.']], [0.32, 0.68], 9)
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
    para('Classic downward compression: Threshold = -10 dB, Ratio = 5:1. In this example, audio above Threshold remains eligible for compression. A steady detector level of 0 dB is 10 dB above Threshold; compression retains 2 dB of that excess, giving <b>-8 dB</b>. A level of -12 dB is below Threshold and is not attenuated.', 9.6, 8)
    para('This example uses internal detection, 0 dB Input, Makeup and Output Gain, and Mix=100%. The formula describes a steady detector level, not a hard limit on individual samples. For changing signals and the Display, see pages 15-16.', 8.8, 8)
    heading('Upward compression: lift audio above Threshold')
    para('Classic and Super both support upward compression. Set Ratio below 1:1 to lift audio above Threshold. Their gain curves differ, but both use Threshold and Range to select the audio to process; see pages 7-8.', 9.6, 8)
    note('Minimum threshold and remembered choice', 'Normal and Limiter use the same algorithm limits: Classic reaches -90 dB and Super retains -inf. Scale changes the visible range only; see page 15. New normal-mode instances remember the last manually selected algorithm; projects and A/B recall saved choices. Limiter settings are separate. Short fades reduce switching transients.')
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
    start('Single: Threshold and Range', 'Choose which levels to process, then set the amount.')
    heading('Threshold: compression starts above this level')
    para('Threshold sets the level at which processing begins. Audio at or below it receives no compression. Above it, Ratio determines how much to attenuate or lift.')
    heading('Range: compression stops at this level')
    para('Range preserves the dynamics of louder audio. At or above Range, neither attenuation nor upward gain is applied. Compression therefore acts only between Threshold and Range. In normal Single mode, the Range fader is above Threshold.')
    heading('How to use it')
    para('For example, set Threshold to -30 dB and Range to -10 dB:', 9.6, 6)
    table(['Signal level', 'What happens'], [['-30 dB or below', 'No compression.'], ['Above -30 dB, below -10 dB', 'Attenuate or lift according to Ratio.'], ['-10 dB or above', 'No compression; preserve these dynamics.']], [0.42, 0.58], 9.3)
    para('Lower Range to exclude more loud audio from compression; raise it to include louder audio. Keep Threshold fixed first, then move Range and listen for the parts that regain their original dynamics.', 9.5)
    heading('Range OFF: disable the Range control')
    para("Move Range to OFF at the top to stop excluding louder audio. Levels above Threshold continue to follow the selected algorithm. OFF disables Range's effect; compression itself remains active.", 9.5)
    note('Special cases', 'Range = 0 dB stops compression at 0 dB. Range OFF removes this stopping level.<br/>When Range and Threshold coincide, no processing interval remains and dynamic processing stops. The faders move together when they meet; drag back to separate them.<br/>Processing tapers off near Range and stops at it. This transition follows signal level; it adds no Attack / Release stage.')
    para('<b>If you hear crackles:</b> Range can produce crackles on some material. Try enabling SAFE for more stable detection, then compare. SAFE may also attenuate audio before and after loud events, so listen for both effects; see pages 18-19.', 9.3)
    para('Here, “no compression” means no dynamic attenuation or lift. Input, Makeup, Mix and Output Gain still apply. With EXT, the external sidechain level determines the region. Limiter does not use Range; see page 28.', 9.1)
    start('Downward and upward compression', 'First understand the gain change, then compare after matching levels.')
    heading('Downward compression: attenuate audio above Threshold')
    para('In SINGLE, turn Ratio right from 1:1 to values such as 2:1, 4:1 or 8:1. Audio above Threshold is attenuated, reducing the difference between loud and quiet levels. Audio at or below Threshold is not attenuated.')
    para('The main sound may become quieter. Raising Makeup or Output to match its original level also raises the uncompressed audio below Threshold.', 9.6)
    heading('Upward compression: lift audio above Threshold')
    para('Turn Ratio left from 1:1 to values such as 1:2, 1:4 or 1:8 for upward compression. Within the active region, quieter audio generally receives more lift than louder audio, reducing the dynamic difference. Audio at or below Threshold is not lifted.')
    para('Classic and Super both support these directions. Their gain curves differ, but the threshold rules here are the same. When Range is enabled, levels at or above it are neither attenuated nor lifted; see page 7.', 9.6)
    heading('Compare before adding level compensation')
    table(['Level region', 'Downward compression', 'Upward compression'], [['At or below Threshold', 'No attenuation', 'No lift'], ['Active region above Threshold', 'Attenuate to reduce dynamic differences', 'Lift to reduce dynamic differences'], ['At or above an active Range', 'No attenuation', 'No lift']], [0.37, 0.3, 0.33], 9.3)
    heading('The symmetry between upward and downward compression')
    para('Within the same algorithm, level-matching audio above Threshold can make downward 8:1 and upward 1:8 produce the same result in the main processing region. Classic and Super each have this symmetry. Overall compensation also affects audio below Threshold, where the results differ; see page 14 for numbers.', 9.6)
    note('Choosing between them', 'To raise quiet audio below Threshold as well, try downward compression followed by compensation. To lift selected details while retaining quieter noise, breaths or ambience, try upward compression with Threshold above the sounds you want to leave alone. Compare in the full mix.')
    para('Upward gain fades in near Threshold; processing tapers off near Range. Do not expect exact equivalence around those boundaries. Changing Mix also calls for a fresh listening comparison.', 9.3)
    start('Dual: lift quieter parts, control louder ones', 'Click the small button beside Ratio to change SINGLE to DUAL.')
    picture('manual-light-dual.png', 'DUAL: UP RATIO and DOWN RATIO control the two directions separately. UP / DOWN thresholds replace Threshold / Range.', maxh=245)
    regions([('Original level', 'At or below UP'), ('Boosted by UP RATIO', 'Above UP, below DOWN'), ('Cut by DOWN RATIO', 'Above DOWN')], ['UP threshold', 'DOWN threshold'], ['#f3f6f7', '#eaf6f8', '#eaf6f8'])
    para('At DOWN, dynamic gain returns to 0 dB. Above it, upward processing stops and only Down Ratio applies. The two gains never stack at the same detector level. Dual has no Range control.', 9.7)
    table(['Starting a new instance', 'How to read it'], [['Normal UP: -90 dB in Classic, -inf in Super. DOWN: 0 dB.', 'The thresholds start at opposite ends; this does not disable upward processing.'], ['Up Ratio = 1:1；Down Ratio = 1:1', 'Neither direction changes dynamics until Ratio is adjusted.']], [0.48, 0.52], 9.2)
    note('Threshold order and collision', 'DOWN must stay at or above UP. When one fader reaches the other, it pushes the other along. Dynamic processing stops when the thresholds coincide.')
    start('Dual switches: audition each direction', 'UP / DOWN each have an on/off switch and an algorithm button for separate auditioning and curve selection.')
    picture('manual-light-dual-controls.png', 'Upward and downward compression are on. Each algorithm is shown in its Ratio area. The switches preserve Ratio values.', maxh=148, width=245)
    table(['UP / DOWN', 'Processing you hear'], [['ON / ON', 'Boost between UP and DOWN; attenuation above DOWN.'], ['ON / OFF', 'Upward only. Boost still stops at DOWN.'], ['OFF / ON', 'Downward only. No dynamic processing below DOWN.'], ['OFF / OFF', 'No dynamic boost or attenuation. Other gain and Mix settings still apply.']], [0.27, 0.73], 9.5)
    step(1, 'Set up the full Dual sound', 'Set both thresholds and Ratios. Play material with quiet and loud passages.')
    step(2, 'Hear what each direction contributes', 'Turn DOWN off to hear upward compression, then UP off to hear downward compression, then enable both. Each direction can use Classic or Super; changing the UP algorithm does not change DOWN.')
    step(3, 'Check the display and compare at similar loudness', 'The switches update dynamic gain and the Display history reference. Compensate overall level, then compare detail, articulation and consistency.')
    note('Each switch affects only its own compression direction', 'Each ON / OFF switch uses a short crossfade to reduce transients. Disabling one direction does not move thresholds or extend the other into its region. Normal LR / MS allows separate channel switches. With Mode LINK enabled, corresponding UP switches follow each other, as do DOWN switches; UP and DOWN remain independent.')
    start('Ratio LINK: opposite proportional changes', 'Normal DUAL has a small LINK between its two Ratio controls. Limiter does not.')
    picture('manual-light-dual-controls.png', 'Up and Down retain their existing relative relationship. Enabling LINK does not make values jump.', maxh=100)
    para('Ratio LINK defaults to on at first use, then remembers your last choice. When reopening a saved project, the LINK state saved in that project takes priority.')
    table(['Initial Down / Up', 'Adjust Down', 'Linked Up'], [['1:1 / 1:1', '1:1 → 2:1', '1:2'], ['4:1 / 1:2', '4:1 → 8:1', '1:4'], ['8:1 / 1:4', '8:1 → 4:1', '1:2']], [0.36, 0.32, 0.32], 9.6)
    heading('Preserve the relationship, without re-pairing')
    para('For example, doubling Down halves the Up value; adjusting Up uses the same inverse relationship. Starting at 4:1 and 1:2 will not force them into a reciprocal pair. Dragging, fine adjustment and direct entry all follow this rule.')
    note('Linked controls stop together at their limits', 'Normal compression offers Up from 1:200 to 1:1 and Down from 1:1 to 200:1. If either would exceed its range, both stop at the shared limit. Reverse the adjustment to continue. Toggling LINK alone does not change the ratios.')
    heading('To hear upward processing alone')
    para('Switch DOWN off to preserve its Ratio while auditioning Up. To adjust Up Ratio independently as well, turn off the small Ratio LINK; otherwise the Down value still follows your adjustment.')
    heading('Also available in LR / MS')
    para('In normal LR / MS, the small LINK connects Up and Down ratios within each channel. Mode LINK connects channels; see page 22. Limiter UP / DOWN ratios are always independent: set downward compression first, then add a little upward compression; see page 29.')
    start('Input / Output LINK: opposite gain changes', 'Above and to the right of INPUT GAIN. It is separate from Ratio LINK.')
    picture('manual-input-output-link.png', 'Example: Input +3 dB, Output -3 dB, with LINK enabled.', maxh=105)
    para('With LINK on, raising Input Gain in the interface lowers Output Gain by the same number of dB. Adjusting Output moves Input in the opposite direction too. This helps reduce simple level changes while you explore a different level entering the processor.')
    table(['Initial Input / Output', 'Adjustment', 'Result'], [['0 / 0 dB', 'Raise Input by 3 dB', '+3 / -3 dB'], ['+2 / -1 dB', 'Raise Input by 3 dB', '+5 / -4 dB'], ['+5 / -4 dB', 'Raise Output by 2 dB', '+3 / -2 dB']], [0.35, 0.33, 0.32], 9.5)
    heading('Keep an existing offset')
    para('The link applies opposite gain changes; it does not force the two values to be exact opposites. Toggling LINK does not change existing gains. If either control reaches its limit, both stop together.')
    heading('Remember the last choice')
    para('LINK is on initially. New instances then use your last choice; a saved project restores its own state. Projects and A/B save the current LINK setting. Check the link switches after recalling a snapshot before making further adjustments.')
    note('Limiter uses a different link', 'Normal Input / Output LINK only applies opposite gain changes; it does not replace Match. In Limiter it is hidden and inactive. LINK beside Output connects the downward threshold and output compensation; see page 28.')
    start('Match and Makeup: measure from playback start', 'Click during playback; each click uses the entire current playback up to that point.')
    table(['Control', 'Use'], [['Makeup', 'Adjusts Wet level before Mix: normal mode ±30 dB; Limiter ±120 dB.'], ['Output Gain', 'Normal compression: ±24 dB; Limiter: ±120 dB. Headphone comparisons: pages 32-33.'], ['Match', 'Each click sets Makeup from cumulative loudness; this is not continuous automatic gain.']], [0.24, 0.76], 9.5)
    step(1, 'Start playback and establish a measurement', 'Choose normal compression or Limiter and the headphone state first. Play representative, non-silent material. Once valid loudness data is available, you can click Match during playback.')
    step(2, 'Click again whenever needed', 'The first click does not clear the measurement. Clicking at 10 and 20 seconds uses the first 10 and first 20 seconds respectively. It does not add the previous compensation again.')
    step(3, 'A new playback starts a new measurement', 'Stopping retains the result. The next playback starts a new cumulative measurement. Loops and seeks during uninterrupted playback still belong to the same Match measurement.')
    note('FULL and ECO', 'FULL continues measuring with the editor closed. In ECO, open the editor before playback and keep it open for a complete Match measurement. Missing analysis makes Match unavailable for that playback; restart playback to restore it.')
    heading('Fix the comparison conditions before judging the sound')
    para('Normal compression compares Dry with Wet before compensation. Limiter headphone mode compares the original input with the actual limited monitoring output, then applies one shared Makeup value. Neither Match clicks nor Makeup adjustments reset the measurement start.', 9.5)
    para('Switching Limiter or headphone state changes the measurement reference and clears the statistics. Choose these before measuring. Silence or insufficient data cannot be matched. To compare only new settings, stop and replay the same material.', 9.5)
    note('Loudness and peaks are separate', 'Match compensates loudness; Ceiling controls final peaks. Check output and TP afterwards, and use Bypass / A/B to compare attacks, tails and dynamics.')
    start('A/B: compare algorithms and directions')
    picture('manual-top-controls.png', 'A and B hold two sound settings. A→B and B→A copy between them.', maxh=62)
    step(1, 'Save the first idea', 'Set the algorithm and processing amount in A. Copy A→B, then switch to B. Copying replaces the destination sound settings.')
    step(2, 'Change the part you want to compare', 'For example, use Classic in A and Super in B, or compare downward and upward processing. Keep Lookahead, detector source and monitoring the same where possible.')
    step(3, 'Match each, then switch', 'Play the same material for each setting and use Match; trim Makeup if needed. Click A / B and compare the sound, without mistaking louder for better.')
    table(['Recalled by A/B', 'Outside the A/B sound snapshot'], [['Algorithms, Single / Dual, thresholds, ratios, upward/downward switches, gains, Mix, Lookahead, SAFE, Window, overall OS, sidechain, Limiter, TP, Ceiling, TP recovery, LINK states and both sets of processing parameters.', 'Theme, FULL / ECO, 1:1 headphone monitoring, channel solo, Bypass and SC LISTEN.']], [0.6, 0.4], 9.3)
    heading('Upward/downward symmetry: 8:1 and 1:8')
    para('Compare within Classic or within Super. Use the same Threshold, Lookahead, Window and SAFE state, Range OFF, Mix 100%, and no Ceiling or other extra processing. Match levels using audio above Threshold: the main processing region can then give the same result.', 9.4)
    para('Example: Classic, Threshold = -24 dB. A uses downward 8:1 followed by +21 dB compensation. B uses upward 1:8 with no compensation. Steady-level results:', 9.3, 6)
    table(['Input level', 'A: Down + compensation', 'B: Up'], [['-12 dB (above Threshold)', '-1.5 dB', '-1.5 dB'], ['-6 dB (above Threshold)', '-0.75 dB', '-0.75 dB'], ['-36 dB (below Threshold)', '-15 dB', '-36 dB']], [0.42, 0.29, 0.29], 9.1)
    para("Neither setting compresses below Threshold, but A's overall compensation raises that audio too, producing a difference. Super has the same type of symmetry with a different compensation amount. This does not mean that Super and Classic sound identical.", 9.3)
    para('The upward transition near Threshold, an active Range, a different Mix or extreme boosts can break this correspondence. Match measures loudness across the whole playback, including audio below Threshold. To examine this example, align the level of the main sound above Threshold.', 9.1)
    note('Switching and latency', 'A/B uses transitions to reduce switching artifacts. Keep Lookahead and OS equal to focus on the sound; changing modes or rates may make the host update delay compensation. A/B recalls each saved Window value and realigns the Display accordingly.')
    start('Display and Scale: choose the detail', 'Normal and Limiter share the same display ranges.')
    para('Scale sets how far the Display extends below 0 dB. Click Scale above the graph to cycle through -30, -60 and -90 dB; the default is -90 dB. A smaller range makes the same level difference appear larger.',9.6)
    picture('manual-scale-30.png','Scale sits above the Display. The -30 dB view makes details in louder audio easier to see.',maxh=126,crop=(32,152,2336,1072))
    table(['Scale','Useful for'],[['-30 dB','Loud main material, mastering and Limiter detail.'],['-60 dB','Main material, sustain and quieter passages.'],['-90 dB','Low tails and a broader view of level changes.']], [.23,.77],9)
    heading('Fader scales follow; existing values stay unchanged')
    para('Threshold, UP, DOWN and normal Single Range faders align with the graph. Drag and Shift-drag sensitivity follows Scale. Zooming changes no audio, compression or latency. Projects save Scale; switching Limiter or A/B keeps the current range for a consistent comparison.',9.2)
    note('When a threshold is outside the view','A -50 dB threshold remains -50 dB when Scale changes to -30 dB. Its thumb stays at the bottom and the readout shows *. Select -60 / -90 dB to see its position. Double-click still accepts the actual threshold; Alt-click still resets to the algorithm minimum. Scale does not change the meaning of -inf or Range OFF.')
    table(['Curve / meter','Meaning'],[['Grey Dry / Input','Input history before Input Gain.'],['Blue Cut / Mix, green Boost / Mix','Attenuation or boost references under current settings.'],['Orange Output','Output reference including gains, Mix and Ceiling.'],['External key, right-hand GAIN +/-','Sidechain reference; live gain change and Hold.']], [.36,.64],8.5)
    para('Eight seconds of history are recalculated using current settings. HPF UPDATING indicates a refresh. Display is a level reference, not a recorded final waveform. At Ratio 1:1, zero gains, internal full-band detection and no Ceiling, input and output should overlap. At 0 ms, SAFE does not create a different dynamics trace.',8.8)

    start('Lookahead and Window: separate delay from sound', 'Keep latency fixed while comparing the sound of shorter Lookahead lengths.')
    para('Lookahead reserves time to read ahead. The plug-in delays output so it can analyze upcoming audio; this sets the base latency and the maximum Window. Window sets how far before and after the current sample the processor looks for peaks when calculating gain.')
    table(['Control', 'What it sets'], [['Lookahead', 'Sets base latency: 0, 26, 40, 80 or 100 ms; default 26 ms. Changing it may require the host to update delay compensation.'], ['Window', 'Sets actual detection length from 0 to the selected Lookahead. Changes the sound while retaining the selected latency.']], [0.24, 0.76], 10)
    note('Window shortens effective Lookahead at fixed latency', 'Compare Lookahead 26 ms / Window 10 ms with Lookahead 10 ms / Window 10 ms. With other settings equal and settled, and the audio delay-aligned, core compression and harmonic coloration are the same. The first setting simply retains 16 ms more base latency. You can therefore compare shorter Lookahead sounds without repeatedly changing plug-in latency.')
    heading('Example: keep Lookahead at 26 ms')
    table(['Window', 'Actual detection length', 'Base latency'], [['26 ms', 'Full 26 ms', '26 ms, unchanged'], ['10 ms', 'Reduced to 10 ms', '26 ms, unchanged'], ['0 ms', 'Current-sample detection', '26 ms, unchanged']], [0.24, 0.45, 0.31], 9.8)
    para('Base latency excludes OS filters and Limiter Ceiling latency. With other settings fixed, adjusting only Window leaves the reported total latency unchanged too.', 9.5)
    heading('Why this helps continuous listening comparisons')
    para('Changing Lookahead directly may make the host adjust delay compensation, causing a brief pause. Window retains the same latency, allowing continuous comparison of attacks, tails, dynamics and coloration without that compensation change.')
    heading('Window is a detection duration, not an update interval')
    para('Window = 10 ms does not mean compression happens once every 10 ms. Each gain calculation considers peaks within 10 ms before and after the current sample; Normal / Safe then chooses between them. Detection moves and gain updates on every sample. Shortening Window changes the reference range, not the update rate.', 9.6)
    start('Lookahead and Window: controls and 0 ms', 'Choose the sound with Window first, then decide whether to reduce Lookahead latency.')
    picture('manual-safe-timing.png', 'Lookahead and Window share a row in the lower-right controls. Window always uses milliseconds.', maxh=115, width=140)
    table(['Lookahead', 'Window range', 'Default / Alt-click'], [['0 ms', 'Fixed at 0 ms; not editable', '0 ms'], ['26 ms', '0-26 ms', '26 ms'], ['40 / 80 / 100 ms', '0 to selected Lookahead', 'Equals selected Lookahead.']], [0.25, 0.36, 0.39], 9.3)
    table(['Adjustment', 'Result'], [['Double-click / vertical drag / Shift-drag', 'Type milliseconds / continuous adjustment / fine adjustment.'], ['Alt-click Window', 'Restore the maximum allowed by the current Lookahead.'], ['Change Lookahead manually', 'Window follows the new Lookahead and resets to its maximum.'], ['A/B, project restore, reopen editor', 'Recall saved Window; do not treat recall as a manual reset.']], [0.43, 0.57], 9.1)
    note('26 ms / Window 0 ms versus 0 ms / Window 0 ms', 'With the same algorithm, parameters and OS, both detect the current sample. The first retains 26 ms of base latency; the second removes it. If Window = 0 ms sounds best, try 0 ms Lookahead, allowing the host to finish delay compensation. For Limiter listening comparisons, see page 34.')
    para('Most normal compression can retain the full Window. Shortening it can increase local dynamics, harmonic coloration and crackles. At low values, especially below 10 ms, oversampling is recommended; see page 40. Window avoids pauses caused by changing delay compensation; it still changes the processing sound.', 9.5)
    para('Old 10 ms Lookahead sessions migrate to 26 ms. Set Window to 10 ms to retain that detection length. A 0 ms Lookahead setting does not guarantee zero total latency: OS filters and Limiter Ceiling can still add delay.', 9.2)
    start('SAFE: detection and pre-attenuation', 'Choose a detection strategy without changing the selected Lookahead latency.')
    picture('manual-safe-button.png', 'SAFE is on the second row at the lower right. Yellow means Safe; unlit means Normal. New instances default to OFF.', maxh=42, width=105)
    para('First set Lookahead and Window as described on the previous two pages. Window sets detection length; SAFE determines how the past and future peaks within that length are used. They control the range and the choice separately.')
    table(['Detector', 'How past/future peaks are used'], [['Normal / SAFE off', "Find the peak in each past/future Window, then use the smaller one. Usually retains more local dynamics and reduces a loud event's influence on nearby quiet audio."], ['Safe / SAFE lit', 'Use the larger peak. A longer Window stabilizes detection and helps suppress continuous crackles caused by rapid gain changes.']], [0.25, 0.75], 9.6)
    heading('When to try SAFE')
    para('Clean guitar, vocals or other material containing delay or reverb can cause rapid gain changes as sounds overlap. Range in normal Single mode can also cause crackles on some material. Try SAFE with the full Window, then compare the main sound, echoes and sustain.', 9.6)
    note('The tradeoff: pre- and post-attenuation', 'An upcoming loud event can attenuate nearby quiet audio before it arrives. After it ends, the past Window may still contain its peak and attenuate following quiet audio. Dips may appear on either side of loud events. A shorter Window reduces their duration, but also weakens crackle suppression.')
    heading('How SAFE differs from Classic / Super')
    para('SAFE changes how the detector level is selected. Classic / Super chooses the dynamics curve applied to that level. Pre- and post-attenuation come from the selected Window; no hidden Attack / Release stage is added to the dynamics core.', 9.5)
    heading('Defaults, switching and 0 ms')
    para('SAFE defaults to OFF and is saved with the project and A/B. Toggling it keeps the current Window and latency. At 0 ms Lookahead it is greyed out, while its stored selection is retained. Whenever the actual Window is 0 ms, Normal and Safe detect identically.', 9.5)
    para('The next page describes comparing SAFE at fixed settings, then adjusting Window.', 9.5)
    start('SAFE: listening for crackles and pre-attenuation', 'Start with the same clean source containing delay / reverb, then return to the full mix.')
    step(1, 'Start with a full-Window comparison', "Start at Lookahead 26 ms, SAFE off and Window 26 ms. Fix Ratio, thresholds, Makeup, Mix and Output, and listen for crackles and the main sound's dynamics.")
    step(2, 'Enable SAFE with the full Window', 'Safe uses the larger past/future peak. A longer Window helps suppress crackles caused by rapid gain changes. Compare attacks, sustain and repeated echoes with Normal, not just loudness.')
    step(3, 'Shorten Window gradually', 'If audio around loud events is attenuated too much, shorten Window gradually to balance crackle control against pre-attenuation. Lookahead and base latency stay unchanged. Alt-click restores the full Window.')
    step(4, 'Compare overall OS last', 'At the selected Window, compare 1x, 4x, 8x and 16x. Higher OS reduces aliasing but does not replace Window selection. Finish with a level-matched A/B comparison and check final output peaks.')
    para('Range in normal Single mode can also cause crackles on some material. Try SAFE here too, starting with the full Window and shortening it by ear.', 9.5)
    table(['Goal', 'Try first'], [['Retain more local dynamics', 'Start with SAFE off and the full Window.'], ['Add coloration deliberately', 'Shorten Window gradually in Normal, listen for crackles and roughness, and enable OS.'], ['Reduce Window-related crackles', 'Enable SAFE with the full Window, then shorten it to suit the source.'], ["Shorten Safe's surrounding dips", 'Shorten Window; protection also weakens. At Window 0 ms, both detectors are identical.']], [0.38, 0.62], 9.3)
    note('Listen to both sides of the Safe tradeoff', 'A longer Window can attenuate quiet audio before a loud event arrives and keep attenuating briefly after it ends. This is pre- and post-attenuation, not an added Attack / Release stage. Safe targets this type of rapid gain modulation; it cannot repair clipped input or host dropouts.')
    para('No visible dip does not mean equal distortion at two Window settings: gain may vary differently within steady cycles. Listen to actual audio and examine harmonics and level steps together. The plug-in Display helps explain settings but does not replace these checks.', 9.5)
    start('ST and LR: together or independently')
    para('In normal compression, Mode at the lower right cycles ST → MS → LR. Start with ST; choose LR for separate channel settings. Limiter replaces this selector with a single ST display and a numeric L/R Link control; see page 27.')
    picture('manual-light-LR.png', 'LR: L above, R below, with independent controls.', maxh=373)
    table(['Mode', 'How to use it'], [['ST', 'Uses common compression gain for stereo; a useful starting point for tracks and buses.'], ['LR', 'Left and right have independent ratios, thresholds, Makeup and Mix; DUAL adds UP / DOWN per channel. Check for stereo image shifts on asymmetric material.']], [0.17, 0.83], 9.4)
    note('MONITOR', 'Choose L or R to audition that side centered. Return to ALL to check normal stereo, and do not leave one-sided audition active for the final output.')
    start('MS: control center and sides')
    picture('manual-light-dual-MS.png', 'MS / DUAL: Mid is above Side. Each channel has its own UP / DOWN Ratios and thresholds.', maxh=340)
    para('Mid mainly represents content common to both sides; Side represents their differences. They are not a precise separation of a center track from every other track.')
    step(1, 'Listen to M and S first', 'Use M / S under MONITOR, then return to ALL.')
    step(2, 'Adjust each for a reason', 'For steadier center dynamics, start with M Ratio / Mix. Treat S cautiously when preserving ambience and width.')
    note('Recheck the full mix', 'Compression or level changes in M/S can alter the sense of space. In ALL, check center, width and mono compatibility, not only soloed domains.')
    start('Channel LINK: adjust together, keep differences')
    para('In normal LR / MS, LINK beside Mode connects matching controls across channels. Set the channels separately, then enable it to adjust them together without forcing equal values.')
    picture('manual-light-LR-link.png', 'SINGLE example: channel LINK beside Mode is on, while the left and right Ratios remain at 3:1 and 5:1.', maxh=110)
    table(['Parameter', 'Before', 'After an equal change'], [['Ratio', '3:1 / 5:1', '4:1 / 6:1'], ['Threshold', '-20 / -10 dB', '-18 / -8 dB'], ['Makeup', '-3 / +1 dB', '-2 / +2 dB'], ['Mix', '100% / 70%', '90% / 60%']], [0.26, 0.36, 0.38])
    step(1, 'Set the relationship with LINK off', 'Decide which domain needs more compression or a different blend.')
    step(2, 'Enable LINK for joint adjustments', "Adjust either channel's control and the other follows by the same amount. In Dual, toggling UP ON synchronizes the other channel's UP ON; DOWN ON works the same way. Upward and downward switches remain independent.")
    note('Linked controls stop together at their limits', 'If either side would exceed its range, both stop where the difference can be retained. In Super, one Threshold at -inf cannot form a finite dB offset; set both thresholds to finite values first if you want that relationship.')
    heading('Three LINK controls, three roles')
    para('The small LINK between the Ratios links Up / Down within each channel. LINK beside Mode links matching L/R or M/S parameters. With both enabled, channels move together while each channel retains its existing Up / Down relationship. LINK beside Input only controls opposite input/output gain changes; see page 12.')
    start('Sidechain: start with the panel', 'Click SC: INT / SC: EXT at the top right. Its label shows the current source.')
    picture('manual-sidechain-panel.png', maxh=185, width=415)
    table(['Control', 'How to use it'], [['INT / EXT', 'INT follows the current track itself. EXT uses a signal sent from another track to control boost or attenuation on the current track.'], ['KEY GAIN', 'Available only with EXT. Adjusts the external detector level and trigger strength, not the main track level.'], ['HPF', 'Reduces low frequencies in the INT or EXT detector key. OFF is full-band; active range is 20-500 Hz.'], ['KEY LEVEL', 'Check that the detector receives a signal. With EXT showing N/A, first check that the host sidechain bus is enabled.'], ['SC LISTEN', 'Temporarily audition the detector key to check routing and HPF. Switch it off before judging normal output.']], [0.24, 0.76], 9.4)
    note('Two common tasks', 'If bass content compresses everything, try INT + HPF; see page 24.<br/>To make bass duck when the kick arrives, use EXT; see page 25.')
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
    para('There are no conventional Attack / Release controls. Ducking depends on the trigger material, Lookahead, SAFE, Window and compression settings. It is not necessarily smoother than conventional sidechain compression on every source.', 9.3)
    start('Apply it in a real mix')
    heading('Vocals, guitar, piano: consistency with articulation')
    para('Try SINGLE / ST / INT and 26 ms. Raise Ratio above 1:1, then use Mix to keep consonants, plucks or hammer attacks natural. To process only louder parts, raise Threshold and leave Range OFF. Use Match and compare in the mix.')
    para('If clean guitar or vocals containing delay / reverb produce continuous crackles, enable SAFE with the full Window, then shorten Window gradually and compare; see page 19.', 9.3, 8)
    heading('Tails and quiet phrases: bring detail forward')
    para('To lift quiet syllables, plucked-note tails or ambience, try upward Single compression. Use Threshold to exclude tiny sounds you do not want to lift, and Range to exclude louder parts. To control loud parts too, switch to Dual and adjust Up and Down separately. Use their independent ON / OFF switches to compare; disable Ratio LINK for independent ratio adjustments.')
    heading('Mix bus / mastering: a more cohesive balance')
    para('When you want a steadier, more cohesive mix without deliberately reshaping drum attacks through conventional Attack / Release, try it as an alternative to G Bus-style compression. Start gently, compare balance across sections and lower Mix when needed.')
    para('If every kick pulls the whole mix down, try INT + HPF. Do not raise the cutoff merely to make the GR number smaller. At similar loudness, check drum attack, low end, space and the song’s larger dynamic movement.')
    heading('Vocal-keyed accompaniment: make space for words')
    para('Insert the plugin on the backing bus, send the vocal to its external sidechain and choose EXT. Confirm the source with SC LISTEN. Select SINGLE, Ratio above 1:1 and Range OFF, then adjust Key Gain, Threshold and Mix so the backing steps back while the vocal plays.')
    para('If plosives trigger too much ducking, try raising sidechain HPF. If breaths or noise also make the accompaniment dip, check the sent signal and Threshold; HPF alone will not solve every unwanted trigger.')
    note('For every comparison', 'Turn off SC LISTEN. In normal LR / MS, return MONITOR to ALL. Check that before/after loudness is similar. For final peak control, enable Limiter and set Ceiling; use headphones and Match for level-matched listening, as on pages 28-34.')
    start('Limiter L/R Link: connect left and right limiting', 'One ST Display and one set of controls, with independent left/right limiting.')
    picture('manual-limiter-light.png', 'The first row at the lower right is numeric L/R Link. Limiter has no ST / LR / MS selector or channel Monitor.', maxh=200)
    table(['L/R Link', 'Response'], [['0% (default / Alt reset)', 'Each channel detects and controls its own attenuation. A loud event on one side does not attenuate the other merely through linking.'], ['Between 0% and 100%', 'Gradually increase shared attenuation; choose the balance between independent dynamics and stereo stability by ear.'], ['100%', 'Apply the stronger attenuation to both channels to reduce image movement caused by different gain changes.']], [0.3, 0.7], 9.5)
    para('L/R Link applies to dynamics and final Ceiling. Upward gain is not transferred to the other channel merely by linking. Double-click to type a percentage, drag vertically, or hold Shift for fine adjustment. Alt-click resets to 0%.', 9.4)
    heading('Loudness, detail and stereo stability')
    para('Start at 0% for maximum loudness. Increase L/R Link when stereo detail and image stability matter more. The channels then share more attenuation; this does not restore detail already lost to compression. Compare at similar loudness.', 9.4)
    heading('Shared settings do not mean identical detection')
    para('Ratio, Makeup, Mix and thresholds have one shared set of controls. Both channels use these settings. Dual UP / DOWN ratios remain independent, with no Ratio LINK. LINK beside Output still connects the downward threshold, Makeup and output compensation; it is separate from the audio linking amount here.', 9.5)
    note('Opening older Limiter sessions', 'Old ST settings migrate to 100% L/R Link; old LR / MS to 0%. Saved thresholds are retained and old Range no longer processes Limiter audio. Values already changed to -45 dB by an earlier version cannot be reconstructed automatically. The former L set (M in MS) supplies shared controls; old MS becomes left/right processing. Sessions with unequal channel settings may sound different: check and listen. Normal LR / MS retains its previous behavior.')
    start('Limiter workflow: downward compression, link and Window', 'Set downward compression first, then balance loudness, dynamics and stereo detail.')
    picture('manual-limiter-light.png', 'One ST display and shared controls. Scale selects -30 / -60 / -90 dB and the fader scale follows. Limiter has no Range.', maxh=205)
    step(1, 'Start with downward SINGLE compression', 'Begin with Ratio at 20:1 or more, up to 1000:1, and Mix at 100%. Set Ceiling and TP; keep LINK beside Output enabled. Lower Threshold gradually: Output compensates by the same number of dB in the opposite direction.')
    step(2, 'Choose the stereo relationship with L/R Link', 'For greater loudness, start at the default 0% for independent limiting. Increase it for greater stereo detail and image stability. More attenuation is shared, which can also affect loudness.')
    step(3, 'Balance loudness and dynamics with Window', 'Normal compression can usually retain a full Window. In Limiter, Window is especially important for loudness and dynamics. Keep Lookahead fixed and shorten Window gradually. At low values, especially below 10 ms, use oversampling.')
    step(4, 'If Window 0 sounds best, try 0 ms Lookahead', 'With other parameters and OS equal, both use Window = 0 ms detection. Choose the sound at the existing latency first, then select 0 ms to reduce base latency. Allow the host to update compensation when changing Lookahead; see page 34.')
    note('Distinguish the two links', 'L/R Link controls shared left/right attenuation. Output LINK connects the downward threshold, Makeup and output compensation. After these adjustments, consider adding a little UP, then compare at matched loudness with headphones, Match and Bypass.')
    start('Limiter Dual: add gentle upward compression', 'Use Dual to add compensation after setting the standard downward compression.')
    picture('manual-limiter-dual-gentle.png', 'Example: DOWN remains 100:1, UP is 1:1.2 and Output LINK is on. Up and Down ratios are independent; there is no Ratio LINK.', maxh=205)
    step(1, "Complete the previous page's downward setup", 'Use a Ratio of 20:1-1000:1, Threshold and Output LINK to set the desired Limiter compression. Note the algorithm and Ratio.')
    step(2, 'Switch to DUAL and retain the downward setup', 'Single Threshold carries over to the DOWN threshold. DOWN RATIO and Single Ratio are stored separately: check the DOWN algorithm and Ratio, and set them to your Single values if needed to retain the compression you established.')
    step(3, 'Add only a little UP', 'Keep Output LINK enabled. Limiter has no Ratio LINK, so adjusting UP does not change the DOWN ratio. Enable UP and move gently from 1:1 toward 1:1.2; this small range is often enough. Use the UP threshold to choose the audio to lift.')
    para('UP Threshold is independent of Output Gain and Makeup, even with Output LINK on. Scale offers -30 / -60 / -90 dB in both modes. UP cannot exceed DOWN; changing Scale does not rewrite thresholds.', 9.2, 8)
    note('Listen closely to kick and snare sustain', 'Upward compression can raise kick and snare tails, making them more prominent. Listen beyond attacks and loudness: check whether sustain fills the spaces between hits or makes the rhythm feel sluggish.')
    heading('If Dual does not work well, try segmented compression')
    para('Use gentle upward compression to add detail to the established downward setup. Keep the original DOWN RATIO, then fine-tune if needed. If adding UP does not improve the result, reduce or disable it and try segmented compression to control dynamics; this can work better. Decide whether to use Mix after settling these choices.', 9.6)
    start('Ceiling and TP: choose the final limit')
    picture('manual-limiter-light.png', 'Ceiling, recovery and TP sit below Output. The unit is dBTP with TP enabled.', maxh=174, width=160, crop=(1700, 1230, 336, 356))
    table(['Setting', 'Meaning'], [['Ceiling', '-24 to 0 dB; default 0. Sets the reference upper limit for final peak protection.'], ['TP on', 'Controls true peaks with reconstruction detection and gain recovery. All four overall OS rates are available.'], ['TP off', 'Hard Clip: native-rate clipping at 1x, or clipping at the selected 4x / 8x / 16x audio rate.'], ['Overall OS', 'Dynamics and Ceiling share one audio rate; there is no separate Ceiling OS setting.']], [0.25, 0.75], 9.5)
    para('TP retains a small reconstruction margin, so readings need not sit exactly on the line. Hard Clip has different recovery and distortion behavior. With TP off, inter-sample peaks can still exceed the dBFS ceiling; check the actual TP reading.', 9.6)
    heading('Editing the value')
    para('Double-click Ceiling to type a value; drag vertically, hold Shift for fine adjustment, or Alt-click to reset to 0. Undo and redo are supported. Recovery mode affects TP only; see page 41.', 9.5)
    note('TP is available at 1x', 'At overall OS 1x, audio stays at its native rate while TP uses a separate reconstruction detector. It does not silently switch all audio to 8x. Final peak protection and latency still follow the active path; see page 40.')
    start('TP meter: measure output and hold peaks')
    picture('manual-limiter-tp-meter.png', 'The TP L/R readout measures true peaks in the final left/right stereo output.', maxh=151, width=280)
    heading('Measured after Ceiling')
    para('The TP meter reads actual output, including Makeup, Mix, Output Gain, Ceiling and headphone 1:1 compensation. It does not simply display the Ceiling setting, or clamp its reading to that setting.')
    heading('Hold and reset')
    step(1, 'Play the passage to check', 'The peak reading holds for about 20 seconds. A new higher peak updates the value and restarts the timer. Hold advances with audio playback time.')
    step(2, 'Start a fresh measurement', 'Double-click the TP readout to clear it immediately, then play the passage again. There is no need to wait for the previous held peak to expire.')
    step(3, 'Measure again after changing Ceiling', 'A held peak may come from earlier settings. Clear it before deciding whether the current limit behaves as expected.')
    table(['What you see', 'How it works'], [['TP exceeds Ceiling with TP off', 'The meter still measures true peaks. Sample-peak protection does not also guarantee the intersample peak limit.'], ['TP changes with headphones on', 'The readout includes listening compensation. Judge it against the converted MON limit.'], ['Normal MS domain meters differ from TP', 'TP measures final L/R. Individual M/S levels and left/right output limits are different quantities.']], [0.38, 0.62], 9.2)
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
    step(1, 'Compare after setting up the Limiter', 'Set downward compression as on page 28, or Dual compensation as on page 29, then enable headphones. Keep Ceiling and TP fixed, and turn off SC LISTEN. Limiter always outputs full stereo.')
    step(2, 'Play the same representative passage', 'Enable headphones before starting playback, then wait for valid measurements. You can click Match during playback. To measure only new settings, stop and restart playback.')
    step(3, 'Click Match to compensate Makeup', 'Headphone mode compares the original input with the actual output after Ceiling and headphone compensation. Actual left/right results determine one shared compensation amount, written to the single Makeup control. Dry/wet mixing is included.')
    step(4, 'Compare the original with plugin Bypass', 'Bypass returns delay-aligned original input without limiting or headphone gain compensation. Listen for attack, tails, density and groove; trim Makeup if needed.')
    step(5, 'Finish listening and restore normal output', 'Switch headphones off and keep the adjusted settings. Check TP and Ceiling again before judging the final signal sent downstream.')
    heading('Match applies one correction, not continuous gain riding')
    para('Further clicks still use everything since this playback started, without clearing data or stacking compensation. To compare revised settings alone, stop and replay before Match. Compensation can be limited when Limiter LINK is off, controls reach their limits, or Mix is fully dry.', 9.5)
    note('When comparing A/B', 'Play the same material for A and B, then Match each. Headphone state is independent of A/B; Limiter / TP / Ceiling, rates and recovery are recalled with A/B. Keep Lookahead and rates alike where possible to reduce latency changes.')
    start('Limiter: compare Window, then choose 0 ms', 'Window changes detection length; Lookahead sets base latency.')
    heading('Keep Lookahead fixed while comparing Window')
    para('After setting downward compression and L/R Link, retain Lookahead 26 ms and shorten Window from 26 ms. Reported latency stays unchanged, avoiding pauses from compensation changes while you compare loudness, attacks, tails and contrasts between sections.')
    para('With SAFE off, shorter Window increases local dynamics and coloration, but may also add roughness or crackles. With SAFE on, it reduces attenuation around loud events while weakening protection. Most normal compression needs little adjustment; Limiter rewards careful comparison.', 9.6)
    note('If Window = 0 ms sounds best', 'You can also select Lookahead 0 ms. With other parameters and OS equal, delay aligned and switching transitions excluded, both use the same sample-by-sample, zero-Window detection. The original setting retains Lookahead latency; 0 ms removes that part. Do not subtract two unaligned renders.')
    heading('Loudness and dynamics at 0 ms')
    para('Limiter at 0 ms can increase loudness while retaining contrasts between sections. Results depend on the material, algorithm and amount. Listen to both choruses and quiet sections, rather than judging loudness alone. With Window 0, SAFE and Normal detect identically.', 9.6)
    heading('Use oversampling and compare TP recovery')
    para('At low Window values, especially below 10 ms, and when using Limiter Ceiling, compare 4x / 8x / 16x. Oversampling reduces aliasing, not all harmonics. With TP on, compare TIGHT, AUTO and SMOOTH recovery; see page 41.', 9.6)
    note('Lookahead is not the total plug-in latency', "Ceiling adds two approximately 3 ms lookahead stages plus reconstruction alignment. OS filters can also add latency. Normal mode does not carry Ceiling's extra delay, so toggling Limiter may make the host adjust compensation even at the same Lookahead. Window adjustments do not cause this latency switch.")
    start('Quick reference: algorithms and dynamics')
    table(['Parameter', 'Range / choices', 'Notes'], [['Algorithm', 'Classic / Super', 'One choice in Single; separate UP / DOWN choices in Dual.'], ['SINGLE / DUAL', 'Single / Dual', 'Limiter switching preserves downward-threshold continuity; see page 39.'], ['Single Ratio', 'Normal 1:200 to 200:1; Limiter 1:8 to 1000:1', 'Default / reset: 1:1. Standard Limiter use: 20:1 to 1000:1.'], ['Up Ratio', 'Normal 1:200 to 1:1; Limiter 1:8 to 1:1', 'Default 1:1. Start gentle Limiter upward compensation between 1:1 and 1:1.2.'], ['Down Ratio', 'Normal 1:1 to 200:1; Limiter 1:1 to 1000:1', 'Default 1:1. Check the established downward Ratio when entering Limiter Dual.'], ['UP / DOWN switches', 'ON / OFF', 'Both ON by default; audition independently while retaining Ratios.'], ['Threshold / UP THR', 'Classic: -90 to 0 dB; Super: -inf to 0 dB', 'Same in Normal and Limiter; visible scale follows Scale.'], ['Range', 'Level or OFF', 'Normal Single: no compression at or above Range. OFF disables this rule. Not used in Limiter.'], ['DOWN THR', 'Minimum follows its algorithm', 'Visible scale follows Scale; DOWN cannot be below UP.'], ['Ratio LINK', 'Normal DUAL: OFF / ON', 'Link UP / DOWN ratios inversely; initially ON. Not available in Limiter.'], ['Mode / channel LINK', 'Normal: ST / MS / LR; OFF / ON', 'Limiter uses L/R Link 0-100%; see page 27.']], [0.23, 0.36, 0.41], 9.1)
    note('Fader order and defaults', 'Range cannot be below Threshold; DOWN cannot be below UP. Faders move together when they meet; coincident thresholds stop dynamic processing. New Limiter instances use Single Threshold / Dual DOWN at 0 dB, Ratio 1:1, and UP at the minimum of its selected algorithm. Saved thresholds are retained when Scale is reduced.')
    para("Super's lowest numeric threshold is -119.99 dB; below that is -inf (the Single lower bound may show OFF). Scale defaults to -90 dB and offers -30 / -60 / -90 dB. Values outside the view are retained. Range = 0 dB stops compression at 0 dB; OFF disables this Range behavior.", 9.3)
    start('Quick reference: gain, monitoring and SC')
    table(['Parameter', 'Range / choices', 'Notes'], [['Input Gain', 'Normal / Limiter: ±24 dB', 'Each mode retains its own input setting.'], ['Makeup', 'Normal ±30 dB; Limiter ±120 dB', 'Default 0; between Wet and Mix.'], ['Mix', '0 to 100%', 'Default 100%; blends the dynamics processing.'], ['Output Gain', 'Normal +/-24 dB; Limiter +/-120 dB', 'Headphones cancel the direct monitoring gain from Limiter Output.'], ['Input / Output LINK', 'OFF / ON', 'Equal-and-opposite gain linking in normal mode.'], ['MONITOR', 'ALL / L / R or M / S', 'Normal LR / MS only; no channel monitoring in Limiter.'], ['Lookahead', '0 / 26 / 40 / 80 / 100 ms', 'Initially 26 ms; manual changes reset to a full Window.'], ['Window', '0 to current Lookahead, in ms', 'Default / Alt: maximum. Adjust detection length at fixed latency.'], ['SAFE', 'Default OFF', 'Choose detector strategy; disabled at 0 ms Lookahead.'], ['Overall OS', '1x / 4x / 8x / 16x', 'Active at every Lookahead setting; initially 8x.'], ['SOURCE / KEY GAIN', 'INT / EXT；±24 dB', 'KEY GAIN is adjustable only in EXT.'], ['HPF / SC LISTEN', 'OFF / 20-500 Hz；OFF / ON', 'Filters detection only. Listening ends when the panel or editor closes.'], ['Limiter / TP', 'Defaults: OFF / ON', 'Uses Hard Clip when TP is off.'], ['Ceiling', '-24 to 0 dB', 'Default 0 dB. Uses overall OS; TP is also available at 1x.'], ['TP recovery', 'TIGHT / AUTO / SMOOTH', 'Default AUTO; inactive with TP off.'], ['Headphone 1:1', 'Default OFF', 'MON = Ceiling - Output; saved in projects, independent of A/B.'], ['FULL / ECO', 'New instances recall the last manual choice', 'Per instance; saved in projects, independent of A/B.']], [0.23, 0.35, 0.42], 8.6)
    para('Normal compression and Limiter retain separate level, detection and timing settings; see page 39. ECO stop/hidden-editor behavior and FULL near-silence sleep are covered on pages 43-44.', 9.3)
    start('Upward / Dual troubleshooting')
    table(['Symptom', 'First check'], [['No audible boost after adjustment', 'Check that the relevant Ratio is below 1:1, Mix is above 0 and Bypass is off. In Dual, the relevant upward/downward switch must be ON.'], ['No upward effect in Dual', 'The detector must be above UP and below DOWN. Below UP, the original level is retained; above DOWN, only downward processing applies.'], ['Faders set, but no processing', 'Do Threshold and Range (UP and DOWN in Dual) coincide? This stops all dynamic processing. Separate them and compare.'], ['Loud parts suddenly stop being processed', 'Is Range enabled in normal SINGLE? Compression stops at or above it. Set Range to OFF to keep processing louder audio.'], ['Adjusting Up also changes Down', 'Normal Ratio LINK is initially enabled; disable it for independent adjustment. Limiter ratios are always independent.'], ['LINK will not move further', 'The other control may have reached a limit, stopping both. Reverse the adjustment to continue.'], ['Main track level does not explain triggering', 'Check whether SOURCE is EXT or HPF is changing the detector signal. Threshold decisions follow the detector signal.']], [0.33, 0.67], 9.2)
    heading('Check Dual upward processing alone')
    step(1, 'Keep other variables fixed', 'Select DUAL and turn DOWN off. In normal compression, also disable Ratio LINK. Keep Mix at 100% and all gains at 0 dB; choose a suitable Lookahead.')
    step(2, 'Place the signal between the thresholds', 'For example, set UP to -50 dB and DOWN to -20 dB. Check that the detector level of a quiet passage lies mainly between them. With Ratio still at 1:1, there is no boost yet.')
    step(3, 'Increase upward strength from 1:1', 'Move Up Ratio through 1:2, 1:4 and 1:8 while listening and watching Boost / Mix and positive dynamic gain. Once confirmed, enable DOWN to add attenuation and compare overall loudness again.')
    start('Troubleshooting')
    heading('Why can Classic output fall below the threshold?')
    para('Threshold constrains the gain curve at the detector level, not a minimum final output level. Quieter audio within a Window also receives the gain set by its detected peak. Shared ST gain, an external sidechain or HPF can make the detected level differ from the audio you see. Makeup and Output Gain then further change output.', 9.5)
    para('With Range OFF, internal detection, gains at 0 dB and Mix 100%, a steady level above Threshold remains at or above it under the Classic formula. If only the Display dips, remember that it is a reference calculated from current settings; check actual audio before attributing every dip to pre-attenuation.', 9.5)
    table(['Symptom', 'First check'], [['EXT has no effect', 'Enable the host sidechain input and send. Select EXT and check KEY LEVEL. N/A means no external bus is available.'], ['Key Gain is disabled', 'Normal with INT: it adjusts only the external key.'], ['HPF does not cut output bass', 'It filters the detector only. History refreshes after release; wait for HPF UPDATING to clear.'], ['Sidechain sound / narrow image', 'Turn off SC LISTEN; return normal LR / MS MONITOR to ALL. Limiter has no channel monitoring.'], ['Match is unavailable', 'Play enough non-silent audio to establish valid readings. If ECO missed analysis, reopen the editor and restart playback.'], ['Pause after changing Lookahead', 'Latency changes, so the host may readjust compensation. Also consider latency when monitoring a live recording.'], ['Curves disappear below the graph', 'Check Scale: -30 / -60 / -90 dB. Lower audio remains present; * beside a threshold means it is below the visible range.']], [0.3, 0.7], 8.9)
    para('See pages 27-34 for Limiter, TP, Ceiling and headphones, and pages 39-44 for advanced settings. For installation, updates and scanning, use the installation guide for your platform.', 9)
    para('License: Qing Audio Non-Commercial Source-Share License 1.0. Commercial use is prohibited; see the project repository for full terms and corresponding source.', 8.7)
    para('<link href="https://github.com/Ziqing-Gu/QQ-Super-Compression" color="' + TEAL + '">github.com/Ziqing-Gu/QQ-Super-Compression</link>', 9)
    start('Normal and Limiter: separate settings', "Keep each mode's established sound when switching between them.")
    para('Normal compression and Limiter retain separate Input, Makeup, Mix, Output, algorithms, Single / Dual, ratios, thresholds, sidechain, Lookahead, Window, SAFE and overall OS settings. Normal mode also stores ST / LR / MS; Limiter stores numeric L/R Link. First entry into Limiter uses its own defaults, not a copy of normal mode.')
    table(['Adjustment', 'Result'], [['Set normal Input to +3 dB, then enter Limiter', 'Limiter recalls its own Input; normal mode retains +3 dB.'], ['Select Dual and set Ceiling in Limiter', 'Leaving restores normal mode. Re-entering recalls your Limiter setup.'], ['Switch A/B', 'Recalls both mode parameter sets and the active mode saved in that snapshot.']], [0.47, 0.53], 9.7)
    heading('After downward compression, add UP in Dual')
    para('Set downward compression as on page 28, then switch to Dual. Keep the downward algorithm and amount; add only a little UP between 1:1 and 1:1.2. Check kick and snare sustain. If Dual is unsuitable, prefer separate processing of sections; see page 29.', 9.6)
    heading('Switching Single / Dual within Limiter')
    para('Switching to Dual carries Single Threshold over to the DOWN threshold. Switching back to Single carries the current DOWN threshold over to Threshold.')
    para('Entering Dual retains UP. If threshold order conflicts, both are moved to the same position so UP cannot exceed DOWN. Returning to Single still does not enable Range. Ratios are retained separately; Input, Makeup, Mix and Output retain their current Limiter settings.', 9.7)
    note('Example: preserve a manually set threshold', 'Switch from Single Threshold = -18 dB to Dual: DOWN becomes -18 dB. This threshold is not recalculated from Output, even with Output LINK off or with Output and Threshold not equal and opposite.')
    note('Additional Output LINK rules', 'Increasing Makeup by 1 dB decreases Output by 1 dB. Ratio, Mix and algorithm changes do not directly move Output. With downward processing active, Output moves inversely with Single Threshold or Dual DOWN. Dual UP is independent of Output / Makeup. Scale changes the visible graph and fader scale without rewriting existing thresholds. Ordering and algorithm limits still apply.')
    heading('Instance operating settings')
    para('FULL / ECO, headphone 1:1 and normal LR / MS channel solos are instance controls independent of A/B. Changing FULL / ECO in another instance does not change this one.', 9.8)
    para('Reopening a project restores its saved instance state. Older projects undergo compatible restoration; check the mode, values and host latency before continuing.', 9.6)
    start('When to enable oversampling', 'One OS control covers dynamics processing and Limiter Ceiling.')
    heading('Oversampling is recommended in these three situations')
    table(['Situation', 'Why use OS?'], [['1  Lookahead is 0 ms', 'Rapid gain changes at Window 0 ms can produce more harmonics and aliasing. Compare 4x / 8x first, then increase if needed.'], ['2  Low Window, especially below 10 ms', 'Shorter Window can increase harmonics and modulation coloration with SAFE on or off. Use OS, then choose a rate by sound and CPU load.'], ['3  Limiter is in use', 'Ceiling clipping or limiting produces harmonics; OS reduces their aliasing. Overall OS works with TP on or off.']], [0.34, 0.66], 9.8)
    picture('manual-limiter-os.png', 'OS is on the fourth row at the lower right. Normal compression and Limiter save their own rates.', maxh=103, width=114)
    table(['Rate', 'How to choose'], [['1x', 'No audio oversampling; lighter processing and a useful listening reference.'], ['4x', 'A lower-load starting point. Compare roughness, high frequencies and sustained tails.'], ['8x / 16x', 'Higher processing rates. Default is 8x; choose by sound and available CPU headroom.']], [0.25, 0.75], 9.4)
    para('This is listening guidance, not a claim that distortion disappears at 10 ms. Oversampling mainly reduces aliasing; it does not remove all harmonics or replace SAFE / Window control of crackles and pre-attenuation.', 9.5)
    note('Window retains the selected base latency', 'At Lookahead 26 ms, moving Window from 26 to 0 ms leaves base latency at 26 ms. The host also accounts for OS filters and Limiter protection. After changes that affect latency, let compensation settle before comparing.')
    para("Older sessions' OS setting, once used only at 0 ms, now covers all Lookahead settings. The former Ceiling OS no longer controls a separate rate. At 1x, TP can still use its independent reconstruction detector without upsampling the full audio path.", 9.1)
    start('TP recovery: TIGHT / AUTO / SMOOTH', 'Click the small button between Ceiling and TP to choose how gain recovers after peaks.')
    picture('manual-recovery-auto.png', 'AUTO is shown. All three modes belong to final TP protection.', maxh=90, width=270)
    table(['Mode', 'Character and listening comparison'], [['TIGHT', 'Releases peak attenuation faster, allowing short-term loudness to recover readily. Listen for stronger fluctuations on bass or sustained amplitude modulation.'], ['AUTO', 'The default. Adjusts recovery to signal cycles and recurring changes, balancing isolated peaks and continuous variation.'], ['SMOOTH', 'Slower, more gradual recovery. Compare bass stability and movement after deep limiting; attenuation may persist longer.']], [0.2, 0.8], 10)
    step(1, 'Keep the same passage and ceiling', 'Keep Ceiling, Ratio, Output, Mix and overall OS fixed; change only the recovery mode.')
    step(2, 'Listen to short peaks and sustained bass', 'Short drum hits, sustained bass and repeated level changes can reveal different trade-offs. Do not judge from a single peak.')
    step(3, 'Compare at matched loudness again', 'Recovery affects average loudness. Replay and Match if needed, then compare tails, bass and attacks.')
    note('Same protection setting, different recovery shape', 'These choices do not change host latency at the current rate. They work only with TP on. With TP off, Hard Clip does not use these three recovery envelopes. Recovery mode is saved in projects and A/B.')
    para('FULL near-silence sleep waits for tails and TP recovery to finish; SMOOTH may extend that wait. ECO suspends immediately and cuts tails when the host reports Stop; see page 44.', 9.6)
    start('Output LUFS-I: loudness over a passage', 'The Limiter Display includes OUTPUT / LUFS-I.')
    picture('manual-limiter-lufs.png', 'Accumulated output loudness, with measurement duration at the lower right. This example is not a target value.', maxh=184, width=390)
    para('LUFS-I measures integrated loudness over a passage; TP measures peaks. LUFS-I reads the actual monitoring path, including Ceiling, headphone compensation and the current audition settings.')
    table(['State', 'How it works'], [['READY', 'No measurement accumulated yet. Play the passage you want to assess.'], ['MEASURING', 'Accumulating output loudness from the playback signal.'], ['HOLD', 'Retains the last measurement after stopping.'], ['--.-', 'No valid loudness reading yet. A signal that is too brief or too quiet may not produce one.']], [0.26, 0.74], 9.8)
    para('A new playback start, seek or loop restarts accumulation; stopping holds the result. If the host supplies no transport state, measurement runs continuously while Limiter is enabled.', 9.5)
    heading('Use the same conditions for every comparison')
    para('Play the same material with SC LISTEN and headphones in consistent states. Limiter outputs full stereo and has no channel MONITOR. To judge the normal signal sent downstream, turn headphones and SC LISTEN off.')
    note('It does not automatically set a target loudness', 'LUFS-I is a reading; Match is a one-time loudness adjustment written to Makeup. They have separate measurement purposes: a LUFS-I reading is not a Match completion signal. ECO pauses analysis with the editor hidden; reopening restarts LUFS-I. Match requires complete analysis of the current playback; restart playback if any was missed. See page 13.')
    para('Loudness measurement uses gating, so very low levels may not produce valid LUFS-I. Also listen to attacks, density, tails and placement in the full mix when judging changes.', 9.6)
    start('FULL / ECO: choose for each instance', 'Clicking FULL / ECO at the top changes only the current plugin instance.')
    picture('manual-performance-eco.png', 'The ECO button. Keep an instance in FULL when it needs continuous analysis.', maxh=64, width=156)
    para('The table covers playback, recording and offline rendering. Host Stop behavior is described below and on page 44.', 9.4)
    table(['State', 'Editor visible', 'Editor closed / hidden'], [['FULL', 'Normal processing and analysis', 'Keeps background analysis and live monitoring active.'], ['ECO', 'Normal processing and analysis', 'Pauses editor analysis; audio retains its settings.']], [0.19, 0.34, 0.47], 9.8)
    heading('Ten instances: one FULL, nine ECO')
    para('For example, choose FULL for an instance that must monitor live input while stopped, and ECO for the other nine. Instances are independent, project-saved and unaffected by A/B. New instances recall the last manual choice; saved projects keep their own choices.')
    heading('Reopening the editor in ECO')
    para('Display and LUFS-I measurements resume. If the editor was closed during ECO playback, Match will not treat the missing analysis as a complete record. Keep the editor open, stop and restart playback. Applied Makeup, Output and other settings are retained.')
    note('ECO preserves quality and latency while processing', 'During playback, recording and offline rendering, ECO processes at the current settings. Closing the editor pauses analysis without lowering overall OS or removing compensation. On a known host Stop, it mutes and suspends processing regardless of input noise.')
    heading('At Stop, the modes differ')
    para('When the host explicitly reports Stop, ECO immediately mutes and skips compression, OS and Ceiling, regardless of editor visibility. FULL retains live input monitoring and sleeps only after silence or extremely low residual input and stable internal tails.', 9.8)
    start('Suspension and recovery after Stop', 'ECO follows the DAW transport; FULL retains live monitoring.')
    heading('ECO: Stop suspends processing')
    para('When the host explicitly reports Stop, with neither recording nor offline rendering active, ECO zeros the main output in that block and suspends dynamics, overall OS and Ceiling. Hardware noise or an active sidechain does not prevent suspension; remaining delay and TP tails are cut off. Editor visibility does not change this behavior.')
    para('Use FULL on an instance that must monitor live input while stopped. ECO keeps processing when the host provides no reliable transport state.', 9.7)
    heading('When processing resumes')
    table(['Host state / action', 'Response'], [['Playback or recording starts', 'Full processing resumes in the first callback block.'], ['Offline rendering', 'Full processing continues, even if Play is not flagged.'], ['Switch to FULL while stopped', 'Live processing and monitoring resume in that block.'], ['Silent passage during playback', 'Full processing continues; zero samples do not trigger ECO Stop.']], [0.43, 0.57], 9.6)
    note('No extra wake delay or transport fade', 'Before the resumed block, old audio, filter and Ceiling histories are cleared and current settings applied. Existing Lookahead, oversampling latency and host PDC remain. The plugin adds no Stop/Start crossfade to its main output; leave monitoring fades to the DAW.')
    heading('FULL low-residual sleep and ASIO-Guard')
    para('FULL still processes live input and ordinary noise after Stop. It can suspend main audio work only on a known Stop without recording or offline rendering, when raw main and selected-sidechain peaks are at or below −180 dBFS, the actual gain-aware output has stayed below −160 dBFS for at least one second, and filter, delay and TP recovery are settled. Extremely low residue such as −200 dBFS becomes zero during sleep. Live input or selected-sidechain activity wakes processing in the current block. Extreme gain may conservatively keep FULL awake.', 9.5)
    para("ASIO-Guard reflects the host's overall scheduling; per-plugin CPU time cannot be converted directly to its percentage. To check ECO at Stop, first confirm the host reports Stop. Export alignment still depends on host rendering and latency compensation.", 9.5)
    finish()
    c.save()
    doc = PdfReader(a.output / name)
    assert len(doc.pages) == 44
    txt = '\n'.join((p.extract_text() for p in doc.pages))
    for banned in ('Revision', 'Rev 2', 'Plan A', 'Plan B', 'Plan C', 'JUCE', 'offscreen', 'offscreen', '2.2x', 'normalized', 'cache', 'v1.0.1', 'appendix', 'addendum', 'schema'):
        assert banned not in txt, (lang, banned)
    import json
    (Path(__file__).parent / 'English-layout-audit.json').write_text(json.dumps({'version': '1.3.5', 'language': 'en', 'pages': len(doc.pages), 'minimum_content_baselines_pt': mins, 'status': 'awaiting_visual_review'}, indent=2), encoding='utf-8')
    print(lang, len(doc.pages), 'pages; lowest content baselines:', mins)

assert min(mins) >= 53, [(i+1,y) for i,y in enumerate(mins) if y < 53]
