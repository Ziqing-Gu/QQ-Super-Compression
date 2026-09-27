from pathlib import Path
import hashlib,json,re
from pypdf import PdfReader
import pdfplumber
from PIL import Image,ImageDraw,ImageFont

HERE=Path(__file__).resolve().parent
PDF=Path('D:/Codex/Outputs/QQ Super Compression/1.2.7 English Manual/QQ Super Compression User Manual English_v1.2.7.pdf')
TEMP=Path('D:/Codex/Temp/QQSCManual127EN')
r=PdfReader(PDF);assert len(r.pages)==len(r.outline)==35
texts=[p.extract_text() for p in r.pages];text='\n'.join(texts)
assert not re.search('[\u4e00-\u9fff]',text),'Untranslated Chinese remains'
for token in ['Threshold=-24 dB','Ratio=4:1','-8 dB','MON = Ceiling - Output Gain','20 seconds','20:1 to 1000:1','8.17 ms','1.2.7']:
    assert token in text,token
for token in ['1.2.6','not a brickwall limiter','another suitable limiter','Theme, all LINK','28 pages']:
    assert token not in text,token
for page,title in [(5,'Classic and Super'),(25,'Limiter: control peaks'),(26,'Limiter: Ratio'),(27,'Ceiling and TP'),(28,'TP meter'),(29,'Headphone 1:1'),(30,'Limiter comparison'),(31,'Limiter tips'),(32,'Quick reference'),(33,'Quick reference')]:
    assert title in texts[page-1],(page,title)
issues=[]
with pdfplumber.open(PDF) as pdf:
    for i,p in enumerate(pdf.pages,1):
        for ch in p.chars:
            if ch['x0']<23 or ch['x1']>p.width-23 or ch['top']<12 or ch['bottom']>p.height-12:
                issues.append([i,ch['text'],ch['x0'],ch['top']])
assert not issues,issues[:10]
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',20)
pages=sorted(TEMP.glob('page-*.png'));assert len(pages)==35,len(pages)
for group in range(7):
    sheet=Image.new('RGB',(960,2100),'#dfe3e5');draw=ImageDraw.Draw(sheet)
    for i,p in enumerate(pages[group*5:(group+1)*5]):
        im=Image.open(p).convert('RGB');im.thumbnail((460,654))
        x=10+(i%2)*480;y=10+(i//2)*700
        sheet.paste(im,(x,y+27));draw.text((x+4,y),p.stem,fill='#253c43',font=font)
    sheet.save(TEMP/f'contact-{group+1}.png')
report={'PDF':str(PDF),'Pages':35,'Bookmarks':35,'Language':'English','MatchedChinesePages':35,
    'Bytes':PDF.stat().st_size,'SHA256':hashlib.sha256(PDF.read_bytes()).hexdigest().upper(),
    'TextBoundsIssues':issues,'RequiredContent':'passed','UntranslatedChinese':False,
    'VisualReview':'pending','RenderedPages':str(TEMP)}
(HERE/'QA-EN.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
(TEMP/'extracted.txt').write_text('\n\n'.join(f'PAGE {i+1}\n'+t for i,t in enumerate(texts)),encoding='utf-8')
print(json.dumps(report,indent=2))
