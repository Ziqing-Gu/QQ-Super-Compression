    static void ecoUiVisibilityChecks(const juce::File& root)
    {
#if JUCE_WINDOWS
        struct Head final : juce::AudioPlayHead {
            int64_t sample=0; bool playing=false;
            juce::Optional<PositionInfo> getPosition() const override {
                PositionInfo p;p.setIsPlaying(playing);p.setTimeInSamples(sample);return p;
            }
        };
        const auto require=[](bool ok,const char* message) {
            if(!ok)throw std::runtime_error(message);
        };
        const auto set=[](auto& p,const char* id,float value) {
            auto* parameter=p.getAPVTS().getParameter(id);
            if(!parameter)throw std::runtime_error(id);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        const auto settings=[&](const juce::String& name) {
            juce::PropertiesFile::Options options;options.storageFormat=juce::PropertiesFile::storeAsXML;
            return std::make_unique<juce::PropertiesFile>(root.getChildFile(name+".settings"),options);
        };
        struct HiddenHost {
            HWND handle=CreateWindowExW(0,L"STATIC",L"QQSC hidden-parent regression",WS_OVERLAPPED,
                0,0,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            ~HiddenHost(){if(handle)DestroyWindow(handle);}
        };
        int cases=0;float worstAudio=0.f;
        for(int family:{0,1,2})for(bool freeze:{false,true})for(bool eco:{false,true})
        {
            const auto name="eco-visibility-"+juce::String(cases);
            Head head;
            QQSuperCompressionAudioProcessor p(settings(name+"-processor")),reference(settings(name+"-reference"));
            QQSuperCompressionAudioProcessorEditor e(p,settings(name+"-editor"));
            e.stopTimer();e.display.stopTimer();
            for(auto* q:{&p,&reference}) {
                q->setProcessorModeFromEditor(family);
                set(*q,q->soundParameterID("processingMode"),0);
                set(*q,q->soundParameterID("algorithmMode"),0);
                set(*q,q->soundParameterID("lookaheadMs"),25.3f);
                set(*q,q->soundParameterID("oversampling"),0);
                set(*q,"truePeakLimiting",0);set(*q,"limiterLink",0);
                for(const auto* id:{"ratio","ratioL","ratioR","ratioM","ratioS"})set(*q,q->soundParameterID(id),20);
                for(const auto* id:{"thresholdDb","thresholdLDb","thresholdRDb","thresholdMDb","thresholdSDb"})set(*q,q->soundParameterID(id),-20);
                q->setDisplayFreezeOnStop(freeze);q->setPlayHead(&head);
                q->setRateAndBufferSizeDetails(48000,512);q->prepareToPlay(48000,512);
            }
            p.setEcoMode(eco);reference.setEcoMode(eco);reference.setEditorOpen(true);
            HiddenHost host;require(host.handle!=nullptr,"Hidden host creation failed");
            juce::Component wrapper;wrapper.setSize(1200,800);wrapper.addAndMakeVisible(e);
            wrapper.addToDesktop(0,host.handle);wrapper.setVisible(true);
            const auto native=static_cast<HWND>(wrapper.getPeer()->getNativeHandle());
            // Never show this test window. Reproduce VST3 attachment to an
            // invisible native ancestor, with JUCE's own visible flags true.
            require(!IsWindowVisible(native),"Test child must be hidden by native host");
            require(e.isShowing(),"Fixture no longer reproduces Component::isShowing blind spot");
            e.timerCallback();
            std::cout<<"ECO_VISIBILITY family="<<family<<" freeze="<<freeze<<" eco="<<eco
                     <<" nativeVisible=0 componentShowing=1 editorOpen="<<p.isEditorOpen()<<"\n";
            require(!p.isEditorOpen(),"Native-hidden editor incorrectly published as open");
            require(p.shouldRunUiAnalysis()==!eco,"Native-hidden ECO analysis gate stayed enabled");
            const auto replayGeneration=e.display.replayRequestGeneration->load();
            e.display.timerCallback();
            if(eco) {
                require(e.display.replayRequestGeneration->load()>replayGeneration,"Native-hidden ECO did not cancel Display replay");
                require(!e.display.requestHpfHistoryRefresh(),"Native-hidden ECO scheduled Display replay");
            }
            juce::MidiBuffer midi;
            const auto feed=[&](int blocks,bool playing,bool compare) {
                head.playing=playing;
                for(int b=0;b<blocks;++b) {
                    juce::AudioBuffer<float> a(2,512),other(2,512);
                    for(int c=0;c<2;++c)for(int i=0;i<512;++i) {
                        const double t=double(head.sample+i)/48000.;
                        a.setSample(c,i,playing?float((.28+.09*std::sin(t*6.))*std::sin(t*(c==0?6283.185:4300.))):0.f);
                    }
                    other.makeCopyOf(a);p.processBlock(a,midi);reference.processBlock(other,midi);
                    if(playing && compare)for(int c=0;c<2;++c)for(int i=0;i<512;++i)
                        worstAudio=std::max(worstAudio,std::abs(a.getSample(c,i)-other.getSample(c,i)));
                    head.sample+=512;p.refreshMatchResults();reference.refreshMatchResults();
                }
            };
            feed(1,false,false);const auto start=p.getDisplayKeyHistoryPosition().counter;
            feed(200,true,true);feed(1,false,false);e.timerCallback();
            const auto captured=p.getDisplayKeyHistoryPosition().counter-start;
            require(eco ? captured==0 : captured>=102400,"Hidden Display capture disagrees with ECO/FULL");
            require(p.hasMatchData()==!eco,"Hidden playback Match validity disagrees with ECO/FULL");
            require(p.applyMatchForCurrentMode()==!eco,"Hidden incomplete playback allowed Match");
            if(eco) {
                require(p.getMeterState().outputLoudnessSeconds.load()==0.f,"Hidden ECO still accumulated LUFS");
                // Processor-only reopening: no test window is ever shown.
                // A partial pass must stay invalid even after enough new data.
                feed(1,true,false);p.setEditorOpen(true);feed(200,true,false);
                require(!p.hasMatchData() && !p.applyMatchForCurrentMode(),"Reopening promoted a partial pass to Match-ready");
                feed(1,false,false);
                require(!p.hasMatchData(),"Stopping promoted an incomplete pass to Match-ready");
                feed(200,true,false);
                require(p.hasMatchData(),"Complete new UI-open pass did not restore Match");
                // The actual editor visibility poll must override stale flags.
                e.timerCallback();require(!p.isEditorOpen(),"Hidden visibility poll did not correct stale open flag");
            }
            wrapper.setVisible(false);e.timerCallback();
            require(!p.isEditorOpen(),"JUCE-hidden editor incorrectly published as open");
            wrapper.removeChildComponent(&e);wrapper.removeFromDesktop();
            p.setPlayHead(nullptr);reference.setPlayHead(nullptr);
            p.releaseResources();reference.releaseResources();++cases;
        }
        require(worstAudio<1.e-7f,"Hidden UI changed audible processing versus UI-open reference");
        std::cout<<"PASS: "<<cases<<" native-parent-hidden ECO/FULL fixtures across Comp/Clipper/Limiter and Freeze states; no hidden ECO capture/Match/LUFS, replay cancellation, partial-pass rejection, next-pass recovery; worst audio delta="<<worstAudio<<"\n";
#else
        juce::ignoreUnused(root);
        std::cout<<"SKIP: Windows native-parent visibility fixture (platform-specific); cumulative Match remains separately tested.\n";
#endif
    }
