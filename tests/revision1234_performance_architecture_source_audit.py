from pathlib import Path
r=Path(__file__).resolve().parents[1]
proc=(r/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
proch=(r/'Source/PluginProcessor.h').read_text(encoding='utf-8')
editor=(r/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
lim=(r/'Source/LimiterEditor.cpp').read_text(encoding='utf-8')
ceil=(r/'Source/OutputCeiling.h').read_text(encoding='utf-8')
params=(r/'Source/Parameters.h').read_text(encoding='utf-8')
cmake=(r/'CMakeLists.txt').read_text(encoding='utf-8')
checks={
'1.2.41 version':'project(QQSuperCompression VERSION 1.2.41' in cmake,
'ceiling os param':'ceilingOversampling' in params and '"Ceiling Oversampling"' in proc,
'instance ECO persisted':'qqscPerformanceEco' in proc and 'ecoMode { false }' in proch and 'state.setProperty (performanceEcoProperty, isEcoMode()' in proc and 'EcoModeGlobal' not in proc+proch+editor,
'per instance editor gate':'editorOpen.store' in proch and 'shouldRunUiAnalysis' in proch and 'processor.setEditorOpen (isShowing())' in editor,
'analysis gate used':'const bool uiAnalysisEnabled=shouldRunUiAnalysis()' in proc,
'tp meter gated':'if (uiAnalysisEnabled)' in proc and 'updateTruePeakMeter' in proc,
'match gated':'uiAnalysisEnabled && transportPlaying' in proc and 'loudnessMatch.processSample' in proc,
'ceiling 4/8/16 prepared':all(x in ceil for x in ('oversampling4','oversampling8','oversampling16')),
'native ceiling 1x':'if(selectedChoice==0)' in ceil and 'juce::jlimit(-c,c,l)' in ceil,
'matched share':'processSharedOversampledBlock' in ceil and 'currentCeilingSharesCoreOversampling' in proc,
'tp effective promotion':'return isTruePeakSelected() && stored==qqsc::params::ceilingNative ? qqsc::params::ceiling8x : stored;' in proc,
'ceiling ui':'CEILING OS' in editor and 'cycleCeilingOversampling' in editor,
'eco ui':'togglePerformanceMode' in editor and '"ECO" : "FULL"' in editor,
'limiter tooltip updated':'selected Ceiling OS' in lim,
}
failed=[k for k,v in checks.items() if not v]
for k,v in checks.items(): print(('PASS' if v else 'FAIL')+': '+k)
if failed: raise SystemExit('failed: '+', '.join(failed))
print('PASS: revision1234 performance architecture source audit')
