"""Verify the production delta, then freeze the exact validation inputs."""
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parent
accepted = Path(r'D:\Codex\Workspaces\QQSuperCompression-1.2.6-Preview')
evidence = Path(r'D:\Codex\Temp\QQSC-1.2.6-Stable-Validation')
evidence.mkdir(parents=True, exist_ok=True)

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest().upper()

changes = []
for folder in ('Source', 'Assets'):
    actual = sorted(p.relative_to(root).as_posix() for p in (root/folder).rglob('*') if p.is_file())
    reference = sorted(p.relative_to(accepted).as_posix() for p in (accepted/folder).rglob('*') if p.is_file())
    assert actual == reference
    for rel in actual:
        new, old = (root/rel).read_bytes(), (accepted/rel).read_bytes()
        if new == old:
            continue
        if rel in ('Source/PluginProcessor.cpp', 'Source/PluginEditor.cpp'):
            normalised = new.replace(b'options.applicationName = "QQSuperCompression";', b'options.applicationName = "QQSuperCompressionPreview";')
            if rel.endswith('PluginEditor.cpp'):
                normalised = normalised.replace(b'title.setText ("QQ Super Compression",', b'title.setText ("QQ Super Compression Preview",')
        elif rel == 'Source/StaticCompressionEngine.h':
            normalised = new.replace(b'// v1.2.6: align the detector', b'// Preview: align the detector')
        else:
            raise AssertionError('Unexpected production change: '+rel)
        # apply_patch may normalise line endings; the only semantic changes
        # permitted are the exact replacements above.
        assert normalised.replace(b'\r\n',b'\n') == old.replace(b'\r\n',b'\n'), rel
        changes.append(rel)
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8-sig')
for token in ('VERSION 1.2.6', 'PLUGIN_CODE Qscp', 'PRODUCT_NAME "QQ Super Compression"', 'BUNDLE_ID "com.qingaudio.qqsupercompression"'):
    assert token in cmake, token
inputs=[]
for folder in ('Source','Assets','tests'):
    for p in sorted((root/folder).rglob('*')):
        if p.is_file(): inputs.append(dict(Path=p.relative_to(root).as_posix(),SHA256=sha(p)))
inputs.append(dict(Path='CMakeLists.txt',SHA256=sha(root/'CMakeLists.txt')))
(evidence/'VALIDATED_INPUTS.json').write_text(json.dumps(inputs,indent=2),encoding='utf-8')
(evidence/'PRODUCTION_DELTA.json').write_text(json.dumps(dict(
    Source=str(root),Accepted=str(accepted),ProductionAudioChanges=False,
    PermittedChanges=changes,FormalIdentity='Qscp',Version='1.2.6',InputFiles=len(inputs)
),indent=2),encoding='utf-8')
print('PASS: accepted Preview audio implementation unchanged; formal name/preferences and comment only; validation inputs frozen.')
