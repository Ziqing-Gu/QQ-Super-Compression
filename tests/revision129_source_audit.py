#!/usr/bin/env python3
"""Compare this source tree to the supplied 1.2.8 ZIP/tree. No builds or writes."""
from __future__ import annotations
import argparse, hashlib, json, re, zipfile
from pathlib import Path
from limiter_mode_continuity_isolated_test import function

def digest(data:bytes)->str:return hashlib.sha256(data).hexdigest().upper()
def main()->int:
    a=argparse.ArgumentParser();a.add_argument('--baseline',type=Path,required=True);a.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]);ns=a.parse_args()
    if ns.baseline.is_dir():
        def old(name:str)->bytes:return (ns.baseline/name).read_bytes()
    else:
        z=zipfile.ZipFile(ns.baseline);entries={n.split('/',1)[1]:n for n in z.namelist() if '/' in n and not n.endswith('/')}
        def old(name:str)->bytes:return z.read(entries[name])
    def new(name:str)->bytes:return (ns.source/name).read_bytes()
    protected=['Source/Parameters.h','Source/StaticCompressionEngine.h','Source/ABTransfer.h','Source/LimiterModeContinuity.h',
        'Source/BS1770LoudnessMatch.h','Source/IntegratedLoudnessMeter.h','Source/KWeightingFilter.h','LICENSE']
    for f in protected:
        assert old(f)==new(f),f+' changed unexpectedly';print('UNCHANGED',f,digest(new(f)))
    unchanged_functions={
        'Source/PluginProcessor.cpp':['void QQSuperCompressionAudioProcessor::processBlock (',
          'void QQSuperCompressionAudioProcessor::processBlockInternal (',
          'void QQSuperCompressionAudioProcessor::parameterChanged (',
          'void QQSuperCompressionAudioProcessor::continueLimiterCompressionMode (',
          'void QQSuperCompressionAudioProcessor::rebuildBoundaryPairs()',
          'QQSuperCompressionAudioProcessor::ParameterSnapshot QQSuperCompressionAudioProcessor::captureCurrentSnapshot()',
          'qqsc::ABTransfer QQSuperCompressionAudioProcessor::makeABTransfer ('],
        'Source/LimiterMode.cpp':['void QQSuperCompressionAudioProcessor::setCompressionModeFromEditor (']}
    for f,sigs in unchanged_functions.items():
        for sig in sigs:
            assert function(old(f).decode(),sig)==function(new(f).decode(),sig),sig+' changed unexpectedly';print('UNCHANGED FUNCTION',sig)
    cmake0=old('CMakeLists.txt').decode();cmake1=new('CMakeLists.txt').decode()
    assert cmake0.replace('VERSION 1.2.8','VERSION 1.2.9')==cmake1,'CMake changed beyond version'
    src=new('Source/PluginProcessor.cpp').decode();hdr=new('Source/PluginProcessor.h').decode()
    assert 'currentStateSchemaVersion = 23;' in src
    assert 'float maximum = qqsc::normalMaximumMakeupDb' in src and 'addMakeup(id,name,qqsc::maximumMakeupDb)' in src
    assert '(group==3||group==6) ? 0.0f' in src
    assert 'return getLimiterReferencePeakDb(shift, true)' in hdr
    entry=function(new('Source/LimiterMode.cpp').decode(),'void QQSuperCompressionAudioProcessor::enterLimiterMode()')
    assert 'normalModeIds' not in entry and 'normalSoundIds' not in entry and 'getRawParameterValue' not in entry
    assert 'setActualParameterValue(qqsc::params::limiterMode, 1)' in entry
    for f in (ns.source/'Source').iterdir():
        if f.is_file():
            # Balance actual C++ structural delimiters, ignoring strings/comments.
            tokens=re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}\[\]()]')
            stack=[];pair={')':'(',']':'[','}':'{'}
            for m in tokens.finditer(f.read_text()):
                t=m.group()
                if t in '([{':stack.append(t)
                elif t in ')]}':assert stack and stack.pop()==pair[t],f'{f.name}: unmatched {t}'
            assert not stack,f'{f.name}: unclosed delimiters'
    print('PASS: version/schema/defaults/range/link declarations, unchanged parameter-ID definitions, plugin identity, DSP core and 1.2.8 continuity functions; C++ delimiters balanced.')
    print('LIMITATION: static/extracted-source checks do not establish full JUCE compilation, binary identity or audio null equivalence.')
    return 0
if __name__=='__main__':raise SystemExit(main())
