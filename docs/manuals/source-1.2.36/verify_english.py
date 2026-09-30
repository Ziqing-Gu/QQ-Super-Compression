"""Validate the English edition against the approved Chinese source and baseline."""
from pathlib import Path
from pypdf import PdfReader
import pdfplumber,hashlib,json,re,ast,io,tokenize
ROOT=Path(__file__).resolve().parent.parent
SRC=ROOT/'source-1.2.36'
BASE=json.loads((ROOT/'BASELINE.json').read_text(encoding='utf-8-sig'))
PDF=Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.36 Manuals\QQ Super Compression User Manual English_v1.2.36.pdf')
ZH=PDF.parent/'QQ Super Compression 用户手册 中文版_v1.2.36.pdf'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
assert sha(ZH)=='5A1C4F14E4D85DE3CD3F3BA4D7EB278B46D8C4051350BADBE3BCAF42B359EED8'
prior=Path(r'D:\备份文件\Vibe Coding\QQ Super Compression\说明书\QQSC-1.2.36-Chinese-Review-20261001-005527')
for name in ['build_manual_1_2_36_zh.py','manual_capture_1_2_36.cpp']:
    assert sha(SRC/name)==sha(prior/'source-1.2.36'/name)
for p in (SRC/'assets').rglob('*'):
    if p.is_file():assert sha(p)==sha(prior/'source-1.2.36'/p.relative_to(SRC))
r=PdfReader(PDF);z=PdfReader(ZH)
assert len(r.pages)==len(r.outline)==len(z.pages)==41
texts=[p.extract_text() for p in r.pages];text='\n'.join(texts)
assert not re.search(r'[\u4e00-\u9fff，；、：（）]',text)
for bad in ['1.2.7','1.2.35','centerd','1x / 8x / 16x','\ufffd','\x00','Plan A','Plan B','JUCE','schema']:
    assert bad not in text,repr(bad)
checks={
    17:['1x / 4x / 8x / 16x','0 ms Lookahead','quiet sections','whole song'],
    25:['20:1','1000:1','Output LINK','gradually','0 ms'],
    26:['1:1.2','Ratio LINK','kick and snare','segmented compression'],
    31:['standout feature','chorus very loud','quiet sections','whole song','4x, 8x and 16x','CEILING OS remains independent'],
    32:['1:200','200:1','1:8','1000:1','-119.99'],
    33:['±30','±120','Core oversampling','1x / 4x / 8x / 16x','Default FULL'],
    36:['separate settings','Single Threshold','DOWN','independent of A/B'],
    37:['4x → 8x → 16x','older projects retain','share the audio oversampling path'],
    40:['each instance','one FULL, nine ECO','offline output','does not lower Core OS'],
    41:['current audio block','first block containing sound is not skipped','nonzero','Offline rendering','ASIO-Guard']
}
for n,tokens in checks.items():
    compact=re.sub(r'\s+','',texts[n-1]).lower()
    for token in tokens:assert re.sub(r'\s+','',token).lower() in compact,(n,token)
refs={p.indirect_reference.idnum:i+1 for i,p in enumerate(r.pages)};links=[]
for i,p in enumerate(r.pages,1):
    assert '1.2.36  |  '+str(i) in texts[i-1]
    assert len(texts[i-1])>400
    for annot in p.get('/Annots',[]):
        a=annot.get_object()
        if '/Dest' in a:
            assert a['/Dest'][0].idnum in refs
            links.append({'from':i,'to':refs[a['/Dest'][0].idnum]})
assert [d['to'] for d in links]==[3,5,9,12,15,18,21,25,32,36,38,40]
assert [r.get_destination_page_number(d)+1 for d in r.outline]==list(range(1,42))
page_bounds=[]
with pdfplumber.open(PDF) as doc:
    for i,p in enumerate(doc.pages,1):
        bad=[c for c in p.chars if c['x0']<0 or c['x1']>p.width+.2 or c['top']<0 or c['bottom']>p.height+.2]
        assert not bad,(i,bad[:3])
        body=[c for c in p.chars if 45<c['top']<790]
        page_bounds.append({'page':i,'chars':len(p.chars),'body_lowest_bottom_pt':round(max(c['bottom'] for c in body),2)})
product=Path(BASE['product_source']);rows=BASE['product_source_manifest']
for row in rows:assert sha(product/row['path'])==row['sha256'],row['path']
assert {str(p.relative_to(product)) for p in product.rglob('*') if p.is_file()}=={x['path'] for x in rows}
installed=Path(r'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3\Contents\x86_64-win\QQ Super Compression.vst3')
assert sha(installed)==BASE['installed_binary_sha256']
for x in BASE['manual_baseline']:assert sha(x['path'])==x['sha256']
qa={'version':'1.2.36','language':'English','status':'automated_checks_passed_visual_review_pending','pdf':str(PDF),'pdf_sha256':sha(PDF),'pdf_bytes':PDF.stat().st_size,'pages':41,'outline_entries':41,'internal_links':links,'translated_strings':675,'approved_Chinese_pdf_unchanged':True,'approved_Chinese_pdf_sha256':sha(ZH),'Chinese_builder_unchanged':True,'capture_assets_unchanged':True,'product_source_files_unchanged':len(rows),'installed_binary_unchanged':True,'legacy_manuals_unchanged':True,'text_bounds':page_bounds,'required_content_pages':list(checks),'render_directory':r'D:\Codex\Temp\QQSCManual1236\english-render','host_validation_limit':'Documentation translation only; no new audio, Cubase or offline-export test claimed.'}
(ROOT/'QA-English-1.2.36.json').write_text(json.dumps(qa,ensure_ascii=False,indent=2),encoding='utf-8')
(ROOT/'English-1.2.36-extracted.txt').write_text(text,encoding='utf-8')
print(json.dumps({k:qa[k] for k in ['pages','outline_entries','translated_strings','pdf_bytes','pdf_sha256','approved_Chinese_pdf_unchanged','product_source_files_unchanged','installed_binary_unchanged']},indent=2))
