    static void outputLinkChecks(const juce::File& root)
    {
        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
        const std::array<const char*,5> mixIds{"mix","mixL","mixR","mixM","mixS"};
        const std::array<const char*,5> makeupIds{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
        int cases=0;double worstMixDifference=0;
        auto drag=[&](Knob& slider,double value)
        {
            const auto ev=mouse(slider,slider.getLocalBounds().toFloat().getCentre());
            slider.mouseDown(ev);slider.setValue(value,juce::sendNotificationSync);slider.mouseUp(ev);
        };
        auto type=[&](Knob& slider,const juce::String& text)
        {
            slider.showTextBox();juce::TextEditor* entry=nullptr;
            for(auto* child:slider.getChildren())
                if(auto* label=dynamic_cast<juce::Label*>(child))
                    if(label->getCurrentTextEditor()) entry=label->getCurrentTextEditor();
            check(entry!=nullptr,"Real knob text editor missing");
            entry->setText(text);slider.hideTextBox(false);
        };
        for(int algorithm:{0,1})for(int mode:{0,1,2})for(int dual:{0,1})for(bool monitor:{false,true})
        {
            std::vector<double> wetReference;
            for(float mix:{100.f,66.9f,50.f,1.f,0.f,-1.f})
            {
                QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
                QQSuperCompressionAudioProcessorEditor e(p,settings(root));
                set(p,p.soundParameterID("algorithmMode"),float(algorithm));
                set(p,p.soundParameterID("processingMode"),float(mode));
                set(p,p.soundParameterID("compressionMode"),float(dual));
                set(p,p.soundParameterID("domainLink"),0);set(p,p.soundParameterID("dualRatioLink"),0);
                set(p,"limiterLink",1);set(p,"limiterOutputDb",8.24f);set(p,"limiterCalibrationDb",2.3f);
                p.setUnityMonitorEnabled(monitor);
                for(size_t d=0;d<5;++d)
                {
                    set(p,p.soundParameterID(qqsc::params::ratioIds[d]),400);
                    set(p,p.soundParameterID(qqsc::params::downRatioIds[d]),400);
                    set(p,p.soundParameterID(qqsc::params::upRatioIds[d]),1);
                    p.setBoundaryForDomainDb(false,true,int(d),1);
                    p.setBoundaryForDomainDb(false,false,int(d),-16.04f-float(d)*2);
                    p.setBoundaryForDomainDb(true,false,int(d),-55);
                    p.setBoundaryForDomainDb(true,true,int(d),-16.04f-float(d)*2);
                    set(p,p.soundParameterID(makeupIds[d]),1.65f+float(d)*.2f);
                    set(p,p.soundParameterID(mixIds[d]),100);
                }
                sync(p,e);
                const std::array<Knob*,5> mixers{&e.mixSlider,&e.mixLSlider,&e.mixRSlider,&e.mixMSlider,&e.mixSSlider};
                const std::array<Knob*,5> makeups{&e.makeupSTSlider,&e.makeupLSlider,&e.makeupRSlider,&e.makeupMSlider,&e.makeupSSlider};
                // Reproduce the exact missed sequence: change Mix, THEN edit a
                // linked control. Merely checking the Mix callback misses the bug.
                for(size_t d=0;d<5;++d)drag(*mixers[d],mix>=0 ? mix : float(d)*21.f);
                check(std::abs(get(p,"limiterOutputDb")-8.24f)<.001,"Mix immediately moved Output");
                const size_t domain=mode==0 ? 0 : mode==2 ? 1 : 3;
                auto& threshold=dual ? *e.upperBoundarySliders[domain] : *e.lowerBoundaryControls()[domain];
                const auto before=threshold.getValue();
                drag(threshold,before-.01);
                const auto tinyOutput=get(p,"limiterOutputDb");
                if(std::abs(tinyOutput-8.24f)>=.02f)
                    std::cout<<"JUMP algo="<<algorithm<<" mode="<<mode<<" dual="<<dual<<" mix="<<mix<<" output="<<tinyOutput<<"\n";
                check(std::abs(tinyOutput-8.24f)<.02f,"Tiny Threshold edit re-calibrated old Mix/Output offset");
                std::vector<double> results{tinyOutput};
                auto record=[&]
                {
                    results.push_back(get(p,"limiterOutputDb"));
                    for(int d=0;d<5;++d)results.push_back(p.getBoundaryForDomainDb(dual!=0,dual!=0,d));
                };
                // Numeric boundary commits, reverse Output drag and typed edit.
                auto& readout=dual ? e.upperBoundaryValues[domain] : e.lowerBoundaryValues[domain];
                readout.setText(juce::String(before-1.,2),juce::sendNotificationSync);record();
                const auto outputBefore=get(p,"limiterOutputDb");drag(e.outputGainSlider,outputBefore+1.);
                check(std::abs(get(p,"limiterOutputDb")-outputBefore-1)<.011,"Reverse Output link failed at partial/zero Mix");record();
                type(e.outputGainSlider,juce::String(outputBefore+.5,2));record();
                // Only Makeup/Down Threshold reconcile Output. Ratio and mode
                // edits must preserve it, even after changing Mix.
                auto previousOutput=get(p,"limiterOutputDb");
                drag(*makeups[domain],2.65);
                check(get(p,"limiterOutputDb")<previousOutput-.001,"Makeup drag lost inverse Output link");record();
                previousOutput=get(p,"limiterOutputDb");
                type(*makeups[domain],"3.15");
                check(get(p,"limiterOutputDb")<previousOutput-.001,"Makeup text lost inverse Output link");record();
                auto& ratio=dual ? *e.downRatioSliders[domain] : *e.mainRatioControls()[domain];
                previousOutput=get(p,"limiterOutputDb");drag(ratio,800);
                check(get(p,"limiterOutputDb")==previousOutput,"Ratio drag moved Output");record();
                previousOutput=get(p,"limiterOutputDb");type(ratio,"500:1");
                check(get(p,"limiterOutputDb")==previousOutput,"Ratio text moved Output");record();
                set(p,p.soundParameterID("domainLink"),1);set(p,p.soundParameterID("dualRatioLink"),1);sync(p,e);
                previousOutput=get(p,"limiterOutputDb");drag(ratio,600);
                check(get(p,"limiterOutputDb")==previousOutput,"Linked-domain/Dual Ratio moved Output");record();
                drag(threshold,threshold.getValue()-.5);record();
                previousOutput=get(p,"limiterOutputDb");e.algorithmButton.onClick();
                check(get(p,"limiterOutputDb")==previousOutput,"Algorithm moved Output");record();
                // LINK ON must preserve a manually offset pair, not jump back to
                // the original mixed-signal calibration.
                e.limiterLinkButton.onClick();set(p,"limiterOutputDb",6.25f);sync(p,e);
                e.limiterLinkButton.onClick();check(std::abs(get(p,"limiterOutputDb")-6.25f)<.001,"Enabling LINK re-calibrates output");
                record();
                if(mix==100)wetReference=results;
                else for(size_t i=0;i<results.size();++i)
                {
                    worstMixDifference=std::max(worstMixDifference,std::abs(results[i]-wetReference[i]));
                    check(std::abs(results[i]-wetReference[i])<.011,"Control linkage depends on Mix");
                }
                check(std::abs(get(p,"limiterCalibrationDb")-2.3f)<.001,"Control link rewrote audible calibration");
                // Parsing an unchanged number must not leave a pending UI edit
                // that later treats host automation as an interactive gesture.
                (void)makeups[domain]->getValueFromText(juce::String(makeups[domain]->getValue(),2));
                const auto automationOutput=get(p,"limiterOutputDb");
                set(p,p.soundParameterID(makeupIds[domain]),p.readSoundParameter(makeupIds[domain])+.25f);sync(p,e);
                check(std::abs(get(p,"limiterOutputDb")-automationOutput)<.001,"Host automation inherited a stale text gesture");
                // Saved/A-B output offsets become the anchor of the NEXT edit.
                p.copyAToB();p.selectABSlot(1);sync(p,e);
                auto& next=dual ? *e.upperBoundarySliders[domain] : *e.lowerBoundaryControls()[domain];
                const auto last=get(p,"limiterOutputDb");drag(next,next.getValue()-.01);
                check(std::abs(get(p,"limiterOutputDb")-last)<.02,"A/B leaves stale link anchor");
                juce::MemoryBlock saved;p.getStateInformation(saved);
                QQSuperCompressionAudioProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
                QQSuperCompressionAudioProcessorEditor reopened(restored,settings(root));sync(restored,reopened);
                auto& restoredThreshold=dual ? *reopened.upperBoundarySliders[domain] : *reopened.lowerBoundaryControls()[domain];
                const auto restoredOutput=get(restored,"limiterOutputDb");drag(restoredThreshold,restoredThreshold.getValue()-.01);
                check(std::abs(get(restored,"limiterOutputDb")-restoredOutput)<.02,"Reopened project jumps on first edit");
                ++cases;
            }
        }
        // Independent Classic reference: -0.01 dB threshold at 200:1 needs only
        // +0.00995 dB Output compensation, including after changing to 50% Mix.
        QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
        set(p,p.soundParameterID("algorithmMode"),0);set(p,p.soundParameterID("compressionMode"),0);
        set(p,p.soundParameterID("processingMode"),0);set(p,"limiterRatio",200);set(p,"limiterRangeDb",1);
        p.setBoundaryForDomainDb(false,false,0,-18);set(p,"limiterOutputDb",17.91f);set(p,"limiterCalibrationDb",0);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
        drag(e.mixSlider,50);drag(e.thresholdSlider,-18.01);
        check(std::abs(get(p,"limiterOutputDb")-17.91995)<.006,"Classic 200:1 delta reference");
        sync(p,e);p.getUndoManager().clearUndoHistory();const auto previous=get(p,"limiterOutputDb");
        drag(e.thresholdSlider,-19.01);sync(p,e);
        const auto changed=get(p,"limiterOutputDb");
        p.getUndoManager().undo();sync(p,e);
        check(std::abs(get(p,"limiterOutputDb")-previous)<.011 && std::abs(p.getBoundaryForDomainDb(false,false,0)+18.01)<.011,"Linked Output/Threshold undo split");
        p.getUndoManager().redo();sync(p,e);check(std::abs(get(p,"limiterOutputDb")-changed)<.011,"Link redo");
        set(p,"limiterRangeDb",-6);sync(p,e);const auto fixed=p.getBoundaryForDomainDb(false,false,0);
        drag(e.outputGainSlider,e.outputGainSlider.getValue());
        check(p.getBoundaryForDomainDb(false,false,0)==fixed,"Zero Output gesture moves Threshold on flat reference");
        std::cout<<"PASS: "<<cases<<" Mix-change-then-link scenarios, Classic/Super, Single/Dual, ST/LR/MS, monitor on/off, Mix 100/66.9/50/1/0 and asymmetric; drag/text, both directions, Makeup/Ratio/algorithm, LINK re-enable, A/B and project reopen; worst Mix-dependent difference="<<worstMixDifference<<" dB. Classic analytic delta and undo/redo passed.\n";
    }
