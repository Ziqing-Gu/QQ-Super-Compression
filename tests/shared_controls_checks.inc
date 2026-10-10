    static void sharedControlsChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;p.setEcoMode(false);p.enterLimiterMode();
        set(p,"limiterDomainLink",1);set(p,"limiterLink",0);set(p,"limiterLookaheadMs",26);
        set(p,"limiterRatioL",4);set(p,"limiterRatioR",30);
        set(p,"limiterMakeupGainLDb",3);set(p,"limiterMakeupGainRDb",-9);
        set(p,"limiterMixL",63);set(p,"limiterMixR",0);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.setSize(1200,800);sync(p,e);e.timerCallback();
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        for(int dual:{0,1})for(int width:{1008,1200})
        {
            p.setCompressionModeFromEditor(dual);e.theme=theme;e.applyTheme();e.setSize(width,width*2/3);sync(p,e);e.timerCallback();e.resized();
            check(e.thresholdLSlider.isVisible() && !e.thresholdRSlider.isVisible() && !e.thresholdSlider.isVisible(),"One common threshold");
            check(e.upperBoundarySliders[1]->isVisible()==bool(dual) && !e.upperBoundarySliders[2]->isVisible(),"Limiter has only common Dual upper boundary");
            check(e.lowerBoundaryNames[1].getText()==(dual?"UP THR":"THRESHOLD"),"Common boundary label");
            check(e.thresholdLSlider.getHeight()>400,"Full-height ST boundary rail");
            check(e.display.getBoundaryPlotForDomain(1)==e.display.getBoundaryPlotForDomain(0),"ST plot geometry");
            check(!e.monitorLabel.isVisible() && !e.monitorAllButton.isVisible()
                && !e.monitorFirstButton.isVisible() && !e.monitorSecondButton.isVisible(),"Limiter has no Monitor UI");
            check(!e.dualRatioLinkButton.isVisible(),"Limiter has no UP/DOWN Ratio LINK");
            check(!e.linkButton.isVisible() && e.limiterStereoLinkValue.isVisible(),"Only numeric L/R Link remains");
            check(e.ratioLSlider.isVisible() && !e.ratioRSlider.isVisible() && !e.ratioSlider.isVisible(),"Single common ratio group");
            check(e.makeupLSlider.isVisible() && !e.makeupRSlider.isVisible() && !e.makeupSTSlider.isVisible(),"Single common Makeup");
            check(e.mixLSlider.isVisible() && !e.mixRSlider.isVisible() && !e.mixSlider.isVisible(),"Single common Mix");
            check(!e.ratioChannel0Label.isVisible() && !e.makeupChannel0Label.isVisible() && !e.mixChannel0Label.isVisible(),"No L/R knob labels");
            check(e.downRatioSliders[1]->isVisible()==bool(dual) && !e.downRatioSliders[2]->isVisible(),"One Dual ratio pair");
            check(e.upEnabledButtons[1].isVisible()==bool(dual) && !e.upEnabledButtons[2].isVisible(),"One Dual ON pair");
            check(e.makeupLSlider.getWidth()==100 && e.mixLSlider.getWidth()==100,"Full common dial size");
            if(!dual)check(e.ratioLSlider.getWidth()==100,"Full single ratio size");
            else check(e.upRatioNames[1].getText()=="UP RATIO" && e.downRatioNames[1].getText()=="DOWN RATIO","Common Dual labels");
            auto img=e.createComponentSnapshot(e.getLocalBounds());
            auto out=root.getChildFile("shared-"+juce::String(int(theme))+"-"+juce::String(dual)+"-"+juce::String(width)+".png").createOutputStream();
            check(out && juce::PNGImageFormat().writeImageToStream(img,*out),"Shared layout screenshot");
        }
        p.setCompressionModeFromEditor(0);sync(p,e);e.timerCallback();
        e.ratioLSlider.onGestureStart();e.ratioLSlider.setValue(6,juce::sendNotificationSync);e.ratioLSlider.onGestureEnd();
        e.makeupLSlider.onGestureStart();e.makeupLSlider.setValue(5,juce::sendNotificationSync);e.makeupLSlider.onGestureEnd();
        e.mixLSlider.onGestureStart();e.mixLSlider.setValue(74,juce::sendNotificationSync);e.mixLSlider.onGestureEnd();
        for(const auto* id:{"ratioR","makeupGainRDb","mixR"})
            check(std::abs(p.readSoundParameter(id)-(juce::String(id)=="ratioR" ? 6.f : juce::String(id)=="mixR" ? 74.f : 5.f))<.001f,"Common gestures reach right sound");
        p.copyAToB();set(p,"limiterRatioL",9);p.selectABSlot(1);check(std::abs(p.readSoundParameter("ratioR")-6)<.001f,"Shared B recall");
        p.selectABSlot(0);check(std::abs(p.readSoundParameter("ratioR")-9)<.001f,"Shared A recall");
        juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));
        check(std::abs(restored.readSoundParameter("ratioR")-9)<.001f && std::abs(restored.readSoundParameter("mixR")-74)<.001f,"Shared project recall");
        p.setCompressionModeFromEditor(1);sync(p,e);
        for(bool up:{false,true})
        {
            const auto* left=up?"limiterUpEnabledL":"limiterDownEnabledL";
            const auto* readRight=up?"upEnabledR":"downEnabledR";
            set(p,left,1);sync(p,e);(void)p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
            (up?e.upEnabledButtons[1]:e.downEnabledButtons[1]).onClick();sync(p,e);
            check(p.readSoundParameter(readRight)==0,"Shared Dual OFF controls both");
            (void)p.getAPVTS().copyState();p.getUndoManager().undo();sync(p,e);
            check(p.readSoundParameter(readRight)==1,"Shared Dual undo controls both");
        }
        // A legacy saved LINK must not couple Limiter gestures or numeric edits.
        for(float legacyLink:{0.f,1.f})
        {
            set(p,"limiterDualRatioLink",legacyLink);set(p,"limiterUpRatioL",.5f);set(p,"limiterDownRatioL",4);
            sync(p,e);e.dualRatioLinkButton.setToggleState(true,juce::dontSendNotification);
            e.downRatioSliders[1]->onGestureStart();e.downRatioSliders[1]->setValue(8,juce::sendNotificationSync);e.downRatioSliders[1]->onGestureEnd();
            check(std::abs(p.readSoundParameter("upRatioL")-.5f)<.0001f,"Limiter Down edit moved UP");
            e.ratioLSlider.onGestureStart();e.ratioLSlider.setValue(.25f,juce::sendNotificationSync);e.ratioLSlider.onGestureEnd();
            check(std::abs(p.readSoundParameter("downRatioL")-8)<.0001f,"Limiter UP edit moved Down");
            const auto parsed=e.ratioLSlider.valueFromTextFunction("1:2");e.ratioLSlider.setValue(parsed,juce::sendNotificationSync);
            check(std::abs(p.readSoundParameter("upRatioL")-.5f)<.0001f && std::abs(p.readSoundParameter("downRatioL")-8)<.0001f,"Limiter numeric Ratio edit coupled opposite branch");
            e.ratioLSlider.onGestureStart();e.ratioLSlider.setValue(1,juce::sendNotificationSync);e.ratioLSlider.onGestureEnd();
            check(std::abs(p.readSoundParameter("downRatioL")-8)<.0001f,"Limiter unity reset coupled DOWN");
        }
        // Ensure ordinary compression can still operate with separate values.
        p.leaveLimiterMode();set(p,"processingMode",2);set(p,"ratioL",2);set(p,"ratioR",7);sync(p,e);
        check(e.ratioLSlider.isVisible() && e.ratioRSlider.isVisible() && e.linkButton.isVisible(),"Normal LR pair retained");
        check(e.monitorLabel.isVisible() && e.monitorFirstButton.isVisible(),"Normal LR Monitor retained");
        check(std::abs(p.readSoundParameter("ratioR")-7)<.001f,"Normal R independent");
        std::cout<<"PASS: 12 shared-control layouts, gestures, hidden LINK, Dual ON/undo, A/B/project and independent Normal LR.\n";

        set(p,"processingMode",0);set(p,"compressionMode",1);set(p,"dualRatioLink",1);
        set(p,"upRatio",.5f);set(p,"downRatio",4);sync(p,e);
        check(e.dualRatioLinkButton.isVisible() && e.dualRatioLinkButton.getToggleState(),"Normal Dual LINK retained");
        e.downRatioSliders[0]->onGestureStart();e.downRatioSliders[0]->setValue(8,juce::sendNotificationSync);e.downRatioSliders[0]->onGestureEnd();
        check(std::abs(p.readSoundParameter("upRatio")-.25f)<.0001f,"Normal inverse Ratio LINK stopped working");
        std::cout<<"PASS: Limiter UP/DOWN independent for legacy LINK on/off, drag, numeric entry and unity reset; Normal Dual LINK retained.\n";

        for(int os:{0,1,2,3})for(int dual:{0,1})
        {
            QQSuperCompressionAudioProcessor x;x.setEcoMode(false);
            x.setDomainMonitorSelection(qqsc::params::leftRight,qqsc::params::monitorFirst);
            x.enterLimiterMode();
            set(x,"limiterOversampling",float(os));set(x,"limiterLookaheadMs",26);set(x,"limiterAlgorithmMode",0);
            set(x,"limiterDownAlgorithmMode",0);set(x,"limiterCompressionMode",float(dual));
            for(const auto* id:{"limiterRatioL","limiterDownRatioL"})set(x,id,4);
            for(const auto* id:{"limiterRatioR","limiterDownRatioR"})set(x,id,1);
            for(const auto* id:{"limiterThresholdLDb","limiterThresholdRDb","limiterDownThresholdLDb","limiterDownThresholdRDb"})set(x,id,-12);
            set(x,"limiterThresholdRDb",-50);set(x,"limiterDownThresholdRDb",-50);
            set(x,"limiterUpEnabledL",0);set(x,"limiterUpEnabledR",1);
            set(x,"limiterMakeupGainLDb",3);set(x,"limiterMakeupGainRDb",-20);set(x,"limiterMixL",63);set(x,"limiterMixR",0);
            set(x,"truePeakLimiting",0);set(x,"ceilingDb",0);set(x,"limiterStereoLink",0);
            x.setRateAndBufferSizeDetails(48000,257);x.prepareToPlay(48000,257);juce::MidiBuffer midi;
            std::array<float,2> last{};
            for(int n=0;n<12000;n+=257)
            {
                juce::AudioBuffer<float>b(2,257);for(int i=0;i<257;++i){b.setSample(0,i,.8f);b.setSample(1,i,.6f);}
                x.processBlock(b,midi);last={b.getSample(0,256),b.getSample(1,256)};
            }
            const auto expected=[](float input){const float gain=std::pow(juce::Decibels::decibelsToGain(-12.f)/input,.75f);return input*(.37f+.63f*juce::Decibels::decibelsToGain(3.f)*gain);};
            check(std::abs(last[1]/last[0]-expected(.6f)/expected(.8f))<.00005f,"Limiter stereo output ignores saved Normal solo");
            x.leaveLimiterMode();
            check(x.getDomainMonitorSelection(qqsc::params::leftRight)==qqsc::params::monitorFirst,"Normal solo state preserved");
        }
        std::cout<<"PASS: eight real-processor Single/Dual shared-ratio/Makeup/Mix audio references across all four OS rates.\n";
        QQSCReviewCheck::sharedSnapshotMatchChecks();
        QQSCVisualCheck::verifyStereoLimiterPresentation();
    }
