#!/usr/bin/env python3
"""Run actual extracted C++ bank/reference/editor functions without JUCE.

The APVTS, Undo, slider and range adapters are test doubles. This checks actual
math/control flow, NOT a Windows build, audio render, real GUI or serialization.
Use QQSCLimiterCheck ... revision129 for the additional real-JUCE checks.
"""
from __future__ import annotations
import argparse, re, shutil, subprocess, tempfile
from pathlib import Path
from limiter_mode_continuity_isolated_test import function

CPP = r'''
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "DynamicsLimits.h"
namespace juce {
template<class T>T jmin(T a,T b){return std::min(a,b);}
template<class T>T jmax(T a,T b){return std::max(a,b);}
template<class T>T jlimit(T a,T b,T v){return std::clamp(v,a,b);}
inline int roundToInt(float v){return int(std::lround(v));}
struct String:std::string {using std::string::string; String(std::string s):std::string(std::move(s)){}
 String toString() const{return *this;} bool isNotEmpty()const{return !empty();}};
template<class T>struct ScopedValueSetter{T&r;T old;ScopedValueSetter(T&v,T n):r(v),old(v){r=n;}~ScopedValueSetter(){r=old;}};
struct Decibels {
 static float decibelsToGain(float d,float floor=-100.f){return d>floor?std::pow(10.f,d/20.f):0.f;}
 static float gainToDecibels(float v,float floor=-100.f){return v>0?std::max(floor,20.f*std::log10(v)):floor;}
};
struct UndoableAction{virtual~UndoableAction()=default;virtual bool perform()=0;virtual bool undo()=0;};
struct UndoManager {
 bool replaying=false;int transactions=0;std::vector<std::unique_ptr<UndoableAction>> actions;
 bool isPerformingUndoRedo()const{return replaying;}
 void beginNewTransaction(const String& = {}){++transactions;}
 bool perform(UndoableAction* a){actions.emplace_back(a);return a->perform();}
};
constexpr int sendNotificationSync=1,dontSendNotification=0;
struct RangedAudioParameter {
 std::atomic<float> value{0};float lo=-120,hi=120,step=.01f;int depth=0,starts=0,ends=0,writes=0;
 std::function<void(float)> changed;
 float convertTo0to1(float v)const{return (std::clamp(v,lo,hi)-lo)/(hi-lo);}
 float convertFrom0to1(float n)const {auto v=lo+std::clamp(n,0.f,1.f)*(hi-lo);return step>0?std::clamp(std::round(v/step)*step,lo,hi):v;}
 float getValue()const{return convertTo0to1(value.load());}
 void beginChangeGesture(){if(depth)throw std::runtime_error("nested gesture");++depth;++starts;}
 void endChangeGesture(){if(--depth!=0)throw std::runtime_error("unbalanced gesture");++ends;}
 void setValueNotifyingHost(float n){float v=convertFrom0to1(n);++writes;value.store(v);if(changed)changed(v);}
};
}
namespace qqsc::params {
// @@IDS@@
constexpr int stereoLinked=0,midSide=1,leftRight=2,keyInternal=0,classicAlgorithm=0,singleCompression=0;
constexpr float thresholdOffDb=-120,rangeOffDb=1,keyHpfOffHz=0;
inline float snapLookaheadMs(float v){return v;} // timing is not under test
// @@PARAM_MATH@@
}
namespace qqsc {
enum class CompressionAlgorithm{classic,super};
struct StaticCompressionEngine {
// @@CURVES@@
};
// @@TRANSFER_STRUCT@@
}
struct APVTS {
 mutable std::map<std::string,juce::RangedAudioParameter> data;
 juce::RangedAudioParameter* getParameter(const std::string&id){auto i=data.find(id);return i==data.end()?nullptr:&i->second;}
 std::atomic<float>* getRawParameterValue(const std::string&id)const{return &data.at(id).value;}
};
struct QQSuperCompressionAudioProcessor {
 APVTS apvts;juce::UndoManager undoManager;
 std::atomic<bool> limiterBankInitialised{false},restoringDynamicsState{false};std::atomic<int>limiterCompressionModeForAudio{0};
 // @@SNAPSHOT@@
 QQSuperCompressionAudioProcessor();
 APVTS& getAPVTS(){return apvts;}
 juce::UndoManager& getUndoManager(){return undoManager;}
 bool isLimiterMode()const{return raw("limiterMode")>=.5f;}
 bool isLimiterLinked()const{return isLimiterMode()&&raw("limiterLink")>=.5f;}
 bool isTruePeakSelected()const{return true;}
 bool isClassicAlgorithm()const{return readSoundParameter("algorithmMode")<.5f;}
 bool isRestoringSoundState()const{return restoringDynamicsState.load();}
 bool isClassicBoundary(bool upper)const{return readSoundParameter(readSoundParameter("compressionMode")>0?(upper?"downAlgorithmMode":"upAlgorithmMode"):"algorithmMode")<.5f;}
 float raw(const char*id)const{return apvts.getRawParameterValue(id)->load();}
 void seed(const char*id,float v){apvts.getRawParameterValue(id)->store(v);if(std::strcmp(id,"limiterCompressionMode")==0)limiterCompressionModeForAudio.store(int(v));}
 float getBoundaryForBankDb(int bank,bool upper,int d)const;
 float getBoundaryForDomainDb(bool dual,bool upper,int d)const {return getBoundaryForBankDb((isLimiterMode()?2:0)+(dual?1:0),upper,d);}
 void setBoundaryForDomainDb(bool dual,bool upper,int d,float v);
 void rebuildBoundaryPairs(){} void notifyHostProcessingLatency(){}
 void queueABTransfer(const ParameterSnapshot&,const ParameterSnapshot&){}
 ParameterSnapshot captureCurrentSnapshot()const noexcept;
 static ParameterSnapshot activeSnapshot(ParameterSnapshot)noexcept;
 static qqsc::ABTransfer makeABTransfer(const ParameterSnapshot&)noexcept;
 void setActualParameterValue(const char*,float);
 const char* soundParameterID(const char*)const noexcept;
 float readSoundParameter(const char*)const noexcept;
 float effectiveSingleRatio(size_t)const noexcept;
 void enterLimiterMode();void leaveLimiterMode();
 float getLimiterReferencePeakDb(float shift=0,bool includeMix=true)const noexcept;
 // @@LINK_REFERENCE@@
};
struct FineKnob {
 juce::RangedAudioParameter* parameter=nullptr;std::map<std::string,juce::String> props;double value=0;bool native=false;
 std::function<void()> onBeforeValueEdit,onGestureStart,onGestureEnd,onValueChange;
 auto&getProperties(){return props;}
 double getValue()const{return value;}double getMinimum()const{return parameter->lo;}double getMaximum()const{return parameter->hi;}
 bool hasActiveNativeGesture()const{return native;}
 void bind(QQSuperCompressionAudioProcessor&p,const char*id){props["qqscParameterID"]=id;parameter=p.apvts.getParameter(id);value=parameter->value.load();}
 void refresh(){value=parameter->value.load();}
 void setValue(double v,int notification){v=std::clamp(v,getMinimum(),getMaximum());if(std::abs(v-value)<1e-10)return;value=v;
  if(notification){parameter->setValueNotifyingHost(parameter->convertTo0to1(float(v)));value=parameter->value.load();if(onValueChange)onValueChange();}}
};
struct Button{bool value=false;bool getToggleState()const{return value;}};
struct QQSuperCompressionAudioProcessorEditor {
 using FineKnob=::FineKnob;
 enum class LinkedPair{none,inputOutput,makeupLR,makeupMS,mixLR,mixMS,ratioLR,ratioMS,downRatioLR,downRatioMS,upperBoundaryLR,upperBoundaryMS,thresholdLR,thresholdMS};
 QQSuperCompressionAudioProcessor&processor;
 bool limiterControlsReady=true,limiterUpdating=false,limiterOutputGesture=false,limiterLinkAnchorValid=false;
 bool limiterReferenceEditInProgress=false,linkedValueUpdateInProgress=false,dualRatioValueUpdateInProgress=false,boundaryGestureActive=false;
 float limiterLinkReference=0,limiterLinkOutput=0,limiterStartOutput=0;
 FineKnob*limiterReferenceGestureSource=nullptr,*activeLinkSource=nullptr,*activeLinkTarget=nullptr;
 LinkedPair activeLinkedPair=LinkedPair::none;double activeLinkSourceStart=0,activeLinkTargetStart=0;
 int attachedCompressionMode=0;
 bool dualRatioGestureActive=false,dualRatioGestureUpward=false,dualRatioGestureCoupled=false;
 int dualRatioGestureDomain=0,dualRatioGesturePartner=0;
 std::array<double,5>dualRatioStartUp{},dualRatioStartDown{};
 std::vector<juce::RangedAudioParameter*>companionGestureParameters;
 FineKnob outputGainSlider;std::array<FineKnob,5>ratio,makeup,mix;std::array<std::unique_ptr<FineKnob>,5>downRatioSliders;
 Button linkButton,dualRatioLinkButton,inputOutputLinkButton;
 explicit QQSuperCompressionAudioProcessorEditor(QQSuperCompressionAudioProcessor&p):processor(p){bind();}
 std::array<FineKnob*,5>mainRatioControls(){return{&ratio[0],&ratio[1],&ratio[2],&ratio[3],&ratio[4]};}
 void bind();void beginUndoTransaction(const juce::String&s){processor.undoManager.beginNewTransaction(s);}
 void linkLimiterReferenceControl(FineKnob&);void captureLimiterLinkAnchor();void reconcileLimiterOutput();
 void beginLinkedGesture(LinkedPair,FineKnob&,FineKnob&,const juce::String&);
 void endLinkedGesture();void beginDualRatioGesture(int,bool);
 double applyDualRatioChange(double,bool);void handleDualRatioChange(int,bool);
 void handleLinkedValueChange(LinkedPair,FineKnob&,FineKnob&);
 void setLinkedControlValue(FineKnob&,double);
 double applyLimiterOutputChange(double);void beginLimiterOutputGesture();
 void drag(FineKnob&k,double v){if(k.onBeforeValueEdit)k.onBeforeValueEdit();if(k.onGestureStart)k.onGestureStart();k.native=true;k.setValue(v,1);k.native=false;if(k.onGestureEnd)k.onGestureEnd();}
};
// @@ACTUAL_FUNCTIONS@@
static void check(bool v,const std::string&m){if(!v)throw std::runtime_error(m);}
static bool close(double a,double b,double tolerance=.011){return std::abs(a-b)<=tolerance;}
QQSuperCompressionAudioProcessor::QQSuperCompressionAudioProcessor(){
 for(const auto*id:qqsc::params::normalSoundIds)apvts.data.try_emplace(id);
 for(const auto*id:qqsc::params::limiterSoundIds)apvts.data.try_emplace(id);
 for(const auto*id:qqsc::params::normalModeIds)apvts.data.try_emplace(id);
 for(const auto*id:qqsc::params::limiterModeIds)apvts.data.try_emplace(id);
 for(const auto*id:{"limiterMode","limiterLink","limiterOutputDb","limiterCalibrationDb","ceilingDb","outputGainDb","inputOutputLink"})apvts.data.try_emplace(id);
 for(int lim:{0,1}){
  const auto&sound=lim?qqsc::params::limiterSoundIds:qqsc::params::normalSoundIds;
  const auto&settings=lim?qqsc::params::limiterModeIds:qqsc::params::normalModeIds;
  for(size_t i=0;i<35;++i){auto&param=apvts.data.at(sound[i]);int group=int(i/5);
   if(group<3){param.lo=group==2?1.f:.005f;param.hi=lim?1000:200;param.step=0;}
   else {param.lo=-120;param.hi=group==4?1:0;}
   if(lim){const float initial=/*@@LIMITER_INITIAL@@*/;param.value.store(initial);}
   else param.value.store(group<3?1:group==4?1:group==6?0:-120);
  }
  for(size_t i=0;i<33;++i){auto&param=apvts.data.at(settings[i]);
   if(i>=1&&i<=5){const float maximum=lim?qqsc::maximumMakeupDb:/*@@NORMAL_MAX@@*/;param.lo=-maximum;param.hi=maximum;}
   if(i>=6&&i<=10){param.lo=0;param.hi=100;param.step=.1f;param.value.store(100);}
   if(i>=14&&i<=25)param.value.store(1);
  }
 }
 seed("limiterLink",1);seed("limiterMode",0);
 apvts.data.at("limiterCompressionMode").changed=[this](float v){limiterCompressionModeForAudio.store(int(v));};
}
float QQSuperCompressionAudioProcessor::getBoundaryForBankDb(int b,bool u,int d)const {
 const auto&ids=b>=2?qqsc::params::limiterSoundIds:qqsc::params::normalSoundIds;
 const int offset=b%2==0?(u?20:15):(u?30:25);return raw(ids[size_t(offset+d)]);
}
void QQSuperCompressionAudioProcessor::setBoundaryForDomainDb(bool dual,bool u,int d,float v){
 const auto&ids=isLimiterMode()?qqsc::params::limiterSoundIds:qqsc::params::normalSoundIds;
 const int offset=dual?(u?30:25):(u?20:15);auto*parameter=apvts.getParameter(ids[size_t(offset+d)]);parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}
void QQSuperCompressionAudioProcessorEditor::bind(){
 attachedCompressionMode=int(processor.readSoundParameter("compressionMode"));
 linkButton.value=processor.readSoundParameter("domainLink")>.5f;dualRatioLinkButton.value=processor.readSoundParameter("dualRatioLink")>.5f;
 outputGainSlider.bind(processor,processor.soundParameterID("outputGainDb"));
 for(int d=0;d<5;++d){const auto i=size_t(d);downRatioSliders[i]=std::make_unique<FineKnob>();
  ratio[i].bind(processor,processor.soundParameterID((attachedCompressionMode?qqsc::params::upRatioIds:qqsc::params::ratioIds)[i]));
  downRatioSliders[i]->bind(processor,processor.soundParameterID(qqsc::params::downRatioIds[i]));
  makeup[i].bind(processor,processor.soundParameterID(qqsc::params::normalModeIds[1+i]));
  mix[i].bind(processor,processor.soundParameterID(qqsc::params::normalModeIds[6+i]));
  if(attachedCompressionMode){ratio[i].parameter->lo=.005f;ratio[i].parameter->hi=1.f;downRatioSliders[i]->parameter->lo=200.f;}
  const auto partner=size_t(d==0?0:d%2?d+1:d-1);
  const auto setup=[&,this,i,partner,d](FineKnob&k,FineKnob&other,LinkedPair pair){
   if(d){k.onGestureStart=[this,&k,&other,pair]{beginLinkedGesture(pair,k,other,"pair");};k.onValueChange=[this,&k,&other,pair]{handleLinkedValueChange(pair,k,other);};}
   else k.onGestureStart=[this]{beginUndoTransaction("ST");};
   k.onGestureEnd=[this]{endLinkedGesture();};
  };
  setup(makeup[i],makeup[partner],d<3?LinkedPair::makeupLR:LinkedPair::makeupMS);
  setup(mix[i],mix[partner],d<3?LinkedPair::mixLR:LinkedPair::mixMS);
  setup(ratio[i],ratio[partner],d<3?LinkedPair::ratioLR:LinkedPair::ratioMS);
  if(attachedCompressionMode){ratio[i].onGestureStart=[this,d]{beginDualRatioGesture(d,true);};ratio[i].onValueChange=[this,d]{handleDualRatioChange(d,true);};}
  downRatioSliders[i]->onGestureStart=[this,d]{beginDualRatioGesture(d,false);};
  downRatioSliders[i]->onValueChange=[this,d]{handleDualRatioChange(d,false);};
  downRatioSliders[i]->onGestureEnd=[this]{endLinkedGesture();};
  linkLimiterReferenceControl(ratio[i]);linkLimiterReferenceControl(*downRatioSliders[i]);linkLimiterReferenceControl(makeup[i]);linkLimiterReferenceControl(mix[i]);
 }
}
static void seedCase(QQSuperCompressionAudioProcessor&p,int dual,int mode,int alg,int upalg,int link,int domainlink,int ratiolink,float mix){
 p.enterLimiterMode();p.seed("limiterCompressionMode",float(dual));p.seed("limiterProcessingMode",float(mode));p.seed("limiterAlgorithmMode",float(alg));
 p.seed("limiterUpAlgorithmMode",float(upalg));p.seed("limiterDownAlgorithmMode",float(alg));
 p.seed("limiterLink",float(link));p.seed("limiterDomainLink",float(domainlink));p.seed("limiterDualRatioLink",float(ratiolink));
 p.seed("limiterOutputDb",11.73f);p.seed("limiterCalibrationDb",2.0f);p.seed("ceilingDb",-.9f);
 for(size_t d=0;d<5;++d){const auto&v=qqsc::params::limiterSoundIds;
  p.seed(v[d],400+float(d)*10);p.seed(v[5+d],.5f);p.seed(v[10+d],400+float(d)*10);
  p.seed(v[15+d],-24.3f-float(d));p.seed(v[20+d],1);p.seed(v[25+d],-70);p.seed(v[30+d],-24.3f-float(d));
  p.seed(qqsc::params::limiterModeIds[1+d],float(d)-2);p.seed(qqsc::params::limiterModeIds[6+d],mix);
 }
}
// Independent double-precision 0 dB detector-point oracle, not a call back
// into the reference function. No lookahead, waveform or LUFS is simulated.
static double oracle(const QQSuperCompressionAudioProcessor&p){
 const auto s=p.captureCurrentSnapshot();
 const auto t=QQSuperCompressionAudioProcessor::makeABTransfer(s);std::array<double,5>h;
 for(size_t d=0;d<5;++d){double g=1,lo=t.lower[d],hi=t.upper[d];auto algo=t.dual?t.downAlgorithm:t.algorithm;
  if(t.dual){if(t.upAlgorithm==qqsc::CompressionAlgorithm::classic)lo=std::max(lo,double(qqsc::classicThresholdMinimumGain));
   if(algo==qqsc::CompressionAlgorithm::classic)hi=std::max(hi,double(qqsc::classicThresholdMinimumGain));
   if(lo<hi&&1>lo&&1>hi)g=algo==qqsc::CompressionAlgorithm::classic?std::pow(hi,1-1/double(t.downRatio[d])):(1+(t.downRatio[d]-1)*hi)/t.downRatio[d];
  }else{
   if(algo==qqsc::CompressionAlgorithm::classic){lo=std::max(lo,double(qqsc::classicThresholdMinimumGain));hi=std::max(hi,double(qqsc::classicThresholdMinimumGain));}
   if(lo<1&&1<hi&&lo<hi&&t.ratio[d]>1)g=algo==qqsc::CompressionAlgorithm::classic?std::pow(lo,1-1/double(t.ratio[d])):(1+(t.ratio[d]-1)*lo)/t.ratio[d];
  }
  h[d]=(1-double(t.mix[d]))+double(t.mix[d])*t.makeup[d]*g;
 }
 const double peak=t.mode==1?h[3]+h[4]:t.mode==2?std::max(h[1],h[2]):h[0];return 20*std::log10(std::max(peak,1e-18));
}
static void balanced(QQSuperCompressionAudioProcessor&p){for(auto&item:p.apvts.data)check(item.second.depth==0,"open gesture: "+item.first);}
int main(){try{
 int bankCases=0,edits=0,mathCases=0;
 {QQSuperCompressionAudioProcessor p;for(size_t d=0;d<5;++d){
   const auto*n=p.apvts.getParameter(qqsc::params::normalModeIds[1+d]);const auto*l=p.apvts.getParameter(qqsc::params::limiterModeIds[1+d]);
   check(n->lo==-30&&n->hi==30&&l->lo==-120&&l->hi==120,"Makeup range declaration");
   check(p.raw(qqsc::params::limiterSoundIds[15+d])==0&&p.raw(qqsc::params::limiterModeIds[6+d])==100,"Limiter factory declaration");
 }}
 for(int limedited:{0,1})for(int link:{0,1}){
  QQSuperCompressionAudioProcessor p;for(size_t i=0;i<33;++i)p.seed(qqsc::params::normalModeIds[i],float(i+1));p.seed("outputGainDb",17);
  if(limedited){p.seed("limiterMakeupGainDb",75);p.seed("limiterMix",32);p.seed("limiterOutputDb",-15);}
  p.seed("limiterLink",float(link));std::map<std::string,float>before;for(auto&v:p.apvts.data)before[v.first]=v.second.value.load();
  p.enterLimiterMode();for(auto&v:p.apvts.data)if(v.first!="limiterMode")check(v.second.value.load()==before.at(v.first),"first entry copied "+v.first);
  check(p.limiterBankInitialised.load(),"first-entry marker missing");p.leaveLimiterMode();p.enterLimiterMode();
  for(auto&v:p.apvts.data)if(v.first!="limiterMode")check(v.second.value.load()==before.at(v.first),"bank revisit reset "+v.first);
  balanced(p);++bankCases;
 }
 for(int dual:{0,1})for(int mode:{0,1,2})for(int alg:{0,1})for(int upalg:{0,1})for(int link:{0,1})for(int dl:{0,1})for(int rl:{0,1})for(float m:{0.f,37.f,100.f}){
  QQSuperCompressionAudioProcessor p;seedCase(p,dual,mode,alg,upalg,link,dl,rl,m);QQSuperCompressionAudioProcessorEditor e(p);
  const size_t d=mode==0?0:mode==2?1:3;
  const auto edit=[&](FineKnob&k,double value){const double before=oracle(p)+p.raw("limiterOutputDb");const float old=p.raw("limiterOutputDb");e.drag(k,value);
   check(close(p.getLimiterReferencePeakDb(),oracle(p),.00008),"reference disagrees with oracle");
   check(link?close(before,oracle(p)+p.raw("limiterOutputDb")):p.raw("limiterOutputDb")==old,"reference/output invariant");
   check(p.raw("limiterCalibrationDb")==2&&p.raw("ceilingDb")==-.9f,"link changed calibration/ceiling");balanced(p);++edits;
  };
  if(dual){edit(*e.downRatioSliders[d],620);const float old=p.raw("limiterOutputDb");edit(e.ratio[d],.4);if(!rl)check(close(old,p.raw("limiterOutputDb"),.0001),"UP-only moved Output");}
  else edit(e.ratio[d],620);
  edit(e.makeup[d],6);edit(e.mix[d],50);edit(e.mix[d],0);edit(e.makeup[d],90);edit(e.mix[d],100);edit(e.makeup[d],-4);edit(e.mix[d],m);
  // State / host playback must not create another gain compensation.
  const float o=p.raw("limiterOutputDb");e.mix[d].setValue(42,1);check(p.raw("limiterOutputDb")==o,"passive/host change generated compensation");
  p.restoringDynamicsState.store(true);e.mix[d].native=true;e.mix[d].setValue(44,1);e.mix[d].native=false;p.restoringDynamicsState.store(false);check(p.raw("limiterOutputDb")==o,"state restore moved Output");
  p.undoManager.replaying=true;e.mix[d].native=true;e.mix[d].setValue(45,1);e.mix[d].native=false;p.undoManager.replaying=false;check(p.raw("limiterOutputDb")==o,"Undo replay moved Output");
 }
 for(int dual:{0,1})for(int mode:{0,1,2})for(int alg:{0,1})for(float makeup:{-120.f,-30.f,0.f,30.f,120.f})for(float m:{0.f,.1f,50.f,99.9f,100.f})for(float threshold:{-120.f,-90.f,-24.3f,0.f}){
  QQSuperCompressionAudioProcessor p;seedCase(p,dual,mode,alg,1-alg,1,0,0,m);
  for(size_t d=0;d<5;++d){p.seed(qqsc::params::limiterModeIds[1+d],makeup);p.seed(qqsc::params::limiterSoundIds[15+d],threshold);p.seed(qqsc::params::limiterSoundIds[30+d],threshold);p.seed(qqsc::params::limiterSoundIds[25+d],-120);}
  check(close(p.getLimiterReferencePeakDb(),oracle(p),.00008),"extreme reference math");++mathCases;
 }
 // A finite Range returns to unity at the 0 dB reference; equal Dual gates
 // disable compression. Neither may be perturbed by a virtual +.01 dB gap.
 for(int dual:{0,1})for(int alg:{0,1}){QQSuperCompressionAudioProcessor p;seedCase(p,dual,0,alg,alg,1,0,0,100);
  p.seed("limiterMakeupGainDb",-120);if(dual){p.seed("limiterUpThresholdDb",-24.3f);p.seed("limiterDownThresholdDb",-24.3f);}else p.seed("limiterRangeDb",0);
  check(close(p.getLimiterReferencePeakDb(),-120,.00005),"equal gates/finite Range reference");}
 // Published worked example and no-Mix behavior, including a manual offset.
 {QQSuperCompressionAudioProcessor p;seedCase(p,1,0,0,0,1,0,0,100);p.seed("limiterMakeupGainDb",0);p.seed("limiterDownRatio",200);p.seed("limiterOutputDb",24.18f);QQSuperCompressionAudioProcessorEditor e(p);
  check(close(p.getLimiterReferencePeakDb(),-24.1785,.00005),"200:1 example");e.drag(*e.downRatioSliders[0],1000);check(close(p.raw("limiterOutputDb"),24.28,.011),"1000:1 example");e.drag(e.mix[0],50);check(close(p.raw("limiterOutputDb"),5.51,.011),"Mix50 example");e.drag(e.mix[0],0);check(close(p.raw("limiterOutputDb"),0,.011),"Mix0 example");
  const auto threshold=p.raw("limiterDownThresholdDb");e.beginLimiterOutputGesture();double result=e.applyLimiterOutputChange(12);e.setLinkedControlValue(e.outputGainSlider,result);e.endLinkedGesture();check(close(p.raw("limiterOutputDb"),12)&&p.raw("limiterDownThresholdDb")==threshold,"Mix0 inverse Output fabricated a threshold");balanced(p);
 }
 // Typed Dual: parse updates companions before native source commit. It must
 // still use the OLD full reference, not a half-updated Ratio pair.
 for(int mode:{0,1,2})for(int dl:{0,1}){QQSuperCompressionAudioProcessor p;seedCase(p,1,mode,0,1,1,dl,1,37);QQSuperCompressionAudioProcessorEditor e(p);int d=mode==0?0:mode==2?1:3;auto&k=*e.downRatioSliders[size_t(d)];
  const auto before=oracle(p)+p.raw("limiterOutputDb");k.onBeforeValueEdit();e.beginDualRatioGesture(d,false);const double value=e.applyDualRatioChange(710,false);e.endLinkedGesture();k.native=true;k.setValue(value,1);k.native=false;
  check(close(before,oracle(p)+p.raw("limiterOutputDb")),"typed Dual group reanchored halfway");balanced(p);
 }
 // Coupled bounds must publish the final source (not its unclamped request).
 for(int mode:{0,1,2}){QQSuperCompressionAudioProcessor p;seedCase(p,1,mode,0,1,1,1,1,37);QQSuperCompressionAudioProcessorEditor e(p);size_t d=mode==0?0:mode==2?1:3;
  for(double request:{2000.,200.,1000.}){const auto total=oracle(p)+p.raw("limiterOutputDb");e.drag(*e.downRatioSliders[d],request);check(close(total,oracle(p)+p.raw("limiterOutputDb")),"bounded Dual group compensation");}
  const auto total=oracle(p)+p.raw("limiterOutputDb");e.drag(e.ratio[d],1);check(close(total,oracle(p)+p.raw("limiterOutputDb")),"clamped reciprocal source");balanced(p);
 }
 // Inverse Output edits use the same mixed reference. The requested change
 // may hit a dry-path/threshold bound; the returned feasible value is checked.
 for(int dual:{0,1})for(int mode:{0,1,2})for(float m:{37.f,100.f}){QQSuperCompressionAudioProcessor p;seedCase(p,dual,mode,0,1,1,1,1,m);QQSuperCompressionAudioProcessorEditor e(p);
  for(double delta:{-1.,1.5,3.}){const auto total=oracle(p)+p.raw("limiterOutputDb");e.beginLimiterOutputGesture();double out=e.applyLimiterOutputChange(p.raw("limiterOutputDb")+delta);e.setLinkedControlValue(e.outputGainSlider,out);e.endLinkedGesture();check(close(total,oracle(p)+p.raw("limiterOutputDb"),.025),"mixed inverse Output invariant");balanced(p);}
 }
 {QQSuperCompressionAudioProcessor p;seedCase(p,1,0,0,1,1,0,0,100);QQSuperCompressionAudioProcessorEditor e(p);const auto before=p.raw("limiterOutputDb");for(int i=0;i<200;++i){e.drag(e.mix[0],50);e.drag(e.mix[0],100);}check(close(before,p.raw("limiterOutputDb"),.012),"Mix round-trip drift");balanced(p);}
 // Clamp the result at +/-120 instead of claiming an impossible invariant.
 {QQSuperCompressionAudioProcessor p;seedCase(p,0,0,0,0,1,0,0,100);p.seed("limiterOutputDb",119);p.seed("limiterMakeupGainDb",0);QQSuperCompressionAudioProcessorEditor e(p);e.drag(e.makeup[0],-120);check(p.raw("limiterOutputDb")<=120&&p.raw("limiterOutputDb")>=119,"Output boundary clamp");balanced(p);}
 std::cout<<"PASS: "<<bankCases<<" actual first-entry/revisit bank cases (including preset hidden values).\n";
 std::cout<<"PASS: "<<edits<<" extracted editor edits: Single/Dual, ST/LR/MS, Classic/Super, 3 Link flags, full mixed-reference invariants.\n";
 std::cout<<"PASS: "<<mathCases<<" reference/oracle cases including +/-120 dB Makeup, Mix endpoints and 4 thresholds.\n";
 std::cout<<"PASS: typed Dual coupling, reciprocal/group bounds, 36 mixed inverse edits, 200 Mix round trips, passive/state/Undo suppression, equal gates, finite Range, Output clamp, balanced mock gestures.\n";
 std::cout<<"LIMITATION: adapters are mocks; no JUCE GUI/APVTS serialization, real Undo, audio or Windows binary was tested.\n";
 return 0;
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
'''

def generate(root: Path) -> str:
    src = root/'Source'
    proc = (src/'PluginProcessor.cpp').read_text()
    hdr = (src/'PluginProcessor.h').read_text()
    mode = (src/'LimiterMode.cpp').read_text()
    ed = (src/'PluginEditor.cpp').read_text()
    le = (src/'LimiterEditor.cpp').read_text()
    params = (src/'Parameters.h').read_text()
    math = (src/'StaticCompressionEngine.h').read_text()
    funcs = []
    for text, sigs in [(proc, ['QQSuperCompressionAudioProcessor::ParameterSnapshot QQSuperCompressionAudioProcessor::captureCurrentSnapshot',
                               'QQSuperCompressionAudioProcessor::ParameterSnapshot QQSuperCompressionAudioProcessor::activeSnapshot',
                               'qqsc::ABTransfer QQSuperCompressionAudioProcessor::makeABTransfer',
                               'void QQSuperCompressionAudioProcessor::setActualParameterValue']),
                       (mode, ['void QQSuperCompressionAudioProcessor::enterLimiterMode', 'void QQSuperCompressionAudioProcessor::leaveLimiterMode',
                               'const char* QQSuperCompressionAudioProcessor::soundParameterID', 'float QQSuperCompressionAudioProcessor::readSoundParameter',
                               'float QQSuperCompressionAudioProcessor::effectiveSingleRatio', 'float QQSuperCompressionAudioProcessor::getLimiterReferencePeakDb']),
                       (le, ['void QQSuperCompressionAudioProcessorEditor::linkLimiterReferenceControl',
                             'void QQSuperCompressionAudioProcessorEditor::captureLimiterLinkAnchor',
                             'void QQSuperCompressionAudioProcessorEditor::reconcileLimiterOutput',
                             'void QQSuperCompressionAudioProcessorEditor::beginLimiterOutputGesture',
                             'double QQSuperCompressionAudioProcessorEditor::applyLimiterOutputChange']),
                       (ed, ['void QQSuperCompressionAudioProcessorEditor::beginLinkedGesture', 'void QQSuperCompressionAudioProcessorEditor::endLinkedGesture',
                             'void QQSuperCompressionAudioProcessorEditor::beginDualRatioGesture', 'double QQSuperCompressionAudioProcessorEditor::applyDualRatioChange',
                             'void QQSuperCompressionAudioProcessorEditor::handleDualRatioChange', 'void QQSuperCompressionAudioProcessorEditor::setLinkedControlValue',
                             'void QQSuperCompressionAudioProcessorEditor::handleLinkedValueChange'])]:
        funcs.extend(function(text,sig) for sig in sigs)
    actual='\n'.join(funcs)
    arrays='\n'.join(re.findall(r'inline constexpr std::array<const char\*,\s*\d+>\s+\w+\s*\{[^}]+\};',params))
    ids='\n'.join(re.findall(r'inline constexpr (?:auto\*|const char\*|auto)\s+\w+\s*=\s*"[^"]+";',params))
    # Source currently declares most IDs with constexpr const char*. Include all,
    # then generate only missing scalar IDs referenced by the extracted functions.
    used=set(re.findall(r'qqsc::params::(\w+)',actual+function(hdr,'struct ParameterSnapshot')))
    present=set(re.findall(r'\b(\w+)\s*(?:=|\{)',arrays+ids))
    known={'stereoLinked','midSide','leftRight','keyInternal','classicAlgorithm','singleCompression','thresholdOffDb','rangeOffDb','keyHpfOffHz',
           'limiterRatio','upwardRatio','isThresholdEnabled','isRangeEnabled','thresholdLinear','rangeLinear','snapLookaheadMs'}
    for name in sorted(used-present-known):
        ids+=f'\ninline constexpr const char* {name}="{name}";'
    pm='\n'.join(function(params,s) for s in ['inline float limiterRatio (','inline float upwardRatio (',
        'inline bool isThresholdEnabled (','inline float thresholdLinear (','inline bool isRangeEnabled (','inline float rangeLinear ('])
    curves=math[math.index('    static float gainForLevel ('):math.index('    float getCurrentLevel()')]
    snapshot=function(hdr,'struct ParameterSnapshot').rstrip()+';'
    transfer=function((src/'ABTransfer.h').read_text(),'struct ABTransfer').rstrip()+';'
    initial=re.search(r'const float initial=([^;]+);',proc).group(1)
    normalmax=re.search(r'auto addMakeup = \[&\] \(const char\* id, const juce::String& name, float maximum = ([^)]+)\)',proc).group(1)
    return (CPP.replace('// @@IDS@@',ids+'\n'+arrays).replace('// @@PARAM_MATH@@',pm)
        .replace('// @@CURVES@@',curves).replace('// @@TRANSFER_STRUCT@@',transfer).replace('// @@SNAPSHOT@@',snapshot)
        .replace('// @@LINK_REFERENCE@@',function(hdr,'float getLimiterLinkReferencePeakDb'))
        .replace('// @@ACTUAL_FUNCTIONS@@',actual).replace('/*@@LIMITER_INITIAL@@*/',initial).replace('/*@@NORMAL_MAX@@*/',normalmax))

def main() -> int:
    ap=argparse.ArgumentParser();ap.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]);ap.add_argument('--sanitize',action='store_true');ap.add_argument('--keep',type=Path)
    ap.add_argument('--negative-control',choices=['entry','wet-reference']);ap.add_argument('--baseline',type=Path)
    a=ap.parse_args();cxx=shutil.which('g++') or shutil.which('clang++')
    if not cxx: ap.error('A C++17 compiler is required')
    with tempfile.TemporaryDirectory(prefix='qqsc129_') as tmp:
        work=a.keep or Path(tmp);work.mkdir(parents=True,exist_ok=True);cpp=work/'actual129.cpp';generated=generate(a.source)
        if a.negative_control:
            if not a.baseline: ap.error('--baseline is required for negative controls')
            file,signature = ('LimiterMode.cpp','void QQSuperCompressionAudioProcessor::enterLimiterMode') if a.negative_control=='entry' else ('PluginProcessor.h','float getLimiterLinkReferencePeakDb')
            old=function((a.baseline/'Source'/file).read_text(),signature)
            new=function((a.source/'Source'/file).read_text(),signature)
            assert generated.count(new)==1
            generated=generated.replace(new,old)
            print('NEGATIVE CONTROL: restore only baseline '+signature+' into otherwise-current harness',flush=True)
        cpp.write_text(generated)
        binary=work/'actual129';cmd=[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-I',str(a.source/'Source'),str(cpp),'-o',str(binary)]
        if a.sanitize:cmd[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        print('Compiling actual source functions:',a.source.resolve(),flush=True)
        subprocess.run(cmd,check=True);return subprocess.run([str(binary)],check=False).returncode
if __name__=='__main__':raise SystemExit(main())
