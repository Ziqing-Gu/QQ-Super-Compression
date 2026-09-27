from pathlib import Path
import hashlib,json,re
from pypdf import PdfReader
import pdfplumber
from PIL import Image,ImageDraw,ImageFont

HERE=Path(__file__).resolve().parent
PDF=Path('D:/Codex/Outputs/QQ Super Compression/1.2.7 中文说明书/QQ Super Compression 用户手册 中文版_v1.2.7.pdf')
TEMP=Path('D:/Codex/Temp/QQSCManual127')
r=PdfReader(PDF);assert len(r.pages)==len(r.outline)==35
texts=[p.extract_text() for p in r.pages];alltext='\n'.join(texts)
for token in ['Threshold=-24 dB','Ratio=4:1','-8 dB','MON = Ceiling - Output Gain','20 秒','20:1 至 1000:1','耳机','Limiter','8.17 ms']:
    assert token in alltext,token
for token in ['本插件不是砖墙限幅器','另用合适的限幅工具','各类 LINK','QQ Super Compression 1.2.6','Band Output']:
    assert token not in alltext,token
for page,title in [(5,'Classic 与 Super'),(25,'Limiter：'),(26,'Limiter 的 Ratio'),(27,'Ceiling 与 TP'),(28,'TP 读数'),(29,'耳机 1:1'),(30,'Limiter 等响比较'),(31,'Limiter 使用提示'),(32,'参数速查：算法'),(33,'参数速查：音量'),(35,'遇到问题')]:
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
    subset=pages[group*5:(group+1)*5]
    sheet=Image.new('RGB',(960,2100),'#dfe3e5');draw=ImageDraw.Draw(sheet)
    for i,p in enumerate(subset):
        im=Image.open(p).convert('RGB');im.thumbnail((460,654))
        x=10+(i%2)*480;y=10+(i//2)*700
        sheet.paste(im,(x,y+27));draw.text((x+4,y),p.stem,fill='#253c43',font=font)
    sheet.save(TEMP/f'contact-{group+1}.png')
baseline=HERE.parent/'baseline-1.2.6/held-curve-Chinese-1.2.6.pdf'
assert hashlib.sha256(baseline.read_bytes()).hexdigest().upper()=='687B23A16F2D928088776E63F4355810238397F6789DD5148354EB06127B57F3'
report={'PDF':str(PDF),'Pages':35,'Bookmarks':35,'ChineseOnly':True,'English':'Not created; awaiting Chinese approval',
    'Bytes':PDF.stat().st_size,'SHA256':hashlib.sha256(PDF.read_bytes()).hexdigest().upper(),
    'TextBoundsIssues':issues,'RequiredContent':'passed','ObsoleteStatements':'removed',
    'OriginalHeldDraftSHA256':'unchanged','VisualReview':'pending','RenderedPages':str(TEMP)}
(HERE/'QA.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
