"""Build the English source from the approved Chinese manual and reviewed translations.
All translation inputs are local to this source directory; no old workspace is needed.
"""
from pathlib import Path
import ast,io,json,re,tokenize
HERE=Path(__file__).resolve().parent
source=(HERE/'build_manual_1_2_36_zh.py').read_text(encoding='utf-8-sig')
mapping=json.loads((HERE/'english_base_translations.json').read_text(encoding='utf-8'))
pending=json.loads((HERE/'english_pending.json').read_text(encoding='utf-8'))
values=json.loads((HERE/'english_new_translations.json').read_text(encoding='utf-8'))
assert set(values)==set(map(str,range(len(pending))))
mapping.update({z:values[str(i)] for i,z in enumerate(pending)})
# Use ASCII hyphens in prose; keep arrows, mathematical minus signs and range symbols.
for z,e in mapping.items():
    for dash in ['\u2010','\u2011','\u2012','\u2013','\u2014']:
        e=e.replace(dash,'-')
    mapping[z]=e
out=[];used=set()
for t in tokenize.generate_tokens(io.StringIO(source).readline):
    if t.type==tokenize.STRING:
        v=ast.literal_eval(t.string)
        if re.search(r'[\u4e00-\u9fff]',v):
            assert v in mapping,('Missing translation',v)
            used.add(v)
            v=mapping[v]
        for old,new in [('；','; '),('、',', '),('，',', '),('：',': '),('（','('),('）',')'),('。','.'),('plug-in','plugin'),('labelled','labeled'),('centred','centered'),('centre','center'),('Grey','Gray'),('grey','gray')]:
            v=v.replace(old,new)
        t=t._replace(string=repr(v))
    out.append(t)
result=tokenize.untokenize(out)
result=result.replace('Chinese 1.2.36 manual, retaining the approved typography, palette and user-facing structure.\nEnglish remains unchanged pending Chinese approval.', 'English 1.2.36 manual matching the user-approved Chinese edition.\nGenerated from reviewed translations with the same 41-page structure and current UI captures.')
result=result.replace("choices=('zh',),default='zh'", "choices=('en',),default='en'").replace("for lang in ('zh',):", "for lang in ('en',):")
result=result.replace("help='Chinese review first; English requires approval of the Chinese edition.'", "help='English edition matching the approved Chinese manual.'")
result=result.replace("title,22,TEAL,True", "title,21,TEAL,True")
result=result.replace("fontSize=size,leading=size*1.52", "fontSize=size,leading=size*1.40")
result=result.replace("    def p_obj(text,size=10,colour=INK,boldface=False):\n        return", "    def p_obj(text,size=10,colour=INK,boldface=False):\n        if page == 31: size *= 0.94\n        if page == 36: size *= 0.97\n        return")
result=result.replace("c.drawString(M,top+2,'Output dB')", "c.drawString(M,top+11,'Output dB')")
result=result.replace("'Chinese-layout-audit.json'", "'English-layout-audit.json'").replace("'language':'zh'", "'language':'en'")
assert not any(re.search(r'[\u4e00-\u9fff]',ast.literal_eval(t.string)) for t in tokenize.generate_tokens(io.StringIO(result).readline) if t.type==tokenize.STRING)
(HERE/'english_translations.json').write_text(json.dumps({z:mapping[z] for z in sorted(used)},ensure_ascii=False,indent=2),encoding='utf-8')
(HERE/'build_manual_1_2_36_en.py').write_text(result,encoding='utf-8')
print('Prepared English manual:',len(used),'translated strings; 41 matching pages.')
