    static void windowLinkChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;p.setEcoMode(false);
        set(p,"lookaheadMs",26);set(p,"oversampling",0);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.setSize(1200,800);sync(p,e);
        check(get(p,"limiterStereoLink")==0,"New L/R Link default");
        for(int safe:{0,1})
        {
            set(p,"detectorMode",float(safe));sync(p,e);
            e.detectorWindowValue.reset();
            check(e.detectorWindowLabel.getText()=="WINDOW" && e.detectorWindowValue.getText()=="26.00 ms","Both detectors use ms");
            e.detectorWindowValue.setText("13 ms",juce::sendNotificationSync);
            check(std::abs(p.readSoundParameter("detectorWindowMs")-13)<.002f,"Window entry");
            e.detectorWindowValue.start();e.detectorWindowValue.move(10,false);e.detectorWindowValue.move(10,true);e.detectorWindowValue.finish();
            check(std::abs(p.readSoundParameter("detectorWindowMs")-14.1f)<.003f,"Window drag and Shift");
            e.detectorWindowValue.reset();check(p.readSoundParameter("detectorWindowMs")==26,"Window Alt max");
        }
        p.setLookaheadFromEditor(40);sync(p,e);check(e.detectorWindowValue.getText()=="40.00 ms","Lookahead resets Window");
        p.setLookaheadFromEditor(0);sync(p,e);
        const float selected=get(p,"detectorMode");e.detectorModeButton.onClick();
        check(!e.detectorModeButton.isEnabled() && !e.detectorWindowValue.isEnabled() && get(p,"detectorMode")==selected,"Zero ms SAFE disabled");
        p.setLookaheadFromEditor(26);sync(p,e);check(e.detectorModeButton.isEnabled(),"SAFE re-enabled");
        p.enterLimiterMode();sync(p,e);e.resized();
        check(p.readSoundParameter("processingMode")==2 && !e.modeButton.isVisible() && e.limiterStereoLinkValue.isVisible(),"Limiter uses LR Link control");
        e.limiterStereoLinkValue.setText("37.5 %",juce::sendNotificationSync);check(get(p,"limiterStereoLink")==37.5f,"Link entry");
        e.limiterStereoLinkValue.start();e.limiterStereoLinkValue.move(10,false);e.limiterStereoLinkValue.move(10,true);e.limiterStereoLinkValue.finish();
        check(std::abs(get(p,"limiterStereoLink")-38.6f)<.002f,"Link drag Shift");
        e.limiterStereoLinkValue.setText("130 %",juce::sendNotificationSync);check(get(p,"limiterStereoLink")==100,"Link bound");
        e.limiterStereoLinkValue.reset();check(get(p,"limiterStereoLink")==0,"Link Alt zero");
        set(p,"limiterStereoLink",37);p.copyAToB();set(p,"limiterStereoLink",76);
        p.selectABSlot(1);check(get(p,"limiterStereoLink")==37,"Link B state");p.selectABSlot(0);check(get(p,"limiterStereoLink")==76,"Link A state");
        juce::MemoryBlock saved;p.getStateInformation(saved);
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
        check(get(restored,"limiterStereoLink")==76,"Link project restore");restored.selectABSlot(1);check(get(restored,"limiterStereoLink")==37,"Link inactive B restore");
        auto tree=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize())));
        tree.removeChild(tree.getChildWithProperty("id","limiterStereoLink"),nullptr);
        tree.getChildWithProperty("id","limiterProcessingMode").setProperty("value",0,nullptr);
        tree.getChildWithProperty("id","limiterRatio").setProperty("value",7,nullptr);
        tree.setProperty("qqscAB_B_limiterProcessingMode",0,nullptr);tree.setProperty("qqscAB_B_limiterRatio",9,nullptr);
        tree.removeProperty("qqscAB_B_limiterStereoLink",nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),saved);restored.setStateInformation(saved.getData(),int(saved.getSize()));
        std::cout<<"MIGRATE L="<<std::setprecision(9)<<get(restored,"limiterRatioL")<<" R="<<get(restored,"limiterRatioR")<<" link="<<get(restored,"limiterStereoLink")<<"\n";
        check(std::abs(get(restored,"limiterRatioL")-7)<.0001f && std::abs(get(restored,"limiterRatioR")-7)<.0001f && get(restored,"limiterStereoLink")==100,"Legacy ST parameter migration");
        restored.selectABSlot(1);check(std::abs(get(restored,"limiterRatioL")-9)<.0001f && get(restored,"limiterStereoLink")==100,"Legacy B parameter migration");
        std::cout<<"PASS: Window units, gestures, defaults, 0 ms SAFE, Link gestures, A/B/project and legacy ST restoration.\n";

        for(int limiter:{0})for(int mode:{1,2})
        {
            if(limiter && mode!=2)continue;
            set(p,"limiterMode",float(limiter));set(p,p.soundParameterID("processingMode"),float(mode));
            set(p,p.soundParameterID("compressionMode"),1);sync(p,e);
            const int first=mode==1?3:1,second=first+1;
            for(bool upward:{false,true})
            {
                auto& buttons=upward?e.upEnabledButtons:e.downEnabledButtons;
                const auto& ids=upward?qqsc::params::upEnabledIds:qqsc::params::downEnabledIds;
                const auto& untouched=upward?qqsc::params::downEnabledIds:qqsc::params::upEnabledIds;
                set(p,p.soundParameterID("domainLink"),1);
                for(int d:{first,second}){set(p,p.soundParameterID(ids[size_t(d)]),1);set(p,p.soundParameterID(untouched[size_t(d)]),1);}
                sync(p,e);(void)p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
                buttons[size_t(first)].onClick();sync(p,e);
                check(p.readSoundParameter(ids[size_t(first)])==0 && p.readSoundParameter(ids[size_t(second)])==0,"Paired OFF");
                check(p.readSoundParameter(untouched[size_t(first)])==1 && p.readSoundParameter(untouched[size_t(second)])==1,"Other branch unchanged");
                (void)p.getAPVTS().copyState();p.getUndoManager().undo();sync(p,e);
                check(p.readSoundParameter(ids[size_t(first)])==1 && p.readSoundParameter(ids[size_t(second)])==1,"Paired undo");
                buttons[size_t(second)].onClick();sync(p,e);buttons[size_t(first)].onClick();sync(p,e);
                check(p.readSoundParameter(ids[size_t(first)])==1 && p.readSoundParameter(ids[size_t(second)])==1,"Pair toggles from either side");
                set(p,p.soundParameterID("domainLink"),0);buttons[size_t(first)].onClick();sync(p,e);
                check(p.readSoundParameter(ids[size_t(first)])==0 && p.readSoundParameter(ids[size_t(second)])==1,"Unlinked independent enable");
            }
        }
        std::cout<<"PASS: paired UP/DOWN enables in Normal LR/MS, undo, unlinked independence.\n";

        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        for(int limiter:{0,1})for(int mode:{0,1,2})for(int look:{0,26})
        {
            if(limiter && mode!=2)continue;
            set(p,"limiterMode",float(limiter));set(p,p.soundParameterID("processingMode"),float(mode));
            set(p,p.soundParameterID("lookaheadMs"),float(look));e.theme=theme;e.applyTheme();sync(p,e);e.timerCallback();e.resized();
            check(e.detectorModeButton.getY()>e.modeButton.getBottom(),"SAFE row below Mode");
            check(e.detectorModeButton.getBottom()<e.lookaheadLabel.getY(),"Timing below SAFE");
            check(e.lookaheadCombo.getBottom()<e.oversamplingLabel.getY(),"Monitor OS fourth row");
            check(e.oversamplingButton.getBottom()<=800-20,"Controls stay inside editor");
            check(e.detectorModeButton.isEnabled()==(look>0),"SAFE enabled state");
            check(e.lookaheadCombo.getSelectedItemIndex()==qqsc::params::lookaheadChoiceIndexForMs(float(look)),"Lookahead displayed selection");
            check(e.detectorWindowValue.getText().endsWith(" ms"),"All layouts Window ms");
            check(e.modeButton.isVisible()==!bool(limiter) && e.limiterStereoLinkValue.isVisible()==bool(limiter),"Mode vs Link visibility");
            auto img=e.createComponentSnapshot(e.getLocalBounds());
            auto out=root.getChildFile("layout-"+juce::String(int(theme))+"-"+juce::String(limiter)+"-"+juce::String(mode)+"-"+juce::String(look)+".png").createOutputStream();
            check(out && out->setPosition(0) && out->truncate().wasOk() && juce::PNGImageFormat().writeImageToStream(img,*out),"Layout image");
        }
        std::cout<<"PASS: 24 layout/theme/zero-delay combinations, offscreen screenshots.\n";

        QQSCVisualCheck::verifyStereoLinkProjection();

        // Actual audio: identical transfer settings, asymmetrical stereo input.
        // Expected gain uses the independent static Classic compression law.
        float carrierScale=1.0f;
        for(int os:{0,1,2,3})for(int safe:{0,1})for(float link:{0.f,50.f,100.f})
        {
            QQSuperCompressionAudioProcessor x;x.setEcoMode(false);x.enterLimiterMode();
            set(x,"limiterOversampling",float(os));set(x,"limiterLookaheadMs",26);set(x,"limiterAlgorithmMode",0);
            set(x,"limiterRatioL",4);set(x,"limiterRatioR",4);set(x,"limiterThresholdLDb",-12);set(x,"limiterThresholdRDb",-12);
            set(x,"truePeakLimiting",0);set(x,"ceilingDb",0);set(x,"detectorMode",float(safe));set(x,"limiterStereoLink",link);
            x.setRateAndBufferSizeDetails(48000,257);x.prepareToPlay(48000,257);juce::MidiBuffer midi;
            std::array<float,2> last{};
            for(int offset=0;offset<24000;offset+=257)
            {
                juce::AudioBuffer<float> block(2,257);for(int i=0;i<257;++i){block.setSample(0,i,.8f);block.setSample(1,i,.1f);}
                x.processBlock(block,midi);last={block.getSample(0,256),block.getSample(1,256)};
            }
            const float g=std::pow(juce::Decibels::decibelsToGain(-12.f)/.8f,.75f);
            std::cout<<"LINK_AUDIO os="<<os<<" safe="<<safe<<" link="<<link<<" L="<<last[0]<<" R="<<last[1]<<" expectedL="<<.8f*g<<" threshold="<<x.getBoundaryForDomainDb(false,false,1)<<" range="<<x.getBoundaryForDomainDb(false,true,1)<<" ratio="<<get(x,"limiterRatioL")<<"\n";
            // Normalize the independently uncompressed right carrier to remove
            // the OS filters' fixed passband gain from the dynamics oracle.
            if(link==0)carrierScale=last[1]/.1f;
            check(std::abs(last[0]-.8f*g*carrierScale)<.00001f,"Left gain reference");
            check(std::abs(last[1]-.1f*carrierScale*(1.f+link*.01f*(g-1)))<.00001f,"Right fractional link reference");
        }
        std::cout<<"PASS: 24 real-processor stereo-link gain oracles at all OS rates and both detectors.\n";

        // Changing ONLY the loud left input must not touch the right signal at
        // 0% Link, including TP reconstruction and adaptive recovery hints.
        for(int os:{0,1,2,3})for(bool tp:{false,true})for(bool shared:{false,true})
        {
            if(shared && os==0)continue;
            qqsc::OutputCeiling a,b;a.prepare(48000,tp,-.3f,qqsc::params::tpAuto,os,257);b.prepare(48000,tp,-.3f,qqsc::params::tpAuto,os,257);
            a.setStereoLink(0);b.setStereoLink(0);a.resetForLimiter(tp,-.3f);b.resetForLimiter(tp,-.3f);
            float delta=0,peak=0;const int factor=qqsc::params::oversamplingFactorForChoiceIndex(os);
            for(int offset=0;offset<12000;offset+=257)
            {
                const int n=std::min(257,12000-offset);
                if(shared)
                {
                    juce::AudioBuffer<float> aa(2,n*factor),bb(2,n*factor);
                    for(int i=0;i<n*factor;++i)
                    {
                        float time=float(offset)+float(i)/factor;
                        const float right=.1f*std::sin(time*.11f),left=3*std::sin(time*.071f);
                        aa.setSample(0,i,0);bb.setSample(0,i,left);aa.setSample(1,i,right);bb.setSample(1,i,right);
                    }
                    a.processSharedOversampledBlock(juce::dsp::AudioBlock<float>(aa),n,factor);b.processSharedOversampledBlock(juce::dsp::AudioBlock<float>(bb),n,factor);
                    for(int i=0;i<n;++i)
                    {
                        auto av=a.finishSharedBaseSample(aa.getSample(0,i*factor),aa.getSample(1,i*factor),i);
                        auto bv=b.finishSharedBaseSample(bb.getSample(0,i*factor),bb.getSample(1,i*factor),i);
                        delta=std::max(delta,std::abs(av[1]-bv[1]));peak=std::max(peak,std::abs(bv[0]));
                    }
                }
                else for(int i=0;i<n;++i)
                {
                    float time=float(offset+i),right=.1f*std::sin(time*.11f),left=3*std::sin(time*.071f);
                    auto av=a.process(0,right),bv=b.process(left,right);
                    delta=std::max(delta,std::abs(av[1]-bv[1]));peak=std::max(peak,std::abs(bv[0]));
                }
                check(std::isfinite(peak) && peak<1.005f,"Ceiling safety");
            }
            check(delta<1.e-7f,"Zero link ceiling crosstalk");
            check(b.gainForDisplayLinear(1)>.999f,"Quiet right ceiling meter");
        }
        std::cout<<"PASS: 14 Hard/TP/shared/fallback ceiling independence and safety cases.\n";
    }
