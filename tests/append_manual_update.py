"""Retain the approved 18-page guide; add a short note in its final blank area."""
import io
import sys
from pathlib import Path
from pypdf import PdfReader, PdfWriter
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph

source, target = map(Path, sys.argv[1:3])
target.mkdir(parents=True, exist_ok=True)
for name, filename in [('CN', 'msyh.ttc'), ('CNBold', 'msyhbd.ttc'), ('Body', 'segoeui.ttf'), ('Bold', 'seguisb.ttf')]:
    pdfmetrics.registerFont(TTFont(name, str(Path('C:/Windows/Fonts') / filename), subfontIndex=0))
for old in source.glob('*1.1.8.pdf'):
    zh = 'English' not in old.name
    reader = PdfReader(old)
    assert len(reader.pages) == 18
    width, height = map(float, (reader.pages[-1].mediabox.width, reader.pages[-1].mediabox.height))
    stream = io.BytesIO()
    c = canvas.Canvas(stream, pagesize=(width, height))
    c.setFillColor(HexColor('#009db5'))
    c.setFont('CNBold' if zh else 'Bold', 12)
    heading = '1.1.9 界面更新' if zh else '1.1.9 interface update'
    c.drawString(48, 268, heading)
    text = ('界面改为 3:2 横向比例，为 Display 留出更宽的空间。可拖动右下角调整大小，下次打开时会记住横向界面的大小。'
            'Light、Dark、Classic 仍在右上角切换，并恢复上一次选择。控件用途、操作方法和声音处理不变；前文的使用说明仍然适用。'
            if zh else
            'The interface now uses a wider 3:2 layout, giving the Display more horizontal space. Drag the lower-right corner to resize; the landscape size is remembered when reopening. '
            'Light, Dark and Classic remain available from the top-right theme button, with the last selection restored. Controls, operation and audio processing are unchanged; the instructions in this guide still apply.')
    p = Paragraph(text, ParagraphStyle('update', fontName='CN' if zh else 'Body', fontSize=10,
                  leading=15.2, textColor=HexColor('#293940'), wordWrap='CJK' if zh else None))
    _, h = p.wrap(width-96, 150)
    assert h <= 110
    p.drawOn(c, 48, 247-h)
    c.save()
    writer = PdfWriter()
    writer.clone_document_from_reader(reader)
    writer.pages[-1].merge_page(PdfReader(stream).pages[0])
    writer.add_metadata({'/Title': 'QQ Super Compression 1.1.9 - '+('用户手册' if zh else 'User Manual'), '/Author': 'Qing Audio'})
    dest = target / old.name.replace('1.1.8', '1.1.9')
    writer.write(dest)
    result = PdfReader(dest)
    assert len(result.pages) == 18
    for i, page in enumerate(reader.pages):
        before = page.extract_text()
        after = result.pages[i].extract_text()
        assert after == before if i < 17 else before in after
    assert heading in result.pages[-1].extract_text()
    print('PASS: 18 original pages retained, final-page UI note added: '+dest.name)
