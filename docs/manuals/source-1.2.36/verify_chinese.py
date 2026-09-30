from pathlib import Path
from pypdf import PdfReader
import pdfplumber,hashlib,json,csv,re
root=Path(r'D:\Codex\Workspaces\QQSuperCompression-1.2.36-Manual')
src=root/'source-1.2.36'
pdf=Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.36 Manuals\QQ Super Compression 用户手册 中文版_v1.2.36.pdf')
base=json.loads((root/'BASELINE.json').read_text(encoding='utf-8-sig'))
hashfile=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
r=PdfReader(pdf)
assert len(r.pages)==41
assert len(r.outline)==41
text='\n'.join(p.extract_text() for p in r.pages)
for token in ['1:200','200:1','1:8','1000:1','20:1','1:1.2','底鼓','军鼓','分段压缩','±30','±120','CEILING OS','TIGHT','AUTO','SMOOTH','LUFS-I','FULL / ECO','当前音频块','离线渲染','每个实例各自选择','1x / 4x / 8x / 16x','4x → 8x → 16x','旧工程保留原来的实际倍率']:
    assert token in text,token
for token in ['1.2.7','1.2.35','TP 需有效倍率至少 8x','1x / 8x / 16x','1:1000','下压从 20:1 开始','约 7-8.5 ms','当时采集的最终输出','每次进入 Limiter 都从']:
    assert token not in text,token
# Keep the user-authored 0 ms Limiter feature prominent on all three entry pages.
feature_checks={
    17:['Limiter 的核心特色','0 ms Lookahead','副歌很响','保持安静部分的音量','4x / 8x / 16x'],
    25:['调好下压后，试试 0 ms Lookahead','保留安静部分的音量','4x / 8x / 16x'],
    31:['Limiter 的 0 ms 特色','最大特色，值得一试','不至于把整首歌的音量都抬高很多','4x、8x、16x','核心过采样','CEILING OS']
}
for page_number,tokens in feature_checks.items():
    compact=re.sub(r'\s+','',r.pages[page_number-1].extract_text())
    for token in tokens:assert re.sub(r'\s+','',token) in compact,(page_number,token)
pages=[];outside=[]
with pdfplumber.open(pdf) as doc:
    for i,p in enumerate(doc.pages,1):
        bad=[c for c in p.chars if c['x0']<0 or c['x1']>p.width+.2 or c['top']<0 or c['bottom']>p.height+.2]
        if bad:outside.append(i)
        body=[c for c in p.chars if 45<c['top']<790]
        pages.append({'page':i,'chars':len(p.chars),'body_lowest_bottom_pt':round(max(c['bottom'] for c in body),2)})
assert not outside,outside
refs={p.indirect_reference.idnum:i+1 for i,p in enumerate(r.pages)}
links=[]
for i,p in enumerate(r.pages,1):
    for a in p.get('/Annots',[]):
        a=a.get_object()
        if '/Dest' in a:
            d=a['/Dest'];assert d[0].idnum in refs
            links.append({'from':i,'to':refs[d[0].idnum]})
assert len(links)==12,links
assert [x['to'] for x in links]==[3,5,9,12,15,18,21,25,32,36,38,40]
for i,p in enumerate(r.pages,1):assert '1.2.36  |  '+str(i) in p.extract_text()
assert [r.get_destination_page_number(d)+1 for d in r.outline]==list(range(1,42))
# Documentation work must not change this turn's installed 1.2.36 source baseline.
product=Path(base['product_source'])
source_rows=base['product_source_manifest']
for x in source_rows:assert hashfile(product/x['path'])==x['sha256'],x['path']
assert {str(p.relative_to(product)) for p in product.rglob('*') if p.is_file()}=={x['path'] for x in source_rows}
for x in base['manual_baseline']:assert hashfile(x['path'])==x['sha256']
installed=Path(r'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3\Contents\x86_64-win\QQ Super Compression.vst3')
assert hashfile(installed)==base['installed_binary_sha256']
assets=[{'path':str(p.relative_to(src)),'bytes':p.stat().st_size,'sha256':hashfile(p)} for p in sorted((src/'assets').glob('*.png'))]
qa={'version':'1.2.36','language':'Chinese','status':'automated_checks_passed_visual_review_pending','pdf':str(pdf),'pdf_sha256':hashfile(pdf),'pdf_bytes':pdf.stat().st_size,'pages':41,'outline_entries':41,'internal_links':links,'text_bounds':pages,'outside_page_chars':outside,'current_product_source_files_unchanged':len(source_rows),'installed_binary_unchanged':True,'baseline_Chinese_and_English_unchanged':True,'English_1_2_36_created':(pdf.parent/'QQ Super Compression User Manual English_v1.2.36.pdf').is_file(),'assets':assets,'capture_source':str(src/'manual_capture_1_2_36.cpp'),'capture_source_sha256':hashfile(src/'manual_capture_1_2_36.cpp'),'render_directory':r'D:\Codex\Temp\QQSCManual1236\render','host_validation_limit':'The manual describes callback behavior; no claim of new Cubase GUI/offline-export validation.'}
(root/'QA-Chinese-1.2.36.json').write_text(json.dumps(qa,ensure_ascii=False,indent=2),encoding='utf-8')
(root/'Chinese-1.2.36-extracted.txt').write_text(text,encoding='utf-8')
print(json.dumps({k:qa[k] for k in ['pages','outline_entries','pdf_bytes','pdf_sha256','current_product_source_files_unchanged','installed_binary_unchanged','English_1_2_36_created']},indent=2))
