    static void dualAlgorithmChecks(const juce::File& root)
    {
        using Law=qqsc::CompressionAlgorithm;
        const auto law=[](int i){return i ? Law::super : Law::classic;};
        int projections=0,renders=0;
        for(bool limiter:{false,true})for(int mode:{0,1,2})
        {
            std::array<double,4> upGains{},downGains{};
            for(int up:{0,1})for(int down:{0,1})
            {
                QQSuperCompressionAudioProcessor p;baseline(p);
                if(limiter)p.enterLimiterMode();
                set(p,p.soundParameterID("compressionMode"),1);set(p,p.soundParameterID("processingMode"),float(mode));
                set(p,p.soundParameterID("upAlgorithmMode"),float(up));set(p,p.soundParameterID("downAlgorithmMode"),float(down));
                set(p,p.soundParameterID("outputGainDb"),0);set(p,"limiterCalibrationDb",0);set(p,"ceilingDb",0);
                for(size_t d=0;d<5;++d)
                {
                    p.setBoundaryForDomainDb(true,false,int(d),-40);p.setBoundaryForDomainDb(true,true,int(d),-12);
                    set(p,p.soundParameterID(qqsc::params::upRatioIds[d]),.125f);
                    set(p,p.soundParameterID(qqsc::params::downRatioIds[d]),limiter ? 400.f : 8.f);
                    for(float level:{.003f,.03f,.15f,.4f,.8f})
                    {
                        const float expected=level<.01f ? 1.f : level<std::pow(10.f,-12.f/20)
                            ? qqsc::StaticCompressionEngine::dualGainForLevel(level,.125f,limiter?400.f:8.f,.01f,std::pow(10.f,-12.f/20),law(up))
                            : qqsc::StaticCompressionEngine::dualGainForLevel(level,.125f,limiter?400.f:8.f,.01f,std::pow(10.f,-12.f/20),law(down));
                        check(std::abs(p.getDynamicsGainForDomain(level,int(d))-expected)<2.e-5f,"Display projection uses wrong Dual branch algorithm");
                        ++projections;
                    }
                }
                const auto u=render(p,.03f,127,false,400,48000,1.f);
                const auto d=render(p,.6f,127,false,400,48000,1.f);
                upGains[size_t(up*2+down)]=rmsGain(u,.03f,48000);
                downGains[size_t(up*2+down)]=rmsGain(d,.6f,48000);renders+=2;
            }
            check(std::abs(upGains[0]-upGains[1])<1.e-5&&std::abs(upGains[2]-upGains[3])<1.e-5,"Down algorithm changed steady Up audio");
            check(std::abs(downGains[0]-downGains[2])<1.e-5&&std::abs(downGains[1]-downGains[3])<1.e-5,"Up algorithm changed steady Down audio");
            check(std::abs(upGains[0]-upGains[2])>.05&&std::abs(downGains[0]-downGains[1])>.001,"Dual algorithm controls do not affect their own audio branches");
        }
        double worstStep=0;
        for(double rate:{44100.,48000.,96000.})for(float level:{.03f,.6f})
        {
            QQSuperCompressionAudioProcessor q;baseline(q);set(q,"compressionMode",1);
            set(q,"upAlgorithmMode",0);set(q,"downAlgorithmMode",0);set(q,"upThresholdDb",-40);set(q,"downThresholdDb",-12);
            q.prepareToPlay(rate,64);juce::MidiBuffer midi;float previous=0;
            for(int n=0;n<24576;n+=64)
            {
                if(n==8192){set(q,"upAlgorithmMode",1);set(q,"downAlgorithmMode",1);}
                if(n==8384){set(q,"upAlgorithmMode",0);set(q,"downAlgorithmMode",0);}
                if(n==12288){set(q,"upAlgorithmMode",1);set(q,"downAlgorithmMode",1);}
                juce::AudioBuffer<float> b(2,64);
                for(int i=0;i<64;++i){b.setSample(0,i,level);b.setSample(1,i,level);}
                q.processBlock(b,midi);
                for(int i=0;i<64;++i)
                {
                    const auto value=b.getSample(0,i);check(std::isfinite(value),"Dual algorithm crossfade nonfinite");
                    if(n>=4096)worstStep=std::max(worstStep,std::abs(double(value-previous)));
                    previous=value;
                }
            }
        }
        check(worstStep<.001,"Dual algorithm switch or mid-fade reversal clicked");
        std::cout<<"PASS: Dual algorithm 10 ms crossfades/reversal at 44.1/48/96 kHz; max DC adjacent step="<<worstStep<<".\n";
        // Simulate an old saved session with different global laws in all four
        // A/B x Normal/Limiter banks, then load over an already edited instance.
        QQSuperCompressionAudioProcessor old;baseline(old);set(old,"algorithmMode",0);
        old.enterLimiterMode();set(old,"limiterAlgorithmMode",1);old.copyAToB();old.selectABSlot(1);
        old.leaveLimiterMode();set(old,"algorithmMode",1);old.enterLimiterMode();set(old,"limiterAlgorithmMode",0);
        juce::MemoryBlock legacy;old.getStateInformation(legacy);
        auto xml=juce::AudioProcessor::getXmlFromBinary(legacy.getData(),int(legacy.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);
        for(const auto* id:{"upAlgorithmMode","downAlgorithmMode","limiterUpAlgorithmMode","limiterDownAlgorithmMode"})
        {
            tree.removeChild(tree.getChildWithProperty("id",id),nullptr);
            for(const auto* slot:{"A_","B_"})tree.removeProperty("qqscAB_"+juce::String(slot)+id,nullptr);
        }
        tree.setProperty("qqscStateSchemaVersion",21,nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),legacy);
        QQSuperCompressionAudioProcessor restored;set(restored,"upAlgorithmMode",1);set(restored,"downAlgorithmMode",1);
        restored.setStateInformation(legacy.getData(),int(legacy.getSize()));
        for(int slot:{0,1})for(bool limiter:{false,true})
        {
            restored.selectABSlot(slot);if(limiter)restored.enterLimiterMode();else restored.leaveLimiterMode();
            const float expected=float(slot==0 ? (limiter?1:0) : (limiter?0:1));
            std::cout<<"MIGRATE slot="<<slot<<" limiter="<<limiter<<" single="<<restored.readSoundParameter("algorithmMode")
                     <<" up="<<restored.readSoundParameter("upAlgorithmMode")<<" down="<<restored.readSoundParameter("downAlgorithmMode")<<" expected="<<expected<<"\n";
            check(restored.readSoundParameter("upAlgorithmMode")==expected&&restored.readSoundParameter("downAlgorithmMode")==expected,
                  "Legacy A/B/mode bank did not inherit its own algorithm");
        }
        // Every visible selector stays in the Ratio area, with one shared Up
        // and Down selection in LR/MS. Native offscreen editor renders only.
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"upAlgorithmMode",0);set(p,"downAlgorithmMode",1);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
            for(int mode:{0,1,2})for(bool dual:{false,true})
            {
                set(p,"processingMode",float(mode));set(p,"compressionMode",dual?1.f:0.f);sync(p,e);e.updateAlgorithmUi();
                e.theme=theme;e.applyTheme();e.setSize(1200,800);e.resized();
                check(e.algorithmButton.isVisible()!=dual&&e.upAlgorithmButton.isVisible()==dual&&e.downAlgorithmButton.isVisible()==dual,"Algorithm selector visibility");
                for(auto* button:{&e.algorithmButton,&e.upAlgorithmButton,&e.downAlgorithmButton})if(button->isVisible())
                {
                    check(button->getY()>=e.ratioLabel.getY()&&button->getBottom()<e.ratioSlider.getBottom(),"Algorithm selector is not beside Ratio");
                    for(auto* other:{&e.compressionModeButton,&e.dualRatioLinkButton,&e.inputOutputLinkButton})
                        if(other->isVisible())check(!button->getBounds().intersects(other->getBounds()),"Algorithm selector overlaps nearby button");
                }
                image(e,root.getChildFile(juce::String("algorithms-")+qqsc::ui::themeKey(theme)+"-"+juce::String(mode)+(dual?"-dual.png":"-single.png")));
            }
        set(p,"compressionMode",1);sync(p,e);e.updateAlgorithmUi();
        const auto single=get(p,"algorithmMode"),output=get(p,"outputGainDb");
        p.getUndoManager().clearUndoHistory();e.upAlgorithmButton.onClick();sync(p,e);
        check(get(p,"upAlgorithmMode")==1&&get(p,"downAlgorithmMode")==1&&get(p,"algorithmMode")==single,"Up selector changed another algorithm");
        p.getUndoManager().undo();sync(p,e);check(get(p,"upAlgorithmMode")==0,"Up algorithm undo");
        p.getUndoManager().redo();sync(p,e);check(get(p,"upAlgorithmMode")==1,"Up algorithm redo");
        e.downAlgorithmButton.onClick();check(get(p,"upAlgorithmMode")==1&&get(p,"downAlgorithmMode")==0,"Down selector changed Up");
        check(get(p,"outputGainDb")==output,"Normal branch selector changed Output");
        // User preference is a new-instance default, never a saved-state override.
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor freshEditor(fresh,settings(root));
        check(get(fresh,"upAlgorithmMode")==1&&get(fresh,"downAlgorithmMode")==0,"Last Dual algorithm preferences lost");
        juce::MemoryBlock saved;p.getStateInformation(saved);
        QQSuperCompressionAudioProcessor loaded;loaded.setStateInformation(saved.getData(),int(saved.getSize()));
        auto conflicting=settings(root);conflicting->setValue("lastUpAlgorithmMode",0);conflicting->setValue("lastDownAlgorithmMode",1);
        QQSuperCompressionAudioProcessorEditor loadedEditor(loaded,std::move(conflicting));
        check(get(loaded,"upAlgorithmMode")==1&&get(loaded,"downAlgorithmMode")==0,"Preferences replaced saved branch algorithms");
        std::cout<<"PASS: "<<projections<<" branch-specific display projections, "<<renders<<" real processor renders (four algorithm combinations, ST/LR/MS, Normal/Limiter), legacy four-bank migration, 18 real UI layouts, undo/redo and last-choice preferences.\n";
    }
