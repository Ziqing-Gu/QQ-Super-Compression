#!/usr/bin/env python3
"""Compile the actual boundary/mode functions in a small isolation harness.

No JUCE SDK required. APVTS, parameter normalization, GUI and UndoManager are
minimal test doubles, NOT a substitute for QQSCLimiterCheck ... continuity.
Function bodies and parameter IDs are extracted from the selected source tree.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def function(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise ValueError(f"Function not found: {signature}")
    brace = text.index("{", start)
    # Ignore strings and comments when finding the matching closing brace.
    token = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')
    depth = 0
    for m in token.finditer(text, brace):
        if m.group() == "{":
            depth += 1
        elif m.group() == "}":
            depth -= 1
            if depth == 0:
                return text[start:m.end()] + "\n"
    raise ValueError(f"Unclosed function: {signature}")


HARNESS = r'''
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "LimiterModeContinuity.h"
namespace juce {
using String = std::string;
template<class T> T jlimit(T a,T b,T v) { return std::clamp(v,a,b); }
template<class T> T jmax(T a,T b) { return std::max(a,b); }
template<class T> T jmin(T a,T b) { return std::min(a,b); }
template<class T> struct ScopedValueSetter {
 T& ref; T old;
 ScopedValueSetter(T& r,T v):ref(r),old(r){ ref=v; }
 ~ScopedValueSetter(){ ref=old; }
};
struct MessageManager {
 std::thread::id messageThread = std::this_thread::get_id();
 bool isThisTheMessageThread() const { return messageThread==std::this_thread::get_id(); }
 static MessageManager* getInstanceWithoutCreating(){static MessageManager m; return &m;}
};
struct UndoManager {
 bool replaying=false;
 int transactions=0;
 bool isPerformingUndoRedo() const {return replaying;}
 void beginNewTransaction(const char*) {++transactions;}
};
struct RangedAudioParameter {
 std::atomic<float> value{0};
 std::function<void(float)> callback;
 bool callbackBeforeRaw=false;
 int writes=0, starts=0, ends=0, depth=0;
 // Intentionally identity transforms: this test does not verify JUCE ranges.
 float convertTo0to1(float v) const {return v;}
 float convertFrom0to1(float v) const {return v;}
 void beginChangeGesture(){++starts; ++depth;}
 void endChangeGesture(){++ends; if(--depth<0) throw std::runtime_error("Unbalanced gesture");}
 void setValueNotifyingHost(float v){
  ++writes;
  if(callbackBeforeRaw) {if(callback)callback(v);value.store(v);}
  else {value.store(v);if(callback)callback(v);}
 }
};
}
namespace qqsc::params {
inline constexpr const char* limiterMode="limiterMode";
inline constexpr const char* limiterOutputDb="limiterOutputDb";
inline constexpr const char* compressionMode="compressionMode";
inline constexpr const char* algorithmMode="algorithmMode";
inline constexpr const char* inputOutputLink="inputOutputLink";
inline constexpr const char* outputGainDb="outputGainDb";
// @@ACTUAL_ID_ARRAYS@@
template<std::size_t N> std::array<const char*,5> slice(const std::array<const char*,N>& a, std::size_t n){
 return {a[n],a[n+1],a[n+2],a[n+3],a[n+4]};
}
const auto thresholdIds=slice(normalSoundIds,15);
const auto rangeIds=slice(normalSoundIds,20);
const auto upThresholdIds=slice(normalSoundIds,25);
const auto downThresholdIds=slice(normalSoundIds,30);
const auto limiterThresholdIds=slice(limiterSoundIds,15);
const auto limiterRangeIds=slice(limiterSoundIds,20);
const auto limiterUpThresholdIds=slice(limiterSoundIds,25);
const auto limiterDownThresholdIds=slice(limiterSoundIds,30);
const std::array<const char*,5>& boundaryBankIds(int b,bool u){
 if(b==0) return u?rangeIds:thresholdIds;
 if(b==1) return u?downThresholdIds:upThresholdIds;
 if(b==2) return u?limiterRangeIds:limiterThresholdIds;
 return u?limiterDownThresholdIds:limiterUpThresholdIds;
}
float clampRangeDb(float v){return v<=0?std::clamp(v,-120.f,0.f):1.f;}
}
struct MockAPVTS {
 mutable std::map<std::string,juce::RangedAudioParameter> parameters;
 std::atomic<float>* getRawParameterValue(const std::string& id) const {return &parameters.at(id).value;}
 juce::RangedAudioParameter* getParameter(const std::string& id){return &parameters.at(id);}
 int copyState(){return 0;}
};
struct QQSuperCompressionAudioProcessor {
 MockAPVTS apvts;
 juce::UndoManager undoManager;
 std::array<std::atomic<uint64_t>,20> boundaryPairs{};
 std::atomic<int> limiterCompressionModeForAudio{0};
 std::atomic<bool> restoringDynamicsState{false}, limiterBankInitialised{false}, matchReady{false};
 std::atomic<bool> resetMatchOnNextPlaybackBlock{false},algorithmPreferenceInitialised{false};
 std::atomic<bool> classicAlgorithmForText{true},inputOutputLinkPreferenceInitialised{false};
 std::array<std::atomic<bool>,2> dualAlgorithmPreferenceInitialised{};
 bool unity=false;
 QQSuperCompressionAudioProcessor();
 bool isLimiterMode() const noexcept{return apvts.getRawParameterValue("limiterMode")->load()>=.5f;}
 bool isUnityMonitorEnabled() const noexcept{return unity;}
 void rebuildBoundaryPairs() noexcept;
 void parameterChanged(const juce::String&,float);
 float getBoundaryForBankDb(int,bool,int) const noexcept;
 float getBoundaryForDomainDb(bool,bool,int) const noexcept;
 void continueLimiterCompressionMode(bool) noexcept;
 void setBoundaryForDomainDb(bool,bool,int,float);
 void timerCallback();
 const char* soundParameterID(const char*) const noexcept;
 float readSoundParameter(const char*) const noexcept;
 void setCompressionModeFromEditor(int);
 void host(const char* id,float v){apvts.getParameter(id)->setValueNotifyingHost(v);}
 float raw(const char* id) const {return apvts.getRawParameterValue(id)->load();}
 void seed(const char* id,float v){apvts.getRawParameterValue(id)->store(v);}
 int writes() const {int n=0;for(const auto& p:apvts.parameters)n+=p.second.writes;return n;}
};
thread_local const QQSuperCompressionAudioProcessor* boundaryWriteSource=nullptr;
// @@ACTUAL_FUNCTIONS@@
QQSuperCompressionAudioProcessor::QQSuperCompressionAudioProcessor(){
 for(const auto* id:qqsc::params::normalSoundIds)apvts.parameters.try_emplace(id);
 for(const auto* id:qqsc::params::limiterSoundIds)apvts.parameters.try_emplace(id);
 for(const auto* id:qqsc::params::normalModeIds)apvts.parameters.try_emplace(id);
 for(const auto* id:qqsc::params::limiterModeIds)apvts.parameters.try_emplace(id);
 for(const auto* id:{"limiterMode","limiterLink","limiterOutputDb","limiterCalibrationDb","ceilingDb","outputGainDb","inputOutputLink"})
  apvts.parameters.try_emplace(id);
 for(int b=0;b<4;++b)for(int d=0;d<5;++d){
  seed(qqsc::params::boundaryBankIds(b,false)[d], b==2?-12.f-d: b==0?-45.f-d:-70.f-d);
  seed(qqsc::params::boundaryBankIds(b,true)[d], b==2?1.f:b==3?0.f:-8.f-d);
 }
 seed("limiterMode",1);seed("limiterOutputDb",-10);seed("limiterCalibrationDb",1.25f);seed("ceilingDb",-.9f);
 for(int d=0;d<5;++d){seed(qqsc::params::limiterModeIds[1+d],2.5f+d);seed(qqsc::params::limiterModeIds[6+d],17.f+13.f*d);}
 rebuildBoundaryPairs();
 for(auto& entry:apvts.parameters){auto id=entry.first;entry.second.callback=[this,id](float v){parameterChanged(id,v);};}
}
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
void equal(float a,float b,const std::string& message){require(std::abs(a-b)<.0001f,message+" got="+std::to_string(a)+" expected="+std::to_string(b));}
std::vector<float> normalBoundaries(QQSuperCompressionAudioProcessor& p){
 std::vector<float> a;for(int b=0;b<2;++b)for(int d=0;d<5;++d)for(bool u:{false,true})a.push_back(p.getBoundaryForBankDb(b,u,d));return a;
}
std::vector<float> gains(QQSuperCompressionAudioProcessor& p){
 std::vector<float> a;for(int i=0;i<11;++i)a.push_back(p.raw(qqsc::params::limiterModeIds[i]));
 for(const auto* id:{"limiterOutputDb","limiterCalibrationDb","ceilingDb","outputGainDb","limiterLink","limiterDomainLink","limiterDualRatioLink"})a.push_back(p.raw(id));return a;
}
int main(){
 try{
  juce::MessageManager::getInstanceWithoutCreating();
  int scenarios=0;
  for(bool early:{false,true})for(int link:{0,1})for(int domainLink:{0,1})for(int ratioLink:{0,1})
   for(int channelMode:{0,1,2})for(int algorithm:{0,1})for(float output:{-10.f,0.f,18.f})for(float mix:{0.f,37.f,100.f}){
    QQSuperCompressionAudioProcessor p;
    for(auto& pair:p.apvts.parameters)pair.second.callbackBeforeRaw=early;
    p.seed("limiterLink",float(link));p.seed("limiterDomainLink",float(domainLink));p.seed("limiterDualRatioLink",float(ratioLink));
    p.seed("limiterProcessingMode",float(channelMode));p.seed("limiterAlgorithmMode",float(algorithm));
    p.seed("limiterOutputDb",output);for(int d=0;d<5;++d)p.seed(qqsc::params::limiterModeIds[6+d],mix);
    const auto normal=normalBoundaries(p),before=gains(p);
    int writes=p.writes();p.host("limiterCompressionMode",1);
    require(p.writes()==writes+1,"Audio mode callback recursively wrote a host parameter");
    equal(p.readSoundParameter("compressionMode"),1,"Audio mode not published");
    for(int d=0;d<5;++d){equal(p.getBoundaryForDomainDb(true,true,d),-12.f-d,"Single->Dual recalled stale DOWN threshold");equal(p.raw(qqsc::params::limiterDownThresholdIds[d]),0,"Host callback flushed raw parameters");}
    require(gains(p)==before,"Single->Dual altered shared gains or Link flags");
    p.timerCallback();
    for(int d=0;d<5;++d)equal(p.raw(qqsc::params::limiterDownThresholdIds[d]),-12.f-d,"Timer did not publish canonical DOWN");
    for(int d=0;d<5;++d)p.setBoundaryForDomainDb(true,true,d,-32.f-d);
    p.host("limiterCompressionMode",0);
    for(int d=0;d<5;++d)equal(p.getBoundaryForDomainDb(false,false,d),-32.f-d,"Dual->Single recalled stale Threshold");
    for(int i=0;i<20;++i)p.setCompressionModeFromEditor((i+1)%2);
    require(gains(p)==before,"Repeated switches changed shared gains");
    require(normalBoundaries(p)==normal,"Limiter switch touched Normal banks");
    for(int d=0;d<5;++d)equal(p.getBoundaryForDomainDb(false,false,d),-32.f-d,"Repeated round-trip drift");
    for(const auto& item:p.apvts.parameters)require(item.second.depth==0,"Unclosed UI gesture: "+item.first);
    ++scenarios;
   }
  std::cout<<"PASS: "<<scenarios<<" isolated processor scenarios; two APVTS callback orders, both links, ST/LR/MS, algorithms, both switch directions, shared gains and normal-bank isolation.\n";
  {
   QQSuperCompressionAudioProcessor p;p.seed("limiterMode",0);
   const auto normal=normalBoundaries(p),before=gains(p);
   for(int i=0;i<100;++i)p.setCompressionModeFromEditor((i+1)%2);
   require(normalBoundaries(p)==normal,"Normal independent boundary memories were modified");require(gains(p)==before,"Normal mode changed Limiter gains");
   std::cout<<"PASS: 100 Normal Single/Dual toggles retain independent boundaries.\n";
  }
  {
   QQSuperCompressionAudioProcessor p;
   p.restoringDynamicsState.store(true);p.host("limiterCompressionMode",1);
   p.seed("limiterThresholdDb",-44);p.seed("limiterDownThresholdDb",-8);p.rebuildBoundaryPairs();p.restoringDynamicsState.store(false);
   equal(p.getBoundaryForDomainDb(false,false,0),-44,"Restore rewrote stored Single threshold");
   equal(p.getBoundaryForDomainDb(true,true,0),-8,"Restore rewrote active Dual threshold");
   p.host("limiterCompressionMode",0);equal(p.getBoundaryForDomainDb(false,false,0),-8,"First post-restore switch did not carry active threshold");
   p.seed("limiterDownThresholdDb",-4);p.rebuildBoundaryPairs();p.undoManager.replaying=true;p.host("limiterCompressionMode",1);
   equal(p.getBoundaryForDomainDb(true,true,0),-4,"Undo replay was treated as new switch");p.undoManager.replaying=false;
   std::cout<<"PASS: restore and Undo replay suppression (test doubles, not JUCE state/Undo integration).\n";
  }
  {
   QQSuperCompressionAudioProcessor p;const int before=p.writes();
   std::thread worker([&]{p.host("limiterCompressionMode",1);});worker.join();
   require(p.writes()==before+1,"Worker callback wrote companion parameter");equal(p.getBoundaryForDomainDb(true,true,0),-12,"Worker mode switch not immediate");
   std::cout<<"PASS: headless worker-thread mode callback publishes canonical threshold without timer/host recursion.\n";
  }
  std::size_t count=0;
  for(int cent=-12000;cent<=0;++cent)for(float companion:{-120.f,-96.f,-50.f,-10.f,-.01f,0.f,1.f}){
   const float t=float(cent)*.01f;
   const auto dual=qqsc::continueLimiterModeBoundaries(true,{t,1.f},{std::min(0.f,companion),0.f});
   equal(dual.upperDb,t,"Policy DOWN threshold mismatch");require(dual.lowerDb<=dual.upperDb,"Dual collision invalid");
   const auto single=qqsc::continueLimiterModeBoundaries(false,{-120.f,t},{-120.f,companion});
   equal(single.lowerDb,t,"Policy Single threshold mismatch");require(single.upperDb>=single.lowerDb,"Single collision invalid");
   const auto round=qqsc::continueLimiterModeBoundaries(true,single,dual);equal(round.upperDb,t,"Policy round-trip drift");
   if(companion==1.f)equal(single.upperDb,1.f,"Range OFF was lost");
   count+=3;
  }
  std::cout<<"PASS: "<<count<<" standalone policy transfers; entire 0.01 dB grid, collision ordering, Range OFF, no threshold drift.\n";
  std::cout<<"LIMITATION: no real JUCE, Windows VST3, normalization, audio rendering or real Undo/APVTS serialization was executed by this harness.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
'''


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    src = args.source.resolve()
    pp = (src / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
    lm = (src / "Source/LimiterMode.cpp").read_text(encoding="utf-8")
    params = (src / "Source/Parameters.h").read_text(encoding="utf-8")
    ids = []
    for name in ("normalSoundIds", "limiterSoundIds", "normalModeIds", "limiterModeIds"):
        m = re.search(r"inline constexpr std::array<const char\*,\s*\d+> " + name + r"\s*\{[^;]+;", params)
        if not m:
            raise RuntimeError(f"Missing original parameter IDs: {name}")
        ids.append(m.group())
    bodies = []
    for sig in (
        "uint64_t packDisplayStereoSample", "void unpackDisplayStereoSample",
        "void QQSuperCompressionAudioProcessor::rebuildBoundaryPairs",
        "void QQSuperCompressionAudioProcessor::parameterChanged",
        "float QQSuperCompressionAudioProcessor::getBoundaryForBankDb",
        "float QQSuperCompressionAudioProcessor::getBoundaryForDomainDb",
        "void QQSuperCompressionAudioProcessor::setBoundaryForDomainDb",
        "void QQSuperCompressionAudioProcessor::timerCallback",
    ):
        bodies.append(function(pp, sig))
    for sig in ("const char* QQSuperCompressionAudioProcessor::soundParameterID", "float QQSuperCompressionAudioProcessor::readSoundParameter"):
        bodies.append(function(lm, sig))
    if "void QQSuperCompressionAudioProcessor::continueLimiterCompressionMode" in pp:
        bodies.append(function(pp, "void QQSuperCompressionAudioProcessor::continueLimiterCompressionMode"))
        bodies.append(function(lm, "void QQSuperCompressionAudioProcessor::setCompressionModeFromEditor"))
    else:
        # The 1.2.7 editor only selected the parameter; preserve that behavior
        # when running the negative-control build against the baseline source.
        bodies.append('void QQSuperCompressionAudioProcessor::setCompressionModeFromEditor(int mode){host(soundParameterID("compressionMode"),float(mode));}')
    code = HARNESS.replace("// @@ACTUAL_ID_ARRAYS@@", "\n".join(ids)).replace("// @@ACTUAL_FUNCTIONS@@", "\n".join(bodies))
    with tempfile.TemporaryDirectory(prefix="qqsc-continuity-") as temp:
        build = args.build_dir.resolve() if args.build_dir else Path(temp)
        build.mkdir(parents=True, exist_ok=True)
        target = build / "limiter_mode_continuity_isolated.cpp"
        target.write_text(code, encoding="utf-8")
        # Use the candidate helper even for a baseline negative-control build.
        helper = Path(__file__).resolve().parents[1] / "Source/LimiterModeContinuity.h"
        shutil.copyfile(helper, build / helper.name)
        exe = build / "limiter_mode_continuity_isolated"
        cmd = [args.compiler, "-std=c++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-misleading-indentation", "-pedantic", "-pthread"]
        if args.sanitize:
            cmd += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        cmd += [str(target), "-o", str(exe)]
        print("Compiling actual extracted source functions:", src, flush=True)
        subprocess.run(cmd, check=True)
        return subprocess.run([str(exe)], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
