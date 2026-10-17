    static void ecoUiReopenChecks(const juce::File& root)
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
        struct OffscreenHost {
            // Exercise real native visibility without exposing a window on the
            // user's desktop or touching any running host/application.
            HWND handle=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"STATIC",
                L"QQSC reopen regression",WS_POPUP,-32000,-32000,1200,800,
                nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            ~OffscreenHost(){if(handle)DestroyWindow(handle);}
        };
        int cases=0;float worstAudio=0.f;
        for(int family:{0,1,2})for(int mode:{0,1,2})for(bool freeze:{false,true})for(bool eco:{false,true})
        {
            if(family!=0 && mode!=0)continue;
            std::cout<<"ECO_REOPEN family="<<family<<" domain="<<mode<<" freeze="<<freeze<<" eco="<<eco<<"\n";
            const auto name="eco-reopen-"+juce::String(cases);
            Head head;
            QQSuperCompressionAudioProcessor p(settings(name+"-processor")),reference(settings(name+"-reference"));
            QQSuperCompressionAudioProcessorEditor e(p,settings(name+"-editor"));
            e.stopTimer();e.display.stopTimer();
            auto& d=e.display;
            for(auto* q:{&p,&reference}) {
                q->setProcessorModeFromEditor(family);
                set(*q,q->soundParameterID("processingMode"),float(mode));
                set(*q,q->soundParameterID("algorithmMode"),0);
                set(*q,q->soundParameterID("lookaheadMs"),25.3f);
                set(*q,q->soundParameterID("oversampling"),0);
                set(*q,"truePeakLimiting",0);set(*q,"limiterLink",0);
                for(const auto* id:{"ratio","ratioL","ratioR","ratioM","ratioS"})set(*q,q->soundParameterID(id),20);
                for(const auto* id:{"thresholdDb","thresholdLDb","thresholdRDb","thresholdMDb","thresholdSDb"})set(*q,q->soundParameterID(id),-20);
                q->setDisplayFreezeOnStop(freeze);q->setPlayHead(&head);
                q->setRateAndBufferSizeDetails(48000,512);q->prepareToPlay(48000,512);
                q->setEcoMode(eco);
            }
            reference.setEditorOpen(true);
            OffscreenHost host;require(host.handle!=nullptr,"Offscreen host creation failed");
            juce::Component wrapper;wrapper.setSize(1200,800);wrapper.addAndMakeVisible(e);
            wrapper.addToDesktop(0,host.handle);wrapper.setVisible(true);
            const auto visible=[&](bool show) {
                ShowWindow(host.handle,show?SW_SHOWNOACTIVATE:SW_HIDE);
                e.timerCallback();d.timerCallback();
                require(p.isEditorOpen()==show,"Native visibility transition not published");
            };
            visible(true);
            juce::MidiBuffer midi;
            const auto feed=[&](int blocks,bool playing) {
                head.playing=playing;
                for(int b=0;b<blocks;++b) {
                    juce::AudioBuffer<float> a(2,512),other(2,512);
                    for(int c=0;c<2;++c)for(int i=0;i<512;++i) {
                        const double t=double(head.sample+i)/48000.;
                        a.setSample(c,i,playing?float((.28+.09*std::sin(t*6.))*std::sin(t*(c==0?6283.185:4300.))):0.f);
                    }
                    other.makeCopyOf(a);p.processBlock(a,midi);reference.processBlock(other,midi);
                    for(int c=0;c<2;++c)for(int i=0;i<512;++i)
                        worstAudio=std::max(worstAudio,std::abs(a.getSample(c,i)-other.getSample(c,i)));
                    head.sample+=512;p.refreshMatchResults();reference.refreshMatchResults();d.timerCallback();
                }
            };
            feed(1,false);feed(200,true);
            require(p.hasMatchData(),"Initial complete UI-open pass has no Match data");
            require(d.histories[0].points.size()>50,"Fixture failed to populate initial Display");
            p.setDisplayZoom(-48.f,-6.f,3.f,0.f);d.timerCallback();
            const auto zoom=p.getDisplayZoom();
            const auto old=p.getDisplayKeyHistoryPosition();
            const auto oldFirst=d.histories[0].points.front().captureCounter;
            DynamicDisplay::ReplayResult stale;
            stale.request.requestGeneration=d.replayRequestGeneration->load();
            stale.request.captureGeneration=old.generation;
            visible(false);feed(100,true);
            require(eco ? p.getDisplayKeyHistoryPosition().counter==old.counter
                        : p.getDisplayKeyHistoryPosition().counter>old.counter,
                    "Hidden capture does not follow ECO/FULL");
            if(eco)require(!p.hasMatchData(),"Hidden playback left Match valid");
            visible(true); // No fresh processBlock yet: an old published meter must not reappear.
            const auto resumed=p.getDisplayKeyHistoryPosition();
            if(eco) {
                require(d.histories[0].points.empty() && d.histories[1].points.empty(),
                        "ECO reopen retained stale Display history (Freeze must not preserve a hidden gap)");
                require(resumed.generation!=old.generation && resumed.counter==0,
                        "ECO reopen retained pre-gap raw history for replay");
                for(auto* cache:{&d.renderCaches[0],&d.renderCaches[1],&d.stereoLimiterCache})
                    require(cache->projected.size==0 && cache->inputPath.isEmpty() && cache->outputPath.isEmpty(),
                            "ECO reopen retained a stale rendered curve");
                QQSuperCompressionAudioProcessor::DisplayKeyHistorySnapshot snapshot;
                require(!p.copyDisplayKeyHistory(old.generation,0,old.counter,snapshot),
                        "Old raw generation remains accessible after ECO reopen");
                d.timerCallback();d.timerCallback();
                require(d.histories[0].points.empty(),"Stale meter frame repopulated cleared Display");
            } else {
                require(resumed.generation==old.generation && !d.histories[0].points.empty()
                        && d.histories[0].points.front().captureCounter==oldFirst,
                        "FULL retained history was unexpectedly cleared");
            }
            const auto revision=d.historyRevision;
            d.applyHpfReplay(std::move(stale));
            if(eco)require(d.historyRevision==revision,"Pre-hide replay result changed resumed history");
            require(p.getDisplayZoom()==zoom,"ECO reopen reset user zoom");
            feed(1,true);
            if(eco)require(d.histories[0].points.size()==1
                && d.histories[0].points.front().captureGeneration==resumed.generation
                && d.histories[0].points.front().captureCounter==512,
                "Display did not restart from newly captured audio");
            feed(200,true);
            require(p.hasMatchData()==!eco,"Incomplete resumed pass incorrectly became Match-ready");
            if(eco)require(!p.applyMatchForCurrentMode(),"Incomplete resumed pass allowed Match");
            // Hide again, then stop while hidden. Reopening must not resurrect
            // old waveforms or validate an incomplete pass when no audio runs.
            visible(false);feed(10,true);feed(1,false);visible(true);
            if(eco)require(d.histories[0].points.empty(),"Stopped ECO reopen retained stale history");
            require(p.hasMatchData()==!eco,"Stopped reopen changed partial/full Match validity");
            feed(200,true);
            require(p.hasMatchData(),"Next complete UI-open pass did not restore Match");
            wrapper.setVisible(false);wrapper.removeChildComponent(&e);wrapper.removeFromDesktop();
            p.setPlayHead(nullptr);reference.setPlayHead(nullptr);
            p.releaseResources();reference.releaseResources();++cases;
        }
        require(worstAudio<1.e-7f,"ECO hide/reopen changed audible processing");
        std::cout<<"PASS: "<<cases<<" native hide/reopen cycles; Comp ST/LR/MS, Clipper, Limiter, ECO/FULL, Freeze OFF/ON; fresh histories, stale replay rejection, zoom retention, Match recovery; worst audio delta="<<worstAudio<<"\n";
#else
        juce::ignoreUnused(root);
        std::cout<<"SKIP: native Windows reopen fixture; platform-independent Display/Match contracts run separately.\n";
#endif
    }
