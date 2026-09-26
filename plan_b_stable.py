"""One-time Stable packaging and immutable Plan B snapshot; refuses existing targets."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil

root = Path(__file__).resolve().parent
outputs = Path(r'D:\Codex\Outputs\QQ Super Compression\1.2.5 Stable')
backup = Path(r'D:\备份文件\Vibe Coding\QQ Super Compression\源代码') / ('QQ Super Compression 1.2.5-PlanB-Stable-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
build = Path(r'D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor\QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3')
installed = Path(r'C:\Program Files\Common Files\VST3\QQ Super Compression.vst3')
juce = Path(r'D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15')
accepted = Path(r'D:\Codex\Workspaces\QQSuperCompression-1.2.5-ThresholdFloor')
logs = {
    'Dynamics.log': Path(r'D:\Codex\Temp\QQSC-1.2.5-Stable-Validation\Dynamics.log'),
    'VST3-reference.log': Path(r'D:\Codex\Temp\QQSC-1.2.5-Stable-Validation\VST3-reference.log'),
    'UI.log': Path(r'D:\Codex\Temp\QQSC-1.2.5-ui-final.log'),
}
expected = 'A88D8781740058C2E74D3C90E880CF59C48313278D7407011F8D2B9DCC014294'
def sha(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest().upper()
def files(p):
    return sorted(x for x in p.rglob('*') if x.is_file())
def records(p):
    return [dict(Path=x.relative_to(p).as_posix(), Bytes=x.stat().st_size, SHA256=sha(x)) for x in files(p)]
def write_json(p, data):
    p.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
def verify(p, items):
    assert len(files(p)) == len(items), f'Count mismatch: {p}'
    for r in items:
        x = p / r['Path']
        assert x.stat().st_size == r['Bytes'] and sha(x) == r['SHA256'], str(x)

assert not outputs.exists() and not backup.exists(), 'Refusing existing destination'
for log in logs.values():
    assert log.read_text(encoding='utf-8-sig').strip().splitlines()[-1].startswith('PASS:'), str(log)
for folder in ('Source', 'Assets', 'tests'):
    assert records(root / folder) == records(accepted / folder), folder
assert sha(root / 'CMakeLists.txt') == sha(accepted / 'CMakeLists.txt')
bundle = records(build)
assert bundle == records(installed), 'Installed bundle differs from accepted build'
assert sha(installed / 'Contents/x86_64-win/QQ Super Compression.vst3') == expected

proof = outputs / 'Verification'
proof.mkdir(parents=True)
(outputs / 'Win').mkdir()
shutil.copytree(build, outputs / 'Win' / build.name)
verify(outputs / 'Win' / build.name, bundle)
for name in ('STABLE_1.2.5.md', 'RELEASE_NOTES_1.2.5.md', 'LICENSE'):
    shutil.copy2(root / name, outputs / name)
for name, p in logs.items():
    shutil.copy2(p, proof / name)
inputs = [dict(Path=p.relative_to(root).as_posix(), SHA256=sha(p)) for folder in ('Source', 'Assets', 'tests') for p in files(root / folder)]
inputs.append(dict(Path='CMakeLists.txt', SHA256=sha(root / 'CMakeLists.txt')))
write_json(proof / 'VALIDATED_INPUTS.json', inputs)

manifest_path = root / 'SOURCE_MANIFEST.sha256'
source_items = [r for r in records(root) if r['Path'] != manifest_path.name]
manifest_path.write_text(''.join(r['SHA256'] + '  ' + r['Path'] + '\n' for r in source_items), encoding='utf-8')
source_items = records(root)
dep_items = records(juce)
shutil.copytree(root, backup / 'Source')
shutil.copytree(juce, backup / 'Dependencies' / 'JUCE-8.0.15')
shutil.copytree(proof, backup / 'Verification')
(backup / 'REPRODUCE.md').write_text('''# Reproduce QQ Super Compression 1.2.5 Stable

Run Source/build-stable.cmd under Windows with Visual Studio 2022 Build Tools.
The script discovers the bundled exact JUCE 8.0.15 dependency and builds in D:/Codex/Temp.
Set QQSC_BUILD_PATH to choose another D-drive build directory.
Source/RELEASE_NOTES_1.2.5.md documents behavior and compatibility boundaries.
The accepted Windows binary is in D:/Codex/Outputs/QQ Super Compression/1.2.5 Stable/Win.
This snapshot is immutable once the external PLAN_B_COMPLETION.json reports COMPLETE.
''', encoding='utf-8')
verify(backup / 'Source', source_items)
verify(backup / 'Dependencies' / 'JUCE-8.0.15', dep_items)
assert records(root) == source_items, 'Source changed during copy'
payload = records(backup)
total_bytes = sum(r['Bytes'] for r in payload)
(backup / 'PLAN_B_MANIFEST.sha256').write_text(''.join(r['SHA256'] + '  ' + r['Path'] + '\n' for r in payload), encoding='utf-8')
report = f'''# Plan B verified — QQ Super Compression 1.2.5 Stable

Source: {root}
Snapshot: {backup}
Payload: {len(payload)} files, {total_bytes} bytes (excludes this report and its manifest).
Source: {len(source_items)} files. Exact JUCE 8.0.15: {len(dep_items)} files.
Every payload file was copied and SHA-256 verified. Source was rechecked after copy.
Production source/assets/tests/CMake are identical to the accepted Classic90 build.
Build/output/system bundle parity: {len(bundle)} files; binary SHA-256 {expected}.
Fresh DSP and installed-VST3 tests passed; unchanged UI inputs retain previous passing UI evidence.
No system replacement was needed. No desktop, remote publication or multiband change.
External completion authority: {proof / 'PLAN_B_COMPLETION.json'}
'''
(backup / 'PLAN_B_VERIFICATION.md').write_text(report, encoding='utf-8')
for item in payload:
    assert sha(backup / item['Path']) == item['SHA256']
all_backup = records(backup)
completion = dict(Status='COMPLETE', Version='1.2.5', Revision='Classic90', Stable=True,
    Source=str(root), Snapshot=str(backup), Output=str(outputs), InstalledBundle=str(installed),
    SourceFiles=len(source_items), DependencyFiles=len(dep_items), PayloadFiles=len(payload), PayloadBytes=total_bytes,
    SnapshotFiles=len(all_backup), SnapshotBytes=sum(r['Bytes'] for r in all_backup),
    SnapshotManifestSHA256=sha(backup / 'PLAN_B_MANIFEST.sha256'), Bundle=bundle,
    BinarySHA256=expected, ProductionUnchanged=True, InstalledParity=True,
    Tests='fresh native DSP and installed-VST3 comparisons; existing unchanged offscreen UI evidence',
    Desktop=None, Publication='Not requested', Multiband='Unchanged', CompletedAt=datetime.now().isoformat())
write_json(proof / 'STABLE.json', completion)
shutil.copy2(backup / 'PLAN_B_VERIFICATION.md', proof / 'PLAN_B_VERIFICATION.md')
write_json(proof / 'PLAN_B_COMPLETION.json', completion)
print(json.dumps(completion, ensure_ascii=False, indent=2))
