    static void clipperChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;
        juce::PropertiesFile::Options options;options.applicationName="QQSCClipperCheck";
        auto settings=std::make_unique<juce::PropertiesFile>(root.getChildFile("clipper-ui.settings"),options);
        QQSuperCompressionAudioProcessorEditor e(p,std::move(settings));e.stopTimer();e.display.stopTimer();
        value(p,"ratio",4);value(p,"outputGainDb",-3);
        e.limiterButton.onClick();
        require(p.isClipperMode() && e.limiterButton.getButtonText()=="CLIPPER","Compressor click must enter Clipper");
        require(!p.isLimiterLinked() && p.readSoundParameter("oversampling")==0,"Clipper defaults: Link OFF, OS 1x");
        require(!e.ceilingValue.isVisible() && !e.truePeakButton.isVisible() && !e.unityMonitorButton.isVisible(),"Clipper must not show Ceiling controls");
        require(e.limiterStereoLinkValue.isVisible() && e.limiterLinkButton.isVisible() && !e.modeButton.isVisible(),"Clipper shared stereo controls");
        require(e.ratioLSlider.isVisible() && !e.ratioRSlider.isVisible() && !e.mixRSlider.isVisible(),"Clipper must use one control set");
        value(p,"ratioL",17);value(p,"thresholdLDb",-18);value(p,"outputGainDb",5);
        e.updateCompressionUi();
        e.makeupLSlider.onGestureStart();e.makeupLSlider.setValue(2,juce::sendNotificationSync);e.makeupLSlider.onGestureEnd();
        require(std::abs(p.readSoundParameter("outputGainDb")-5)<.001f,"Clipper Makeup must not move Output with Link OFF");
        e.limiterLinkButton.onClick();
        e.makeupLSlider.onGestureStart();e.makeupLSlider.setValue(3,juce::sendNotificationSync);e.makeupLSlider.onGestureEnd();
        require(std::abs(p.readSoundParameter("outputGainDb")-4)<.001f,"Clipper explicit LINK must work");
        e.limiterStereoLinkValue.start();e.limiterStereoLinkValue.move(500,false);e.limiterStereoLinkValue.finish();
        require(std::abs(p.readSoundParameter("limiterStereoLink")-50)<.001f,"Clipper L/R Link drag targets its own bank");
        require(p.getAPVTS().getRawParameterValue("limiterStereoLink")->load()==0,"Clipper drag changed Limiter bank");
        e.limiterButton.onClick();require(p.isLimiterMode(),"Clipper click must enter Limiter");
        require(e.ceilingValue.isVisible() && e.limiterButton.getButtonText()=="LIMITER","Limiter Ceiling remains visible");
        require(p.readSoundParameter("ratioL")==1 && p.readSoundParameter("outputGainDb")==0,"Clipper changed inactive Limiter bank");
        value(p,"ratioL",9);value(p,"outputGainDb",-2);
        e.limiterButton.onClick();require(!p.isMasteringMode(),"Limiter click must enter Compressor");
        std::cout<<"bank recall ratio="<<std::setprecision(10)<<p.readSoundParameter("ratio")<<" output="<<p.readSoundParameter("outputGainDb")<<std::endl;
        require(std::abs(p.readSoundParameter("ratio")-4)<.0001f && std::abs(p.readSoundParameter("outputGainDb")+3)<.0001f,"Compressor bank changed");
        e.limiterButton.onClick();require(p.isClipperMode(),"Second Clipper entry");
        require(p.readSoundParameter("ratioL")==17 && p.readSoundParameter("outputGainDb")==4,"Clipper bank did not recall");
        p.setCompressionModeFromEditor(1);e.updateCompressionUi();
        require(p.readSoundParameter("compressionMode")==0 && !e.compressionModeButton.isVisible(),"Clipper must be downward Single only");
        require(e.ratioLSlider.getMinimum()==10 && e.ratioLSlider.getMaximum()==1000,"Clipper 10:1 to 1000:1 range");
        require(!e.dualRatioLinkButton.isVisible(),"Clipper Dual controls should remain hidden");
        p.copyAToB();p.selectABSlot(1);value(p,"ratioL",23);value(p,"outputGainDb",8);
        p.selectABSlot(0);require(p.isClipperMode() && p.readSoundParameter("outputGainDb")==4,"A/B Clipper A recall");
        p.selectABSlot(1);require(p.readSoundParameter("ratioL")==23 && p.readSoundParameter("outputGainDb")==8,"A/B Clipper B recall");
        juce::MemoryBlock state;p.getStateInformation(state);
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
        require(restored.isClipperMode() && restored.readSoundParameter("outputGainDb")==8,"Project Clipper recall");
        restored.enterLimiterMode();require(std::abs(restored.readSoundParameter("ratioL")-9)<.0001f && restored.readSoundParameter("outputGainDb")==-2,"Project inactive Limiter recall");
        restored.leaveLimiterMode();require(std::abs(restored.readSoundParameter("ratio")-4)<.0001f && restored.readSoundParameter("outputGainDb")==-3,"Project inactive Compressor recall");
        restored.selectABSlot(0);require(restored.isClipperMode() && restored.readSoundParameter("outputGainDb")==4,"Project inactive A/B recall");
        // Restore a pre-Clipper project into an already-used instance.
        auto old=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize())));
        for(int i=old.getNumChildren();--i>=0;)if(old.getChild(i).getProperty("id").toString().startsWith("clipper"))old.removeChild(i,nullptr);
        for(int i=old.getNumProperties();--i>=0;)if(old.getPropertyName(i).toString().contains("clipper"))old.removeProperty(old.getPropertyName(i),nullptr);
        juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(*old.createXml(),legacy);
        restored.setStateInformation(legacy.getData(),int(legacy.getSize()));
        require(!restored.isClipperMode(),"Legacy project retained previous Clipper mode");
        restored.enterClipperMode();require(!restored.isLimiterLinked() && restored.readSoundParameter("ratioL")==10,"Legacy project retained stale Clipper values");
        p.selectABSlot(0);p.setCompressionModeFromEditor(0);e.updateCompressionUi();e.updateModeUi();
        auto ui=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;
        juce::FileOutputStream image(root.getChildFile("clipper-ui.png"));require(png.writeImageToStream(ui,image),"Clipper UI render");
        std::cout<<"PASS: Clipper UI cycle, defaults, independent banks, Link edits, Single/Dual, A/B, project and old-project recall"<<std::endl;

        int cases=0;float worst=0;
        for(int look:{0,26})for(int os:{0,2})for(int algorithm:{0,1})for(bool dual:{false})
        {
            QQSuperCompressionAudioProcessor clip,normal;clip.enterClipperMode();
            for(auto* x:{&clip,&normal})
            {
                x->setEcoMode(false);value(*x,"processingMode",2);value(*x,"lookaheadMs",float(look));
                value(*x,"oversampling",float(os));value(*x,"distort",0);value(*x,"keyHpfHz",qqsc::params::keyHpfOffHz);
                value(*x,"algorithmMode",float(algorithm));value(*x,"upAlgorithmMode",float(algorithm));value(*x,"downAlgorithmMode",float(algorithm));
                value(*x,"compressionMode",dual?1.f:0.f);
                for(auto id:qqsc::params::ratioIds)value(*x,id,20);
                for(auto id:qqsc::params::upRatioIds)value(*x,id,.5f);
                for(auto id:qqsc::params::downRatioIds)value(*x,id,20);
                for(auto id:qqsc::params::thresholdIds)value(*x,id,-18);
                for(auto id:qqsc::params::upThresholdIds)value(*x,id,-60);
                for(auto id:qqsc::params::downThresholdIds)value(*x,id,-18);
                for(auto id:qqsc::params::rangeIds)value(*x,id,qqsc::params::rangeOffDb);
                value(*x,"makeupGainLDb",2);value(*x,"makeupGainRDb",2);value(*x,"outputGainDb",3);
                value(*x,"mixL",83);value(*x,"mixR",83);value(*x,"limiterStereoLink",0);
                x->setRateAndBufferSizeDetails(48000,128);x->prepareToPlay(48000,128);
            }
            require(clip.getLatencySamples()==normal.getLatencySamples(),"Clipper must not add Ceiling latency");
            juce::MidiBuffer midi;
            for(int block=0;block<100;++block)
            {
                juce::AudioBuffer<float> a(2,128),b(2,128);
                for(int ch=0;ch<2;++ch)for(int i=0;i<128;++i)
                    a.setSample(ch,i,float(.8*std::sin((block*128+i)*.073*(ch?1.7:1.0))));
                b.makeCopyOf(a);clip.processBlock(a,midi);normal.processBlock(b,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<128;++i)worst=std::max(worst,std::abs(a.getSample(ch,i)-b.getSample(ch,i)));
            }
            ++cases;
        }
        require(worst<.00001f,"Clipper differs from independent LR compression without Ceiling");
        std::cout<<"PASS: "<<cases<<" Clipper no-Ceiling audio/latency cases; max sample delta="<<worst<<std::endl;
        for(float link:{0.f,100.f})
        {
            QQSuperCompressionAudioProcessor clip;clip.enterClipperMode();clip.setEcoMode(false);
            value(clip,"lookaheadMs",0);value(clip,"oversampling",0);value(clip,"keyHpfHz",qqsc::params::keyHpfOffHz);
            value(clip,"ratioL",20);value(clip,"thresholdLDb",-20);value(clip,"limiterStereoLink",link);
            clip.prepareToPlay(48000,128);juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,128);
            for(int block=0;block<80;++block)
            {
                for(int i=0;i<128;++i){audio.setSample(0,i,.8f);audio.setSample(1,i,.02f);}
                clip.processBlock(audio,midi);
            }
            const auto leftGain=audio.getSample(0,127)/.8f,rightGain=audio.getSample(1,127)/.02f;
            require(link==0 ? std::abs(rightGain-1.f)<.0001f : std::abs(rightGain-leftGain)<.0001f,"Clipper L/R Link DSP endpoints");
        }
        {
            QQSuperCompressionAudioProcessor clip;clip.enterClipperMode();clip.setEcoMode(false);
            value(clip,"lookaheadMs",0);value(clip,"oversampling",0);
            value(clip,"ceilingDb",-24);value(clip,"truePeakLimiting",1);value(clip,"mixL",0);
            clip.prepareToPlay(48000,128);juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,128);
            for(int block=0;block<80;++block)
            {
                for(int ch=0;ch<2;++ch)for(int i=0;i<128;++i)audio.setSample(ch,i,1.5f);
                clip.processBlock(audio,midi);
            }
            require(clip.getLatencySamples()==0 && std::abs(audio.getSample(0,127)-1.5f)<.0001f,"Clipper must ignore hidden Ceiling/TP");
        }
        std::cout<<"PASS: Clipper L/R Link endpoints and no hidden Ceiling/TP protection"<<std::endl;
    }
