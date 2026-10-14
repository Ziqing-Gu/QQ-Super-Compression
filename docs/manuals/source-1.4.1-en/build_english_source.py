from pathlib import Path
import ast,json,re
base=Path(__file__).resolve().parents[1]
out=base/'source-1.4.1-en'
source=(base/'source-1.4.1-zh/build_manual_1_4_1_zh.py').read_text(encoding='utf-8-sig')
mapping=json.loads((out/'inherited_translations.json').read_text(encoding='utf-8'))
items=json.loads((out/'translation_items.json').read_text(encoding='utf-8'))
new=json.loads((out/'new_translations.json').read_text(encoding='utf-8'))
assert set(new)=={str(x['id']) for x in items}
mapping.update({x['zh']:new[str(x['id'])] for x in items})
mapping.update({
 'QQ Super Compression 用户手册 中文版_v1.4.1.pdf':'QQ Super Compression User Manual English_v1.4.1.pdf',
 'QQ Super Compression 1.4.1 用户手册':'QQ Super Compression 1.4.1 User Manual',
 'ST Link、动态处理、Clipper、Limiter 与侧链使用指南':'ST Link, dynamics, Clipper, Limiter and sidechain guide',
 'QQ Super Compression  ·  用户手册':'QQ Super Compression  ·  User Manual',
 '检测电平由低到高（作用区间示意）':'Detected level: low to high (processing regions)',
 '输出 dB':'Output dB','检测电平 dB':'Detected level dB',
 '静态曲线示意：Threshold=-24 dB、Ratio=4:1，阈值以上继续压缩；未加补偿，未计电平过渡与检测时序。':'Static curves: Threshold -24 dB, Ratio 4:1, processing above threshold; no gain compensation, level transitions or detector timing.',
})
source=source.replace('Chinese 1.4.1 manual matching the user-approved Stable version.','English 1.4.1 manual matching the user-approved Stable version.')
source=source.replace("choices=('zh',),default='zh'","choices=('en',),default='en'")
source=source.replace("help='Chinese review edition.'","help='English review edition matching Chinese 1.4.1.'")
source=source.replace("for lang in ('zh',):","for lang in ('en',):")
source=source.replace("'language':'zh'","'language':'en'").replace('Chinese-layout-audit.json','English-layout-audit.json')
source=source.replace('leading=size*1.52','leading=size*1.38')
source=source.replace("maxh=145)","maxh=137)")
source=source.replace("    def p_obj(text,size=10,colour=INK,boldface=False):\n", "    def p_obj(text,size=10,colour=INK,boldface=False):\n        if page == 15: size *= .93\n        if page in (16,32,44): size *= .98\n")
class Translate(ast.NodeTransformer):
    def visit_Constant(self,node):
        if isinstance(node.value,str):
            value=mapping.get(node.value,node.value)
            value={
                'Lookahead and Window: separate delay from sound':'Lookahead and Window: latency and tone',
                'Limiter workflow: downward compression, link and Window':'Limiter: compression, Link and Window',
                'Limiter comparison: headphone + Match + Bypass':'Limiter: headphones, Match and Bypass',
            }.get(value,value)
            value=value.replace('；','; ').replace('，',', ').replace('：',': ').replace('。','.').replace('、',' / ').replace('（',' (').replace('）',')')
            for dash in ('\u2010','\u2011','\u2012','\u2013','\u2014','\u2212'):
                value=value.replace(dash,'-')
            node.value=value
        return node
tree=Translate().visit(ast.parse(source));ast.fix_missing_locations(tree)
remaining=sorted({n.value for n in ast.walk(tree) if isinstance(n,ast.Constant) and isinstance(n.value,str) and re.search('[\u4e00-\u9fff]',n.value)})
# These literals are negative checks, not displayed text.
assert set(remaining)<= {'离屏','归一化','缓存','附页'},remaining
code=ast.unparse(tree)+'\n'
(out/'build_manual_1_4_1_en.py').write_text(code,encoding='utf-8')
(out/'translation-coverage.json').write_text(json.dumps({'version':'1.4.1','pages':49,'new_strings':len(new),'untranslated_user_text':0,'source':'../source-1.4.1-zh/build_manual_1_4_1_zh.py'},indent=2),encoding='utf-8')
print('English source ready:',len(new),'new translations; no untranslated visible strings.')
