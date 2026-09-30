    static void strictOneToOneLinkChecks(const juce::File& root)
    {
        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
        auto drag=[&](Knob& slider,double value)
        {
            const auto ev=mouse(slider,slider.getLocalBounds().toFloat().getCentre());
            slider.mouseDown(ev);slider.setValue(value,juce::sendNotificationSync);slider.mouseUp(ev);
        };
        int cases=0;
        for(int dual:{0,1})for(int mode:{0,1,2})for(int algorithm:{0,1})for(int link:{0,1})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",float(dual));set(p,"limiterProcessingMode",float(mode));
            set(p,"limiterAlgorithmMode",float(algorithm));set(p,"limiterDownAlgorithmMode",float(algorithm));
            set(p,"limiterLink",float(link));set(p,"limiterOutputDb",6.0f);set(p,"limiterDomainLink",0);
            for(size_t d=0;d<5;++d)
            {
                set(p,qqsc::params::limiterRatioIds[d],400);set(p,qqsc::params::limiterDownRatioIds[d],400);
                set(p,qqsc::params::limiterUpRatioIds[d],.5f);set(p,qqsc::params::limiterModeIds[21+d],1);
                p.setBoundaryForDomainDb(false,true,int(d),1);p.setBoundaryForDomainDb(false,false,int(d),-20);
                p.setBoundaryForDomainDb(true,false,int(d),-60);p.setBoundaryForDomainDb(true,true,int(d),-20);
                set(p,qqsc::params::limiterModeIds[1+d],0);set(p,qqsc::params::limiterModeIds[6+d],100);
            }
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            const size_t d=mode==0?0:mode==2?1:3;
            auto* threshold=dual ? e.upperBoundarySliders[d].get() : e.lowerBoundaryControls()[d];
            auto* makeup=std::array<Knob*,5>{&e.makeupSTSlider,&e.makeupLSlider,&e.makeupRSlider,&e.makeupMSlider,&e.makeupSSlider}[d];

            auto out0=get(p,"limiterOutputDb");auto t0=p.getBoundaryForDomainDb(bool(dual),bool(dual),int(d));
            e.beginBoundaryGesture(int(d),dual!=0);threshold->setValue(t0-3.25,juce::sendNotificationSync);e.endLinkedGesture();
            check(std::abs(get(p,"limiterOutputDb")-(out0+(link?3.25f:0.f)))<.015,"Threshold/Output is not strict 1:1");

            out0=get(p,"limiterOutputDb");const auto m0=makeup->getValue();drag(*makeup,m0+2.5);
            check(std::abs(get(p,"limiterOutputDb")-(out0-(link?2.5f:0.f)))<.015,"Makeup/Output is not strict 1:1");

            // Ratio, Mix and algorithm switches must never move Output.
            out0=get(p,"limiterOutputDb");
            if(dual) drag(*e.downRatioSliders[d],700); else drag(*e.mainRatioControls()[d],700);
            check(std::abs(get(p,"limiterOutputDb")-out0)<.001,"Ratio moved Output");
            drag(*std::array<Knob*,5>{&e.mixSlider,&e.mixLSlider,&e.mixRSlider,&e.mixMSlider,&e.mixSSlider}[d],37);
            check(std::abs(get(p,"limiterOutputDb")-out0)<.001,"Mix moved Output");
            if(dual) e.downAlgorithmButton.onClick(); else e.algorithmButton.onClick();
            check(std::abs(get(p,"limiterOutputDb")-out0)<.001,"Algorithm moved Output");

            // Editing Output drives only the active DOWN Threshold in the opposite direction.
            const auto beforeThreshold=p.getBoundaryForDomainDb(bool(dual),bool(dual),int(d));
            out0=get(p,"limiterOutputDb");drag(e.outputGainSlider,out0+1.75);
            if(link)
            {
                check(std::abs(get(p,"limiterOutputDb")-(out0+1.75f))<.015,"Output edit delta wrong");
                check(std::abs(p.getBoundaryForDomainDb(bool(dual),bool(dual),int(d))-(beforeThreshold-1.75f))<.02,"Output/Threshold inverse is not 1:1");
            }
            else check(std::abs(p.getBoundaryForDomainDb(bool(dual),bool(dual),int(d))-beforeThreshold)<.001,"LINK off moved Threshold");
            ++cases;
        }
        std::cout<<"PASS: "<<cases<<" strict 1:1 Limiter LINK cases; Threshold and Makeup only, Ratio/Mix/algorithms excluded.\n";

        // GAIN +/- must include the same real OutputCeiling attenuation that the
        // Dynamic Display captures. Use the deterministic quarter-rate sequence
        // +1,+1,-1,-1: native samples are exactly 0 dBFS, but the 8x reconstructed
        // waveform overshoots Ceiling. In 1.2.28 TP OFF is itself an 8x Hard
        // Clipper, so BOTH Hard and TP paths must expose positive visual GR.
        auto runVisualMeter=[&](bool tp)
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",0);set(p,"limiterProcessingMode",0);
            set(p,"limiterRatio",400);set(p,"limiterMix",100);set(p,"limiterMakeupGainDb",0);
            set(p,"limiterOutputDb",0);set(p,"ceilingDb",0);set(p,"truePeakLimiting",tp?1.f:0.f);
            p.setBoundaryForDomainDb(false,false,0,0.0f); // no compressor GR at sample peak <= 0 dBFS
            constexpr double rate=48000.0;constexpr int block=256;
            p.setRateAndBufferSizeDetails(rate,block);p.prepareToPlay(rate,block);
            juce::MidiBuffer midi;
            for(int offset=0;offset<int(rate*.45);offset+=block)
            {
                const int size=std::min(block,int(rate*.45)-offset);juce::AudioBuffer<float>b(2,size);
                for(int i=0;i<size;++i){const float x=(((offset+i)&3)<2)?1.f:-1.f;b.setSample(0,i,x);b.setSample(1,i,x);}
                p.processBlock(b,midi);
            }
            const auto meter=p.getMeterState().gainReductionDb0.load();
            const auto hold=p.getMeterState().gainReductionHoldDb0.load();
            const auto display=p.getMeterState().displayCeilingGainReductionDb.load();
            p.releaseResources();
            return std::array<float,3>{meter,hold,display};
        };
        const auto hardVisual=runVisualMeter(false),tpVisual=runVisualMeter(true);
        check(hardVisual[0]>.10f && hardVisual[1]>.10f && hardVisual[2]>.10f,
              "8x Hard Clip attenuation missing from meter/hold/Display");
        check(tpVisual[0]>.10f && tpVisual[1]>.10f && tpVisual[2]>.10f,
              "TP attenuation missing from meter/hold/Display");
        check(std::abs(hardVisual[0]-hardVisual[2])<.08f,
              "GAIN +/- and Display Hard Clip attenuation disagree");
        check(std::abs(tpVisual[0]-tpVisual[2])<.08f,
              "GAIN +/- and Display TP attenuation disagree");
        std::cout<<"PASS: 8x Hard Clip and TP OutputCeiling attenuation are visible in GAIN +/-, Hold, and Dynamic Display telemetry.\n";
    }
