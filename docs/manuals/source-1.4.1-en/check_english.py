from pathlib import Path
import re,json
from PIL import Image,ImageDraw
from pypdf import PdfReader
import pdfplumber
import argparse
parser=argparse.ArgumentParser()
parser.add_argument('--temp',type=Path,required=True)
parser.add_argument('--pdf',type=Path,required=True)
a=parser.parse_args();temp=a.temp;pdf=a.pdf
temp.mkdir(parents=True,exist_ok=True)
reader=PdfReader(pdf);texts=[p.extract_text() or '' for p in reader.pages]
assert len(texts)==49
assert len(reader.outline)==49
assert [reader.get_destination_page_number(d)+1 for d in reader.outline]==list(range(1,50))
page_ids={p.indirect_reference.idnum:i+1 for i,p in enumerate(reader.pages)}
targets=[page_ids[a.get_object()['/Dest'][0].idnum] for a in reader.pages[1]['/Annots']]
assert targets==[3,6,10,13,16,21,23,26,30,32,40,44,46,48]
full='\n'.join(texts)
assert not re.search('[\u4e00-\u9fff]',full)
for forbidden in ('1.3.6','ST → MS → LR','Scale:','Distort','Plan B'):
    assert forbidden not in full,forbidden
assert 'L/R Link' in texts[1] and 'M/S Link' in texts[1]
assert '100%' in texts[20] and '0%' in texts[21]
assert '10:1-1000:1' in texts[29]
assert 'Only Output LINK controls this relationship' in texts[43]
refs=[]
for i,t in enumerate(texts,1):
    for m in re.finditer(r'\b[Pp]ages?\s+(\d+)(?:-(\d+))?',t):
        a,b=int(m[1]),int(m[2] or m[1]);assert 1<=a<=b<=49,(i,m[0]);refs.append([i,a,b])
outside=[]
with pdfplumber.open(pdf) as doc:
    for i,p in enumerate(doc.pages,1):
        for char in p.chars:
            if char['x0']<43 or char['x1']>552 or char['top']<20 or char['bottom']>823:
                outside.append([i,char['text'],char['x0'],char['top'],char['x1'],char['bottom']])
assert not outside,outside[:10]
(temp/'extracted.txt').write_text('\n\n'.join(f'PAGE {i}\n{t}' for i,t in enumerate(texts,1)),encoding='utf-8')
files=sorted((temp/'render').glob('page-*.png'));assert len(files)==49
for start in range(0,49,6):
    sheet=Image.new('RGB',(1260,1230),'#dae0e3');d=ImageDraw.Draw(sheet)
    for n,path in enumerate(files[start:start+6]):
        im=Image.open(path).convert('RGB');im.thumbnail((400,566))
        x=10+(n%3)*420;y=30+(n//3)*610
        sheet.paste(im,(x,y));d.text((x,y-20),f'PAGE {start+n+1}',fill='black')
    sheet.save(temp/f'contact-{start//6+1:02}.png')
report={'pages':49,'bookmarks':49,'contents_links':14,'text_bounds':'pass','terminology':'pass','references':refs,'visual_review':'pending'}
(temp/'qa.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('49 pages; 49 bookmarks; 14 contents targets; text bounds and terminology passed.')
