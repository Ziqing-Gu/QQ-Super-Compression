#define main HistoricalDynamicsMain
#include "updown_processor_test.cpp"
#undef main
#include "PluginEditor.h"

struct UnityPlayHead:juce::AudioPlayHead
{
    int64_t sample=0;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo p;p.setIsPlaying(true);p.setTimeInSamples(sample);return p; }
};

struct Session
{
    QQSuperCompressionAudioProcessor p;
    UnityPlayHead play;
    int64_t offset=0;
    Session(int algo,int mode,float mix=100)
    {
        baseline(p);set(p,"algorithmMode",float(algo));set(p,"processingMode",float(mode));
        for(const auto* id:qqsc::params::rangeIds)set(p,id,1);
        for(const auto* id:qqsc::params::ratioIds)set(p,id,20);
        p.enterLimiterMode();
        for(int d=0;d<5;++d)p.setBoundaryForDomainDb(false,false,d,-26);
        set(p,"limiterOutputDb",18);set(p,"limiterCalibrationDb",0);set(p,"ceilingDb",-3);
        set(p,p.soundParameterID("inputGainDb"),4);
        for(const auto* id:{"mix","mixL","mixR","mixM","mixS"})set(p,p.soundParameterID(id),mix);
        set(p,"limiterOutputDb",-p.getLimiterReferencePeakDb()); // Same linked update as the editor.
        p.setUnityMonitorEnabled(true);p.setPlayHead(&play);p.prepareToPlay(48000,256);
    }
    static std::array<float,2> signal(int64_t n)
    {
        if(n<0)return {};
        const double t=double(n)/48000;
        return {float(.12*(.8+.2*std::cos(2*pi*t))*(.6*std::sin(2*pi*240*t)+.4*std::sin(2*pi*3000*t))),
                float(.09*(.8+.2*std::cos(2*pi*t))*(.8*std::sin(2*pi*240*t+.6)+.2*std::sin(2*pi*2000*t)))};
    }
    double run(double seconds)
    {
        qqsc::BS1770LoudnessMatch measurement;measurement.prepare(48000);
        const int samples=int(seconds*48000);juce::MidiBuffer midi;
        for(int n=0;n<samples;n+=256)
        {
            const int count=std::min(256,samples-n);juce::AudioBuffer<float> b(2,count);
            for(int i=0;i<count;++i){const auto x=signal(offset+i);b.setSample(0,i,x[0]);b.setSample(1,i,x[1]);}
            play.sample=offset;p.processBlock(b,midi);
            if(n>=48000)
                for(int i=0;i<count;++i)
                {
                    const auto x=signal(offset+i-p.getLatencySamples());
                    const auto l=b.getSample(0,i),r=b.getSample(1,i);
                    measurement.processSample(x[0],x[1],l,r,l,r,0,0,0,0);
                }
            offset+=count;
        }
        return measurement.getLatestMatch().st;
    }
};

struct QQSCUnityCheck
{
    static void matchChecks(const juce::File&);
    static void strictMatchChecks();
    static void ui(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;baseline(p);
        juce::PropertiesFile::Options opts;opts.applicationName="UnityCheck";opts.storageFormat=juce::PropertiesFile::storeAsXML;
        QQSuperCompressionAudioProcessorEditor e(p,std::make_unique<juce::PropertiesFile>(root.getChildFile("ui.settings"),opts));
        e.setSize(1200,800);e.resized();
        check(!e.unityMonitorButton.isVisible()&&!p.isUnityMonitorEnabled(),"New instance monitor must be hidden/off");
        e.limiterButton.onClick();e.updateCompressionUi();
        check(p.isTruePeakSelected()&&e.unityMonitorButton.isVisible()&&e.truePeakButton.isVisible(),"Default TP/monitor visibility");
        check(e.unityMonitorButton.getRight()<e.outputGainLabel.getX(),"Headphones must be left of Output");
        e.truePeakButton.onClick();check(!p.isTruePeakSelected(),"TP cannot be disabled");
        e.truePeakButton.onClick();check(p.isTruePeakSelected(),"TP cannot be enabled");
        set(p,"limiterOutputDb",6);set(p,"ceilingDb",-1);
        e.unityMonitorButton.onClick();e.updateLimiterUi();
        check(e.monitorCeilingLabel.isVisible()&&e.monitorCeilingLabel.getText().contains("-7.00"),"Converted monitor ceiling not shown");
        check(e.ceilingValue.getText().contains("-1.00"),"Monitoring rewrote normal Ceiling");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();e.updateLimiterUi();
            for(int mode:{0,1,2})
            {
                set(p,p.soundParameterID("processingMode"),float(mode));e.updateModeUi();e.updateCompressionUi();e.resized();
                check(e.modeButton.getButtonText()==qqsc::params::modeName(mode),"Mode controls are stale");
                for(auto* child:e.contentRoot.getChildren())if(child->isVisible())check(e.contentRoot.getLocalBounds().contains(child->getBounds()),"Control outside editor");
                auto stream=root.getChildFile(juce::String("unity-")+qqsc::ui::themeKey(theme)+"-"+juce::String(mode)+".png").createOutputStream();
                check(stream!=nullptr,"Screenshot stream");
                stream->setPosition(0);stream->truncate();
                juce::PNGImageFormat().writeImageToStream(e.createComponentSnapshot(e.getLocalBounds()),*stream);
            }
        }
        p.copyAToB();e.unityMonitorButton.onClick();p.selectABSlot(1);e.updateCompressionUi();
        check(!p.isUnityMonitorEnabled(),"A/B recalled a monitoring state");
        e.unityMonitorButton.onClick();p.selectABSlot(0);e.updateCompressionUi();
        check(p.isUnityMonitorEnabled(),"A/B disabled 1:1 monitoring");
        juce::MemoryBlock state;p.getStateInformation(state);
        QQSuperCompressionAudioProcessor q;q.setStateInformation(state.getData(),int(state.getSize()));
        check(q.isUnityMonitorActive(),"Project lost 1:1 monitor state");
        // Old sessions without the workflow property restore OFF, including
        // loading them over an instance whose headphone button was already on.
        QQSuperCompressionAudioProcessor old;juce::MemoryBlock legacy;old.getStateInformation(legacy);
        q.setStateInformation(legacy.getData(),int(legacy.getSize()));
        check(!q.isUnityMonitorEnabled(),"Normal session left monitor latched");
        e.limiterButton.onClick();e.updateCompressionUi();
        check(!e.unityMonitorButton.isVisible()&&!e.truePeakButton.isVisible()&&!e.monitorCeilingLabel.isVisible()&&!e.ceilingValue.isVisible(),"OFF controls visible");
        std::cout<<"PASS: headphone placement/three themes/ST-LR-MS, TP default and toggle, converted ceiling, project persistence and A/B independence.\n";
    }
};

void unityAudioChecks()
{
    double worstError=0;
    for(int algo:{0,1})for(int mode:{0,1,2})for(float gain:{-6.f,6.f,18.f})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"algorithmMode",float(algo));set(p,"processingMode",float(mode));
        for(const auto* id:qqsc::params::ratioIds)set(p,id,1);
        p.enterLimiterMode();set(p,"limiterCalibrationDb",0);set(p,"limiterOutputDb",gain);set(p,"ceilingDb",-1);
        set(p,"truePeakLimiting",1);const auto normal=render(p,2,127,false,12000,48000,.7f);
        p.setUnityMonitorEnabled(true);const auto monitored=render(p,2,127,false,12000,48000,.7f);
        const double factor=std::pow(10.,-gain/20.);
        juce::AudioBuffer<float> b(2,int(monitored.output.size()));
        for(int i=0;i<b.getNumSamples();++i)
        {
            worstError=std::max(worstError,std::abs(monitored.output[size_t(i)]-normal.output[size_t(i)]*factor));
            worstError=std::max(worstError,std::abs(monitored.right[size_t(i)]-normal.right[size_t(i)]*factor));
            b.setSample(0,i,monitored.output[size_t(i)]);b.setSample(1,i,monitored.right[size_t(i)]);
        }
        juce::dsp::Oversampling<float> meter(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false);meter.initProcessing(size_t(b.getNumSamples()));
        const auto up=meter.processSamplesUp(juce::dsp::AudioBlock<float>(b));double peak=0;
        for(int ch=0;ch<2;++ch)for(int i=0;i<int(up.getNumSamples());++i)peak=std::max(peak,std::abs(double(up.getSample(ch,i))));
        const double measured=20*std::log10(peak);
        check(measured<=-1-gain+.01,"Converted TP ceiling exceeded");
        check(std::abs(p.getMeterState().truePeakHoldDb.load()-measured)<.2,"TP meter excluded monitor gain");
        const auto bypass=render(p,2,127,true,12000,48000,.7f);
        for(size_t i=0;i<bypass.output.size();++i)
        {
            const float x=i<size_t(bypass.latency)?0:2*float(std::sin(2*pi*12000*double(i-size_t(bypass.latency))/48000));
            check(bypass.output[i]==x,"Bypass inherited monitor attenuation/limiting");
        }
        set(p,"limiterMode",0);const auto off=render(p,.1f);p.setUnityMonitorEnabled(false);const auto off2=render(p,.1f);
        check(off.output==off2.output&&off.right==off2.right,"Monitor leaked into normal mode");
    }
    check(worstError<1.e-6,"1:1 altered the limited waveform instead of only gain");
    std::cout<<"PASS: 18 post-TP cancellation cases, both algorithms/all domains/+negative Output,  TP meter and converted bound, dry bypass, Limiter OFF parity; max error="<<worstError<<".\n";
}

void QQSCUnityCheck::matchChecks(const juce::File& root)
{
    double worst=0;
    for(int algo:{0,1})for(int mode:{0,1,2})for(float mix:{100.f,66.9f,50.f,10.f})
    {
        Session s(algo,mode,mix);
        juce::PropertiesFile::Options opts;opts.applicationName="MatchLinkCheck";opts.storageFormat=juce::PropertiesFile::storeAsXML;
        QQSuperCompressionAudioProcessorEditor editor(s.p,std::make_unique<juce::PropertiesFile>(root.getChildFile("match.settings"),opts));
        const double before=s.run(4);
        check(s.p.hasMatchData(),"Post-ceiling MATCH not ready");
        editor.matchButton.onClick();
        check(std::abs(get(s.p,"limiterOutputDb")+s.p.getLimiterReferencePeakDb()+get(s.p,"limiterCalibrationDb"))<.011,
              "Editor overwrote actual mixed-signal MATCH compensation");
        const double after=s.run(4);worst=std::max(worst,std::abs(after));
        std::cout<<"MATCH algo="<<algo<<" mode="<<mode<<" mix="<<mix<<" before="<<before<<" after="<<after<<" LU\n";
        check(std::abs(after)<.15,"Post-Ceiling MATCH error");
        set(s.p,"limiterOutputDb",get(s.p,"limiterOutputDb")+1);
        check(!s.p.hasMatchData(),"Parameter edit retained stale MATCH");
    }
    std::cout<<"MATCH worst measured error="<<worst<<" LU.\n";
}

void QQSCUnityCheck::strictMatchChecks()
{
    for(int algo:{0,1})for(bool linked:{false,true})
    {
        Session s(algo,0,66.9f);
        set(s.p,"limiterLink",linked ? 1.f : 0.f);
        set(s.p,"limiterRatio",1);set(s.p,"limiterInputGainDb",0);
        set(s.p,"limiterOutputDb",8.24f);set(s.p,"ceilingDb",0);
        // Apply a new curve before transport starts, then reset the engine as
        // a host does on activation. Do not include a prior mode-entry fade.
        s.p.prepareToPlay(48000,256);
        const auto error=s.run(4);
        std::cout<<"MATCH unity reference algo="<<algo<<" linked="<<linked<<" error="<<error<<" ready="<<s.p.hasMatchData()<<"\n";
        check(std::abs(error)<.05&&s.p.hasMatchData(),"Unity MATCH reference not ready");
        const auto output=get(s.p,"limiterOutputDb"),makeup=get(s.p,"limiterMakeupGainDb");
        check(s.p.applyMatchForCurrentMode(),"Unity MATCH failed");
        const auto correction=get(s.p,"limiterMakeupGainDb")-makeup;
        check(std::abs(correction)<.05,"Unity MATCH excessive Makeup correction");
        const auto expected=linked ? output-20*std::log10(.331+.669*std::pow(10.,correction/20.)) : output;
        check(std::abs(get(s.p,"limiterOutputDb")-expected)<.011,"MATCH re-calibrated Output instead of applying only Makeup delta");
    }
    std::cout<<"PASS: MATCH preserves manual Output offset after Ratio -> 1:1, Classic/Super and LINK on/off; only the measured Makeup correction contributes.\n";
}

void transitionChecks()
{
    QQSuperCompressionAudioProcessor p;baseline(p);set(p,"ratio",1);p.enterLimiterMode();set(p,"limiterOutputDb",6);set(p,"limiterCalibrationDb",0);
    p.prepareToPlay(48000,1);juce::MidiBuffer midi;float previous=0;double delta=0;double tailError=0;
    for(int i=0;i<48000*2;++i)
    {
        if(i==24000)p.setUnityMonitorEnabled(true);
        if(i==36000)set(p,"bypass",1);
        if(i==48000)set(p,"bypass",0);
        if(i==60000)p.setUnityMonitorEnabled(false);
        if(i==72000)set(p,"bypass",1);
        if(i==84000)set(p,"bypass",0);
        juce::AudioBuffer<float>b(2,1);b.setSample(0,0,.1f);b.setSample(1,0,.1f);p.processBlock(b,midi);
        if(i>12000)delta=std::max(delta,std::abs(double(b.getSample(0,0)-previous)));
        if(i>37000&&i<47900)tailError=std::max(tailError,std::abs(double(b.getSample(0,0)-.1f)));
        previous=b.getSample(0,0);
    }
    check(delta<.001,"Headphone/Bypass transition clicked");check(tailError==0,"Settled bypass is not exact dry");
    std::cout<<"PASS: headphone/bypass transitions max DC adjacent delta="<<delta<<"; settled bypass exact dry.\n";
}

void unityABChecks()
{
    for(bool tp:{false,true})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"rangeDb",1);p.enterLimiterMode();
        p.setBoundaryForDomainDb(false,false,0,0);set(p,"limiterCalibrationDb",0);
        set(p,"truePeakLimiting",tp?1.f:0.f);set(p,"limiterOutputDb",6);
        p.copyAToB();p.selectABSlot(1);set(p,"limiterOutputDb",18);p.selectABSlot(0);
        p.setUnityMonitorEnabled(true);p.prepareToPlay(48000,1);juce::MidiBuffer midi;
        double error=0,delta=0;float last=0;
        for(int i=0;i<48000;++i)
        {
            if(i==12000||i==18000||i==30000)p.selectABSlot(1);
            if(i==12048||i==24000||i==30048)p.selectABSlot(0);
            juce::AudioBuffer<float>b(2,1);b.setSample(0,0,.02f);b.setSample(1,0,.02f);p.processBlock(b,midi);
            const auto y=b.getSample(0,0);
            if(i>10000){error=std::max(error,std::abs(double(y-.02f)));delta=std::max(delta,std::abs(double(y-last)));}
            last=y;
        }
        check(p.isUnityMonitorEnabled(),"A/B changed monitor selection");
        check(error<.0001 && delta<.00005,"Equal monitored A/B levels made a bump");
        std::cout<<"PASS: A/B equal monitored levels and rapid retarget TP="<<tp<<" max level error="<<error<<" max adjacent delta="<<delta<<".\n";
    }
}

int main(int argc,char** argv)
{
    std::cout<<std::unitbuf;juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        check(argc==2 || argc==3,"Provide test output directory");juce::File root(argv[1]);root.createDirectory();
        QQSCUnityCheck::ui(root);
        QQSCUnityCheck::strictMatchChecks();
        if(argc==2){unityAudioChecks();QQSCUnityCheck::matchChecks(root);transitionChecks();unityABChecks();}
        std::cout<<"PASS: Unity monitor checks complete.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
