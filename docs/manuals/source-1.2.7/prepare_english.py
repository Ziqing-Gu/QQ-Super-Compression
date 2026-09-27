"""Reuse the previous matched bilingual strings, then apply reviewed 1.2.7 translations."""
from pathlib import Path
import ast, json, re, tokenize, io
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'baseline-1.2.6/source-1.2.6'
def strings(s):
    return [t for t in tokenize.generate_tokens(io.StringIO(s).readline) if t.type==tokenize.STRING]
zh=(BASE/'build_manual_1_2_6_zh.py').read_text(encoding='utf-8')
en=(BASE/'build_manual_1_2_6_en.py').read_text(encoding='utf-8')
zs,es=strings(zh),strings(en)
assert len(zs)==len(es),(len(zs),len(es))
mapping={}
for z,e in zip(zs,es):
    zv,ev=ast.literal_eval(z.string),ast.literal_eval(e.string)
    if re.search('[\u0080-\uffff]',zv):mapping[zv]=ev
    elif zv!=ev:
        assert zv in ('zh','zh','English requires approval of the Chinese edition.') or z.start[0]<40,(z.start,zv,ev)
source=(HERE/'build_manual_1_2_7_zh.py').read_text(encoding='utf-8')
missing=[]
for t in strings(source):
    v=ast.literal_eval(t.string)
    if re.search('[\u4e00-\u9fff]',v) and v not in mapping and v not in missing:missing.append(v)
(HERE/'english_pending.json').write_text(json.dumps(missing,ensure_ascii=False,indent=2),encoding='utf-8')
extra=HERE/'english_translations.json'
if not extra.exists():
    for i,v in enumerate(missing):print(str(i)+': '+v)
    print('Missing:',len(missing));raise SystemExit
values=json.loads(extra.read_text(encoding='utf-8'));assert len(values)==len(missing),(len(values),len(missing))
mapping.update(zip(missing,values))
out=[]
for t in tokenize.generate_tokens(io.StringIO(source).readline):
    if t.type==tokenize.STRING:
        v=ast.literal_eval(t.string)
        if v in mapping:t=t._replace(string=repr(mapping[v]))
    out.append(t)
result=tokenize.untokenize(out)
result=result.replace("choices=('zh',),default='zh'", "choices=('en',),default='en'").replace("for lang in ('zh',):", "for lang in ('en',):")
result=result.replace("help='Chinese review first; English requires approval of the Chinese edition.'", "help='English edition matching the approved Chinese manual.'")
result=result.replace("name='QQ Super Compression User Manual English_v1.2.6.pdf'", "name='QQ Super Compression User Manual English_v1.2.7.pdf'")
result=result.replace("title,22,TEAL,True", "title,21,TEAL,True")
result=result.replace("c.drawString(M,top+2,'Output dB')", "c.drawString(M,top+11,'Output dB')")
(HERE/'build_manual_1_2_7_en.py').write_text(result,encoding='utf-8')
print('Prepared English manual matching the approved 35-page Chinese edition.')
