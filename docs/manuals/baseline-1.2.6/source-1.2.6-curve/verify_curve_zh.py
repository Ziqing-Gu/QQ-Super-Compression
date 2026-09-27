from pathlib import Path
from pypdf import PdfReader
import pdfplumber
import hashlib,json,subprocess
HERE=Path(__file__).resolve().parent
OLD=HERE.parent/'QQ-Super-Compression-1.2.6-User-Manual-Chinese.pdf'
NEW=Path('D:/Codex/Outputs/QQ Super Compression/1.2.6 中文说明书曲线更新/QQ Super Compression 用户手册 中文版_v1.2.6.pdf')
TEMP=Path('D:/Codex/Temp/QQSCManual126Curve');TEMP.mkdir(parents=True,exist_ok=True)
a,b=PdfReader(OLD),PdfReader(NEW)
assert len(a.pages)==len(b.pages)==28
assert len(b.outline)==28
text_changed=[];drawing_changed=[]
for i,(before,after) in enumerate(zip(a.pages,b.pages),1):
 if before.extract_text()!=after.extract_text():text_changed.append(i)
 if before.get_contents().get_data()!=after.get_contents().get_data():drawing_changed.append(i)
assert text_changed==[5],text_changed
assert drawing_changed==[5],drawing_changed
for p,h in json.loads((HERE/'baseline.json').read_text()).items():assert hashlib.sha256(Path(p).read_bytes()).hexdigest()==h
issues=[]
with pdfplumber.open(NEW) as pdf:
 for i,p in enumerate(pdf.pages,1):
  for ch in p.chars:
   if ch['x0']<24 or ch['x1']>p.width-24 or ch['top']<12 or ch['bottom']>p.height-12:issues.append([i,ch['text']])
assert not issues,issues[:10]
t=b.pages[4].extract_text()
for token in ['Threshold=-24 dB','Ratio=4:1','Range=OFF','-8 dB','Output Gain','-90 dB','-inf']:assert token in t,token
assert 'Band Output' not in t
subprocess.run(['pdftoppm','-f','5','-l','5','-r','140','-png','-singlefile',str(NEW),str(TEMP/'page-05')],check=True)
report={'pdf':str(NEW),'pages':28,'bookmarks':28,'text_changed_pages':text_changed,'drawing_changed_pages':drawing_changed,'other_pages':'All 27 unchanged pages have identical decoded PDF drawing streams and extracted text.','baseline':'Original published Chinese/English PDFs and builders unchanged.','bounds_issues':issues,'bytes':NEW.stat().st_size,'sha256':hashlib.sha256(NEW.read_bytes()).hexdigest(),'visual_review':'pending'}
(HERE/'QA.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
