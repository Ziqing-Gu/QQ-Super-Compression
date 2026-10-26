    static void ceilingSwitchChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;
        check(p.getAPVTS().getParameter("ceilingEnabled") != nullptr,
              "Limiter needs an independent Ceiling enable parameter");
        check(get(p,"ceilingEnabled") == 1.0f,"Ceiling defaults to ON");
        p.enterLimiterMode();set(p,"ceilingEnabled",0);set(p,"ceilingDb",-6);
        p.copyAToB();p.selectABSlot(1);set(p,"ceilingEnabled",1);
        p.selectABSlot(0);check(!p.isCeilingEnabled(),"A/B lost disabled Ceiling");
        juce::MemoryBlock state;p.getStateInformation(state);
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
        check(!restored.isCeilingEnabled(),"Project lost disabled Ceiling");
        restored.selectABSlot(1);check(restored.isCeilingEnabled(),"Inactive B Ceiling not restored");
        restored.selectABSlot(0);
        p.leaveLimiterMode();p.enterClipperMode();p.enterLimiterMode();
        check(!p.isCeilingEnabled() && get(p,"ceilingDb")==-6,"Mode switching lost Ceiling settings");
        auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        check(xml!=nullptr,"Saved project XML missing");
        auto legacy=juce::ValueTree::fromXml(*xml);
        legacy.removeChild(legacy.getChildWithProperty("id","ceilingEnabled"),nullptr);
        legacy.removeProperty("qqscAB_A_ceilingEnabled",nullptr);legacy.removeProperty("qqscAB_B_ceilingEnabled",nullptr);
        legacy.setProperty("qqscStateSchemaVersion",31,nullptr);juce::MemoryBlock old;
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),old);
        restored.setStateInformation(old.getData(),int(old.getSize()));
        check(restored.isCeilingEnabled(),"Old project loaded into OFF instance must enable Ceiling");
        restored.selectABSlot(1);check(restored.isCeilingEnabled(),"Old inactive B must enable Ceiling");

        int cases=0;juce::String csv="os,tp,lookahead,enabled,latency,output\n";
        for(int os:{0,1,2,3})for(bool tp:{false,true})for(float look:{0.f,25.3f,100.f})
        {
            QQSuperCompressionAudioProcessor audio;audio.enterLimiterMode();audio.setEcoMode(false);
            auto put=[&](const char* id,float v){set(audio,audio.soundParameterID(id),v);};
            put("lookaheadMs",look);put("oversampling",float(os));put("inputGainDb",0);put("outputGainDb",6);
            put("limiterLink",0);put("compressionMode",0);put("ratioL",1);put("makeupGainLDb",0);put("mixL",100);
            set(audio,"ceilingDb",-6);set(audio,"truePeakLimiting",tp?1.f:0.f);
            audio.setRateAndBufferSizeDetails(48000,256);audio.prepareToPlay(48000,256);
            // Reference the unchanged core at the SAME OS. Its 16x filters do
            // not have mathematically exact DC unity gain; Ceiling OFF must
            // match this real path, not an ideal gain multiplier.
            QQSuperCompressionAudioProcessor core;core.setEcoMode(false);
            set(core,"lookaheadMs",look);set(core,"oversampling",float(os));set(core,"processingMode",0);
            set(core,"compressionMode",0);set(core,"ratio",1);set(core,"inputGainDb",0);
            set(core,"outputGainDb",6);set(core,"makeupGainDb",0);set(core,"mix",100);
            core.setRateAndBufferSizeDetails(48000,256);core.prepareToPlay(48000,256);
            juce::MidiBuffer midi;juce::AudioBuffer<float> buffer(2,256),reference(2,256);
            int onLatency=0;
            for(bool enabled:{true,false,true,false})
            {
                set(audio,"ceilingEnabled",enabled?1.f:0.f);
                for(int block=0;block<130;++block)
                {for(int i=0;i<256;++i){buffer.setSample(0,i,.8f);buffer.setSample(1,i,.4f);}reference.makeCopyOf(buffer);audio.processBlock(buffer,midi);core.processBlock(reference,midi);}
                const int latency=audio.getLatencySamples();const float value=buffer.getSample(0,255);
                check(latency==QQSCReviewCheck::ceilingSwitchLatency(audio,look,os),"Host latency disagrees with Ceiling state");
                if(enabled)
                {onLatency=latency;check(value>.1f && value<=juce::Decibels::decibelsToGain(-6.f)+.005f,"Ceiling ON failed to protect output");}
                else
                {
                    check(latency<onLatency,"Ceiling OFF retained protection latency");
                    check(latency==core.getLatencySamples(),"Disabled Ceiling has delay beyond core Lookahead/OS");
                    check(value>1.f,"Ceiling OFF retained a full-scale cap");
                    check(std::abs(value-reference.getSample(0,255))<.00002f,"Ceiling OFF still limits or applies Ceiling trim");
                    if(os==0)check(std::abs(value-.8f*juce::Decibels::decibelsToGain(6.f))<.00002f,"Native OFF path gain is incorrect");
                    check(std::abs(audio.getActiveOutputGainDb()-6.f)<.001f,"Disabled Ceiling changes active output gain");
                }
                check(get(audio,"ceilingDb")==-6 && get(audio,"truePeakLimiting")==float(tp),"Toggle discarded stored settings");
                csv+=juce::String(os)+","+juce::String(int(tp))+","+juce::String(look)+","+juce::String(int(enabled))+","+juce::String(latency)+","+juce::String(value,7)+"\n";
                ++cases;
            }
            audio.releaseResources();
            core.releaseResources();
        }
        root.getChildFile("ceiling-audio.csv").replaceWithText(csv);
        QQSuperCompressionAudioProcessor ui;ui.enterLimiterMode();
        set(ui,"limiterRatioL",1);set(ui,"limiterLink",1);set(ui,"limiterOutputDb",6);set(ui,"limiterMakeupGainLDb",0);
        ui.setBoundaryForDomainDb(false,false,1,-20);
        QQSuperCompressionAudioProcessorEditor e(ui,settings(root));e.stopTimer();e.display.stopTimer();sync(ui,e);
        for(bool enabled:{false,true})
        {
            if(ui.isCeilingEnabled()!=enabled)e.ceilingEnabledButton.onClick();sync(ui,e);
            check(ui.isCeilingEnabled()==enabled && e.ceilingEnabledButton.getToggleState()==enabled,"Ceiling button fails to update state");
            check(e.ceilingValue.isEnabled()==enabled && e.truePeakButton.isEnabled()==enabled,"Ceiling controls not disabled together");
            const float before=ui.readSoundParameter("outputGainDb");const auto event=mouse(e.thresholdLSlider,e.thresholdLSlider.getLocalBounds().toFloat().getCentre());
            e.thresholdLSlider.mouseDown(event);e.thresholdLSlider.setValue(e.thresholdLSlider.getValue()-1,juce::sendNotificationSync);e.thresholdLSlider.mouseUp(event);sync(ui,e);
            check(std::abs(ui.readSoundParameter("outputGainDb")-before-1)<.021f,"Ceiling state affected unconditional Output LINK at Ratio 1:1");
            e.display.clearHistories();
            for(auto& history:e.display.histories){DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=-2;point.captureCounter=100;point.captureGeneration=1;history.points={point};}
            e.display.refreshRenderCaches(2);
            const float projected=e.display.renderCaches[0].projected.output[0];
            check(enabled ? projected<=get(ui,"ceilingDb")+.02f : std::abs(projected-(-2+ui.getActiveOutputGainDb()))<.02f,
                  "Display Ceiling projection does not follow switch");
            image(e,root.getChildFile(enabled?"ceiling-on.png":"ceiling-off.png"));
        }
        std::cout << "PASS: Ceiling defaults, state/A-B/legacy recall, "<<cases<<" audio/latency transitions, UI, Display and independent LINK.\n";
    }
