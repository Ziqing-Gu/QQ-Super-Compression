"""Stage verified output, then make an immutable full Plan B snapshot after installation."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import sys

root=Path(__file__).resolve().parent
outputs=Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.6 Stable')
proof=outputs/'Verification'
build=Path(r'D:\Codex\Temp\QQSC-1.2.6-Stable\QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3')
installed=Path(r'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3')
validation=Path(r'D:\Codex\Temp\QQSC-1.2.6-Stable-Validation')
juce=Path(r'D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15')
logs=('Dynamics.log','Promotion-VST3.log','UI.log')

def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest().upper()
def files(p):return sorted(x for x in p.rglob('*') if x.is_file())
def records(p):return [dict(Path=x.relative_to(p).as_posix(),Bytes=x.stat().st_size,SHA256=sha(x)) for x in files(p)]
def write(p,data):p.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
def verify(p,items):
    assert records(p)==items, f'Copy mismatch: {p}'
def verify_inputs():
    inputs=json.loads((validation/'VALIDATED_INPUTS.json').read_text(encoding='utf-8-sig'))
    for r in inputs:assert sha(root/r['Path'])==r['SHA256'],r['Path']
    return inputs
def verify_logs():
    for name in logs:
        s=(validation/name).read_text(encoding='utf-8-sig').strip()
        assert s.splitlines()[-1].startswith('PASS:'), name

mode=sys.argv[1] if len(sys.argv)==2 else ''
assert mode in ('stage','snapshot'), 'Use stage or snapshot'
inputs=verify_inputs();verify_logs()
bundle=records(build)
assert len(bundle)==2
binary_hash=sha(build/'Contents/x86_64-win/QQ Super Compression.vst3')
if mode=='stage':
    assert not outputs.exists(),'Refusing existing release output'
    proof.mkdir(parents=True)
    shutil.copytree(build,outputs/'Win'/build.name)
    verify(outputs/'Win'/build.name,bundle)
    for name in ('STABLE_1.2.6.md','RELEASE_NOTES_1.2.6.md','LICENSE'):
        shutil.copy2(root/name,outputs/name)
    for p in files(validation):shutil.copy2(p,proof/p.name)
    preview_proof=Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.6 Preview\Verification')
    accepted_dir=proof/'AcceptedPreview'
    accepted_dir.mkdir()
    for name in ('step-results.csv','steady-thd.csv','modulation-results.csv','modulation-summary.json'):
        shutil.copy2(preview_proof/name,accepted_dir/name)
    write(proof/'RELEASE_INPUTS.json',dict(Status='VALIDATED_READY_TO_INSTALL',Version='1.2.6',Source=str(root),BinarySHA256=binary_hash,Bundle=bundle))
    print('PASS: verified 1.2.6 Stable output staged; ready for formal install.')
    sys.exit(0)

assert not (proof/'PLAN_B_COMPLETION.json').exists(),'Plan B already completed; never mutate existing snapshot'
installation=json.loads((proof/'INSTALL_VERIFICATION.json').read_text(encoding='utf-8-sig'))
assert installation['Status']=='INSTALLED_STABLE_PREVIEW_REMOVED'
assert installation['BinarySHA256']==binary_hash
verify(installed,bundle);verify(outputs/'Win'/build.name,bundle)
assert not Path(r'C:\Program Files\Common Files\VST3\QQ Super Compression Preview.vst3').exists()
assert not Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.6 Preview\QQ Super Compression Preview.vst3').exists()
manifest=root/'SOURCE_MANIFEST.sha256'
source_payload=[r for r in records(root) if r['Path']!=manifest.name]
manifest.write_text(''.join(r['SHA256']+'  '+r['Path']+'\n' for r in source_payload),encoding='utf-8')
source_items=records(root);dep_items=records(juce)
backup=Path(r'D:\备份文件\Vibe Coding\QQ Super Compression\源代码')/('QQ Super Compression 1.2.6-PlanB-Stable-'+datetime.now().strftime('%Y%m%d-%H%M%S'))
assert not backup.exists()
shutil.copytree(root,backup/'Source')
shutil.copytree(juce,backup/'Dependencies'/'JUCE-8.0.15')
shutil.copytree(proof,backup/'Verification')
(backup/'REPRODUCE.md').write_text('''# Reproduce QQ Super Compression 1.2.6 Stable

Run Source/build-stable.cmd using Visual Studio 2022 Build Tools on Windows.
The script selects the bundled exact JUCE 8.0.15 dependency and builds in D:/Codex/Temp.
QQSC_BUILD_PATH and QQSC_JUCE_PATH can explicitly select alternative paths.
Read Source/RELEASE_NOTES_1.2.6.md for accepted behavior and measured tradeoffs.
Source contains production, tests, assets, documentation and release/build workflows.
The historical Preview and previous Stable artifacts used for promotion comparisons
are referenced by the verification records; they are not required to build this release.
Only build-stable.cmd, verify-promotion.py, install-stable.ps1 and plan-b-126.py belong
to this release workflow; other versioned scripts are retained historical records.
The snapshot is immutable once the external PLAN_B_COMPLETION.json reports COMPLETE.
''',encoding='utf-8')
verify(backup/'Source',source_items);verify(backup/'Dependencies'/'JUCE-8.0.15',dep_items)
assert records(root)==source_items,'Source changed during snapshot'
verify_inputs()
payload=records(backup)
(backup/'PLAN_B_MANIFEST.sha256').write_text(''.join(r['SHA256']+'  '+r['Path']+'\n' for r in payload),encoding='utf-8')
report=f'''# Plan B verified — QQ Super Compression 1.2.6 Stable

Source: {root}
Snapshot: {backup}
Payload: {len(payload)} files, {sum(r['Bytes'] for r in payload)} bytes, excluding this report and manifest.
Source: {len(source_items)} files. Exact JUCE 8.0.15 dependency: {len(dep_items)} files.
All payload SHA-256 values, file counts and bytes were verified after copying.
Build/output/system bundle parity: {len(bundle)} files; binary SHA-256 {binary_hash}.
Formal identity restored; actual output is bit-identical to accepted Preview in promotion tests.
Fresh native DSP, formal VST3 compatibility/audio and editor checks passed.
Installed Preview and the separate Preview output bundle were removed; source/research evidence retained.
No desktop, remote publication, Mac build or multiband change in this stage.
Completion authority: {proof/'PLAN_B_COMPLETION.json'}
'''
(backup/'PLAN_B_VERIFICATION.md').write_text(report,encoding='utf-8')
for item in payload:assert sha(backup/item['Path'])==item['SHA256']
all_backup=records(backup)
completion=dict(Status='COMPLETE',Version='1.2.6',Stable=True,Source=str(root),Snapshot=str(backup),Output=str(outputs),InstalledBundle=str(installed),
    SourceFiles=len(source_items),DependencyFiles=len(dep_items),PayloadFiles=len(payload),PayloadBytes=sum(r['Bytes'] for r in payload),
    SnapshotFiles=len(all_backup),SnapshotBytes=sum(r['Bytes'] for r in all_backup),SnapshotManifestSHA256=sha(backup/'PLAN_B_MANIFEST.sha256'),
    BinarySHA256=binary_hash,Bundle=bundle,InstalledParity=True,PreviewRemoved=True,AcceptedPreviewAudioParity=True,
    Tests='fresh native DSP; formal/Preview VST3 parity and old state; offscreen UI',Desktop=None,Publication='Not requested',Multiband='Unchanged',CompletedAt=datetime.now().isoformat())
write(proof/'STABLE.json',completion)
shutil.copy2(backup/'PLAN_B_VERIFICATION.md',proof/'PLAN_B_VERIFICATION.md')
write(proof/'PLAN_B_COMPLETION.json',completion)
print(json.dumps(completion,ensure_ascii=False,indent=2))
