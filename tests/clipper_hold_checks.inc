    static void clipperHoldChecks(const juce::File& root)
    {
        struct PlayHead final:juce::AudioPlayHead
        {
            bool playing=true;int64_t samples=0;
            juce::Optional<PositionInfo> getPosition() const override
            {PositionInfo p;p.setIsPlaying(playing);p.setTimeInSamples(samples);return p;}
        } head;
        QQSuperCompressionAudioProcessor p;p.setPlayHead(&head);p.enterClipperMode();p.setEcoMode(false);
        value(p,"algorithmMode",0);value(p,"ratioL",10);value(p,"thresholdLDb",-18);
        value(p,"lookaheadMs",0);value(p,"oversampling",0);value(p,"mixL",100);
        p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
        juce::PropertiesFile::Options options;options.applicationName="QQSCClipperHoldCheck";
        QQSuperCompressionAudioProcessorEditor e(p,std::make_unique<juce::PropertiesFile>(root.getChildFile("ui.settings"),options));
        e.stopTimer();e.display.stopTimer();
        auto& m=p.getMeterState();
        juce::MidiBuffer midi;
        const auto run=[&](int blocks,float left,float right)
        {
            for(int b=0;b<blocks;++b)
            {juce::AudioBuffer<float> a(2,256);for(int i=0;i<256;++i){a.setSample(0,i,left);a.setSample(1,i,right);}
             p.processBlock(a,midi);head.samples+=256;}
        };
        run(20,.5f,.25f);
        const float left=m.clipperGainReductionHoldDb0.load(),right=m.clipperGainReductionHoldDb1.load();
        require(left>right+4 && right>3,"Clipper independent maximum GR capture");
        run(600,.01f,.01f);
        require(m.clipperGainReductionHoldDb0.load()==left && m.clipperGainReductionHoldDb1.load()==right,"Clipper hold expired after two seconds");
        value(p,"detectorWindowMs",0);value(p,"lookaheadMs",26);run(20,.01f,.01f);
        require(m.clipperGainReductionHoldDb0.load()==left,"Lookahead reconfiguration cleared Clipper hold");
        value(p,"detectorWindowMs",5);run(20,.01f,.01f);value(p,"detectorWindowMs",0);run(20,.01f,.01f);
        require(m.clipperGainReductionHoldDb0.load()==left,"Window roundtrip cleared Clipper hold");
        head.playing=false;run(30,.9f,.9f);
        require(m.clipperGainReductionHoldDb0.load()==left && m.clipperGainReductionHoldDb1.load()==right,"Stopped FULL altered hold");
        run(30,0,0);head.playing=true;run(30,0,0);
        require(m.clipperGainReductionHoldDb0.load()==0 && m.clipperGainReductionHoldDb1.load()==0,"New playback did not reset hold");
        run(40,.5f,.25f);
        const auto source=juce::Desktop::getInstance().getMainMouseSource();
        const auto click=[&](juce::Point<float> at)
        {return juce::MouseEvent(source,at,juce::ModifierKeys::leftButtonModifier,1,0,0,0,0,&e.meters,&e.meters,juce::Time::getCurrentTime(),at,juce::Time::getCurrentTime(),2,false);};
        e.meters.mouseDoubleClick(click({10.f,100.f}));
        require(m.clipperGainReductionHoldDb0.load()>0,"Input meter click cleared GR");
        m.truePeakHoldDb.store(2.f);
        e.meters.mouseDoubleClick(click({float(e.meters.getWidth())-12.f,100.f}));
        require(m.clipperGainReductionHoldDb0.load()==0 && m.clipperGainReductionHoldDb1.load()==0,"GR double-click must clear both channels immediately");
        require(m.truePeakHoldDb.load()==2.f,"GR double-click cleared TP");
        head.playing=false;run(30,0,0);require(m.clipperGainReductionHoldDb0.load()==0,"Stopped audio restored pre-reset GR");
        p.setEcoMode(true);p.setEditorOpen(false);head.playing=true;run(50,.5f,.25f);
        require(m.clipperGainReductionHoldDb0.load()>5,"Clipper GR not captured while UI closed in ECO");
        const float closed=m.clipperGainReductionHoldDb0.load();p.setEditorOpen(true);run(30,.01f,.01f);
        require(m.clipperGainReductionHoldDb0.load()==closed,"Reopening editor cleared Clipper hold");
        require(e.meters.getTooltip().contains(juce::String::fromUTF8(u8"双击")),"Chinese GR hover help missing");
        e.updateCompressionUi();
        require(float(e.ratioLSlider.getProperties()["qqscLightOrigin"])==0.f,"Clipper Ratio light origin must be left endpoint");
        require(e.ratioLSlider.valueToProportionOfLength(10)==0,"Clipper minimum position");
        juce::PNGImageFormat png;
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {e.theme=theme;e.applyTheme();e.resized();auto out=root.getChildFile("clipper-ratio-minimum-"+juce::String(int(theme))+".png").createOutputStream();png.writeImageToStream(e.createComponentSnapshot(e.getLocalBounds()),*out);}
        p.setProcessorModeFromEditor(0);e.updateCompressionUi();
        require(float(e.ratioSlider.getProperties()["qqscLightOrigin"])==.5f,"Comp light origin not restored");
        p.setProcessorModeFromEditor(2);e.updateCompressionUi();
        require(float(e.ratioLSlider.getProperties()["qqscLightOrigin"])==.5f,"Limiter light origin not restored");
        p.setProcessorModeFromEditor(1);e.updateCompressionUi();
        require(float(e.ratioLSlider.getProperties()["qqscLightOrigin"])==0.f,"Clipper light origin lost after mode cycle");
        p.setPlayHead(nullptr);
        std::cout<<"PASS: Clipper per-channel maximum GR, no expiry, manual reset, playback reset, stop hold, closed ECO capture, reconfiguration; Chinese help and Ratio lamp mode mapping"<<std::endl;
    }
