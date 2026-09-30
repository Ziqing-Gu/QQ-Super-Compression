#define main HistoricalDynamicsMain
#include "updown_processor_test.cpp"
#undef main
#include "PluginEditor.h"
#include <numeric>
#include <chrono>

struct QQSCVisualCheck
{
    static void ecoHiddenReplay()
    {
        QQSuperCompressionAudioProcessor p;DynamicDisplay display(p);
        DynamicDisplay::HistoryPoint point;point.captureGeneration=1;point.captureCounter=10000;
        display.histories[0].points.push_back(point);
        p.setEcoMode(true);p.setEditorOpen(false);
        check(!display.requestHpfHistoryRefresh(),"hidden ECO scheduled HPF replay");
        const auto before=display.replayRequestGeneration->load();display.timerCallback();
        check(display.replayRequestGeneration->load()==before+1 && !display.hpfReplayBusy,"hidden ECO did not cancel pending replay");
        std::cout<<"PASS hidden retained display blocks and cancels replay\n";
    }
};

struct QQSCReviewCheck
{
    static void configure(QQSuperCompressionAudioProcessor& p,int mode,int core,int ceiling,bool limiter=true)
    {
        set(p,"limiterMode",limiter?1.f:0.f);
        set(p,"truePeakLimiting",0);set(p,"ceilingOversampling",float(ceiling==3 ? qqsc::params::ceiling4x : qqsc::params::migrateLegacyOversamplingChoice(ceiling)));
        set(p,p.soundParameterID("lookaheadMs"),0);set(p,p.soundParameterID("oversampling"),float(core==3 ? qqsc::params::os4x : qqsc::params::migrateLegacyOversamplingChoice(core)));
        set(p,p.soundParameterID("processingMode"),float(mode));
        set(p,p.soundParameterID("inputGainDb"),0);set(p,"outputGainDb",0);
        set(p,"limiterOutputDb",0);set(p,"limiterCalibrationDb",0);set(p,"ceilingDb",0);
        for(const char* id:qqsc::params::ratioIds)set(p,p.soundParameterID(id),1);
    }
    static std::vector<float> stream(QQSuperCompressionAudioProcessor& p,double rate,const std::vector<int>& sizes,int samples,int channels=2)
    {
        juce::MidiBuffer midi;std::vector<float> out;int offset=0;size_t cycle=0;
        while(offset<samples)
        {
            int count=std::min(sizes[cycle++%sizes.size()],samples-offset);
            juce::AudioBuffer<float> b(channels,count);
            for(int i=0;i<count;++i)for(int c=0;c<channels;++c)
                b.setSample(c,i,float(.08*std::sin(2*pi*(c?619:431)*(offset+i)/rate)));
            p.processBlock(b,midi);
            for(int i=0;i<count;++i)for(int c=0;c<channels;++c)
            {float x=b.getSample(c,i);check(std::isfinite(x),"nonfinite output");out.push_back(x);}
            offset+=count;
        }
        return out;
    }
    static void blocksAndEco()
    {
        double worst=0;
        for(double rate:{44100.,48000.,96000.})for(auto choices:{std::pair<int,int>{1,1},{2,2},{1,2},{2,1},{0,0},{0,3},{1,3},{2,3},{3,3},{3,1},{3,2}})
        {
            QQSuperCompressionAudioProcessor p,q;
            configure(p,0,choices.first,choices.second);configure(q,0,choices.first,choices.second);
            p.setRateAndBufferSizeDetails(rate,17);q.setRateAndBufferSizeDetails(rate,17);
            p.prepareToPlay(rate,17);q.prepareToPlay(rate,17);
            check(p.truePeakBuffer.getNumSamples()>=p.configuredMaximumBlockSize,"TP buffer smaller than admitted blocks");
            auto a=stream(p,rate,{17},8192),b=stream(q,rate,{1,31,128,512,2048,3},8192);
            check(a.size()==b.size(),"render length");
            for(size_t i=0;i<a.size();++i)worst=std::max(worst,double(std::abs(a[i]-b[i])));
            check(p.getLatencySamples()==q.getLatencySamples(),"variable-block PDC");
        }
        check(worst<2e-6,"audio depends on host block partition");
        for(int os:{1,2})
        {
            QQSuperCompressionAudioProcessor p,q;configure(p,0,os,os);configure(q,0,os,os);
            set(p,"truePeakLimiting",1);set(q,"truePeakLimiting",1);
            p.setEcoMode(false);p.prepareToPlay(48000,128);
            auto a=stream(p,48000,{128},12000);
            q.setEcoMode(true);q.prepareToPlay(48000,128);
            auto b=stream(q,48000,{128},12000);
            check(a==b,"ECO alters shared Limiter sound");
            q.gainReductionHoldDb[0]=20;q.gainReductionHoldSamplesRemaining[0]=90000;q.gainReductionHoldMode=0;
            q.setEditorOpen(true);stream(q,48000,{128},128);
            check(std::abs(q.getMeterState().gainReductionHoldDb0.load())<.01,"ECO wakes stale GR Hold");
        }
        std::cout<<"PASS block partition/rate/shared+fallback/ECO audio, max delta="<<worst<<"\n";
    }
    static void silenceProfile()
    {
        struct Config {const char* name;bool limiter;float lookahead;int core,ceiling;};
        const Config configurations[]={{"Normal26",false,26,0,1},{"Normal0-16",false,0,2,1},
            {"Limiter26-8",true,26,0,1},{"Limiter0-16shared",true,0,2,2},{"Limiter0-8core16ceiling",true,0,1,2}};
        std::cout<<std::unitbuf<<"PROFILE config,eco,silent,sleep_allowed,us_per_block,one_core_percent\n";
        for(const auto& c:configurations)for(bool eco:{false,true})for(bool silent:{true,false})for(bool allowSleep:{false,true})
        {
            QQSuperCompressionAudioProcessor p;configure(p,0,c.core,c.ceiling,c.limiter);
            set(p,p.soundParameterID("lookaheadMs"),c.lookahead);set(p,"truePeakLimiting",1);
            p.setEcoMode(eco);p.setEditorOpen(false);p.prepareToPlay(48000,256);
            Transport host;p.setPlayHead(&host);
            juce::MidiBuffer midi;juce::AudioBuffer<float> block(2,256),source(2,256);
            for(int i=0;i<256;++i)for(int ch=0;ch<2;++ch)
                source.setSample(ch,i,silent?0.f:float(.08*std::sin(2*pi*(ch?619:431)*i/48000.)));
            for(int i=0;i<256;++i){block.makeCopyOf(source,true);if(!allowSleep)p.stoppedSilentSamples=0;p.processBlock(block,midi);}
            constexpr int repeats=512;
            const auto start=std::chrono::steady_clock::now();
            for(int i=0;i<repeats;++i){block.makeCopyOf(source,true);if(!allowSleep)p.stoppedSilentSamples=0;p.processBlock(block,midi);}
            const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            check(!silent || !allowSleep || p.stoppedDspSleeping,"Profile did not enter sleep");
            p.setPlayHead(nullptr);
            std::cout<<c.name<<","<<eco<<","<<silent<<","<<allowSleep<<","<<seconds*1e6/repeats<<","<<seconds/(repeats*256./48000.)*100<<"\n";
        }
    }
    struct Transport final:juce::AudioPlayHead
    {
        bool playing=false,available=true,recording=false;int64_t sample=0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            if(!available)return {};
            PositionInfo info;info.setIsPlaying(playing);info.setIsRecording(recording);info.setTimeInSamples(sample);return info;
        }
    };
    static void stoppedSilence()
    {
        double worst=0;
        struct Config {int core,ceiling,recovery,mode;bool limiter,mono;double rate;float look;};
        const Config configs[]={{2,2,0,0,true,false,48000,0},{2,2,2,1,true,false,48000,0},
            {1,2,1,2,true,false,48000,0},{0,1,2,0,true,true,44100,26},
            {2,1,1,0,true,false,96000,0},{2,0,1,1,false,true,48000,0},
            {0,0,1,2,false,false,48000,52},{0,3,0,0,true,false,48000,26},{2,3,2,0,true,false,44100,0},{3,3,1,0,true,false,48000,0},{3,0,1,0,false,false,96000,0}};
        for(const auto& c:configs)
        {
            QQSuperCompressionAudioProcessor p,reference;Transport host;
            for(auto* processor:{&p,&reference})
            {
                auto layout=processor->getBusesLayout();
                if(c.mono){layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());}
                layout.inputBuses.set(1,juce::AudioChannelSet::stereo());
                check(processor->setBusesLayout(layout),"silence sidechain layout");
                configure(*processor,c.mode,c.core,c.ceiling,c.limiter);
                processor->setEcoMode(false);
                set(*processor,processor->soundParameterID("lookaheadMs"),c.look);
                set(*processor,"truePeakLimiting",1);set(*processor,"tpRecoveryMode",float(c.recovery));
                set(*processor,"ceilingDb",-6);
                processor->setPlayHead(&host);processor->prepareToPlay(c.rate,17);
            }
            juce::MidiBuffer midi;size_t cycle=0;int slept=0;double firstSleep=-1,stoppedTime=0;
            const int sizes[]={17,256,1,1024,63,4096};
            const auto render=[&](int count,int signal)
            {
                for(int done=0;done<count;)
                {
                    const int n=std::min(count-done,sizes[cycle++%6]);
                    juce::AudioBuffer<float> a(c.mono?3:4,n),b(c.mono?3:4,n);a.clear();
                    for(int i=0;i<n;++i)
                    {
                        if(signal==1)for(int ch=0;ch<(c.mono?1:2);++ch)
                            a.setSample(ch,i,float(1.8*std::sin(2*pi*(ch?73:997)*(host.sample+i)/c.rate)));
                        if(signal==2 && done==0 && i==0)a.setSample(0,i,1.e-15f);
                        if(signal==3)a.setSample(c.mono?1:2,i,float(.4*std::sin(2*pi*113*(host.sample+i)/c.rate)));
                    }
                    b.makeCopyOf(a,true);
                    reference.stoppedSilentSamples=0; // Same stopped host, original always-running DSP.
                    p.processBlock(a,midi);reference.processBlock(b,midi);
                    for(int ch=0;ch<(c.mono?1:2);++ch)for(int i=0;i<n;++i)
                    {
                        check(std::isfinite(a.getSample(ch,i)),"silence wake produced nonfinite audio");
                        worst=std::max(worst,double(std::abs(a.getSample(ch,i)-b.getSample(ch,i))));
                    }
                    if(p.stoppedDspSleeping){++slept;if(firstSleep<0)firstSleep=stoppedTime;}
                    if(!host.playing)stoppedTime+=n/c.rate;
                    if(host.playing || !host.available || signal==3 || (signal==2 && done==0))
                        check(!p.stoppedDspSleeping,"play/live/sidechain/missing-transport did not wake DSP");
                    check(p.getLatencySamples()==reference.getLatencySamples(),"silence sleep changed PDC");
                    host.sample+=n;done+=n;
                }
            };
            host.playing=true;render(int(c.rate*.25),1);
            p.refreshMatchResults();const auto history=p.loudnessMatch.getBlockCount();
            host.playing=false;render(int(c.rate*6),0);
            check(p.stoppedDspSleeping && slept>0,"Stopped Limiter never reached sleep after recovery");
            p.refreshMatchResults();check(p.loudnessMatch.getBlockCount()==history,"Stop sleep reset FULL MATCH history");
            check(p.meterState.outputDb0.load()<=-119,"Stopped meter did not clear");
            render(int(c.rate*.1),2); // Exact input detection, not a noise gate.
            render(int(c.rate*1.2),0);check(p.stoppedDspSleeping,"Tiny input prevented later sleep");
            for(auto* processor:{&p,&reference})set(*processor,processor->soundParameterID("mix"),37);
            render(17,0);check(!p.stoppedDspSleeping,"Parameter edit did not wake DSP");
            render(int(c.rate*1.2),0);
            for(auto* processor:{&p,&reference})set(*processor,processor->soundParameterID("keySource"),1);
            render(int(c.rate*1.2),0);check(p.stoppedDspSleeping,"EXT zero prevented sleep");
            render(int(c.rate*.15),3); // Stopped external sidechain remains active.
            for(auto* processor:{&p,&reference})set(*processor,processor->soundParameterID("keySource"),0);
            render(int(c.rate*1.2),0);
            host.available=false;render(17,0);host.available=true;
            host.playing=true;render(int(c.rate*.2),1);
            p.setPlayHead(nullptr);reference.setPlayHead(nullptr);
            std::cout<<"SILENCE rate="<<c.rate<<" core="<<c.core<<" ceiling="<<c.ceiling
                     <<" recovery="<<c.recovery<<" first_sleep_s="<<firstSleep<<" max_delta="<<worst<<"\n";
        }
        check(worst<1.e-6,"Stopped silence/wake differs from continuously processed reference");
        std::cout<<"PASS stopped silence: drain/recovery, wake audio, tiny input, sidechain, automation, transport, FULL history, mono/stereo, variable blocks and PDC; max delta="<<worst<<"\n";
    }
    static void instanceEco(const juce::File& root)
    {
        using Processor=QQSuperCompressionAudioProcessor;
        juce::PropertiesFile::Options initialOpts;initialOpts.storageFormat=juce::PropertiesFile::storeAsXML;initialOpts.millisecondsBeforeSaving=0;
        const auto initialFile=root.getChildFile("instance-initial-default.settings");
        {juce::PropertiesFile f(initialFile,initialOpts);f.setValue("lastPerformanceEco",false);f.saveIfNeeded();}
        const auto initialPrefs=[&]{return std::make_unique<juce::PropertiesFile>(initialFile,initialOpts);};
        std::array<std::unique_ptr<Processor>,10> instances;
        for(auto& p:instances)
        {
            p=std::make_unique<Processor>(initialPrefs());
            check(!p->isEcoMode() && p->shouldRunUiAnalysis(),"New instance must default to FULL");
            p->setEcoMode(true);
        }
        instances[3]->setEcoMode(false);
        const auto checkChoices=[&]
        {
            for(size_t i=0;i<instances.size();++i)
                check(instances[i]->isEcoMode()==(i!=3) && instances[i]->shouldRunUiAnalysis()==(i==3),
                      "Ten-instance FULL/ECO isolation failed");
        };
        checkChoices();
        // Exercise the real DSP and MATCH gates with one hidden FULL and one
        // hidden ECO instance processing exactly the same transport and audio.
        auto& full=*instances[3];auto& eco=*instances[7];
        struct Playback final:juce::AudioPlayHead
        {
            int64_t sample=0;
            juce::Optional<PositionInfo> getPosition() const override
            { PositionInfo info;info.setIsPlaying(true);info.setTimeInSamples(sample);return info; }
        } play;
        configure(full,0,0,0,false);configure(eco,0,0,0,false);
        full.setPlayHead(&play);eco.setPlayHead(&play);
        full.prepareToPlay(48000,128);eco.prepareToPlay(48000,128);
        juce::MidiBuffer midi;
        const auto renderPair=[&](int count)
        {
            for(int offset=0;offset<count;offset+=128)
            {
                const int n=std::min(128,count-offset);juce::AudioBuffer<float> a(2,n),b(2,n);
                for(int i=0;i<n;++i)for(int c=0;c<2;++c)
                {
                    const auto x=float(.08*std::sin(2*pi*(c?619:431)*(play.sample+i)/48000.));
                    a.setSample(c,i,x);b.setSample(c,i,x);
                }
                full.processBlock(a,midi);eco.processBlock(b,midi);
                for(int i=0;i<n;++i)for(int c=0;c<2;++c)
                    check(a.getSample(c,i)==b.getSample(c,i),"Independent ECO state changed audio");
                play.sample+=n;
            }
            full.refreshMatchResults();eco.refreshMatchResults();
        };
        renderPair(48000);
        check(full.hasMatchData() && !eco.hasMatchData(),"Hidden FULL/ECO MATCH gates are coupled");
        check(full.meterState.inputDb0.load()>-100 && eco.meterState.inputDb0.load()<=-119,
              "Hidden FULL/ECO meter gates are coupled");
        const auto fullBlocks=full.loudnessMatch.getBlockCount();
        eco.setEditorOpen(true);renderPair(48000);
        check(eco.isEcoMode() && eco.hasMatchData(),"Opening ECO editor failed to wake its analysis");
        check(full.loudnessMatch.getBlockCount()>fullBlocks,"Another editor reset FULL history");
        eco.setEditorOpen(false);renderPair(128);
        check(!eco.hasMatchData() && full.hasMatchData(),"Closing ECO editor affected FULL analysis");
        check(full.getLatencySamples()==eco.getLatencySamples(),"Instance performance mode changed latency");
        full.setPlayHead(nullptr);eco.setPlayHead(nullptr);checkChoices();
        for(size_t i=0;i<instances.size();++i)
        {
            juce::MemoryBlock state;instances[i]->getStateInformation(state);
            Processor restored;restored.setEcoMode(i==3);
            restored.setStateInformation(state.getData(),int(state.getSize()));
            const bool expected=i!=3;
            check(restored.isEcoMode()==expected,"Project roundtrip lost instance FULL/ECO");
            restored.copyAToB();restored.selectABSlot(1);
            check(restored.isEcoMode()==expected,"A/B recalled a performance mode");
            restored.setEcoMode(!expected);restored.selectABSlot(0);
            check(restored.isEcoMode()!=expected,"A/B overwrote a later performance selection");
        }
        // Missing-property migration must also clear a previously ECO instance.
        juce::MemoryBlock legacy;eco.getStateInformation(legacy);
        auto xml=juce::AudioProcessor::getXmlFromBinary(legacy.getData(),int(legacy.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);tree.removeProperty("qqscPerformanceEco",nullptr);
        tree.setProperty("qqscStateSchemaVersion",26,nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),legacy);
        Processor restored;restored.setEcoMode(true);restored.setStateInformation(legacy.getData(),int(legacy.getSize()));
        check(!restored.isEcoMode(),"Legacy state inherited a global/stale ECO selection");
        // Real editor buttons remain independent, ignore the old global key,
        // and display the instance's retained choice on reopen.
        Processor first(initialPrefs()),second(initialPrefs());second.setEcoMode(true);
        juce::PropertiesFile::Options opts;opts.storageFormat=juce::PropertiesFile::storeAsXML;opts.millisecondsBeforeSaving=0;
        const auto settings=root.getChildFile("instance-eco-ui.settings");
        {juce::PropertiesFile old(settings,opts);old.setValue("performanceEco",true);old.saveIfNeeded();}
        {
            QQSuperCompressionAudioProcessorEditor a(first,std::make_unique<juce::PropertiesFile>(settings,opts));
            QQSuperCompressionAudioProcessorEditor b(second,std::make_unique<juce::PropertiesFile>(settings,opts));
            check(a.performanceModeButton.getButtonText()=="FULL" && b.performanceModeButton.getButtonText()=="ECO",
                  "Editor adopted global/another instance performance mode");
            a.togglePerformanceMode();b.updatePerformanceModeUi();
            check(first.isEcoMode() && second.isEcoMode(),"First editor changed another instance");
            b.togglePerformanceMode();a.updatePerformanceModeUi();
            check(first.isEcoMode() && !second.isEcoMode() && a.performanceModeButton.getButtonText()=="ECO",
                  "Second editor changed another instance");
            a.toggleTheme();a.lookaheadCombo.setSelectedItemIndex(0,juce::dontSendNotification);a.commitLookaheadChoice();
            check(first.isEcoMode() && !second.isEcoMode(),"Theme/lookahead changed instance FULL/ECO");
        }
        check(first.isEcoMode() && !first.isEditorOpen(),"Closing editor lost instance performance state");
        {
            QQSuperCompressionAudioProcessorEditor reopened(first,std::make_unique<juce::PropertiesFile>(settings,opts));
            check(reopened.performanceModeButton.getButtonText()=="ECO","Editor reopen lost ECO");
        }
        {juce::PropertiesFile old(settings,opts);check(old.getBoolValue("performanceEco"),"Instance toggles wrote the legacy global preference");}
        checkChoices();
        std::cout<<"PASS ten-instance FULL/ECO isolation, live meters/MATCH, project and legacy state, A/B independence, editor controls/reopen, exact audio/PDC parity\n";
    }
    static void mono()
    {
        for(int os:{1,2})
        {
            QQSuperCompressionAudioProcessor p;
            auto layout=p.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());
            check(p.setBusesLayout(layout),"mono layout");configure(p,1,os,os);
            set(p,"limiterMakeupGainMDb",6.0206f);set(p,"limiterMakeupGainSDb",0);
            QQSuperCompressionAudioProcessor referenceProcessor;
            check(referenceProcessor.setBusesLayout(layout),"mono reference layout");configure(referenceProcessor,1,os,os);
            p.prepareToPlay(48000,128);referenceProcessor.prepareToPlay(48000,128);
            auto audio=stream(p,48000,{128},24000,1),referenceAudio=stream(referenceProcessor,48000,{128},24000,1);
            double sum=0,reference=0,idealReference=0;
            for(int i=12000;i<24000;++i)
            {
                sum+=double(audio[size_t(i)])*audio[size_t(i)];
                reference+=double(referenceAudio[size_t(i)])*referenceAudio[size_t(i)];
                const auto x=.08*std::sin(2*pi*431*(i-p.getLatencySamples())/48000.);idealReference+=x*x;
            }
            const auto expected=juce::Decibels::decibelsToGain(get(p,"limiterMakeupGainMDb"));
            const auto gain=std::sqrt(sum/reference);
            std::cout<<"MONO MS OS="<<(os==1?8:16)<<" filtered-reference gain="<<gain<<" expected="<<expected
                     <<" ideal-sine gain="<<std::sqrt(sum/idealReference)<<"\n";
            check(std::abs(gain-expected)<.0002,"mono MS uses Side in its output gain");
        }
        std::cout<<"PASS actual mono MS 8x/16x gain\n";
    }
    static void stateAndTransitions()
    {
        QQSuperCompressionAudioProcessor p;
        set(p,"ceilingOversampling",0);p.selectABSlot(1);set(p,"ceilingOversampling",2);p.selectABSlot(0);
        check(get(p,"ceilingOversampling")==0,"A failed to restore Ceiling OS");
        juce::MemoryBlock state;p.getStateInformation(state);
        QQSuperCompressionAudioProcessor q;q.setStateInformation(state.getData(),int(state.getSize()));q.selectABSlot(1);
        check(get(q,"ceilingOversampling")==2,"saved B Ceiling OS missing");
        configure(p,0,1,0,false);set(p,"truePeakLimiting",0);p.prepareToPlay(48000,128);
        p.abActive=true;p.abFade.setCurrentAndTargetValue(0);p.abFade.setTargetValue(1);
        set(p,"ceilingOversampling",2);p.updateProcessingConfiguration();
        check(p.abActive && p.abFade.getCurrentValue()==0,"hidden Ceiling OS cancels Normal A/B");
        set(p,"ceilingOversampling",0);set(p,"truePeakLimiting",1);p.updateProcessingConfiguration();
        check(p.abActive,"hidden TP promotion cancels Normal A/B");
        std::vector<std::pair<std::pair<int,uint32_t>,juce::String>> ordered;
        std::vector<juce::String> oldOrder;
        for(auto* param:p.getParameters())
        {
            auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(param);check(id!=nullptr,"ID parameter");
            const auto hash=uint32_t(id->paramID.hashCode())&0x7fffffffu;
            ordered.push_back({{param->getVersionHint(),hash},id->paramID});
            if(id->paramID!="ceilingOversampling")check(param->getVersionHint()==1,"old AU version hint changed");
        }
        std::sort(ordered.begin(),ordered.end());check(ordered.back().second=="ceilingOversampling","new AU parameter inserted among old IDs");
        std::cout<<"PASS A/B recall, hidden Normal settings, append-only AU ordering\n";
    }
    static void settings(const juce::File& root)
    {
        juce::PropertiesFile::Options o;o.storageFormat=juce::PropertiesFile::storeAsXML;o.millisecondsBeforeSaving=0;
        auto file=root.getChildFile("review-ui.settings");
        auto props=std::make_unique<juce::PropertiesFile>(file,o);props->setValue("performanceEco",false);props->saveIfNeeded();
        QQSuperCompressionAudioProcessor p;
        QQSuperCompressionAudioProcessorEditor editor(p,std::move(props));
        {juce::PropertiesFile other(file,o);other.setValue("performanceEco",true);other.saveIfNeeded();}
        editor.toggleTheme();
        {juce::PropertiesFile saved(file,o);check(saved.getBoolValue("performanceEco"),"Theme overwrites ECO preference");saved.setValue("performanceEco",false);saved.saveIfNeeded();}
        editor.lookaheadCombo.setSelectedItemIndex(0,juce::dontSendNotification);editor.commitLookaheadChoice();
        {juce::PropertiesFile saved(file,o);check(!saved.getBoolValue("performanceEco",true),"Lookahead overwrites FULL preference");}
        std::cout<<"PASS stale editor settings reload (isolated settings file)\n";
    }
    #include "revision1236_oversampling_checks.inc"
    #include "revision1237_stopped_idle_checks.inc"
    #include "revision1238_eco_checks.inc"
    #include "revision1239_diagnostics_checks.inc"
    #include "revision1240_input_checks.inc"
    #include "revision1241_transport_checks.inc"

    static void sharedPeak()
    {
        for(int factor:{8,16})
        {
            qqsc::OutputCeiling c;c.prepare(48000,false,0,qqsc::params::tpAuto,factor==8?qqsc::params::ceiling8x:qqsc::params::ceiling16x,4);
            juce::AudioBuffer<float> high(2,factor*4);high.clear();
            for(int b=0;b<4;++b){high.setSample(0,b*factor,.25f);high.setSample(0,b*factor+1,.5f);}
            c.processSharedOversampledBlock(juce::dsp::AudioBlock<float>(high),4,factor);
            c.finishSharedBaseSample(0,0,0);
            check(c.inputSamplePeakForDisplayLinear()==.25f && c.inputTruePeakForDisplayLinear()==.5f,"shared pre-Ceiling peak evidence includes repeated Dry");
        }
        std::cout<<"PASS shared pre-Ceiling sample-grid/phase peak telemetry\n";
    }
};
int main(int argc,char** argv)
{
    if(argc!=2 && argc!=3)return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        std::cout<<std::unitbuf;
        juce::File root(argv[1]);root.createDirectory();
        if(argc==3 && juce::String(argv[2])=="--silence-profile"){QQSCReviewCheck::silenceProfile();return 0;}
        if(argc==3 && juce::String(argv[2])=="--idle-1237"){QQSCReviewCheck::stoppedFastPath1237();QQSCReviewCheck::idleInstanceProfile1237();return 0;}
        if(argc==3 && juce::String(argv[2])=="--eco-1238"){QQSCReviewCheck::eco1238();return 0;}
        if(argc==3 && juce::String(argv[2])=="--diagnostics-1239"){QQSCReviewCheck::diagnostics1239(root);return 0;}
        if(argc==3 && juce::String(argv[2])=="--input-1240"){QQSCReviewCheck::inputDiagnostics1240();return 0;}
        if(argc==3 && juce::String(argv[2])=="--transport-1241"){QQSCReviewCheck::ecoTransport1241();return 0;}
        QQSCReviewCheck::ecoTransport1241();
        QQSCReviewCheck::inputDiagnostics1240();
        QQSCReviewCheck::diagnostics1239(root);
        QQSCReviewCheck::eco1238();
        QQSCReviewCheck::oversampling1236(root);
        QQSCReviewCheck::stoppedSilence();
        QQSCReviewCheck::stoppedFastPath1237();
        QQSCReviewCheck::instanceEco(root);
        QQSCVisualCheck::ecoHiddenReplay();
        QQSCReviewCheck::stateAndTransitions();QQSCReviewCheck::sharedPeak();
        QQSCReviewCheck::mono();QQSCReviewCheck::blocksAndEco();QQSCReviewCheck::settings(root);
        std::cout<<"PASS all review regression checks\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
