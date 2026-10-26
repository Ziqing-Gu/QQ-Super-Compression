    static void ceilingLiveDisplayChecks(const juce::File& root)
    {
#if JUCE_WINDOWS
        struct Head final:juce::AudioPlayHead { bool playing=true;int64_t sample=0;
            juce::Optional<PositionInfo> getPosition() const override {PositionInfo p;p.setIsPlaying(playing);p.setTimeInSamples(sample);return p;} };
        struct Host { HWND handle=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",L"QQSC display regression",WS_POPUP,-32000,-32000,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            ~Host(){if(handle)DestroyWindow(handle);} };
        int cases=0;
        for(bool eco:{false,true})for(float look:{0.f,25.3f,100.f})
        {
            const auto dir=root.getChildFile(juce::String(cases));dir.createDirectory();
            QQSuperCompressionAudioProcessor p(settings(dir));Head head;p.setPlayHead(&head);p.enterLimiterMode();p.setEcoMode(eco);p.setDisplayFreezeOnStop(true);
            auto put=[&](const char* id,float v){set(p,p.soundParameterID(id),v);};
            put("limiterLink",0);put("lookaheadMs",look);put("distort",0);put("oversampling",0);put("ratioL",1);
            put("mixL",100);put("makeupGainLDb",0);put("outputGainDb",12);put("keySource",0);put("keyHpfHz",0);put("limiterStereoLink",0);
            set(p,"ceilingEnabled",1);set(p,"ceilingDb",0);set(p,"truePeakLimiting",0);set(p,"tpRecoveryMode",0);
            p.setRateAndBufferSizeDetails(48000,800);p.prepareToPlay(48000,800);
            QQSuperCompressionAudioProcessorEditor e(p,settings(dir));e.stopTimer();e.display.stopTimer();e.setSize(1200,800);
            Host host;check(host.handle!=nullptr,"Display host");juce::Component wrapper;wrapper.setSize(1200,800);wrapper.addAndMakeVisible(e);wrapper.addToDesktop(0,host.handle);wrapper.setVisible(true);ShowWindow(host.handle,SW_SHOWNOACTIVATE);
            e.timerCallback();auto& d=e.display;d.timerCallback();
            juce::AudioBuffer<float> audio(2,800);juce::MidiBuffer midi;
            for(int b=0;b<90;++b)
            {
                const float amp=b%18<5?.4f:.01f;
                for(int i=0;i<800;++i){const float x=i%4<2?amp:-amp;audio.setSample(0,i,x);audio.setSample(1,i,x);}
                p.processBlock(audio,midi);head.sample+=800;d.timerCallback();
            }
            head.playing=false;audio.clear();p.processBlock(audio,midi);d.timerCallback();
            check(d.histories[0].points.size()>40,"Actual captured Display history missing");
            const auto curve=[&]{const auto& h=d.renderCaches[0].projected;return std::vector<float>(h.output.begin(),h.output.begin()+h.size);};
            const auto difference=[&](const auto& a,const auto& b){check(a.size()==b.size(),"Frozen Ceiling edit changed history size");float result=0;for(size_t i=0;i<a.size();++i)result=std::max(result,std::abs(a[i]-b[i]));return result;};
            const auto sp=curve();const auto revision=p.getDisplayProjectionRevision();
            e.truePeakButton.onClick();d.timerCallback();const auto tp=curve();
            check(p.getDisplayProjectionRevision()>revision,"TP does not invalidate history projection");
            const auto tpDiff=difference(sp,tp);
            e.tpRecoveryButton.onClick();d.timerCallback();const auto smooth=curve();
            e.tpRecoveryButton.onClick();d.timerCallback();const auto automatic=curve();
            const auto smoothDiff=difference(tp,smooth),autoDiff=difference(smooth,automatic);
            std::cout<<"Ceiling live display eco="<<eco<<" look="<<look<<" tpDifference="<<tpDiff<<" smoothDifference="<<smoothDiff<<" autoDifference="<<autoDiff<<"\n";
            check(tpDiff>.01f,"Real captured Display ignores TP toggle and its reconstruction reserve");
            check(smoothDiff>.1f && autoDiff>.05f,"Real captured Display ignores recovery selector");
            image(e,dir.getChildFile("ceiling-display.png"));
            wrapper.removeFromDesktop();++cases;
        }
        std::cout<<"PASS: "<<cases<<" real captured Ceiling Display button/timer/Freeze cases.\n";
#endif
    }
