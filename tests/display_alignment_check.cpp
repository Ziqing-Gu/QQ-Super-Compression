#include "PluginEditor.h"
#include <deque>
#include <iostream>
#include <iomanip>

namespace
{
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void value(QQSuperCompressionAudioProcessor& p, const char* id, float v)
{
    auto* parameter=p.getAPVTS().getParameter(p.soundParameterID(id));
    require(parameter!=nullptr,"Missing parameter");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}
float db(float x) { return juce::Decibels::gainToDecibels(x,-120.f); }
float signal(int n,double rate,int channel,bool key)
{
    if(n<0)return 0;
    const int step=int(double(n)*1000/rate)+(channel?11:0)+(key?7:0);
    const int phase=step%83;
    const float amplitude=phase<19?.39f:phase<31?.025f:phase<57?.22f:.055f;
    return amplitude*(channel?.73f:1.f);
}
std::vector<float> detectorOracle(const std::vector<float>& source,int window)
{
    const int length=int(source.size());std::vector<float> past(length),future(length),result(length);
    std::deque<int> q;
    for(int i=0;i<length;++i)
    {
        while(!q.empty() && q.front()<i-window)q.pop_front();
        while(!q.empty() && source[q.back()]<=source[i])q.pop_back();
        q.push_back(i);past[i]=source[q.front()];
    }
    q.clear();
    for(int i=length-1;i>=0;--i)
    {
        while(!q.empty() && q.front()>i+window)q.pop_front();
        while(!q.empty() && source[q.back()]<=source[i])q.pop_back();
        q.push_back(i);future[i]=source[q.front()];
        result[i]=std::min(past[i],future[i]);
    }
    return result;
}
void configure(QQSuperCompressionAudioProcessor& p,double rate,int block,int look,int os,bool external,bool limiter)
{
    p.setEcoMode(false);p.setEditorOpen(true);if(limiter)p.enterLimiterMode();
    if(external)
    {
        auto layout=p.getBusesLayout();layout.inputBuses.set(1,juce::AudioChannelSet::stereo());
        require(p.setBusesLayout(layout),"Sidechain layout");
    }
    value(p,"processingMode",limiter?2.f:0.f);value(p,"keySource",external?1.f:0.f);
    value(p,"keyHpfHz",qqsc::params::keyHpfOffHz);value(p,"keyGainDb",0);
    value(p,"lookaheadMs",float(look));value(p,"detectorWindowMs",float(look));value(p,"oversampling",float(os));
    value(p,"detectorMode",0);value(p,"algorithmMode",0);value(p,"compressionMode",0);
    for(auto id:qqsc::params::ratioIds)value(p,id,200);
    for(auto id:qqsc::params::thresholdIds)value(p,id,-13.52f);
    for(auto id:qqsc::params::rangeIds)value(p,id,qqsc::params::rangeOffDb);
    for(auto id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})value(p,id,0);
    for(auto id:{"mix","mixL","mixR","mixM","mixS"})value(p,id,100);
    value(p,"inputGainDb",0);value(p,"outputGainDb",0);value(p,"limiterOutputDb",0);
    value(p,"limiterCalibrationDb",0);value(p,"ceilingDb",0);value(p,"truePeakLimiting",1);value(p,"limiterStereoLink",100);
    p.setRateAndBufferSizeDetails(rate,block);p.prepareToPlay(rate,block);
}
}

struct QQSCVisualCheck
{
    #include "clipper_checks.inc"
    #include "st_link_checks.inc"
    #include "window_roundtrip_checks.inc"
    #include "clipper_hold_checks.inc"
    static void profile(const juce::File& root,bool capture)
    {
        juce::MemoryOutputStream evidence;
        for(int mode:{0,2,1,3})for(bool drag:{false,true})
        {
            QQSuperCompressionAudioProcessor p;configure(p,48000,256,26,0,true,mode==3);
            const int processing=mode==3?2:mode;value(p,"processingMode",float(processing));
            DynamicDisplay d(p);d.stopTimer();d.setSize(1000,530);d.renderReferenceSampleRate=48000;
            for(int ch=0;ch<2;++ch)for(int n=0;n<480;++n)
            {
                DynamicDisplay::HistoryPoint point;point.captureCounter=uint64_t(n+1)*800;
                point.captureGeneration=1;point.inputDb=-28.f+15.f*std::sin(float(n)*.137f+float(ch));
                point.detectorDb=point.inputDb+3.f;point.externalKeyDb=point.inputDb-2.f;
                point.truePeakExcessDb=.3f;d.pushHistory(d.histories[size_t(ch)],point);
            }
            juce::Image image(juce::Image::RGB,1000,530,true);
            std::vector<double> cacheTimes,paintTimes;
            for(int frame=0;frame<240;++frame)
            {
                d.renderReferenceCounter=384000+uint64_t(frame%3)*200;
                if(drag)for(auto id:qqsc::params::ratioIds)value(p,id,2.f+float(frame%17));
                const auto start=juce::Time::getMillisecondCounterHiRes();d.refreshRenderCaches(processing);
                const auto projected=juce::Time::getMillisecondCounterHiRes();
                {juce::Graphics g(image);d.paint(g);}
                const auto painted=juce::Time::getMillisecondCounterHiRes();
                if(frame>=20){cacheTimes.push_back(projected-start);paintTimes.push_back(painted-projected);}
                // Exact curve/path evidence protects retrospective responsiveness and peaks.
                const auto append=[&](const DynamicDisplay::RenderCache& c)
                {
                    const auto& v=c.projected;
                    for(const auto* a:{&v.input,&v.output,&v.gainReductionBoundary,&v.effectiveGainReduction,&v.externalKey})
                        evidence.write(a->data(),v.size*sizeof(float));
                    for(const auto* path:{&c.inputPath,&c.outputPath,&c.gainReductionPath,&c.gainIncreasePath,
                                         &c.gainReductionShadePath,&c.gainIncreaseShadePath,&c.externalKeyPath})path->writePathToStream(evidence);
                };
                if(mode==3)append(d.stereoLimiterCache);
                else {append(d.renderCaches[0]);if(mode!=0)append(d.renderCaches[1]);}
            }
            std::sort(cacheTimes.begin(),cacheTimes.end());std::sort(paintTimes.begin(),paintTimes.end());
            std::cout<<"profile mode="<<mode<<" drag="<<drag<<" cache-ms median="<<cacheTimes[110]<<" p95="<<cacheTimes[209]
                     <<" paint-ms median="<<paintTimes[110]<<" p95="<<paintTimes[209]<<std::endl;
        }
        const auto file=root.getChildFile("render-evidence.bin");
        if(capture)require(file.replaceWithData(evidence.getData(),evidence.getDataSize()),"Write render baseline");
        else
        {
            juce::MemoryBlock old;require(file.loadFileAsData(old),"Read render baseline");
            require(old.getSize()==evidence.getDataSize() && std::memcmp(old.getData(),evidence.getData(),old.getSize())==0,
                    "Optimisation changed projected curves or visible paths");
        }
        std::cout<<"PASS: render profile "<<(capture?"captured":"exactly preserved")<<std::endl;
    }

    static void safeControls(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;
        juce::PropertiesFile::Options options;options.applicationName="QQSCDisplayAlignmentCheck";
        auto settings=std::make_unique<juce::PropertiesFile>(root.getChildFile("safe-test.settings"),options);
        QQSuperCompressionAudioProcessorEditor e(p,std::move(settings));e.stopTimer();e.display.stopTimer();
        int cases=0;
        for(int family:{0,1,2})for(int slot:{0,1})
        {
            p.selectABSlot(slot);p.setProcessorModeFromEditor(family);
            for(int look:{0,26,100})for(float window:{0.f,.01f,10.f})for(bool safe:{false,true})
            {
                value(p,"lookaheadMs",float(look));value(p,"detectorWindowMs",window);value(p,"detectorMode",safe?1.f:0.f);
                e.refreshDetectorWindowControl();const bool available=look>0 && window>0;
                require(e.detectorModeButton.isEnabled()==available,"SAFE enable state");
                require(e.detectorModeButton.getToggleState()==(available&&safe),"SAFE visible toggle state");
                require((p.readSoundParameter("detectorMode")>=.5f)==safe,"SAFE preference must survive zero Window");
                ++cases;
            }
        }
        std::cout<<"PASS: SAFE controls "<<cases<<" combinations including A/B and Limiter"<<std::endl;
    }

    static void run(const juce::File& root,bool capture)
    {
        int cases=0,points=0;float worstDetector=0,worstInput=0,worstOvershoot=0;
        for(double rate:{48000.,96000.})for(int block:{17,257})for(int look:{0,26})
        for(int os:{0,1,2,3})for(bool external:{false,true})for(bool limiter:{false,true})
        {
            QQSuperCompressionAudioProcessor p;configure(p,rate,block,look,os,external,limiter);
            DynamicDisplay d(p);d.stopTimer();d.setSize(1000,530);
            // Inspect the live path, before asynchronous history replay can hide the fault.
            d.hpfNextReplayMs=std::numeric_limits<double>::max();
            const int length=int(rate*.18),window=int(rate*look*.001),latency=p.getLatencySamples();
            std::array<std::vector<float>,2> raw,key,detector;
            for(int ch=0;ch<2;++ch)
            {
                for(int n=0;n<length+window;++n){raw[ch].push_back(signal(n,rate,ch,false));key[ch].push_back(signal(n,rate,ch,external));}
                if(os==0)detector[ch]=detectorOracle(key[ch],window);
            }
            juce::MemoryOutputStream audioEvidence;juce::MidiBuffer midi;
            for(int offset=0;offset<length;offset+=block)
            {
                const int count=std::min(block,length-offset);juce::AudioBuffer<float> audio(external?4:2,count);
                for(int i=0;i<count;++i)for(int ch=0;ch<2;++ch)
                {audio.setSample(ch,i,raw[ch][offset+i]);if(external)audio.setSample(ch+2,i,key[ch][offset+i]);}
                p.processBlock(audio,midi);
                for(int ch=0;ch<2;++ch)audioEvidence.write(audio.getReadPointer(ch),size_t(count)*sizeof(float));
                d.timerCallback();d.refreshRenderCaches(limiter?2:0);
                if(os!=0 || offset<latency+window || d.histories[0].points.empty())continue;
                std::array<float,2> inputPeak{},detectorPeak{};
                for(int n=offset-latency;n<offset+count-latency;++n)for(int ch=0;ch<2;++ch)
                {inputPeak[ch]=std::max(inputPeak[ch],raw[ch][n]);detectorPeak[ch]=std::max(detectorPeak[ch],detector[ch][n]);}
                if(!limiter){inputPeak[0]=std::max(inputPeak[0],inputPeak[1]);detectorPeak[0]=std::max(detectorPeak[0],detectorPeak[1]);}
                for(int ch=0;ch<(limiter?2:1);++ch)
                {
                    const auto& point=d.histories[size_t(ch)].points.back();
                    worstDetector=std::max(worstDetector,std::abs(point.detectorDb-db(detectorPeak[ch])));
                    worstInput=std::max(worstInput,std::abs(point.inputDb-db(inputPeak[ch])));++points;
                }
                if(!external)
                {
                    const auto& v=limiter?d.stereoLimiterCache.projected:d.renderCaches[0].projected;
                    // A matching internal peak at Ratio 200 cannot leave a large uncompressed spike.
                    if(v.size>0)worstOvershoot=std::max(worstOvershoot,v.output[v.size-1]-(-13.45f));
                }
            }
            const auto file=root.getChildFile("audio-"+juce::String(cases)+".bin");
            if(capture)require(file.replaceWithData(audioEvidence.getData(),audioEvidence.getDataSize()),"Write baseline audio");
            else
            {
                juce::MemoryBlock original;require(file.loadFileAsData(original),"Read baseline audio");
                require(original.getSize()==audioEvidence.getDataSize() && std::memcmp(original.getData(),audioEvidence.getData(),original.getSize())==0,
                        "Display fix changed audio samples");
            }
            ++cases;
        }
        std::cout<<"cases="<<cases<<" native-live-points="<<points<<" max-detector-error-db="<<worstDetector
                 <<" max-input-error-db="<<worstInput<<" max-false-overshoot-db="<<worstOvershoot<<std::endl;
        if(!capture){require(worstDetector<.003f,"Detector and carrier timestamps differ");require(worstInput<.003f,"Input history alignment differs");require(worstOvershoot<.003f,"False uncompressed spike remains");}
        std::cout<<(capture?"BASELINE captured":"PASS: aligned live Display and bit-identical audio")<<std::endl;
    }
};
int main(int argc,char** argv)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;require(argc==3,"Usage: directory capture|verify");
        const juce::File root(argv[1]);require(root.createDirectory().wasOk(),"Create output directory");
        const juce::String action(argv[2]);
        if(action.startsWith("profile"))QQSCVisualCheck::profile(root,action=="profile-capture");
        else if(action=="clipper")QQSCVisualCheck::clipperChecks(root);
        else if(action=="st-links")QQSCVisualCheck::stLinkChecks(root);
        else if(action=="safe")QQSCVisualCheck::safeControls(root);
        else if(action=="window-roundtrip")QQSCVisualCheck::windowRoundtripChecks(root);
        else if(action=="clipper-hold")QQSCVisualCheck::clipperHoldChecks(root);
        else QQSCVisualCheck::run(root,action=="capture");return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<std::endl;return 1;}
}
