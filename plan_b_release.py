"""Freeze exact intended GitHub source, JUCE and accepted Windows binary before publication."""
from pathlib import Path
from datetime import datetime
import subprocess,hashlib,json,shutil
ROOT=Path(__file__).resolve().parent
STABLE=ROOT.parent/'QQSuperCompression-1.2.7-Stable'
OUT=Path('D:/Codex/Outputs/QQ Super Compression/1.2.7 Release')
WIN=Path('D:/Codex/Outputs/QQ Super Compression/1.2.7 Stable/Win/QQ Super Compression.vst3')
INST=Path('C:/Program Files/Common Files/VST3/QQ Super Compression.vst3')
DEP=Path('D:/Codex/Temp/JUCE-8.0.15-src/JUCE-8.0.15')
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest().upper()
def records(root):
    return [{'Path':p.relative_to(root).as_posix(),'Bytes':p.stat().st_size,'SHA256':sha(p)} for p in sorted(root.rglob('*')) if p.is_file() and '__pycache__' not in p.parts]
for d in ('Source','Assets'):assert records(ROOT/d)==records(STABLE/d),d+' changed from accepted binary source'
assert records(WIN)==records(INST),'Installed binary differs'
dllsha=sha(WIN/'Contents/x86_64-win/QQ Super Compression.vst3')
assert dllsha=='292F588A9E91FF8EC5BBD609BDFB3E9CD6165DDA1D2B22C2C28427AE2B2DAAC6'
for lang in ('Chinese','English'):
    p=ROOT/f'docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-{lang}.pdf'
    qa=json.loads((ROOT/f'docs/manuals/source-1.2.7/{"QA.json" if lang=="Chinese" else "QA-EN.json"}').read_text(encoding='utf-8-sig'))
    assert qa['SHA256']==sha(p) and qa['VisualReview']=='passed'
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode('utf-8').split('\0')
paths=sorted(set(p for p in paths if p and (ROOT/p).is_file()))
source=[{'Path':p,'Bytes':(ROOT/p).stat().st_size,'SHA256':sha(ROOT/p)} for p in paths]
snapshot=Path('D:/备份文件/Vibe Coding/QQ Super Compression/源代码')/('QQ Super Compression 1.2.7-PlanB-Release-'+datetime.now().strftime('%Y%m%d-%H%M%S'))
assert not snapshot.exists();(snapshot/'Source').mkdir(parents=True)
print('Snapshot: '+str(snapshot),flush=True)
for p in paths:
    target=snapshot/'Source'/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target)
shutil.copytree(DEP,snapshot/'Dependencies/JUCE-8.0.15',ignore=shutil.ignore_patterns('__pycache__'))
shutil.copytree(WIN,snapshot/'Win'/WIN.name)
shutil.copytree(Path('D:/Codex/Outputs/QQ Super Compression/1.2.7 Stable/Verification'),snapshot/'Verification/Accepted-Windows')
assert {x['Path']:(x['Bytes'],x['SHA256']) for x in records(snapshot/'Source')}=={x['Path']:(x['Bytes'],x['SHA256']) for x in source}
assert records(snapshot/'Dependencies/JUCE-8.0.15')==records(DEP)
assert records(snapshot/'Win'/WIN.name)==records(WIN)
payload=records(snapshot)
(snapshot/'PLAN_B_MANIFEST.sha256').write_text(''.join(x['SHA256']+'  '+x['Path']+'\n' for x in payload),encoding='utf-8',newline='\n')
for x in payload:assert sha(snapshot/x['Path'])==x['SHA256']
report={'Status':'COMPLETE','Version':'1.2.7','Source':str(ROOT),'Snapshot':str(snapshot),'SourceFiles':len(source),'SourceBytes':sum(x['Bytes'] for x in source),'PayloadFiles':len(payload),'PayloadBytes':sum(x['Bytes'] for x in payload),'WindowsSHA256':dllsha,'ProductionSourceParity':True,'InstalledParity':True,'AllPayloadHashesVerified':True,'ManualPages':{'Chinese':35,'English':35},'ManualSHA256':{lang:sha(ROOT/f'docs/manuals/QQ-Super-Compression-1.2.7-User-Manual-{lang}.pdf') for lang in ('Chinese','English')},'ManifestSHA256':sha(snapshot/'PLAN_B_MANIFEST.sha256'),'CompletedAt':datetime.now().isoformat()}
(snapshot/'PLAN_B_VERIFICATION.md').write_text('# Plan B - 1.2.7 release source\n\n'+json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
report['SnapshotFiles']=len(records(snapshot));report['SnapshotBytes']=sum(x['Bytes'] for x in records(snapshot))
(OUT/'Verification').mkdir(parents=True,exist_ok=True)
(OUT/'Verification/PLAN_B_COMPLETION.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2),flush=True)
