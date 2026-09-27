    static void strictOutputLinkChecks(const juce::File& root)
    {
        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
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
                    if(label->getCurrentTextEditor())entry=label->getCurrentTextEditor();
            check(entry!=nullptr,"Strict link: knob text editor missing");
            entry->setText(text);slider.hideTextBox(false);
        };
        int cases=0,unlinkedEdits=0;
        for(int algo:{0,1})for(int mode:{0,1,2})for(bool link:{false,true})for(bool ratioLink:{false,true})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,p.soundParameterID("algorithmMode"),float(algo));
            set(p,p.soundParameterID("processingMode"),float(mode));
            set(p,p.soundParameterID("compressionMode"),0);
            set(p,p.soundParameterID("domainLink"),1);
            set(p,p.soundParameterID("dualRatioLink"),ratioLink ? 1.f : 0.f);
            set(p,"limiterLink",link ? 1.f : 0.f);set(p,"limiterOutputDb",8.24f);set(p,"limiterCalibrationDb",2.3f);
            for(size_t d=0;d<5;++d)
            {
                set(p,qqsc::params::limiterRatioIds[d],400);
                set(p,qqsc::params::limiterDownRatioIds[d],400);
                set(p,qqsc::params::limiterUpRatioIds[d],.5f);
                p.setBoundaryForDomainDb(false,true,int(d),1);
                p.setBoundaryForDomainDb(false,false,int(d),-16.04f);
                p.setBoundaryForDomainDb(true,false,int(d),-55);
                p.setBoundaryForDomainDb(true,true,int(d),-16.04f);
            }
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            auto fixed=[&](const char* label,auto action)
            {
                const auto before=get(p,"limiterOutputDb"),normal=get(p,"outputGainDb"),cal=get(p,"limiterCalibrationDb");
                action();sync(p,e);
                if(get(p,"limiterOutputDb")!=before)std::cerr<<"Output changed after "<<label<<"\n";
                check(get(p,"limiterOutputDb")==before,"Non-whitelisted edit moved Limiter Output");
                check(get(p,"outputGainDb")==normal&&get(p,"limiterCalibrationDb")==cal,"Hidden gain/bank changed");
                ++unlinkedEdits;
            };
            const size_t domain=mode==0 ? 0 : mode==2 ? 1 : 3;
            // Exactly the reported failure, including the unity detent in both
            // directions, reciprocal upward values, typed input and Alt reset.
            for(auto* ratio:e.mainRatioControls())
            {
                for(double value:{1.,.125,1.,200.,1000.,400.})
                    fixed("Single Ratio drag",[&]{drag(*ratio,value);});
                fixed("Single Ratio text unity",[&]{type(*ratio,"1:1");});
                fixed("Single Ratio text down",[&]{type(*ratio,"800:1");});
                fixed("Single Ratio Alt reset",[&]
                {const auto ev=mouse(*ratio,ratio->getLocalBounds().toFloat().getCentre(),true);ratio->mouseDown(ev);ratio->mouseUp(ev);});
                fixed("Single Ratio restore",[&]{drag(*ratio,400);});
            }
            fixed("Input drag",[&]{drag(e.inputGainSlider,3);});
            fixed("Input text",[&]{type(e.inputGainSlider,"-2");});
            for(auto* mix:{&e.mixSlider,&e.mixLSlider,&e.mixRSlider,&e.mixMSlider,&e.mixSSlider})
                fixed("Mix",[&]{drag(*mix,66.9);});
            for(auto& range:e.upperBoundarySliders)
            {
                fixed("Range finite",[&]{drag(*range,-6);});
                fixed("Range OFF",[&]{drag(*range,1);});
            }
            fixed("Classic/Super",[&]{e.algorithmButton.onClick();});
            for(int i=0;i<3;++i)fixed("ST/LR/MS",[&]{e.modeButton.onClick();});
            fixed("Ceiling",[&]{e.ceilingValue.setText("-2.00",juce::sendNotificationSync);});
            fixed("TP",[&]{e.truePeakButton.onClick();});
            fixed("Headphones",[&]{e.unityMonitorButton.onClick();});
            fixed("Single/Dual",[&]{e.compressionModeButton.onClick();});
            fixed("Up Classic/Super",[&]{e.upAlgorithmButton.onClick();});
            fixed("Down Classic/Super",[&]{e.downAlgorithmButton.onClick();});
            for(size_t d=0;d<5;++d)
            {
                auto& up=*e.mainRatioControls()[d];auto& down=*e.downRatioSliders[d];
                fixed("Dual Down drag",[&]{drag(down,800);});
                fixed("Dual Down text",[&]{type(down,"300:1");});
                fixed("Dual Up drag",[&]{drag(up,.125);});
                fixed("Dual Up unity",[&]{type(up,"1:1");});
                fixed("Down OFF",[&]{e.downEnabledButtons[d].onClick();});
                fixed("Down ON",[&]{e.downEnabledButtons[d].onClick();});
                fixed("Up OFF",[&]{e.upEnabledButtons[d].onClick();});
                fixed("Up ON",[&]{e.upEnabledButtons[d].onClick();});
                fixed("Up gate",[&]{drag(*e.lowerBoundaryControls()[d],-60);});
            }
            fixed("Dual/Single",[&]{e.compressionModeButton.onClick();});
            for(size_t d=0;d<5;++d)
                fixed("Host Ratio",[&]{set(p,qqsc::params::limiterRatioIds[d],600);});
            // Ratio/mode/Mix edits above must not be paid back on the NEXT
            // tiny allowed edit, and LINK OFF disables both allowed sources.
            auto& threshold=*e.lowerBoundaryControls()[domain];
            auto before=get(p,"limiterOutputDb");drag(threshold,threshold.getValue()-.01);
            check(link ? std::abs(get(p,"limiterOutputDb")-before)<.02 : get(p,"limiterOutputDb")==before,
                  "Threshold edit caught up an unrelated change / ignored LINK OFF");
            const std::array<Knob*,5> makeups{&e.makeupSTSlider,&e.makeupLSlider,&e.makeupRSlider,&e.makeupMSlider,&e.makeupSSlider};
            before=get(p,"limiterOutputDb");drag(*makeups[domain],makeups[domain]->getValue()+.25);
            check(link ? std::abs(get(p,"limiterOutputDb")-before+.25)<.011 : get(p,"limiterOutputDb")==before,
                  "Makeup edit caught up unrelated change / ignored LINK OFF");
            fixed("LINK toggle",[&]{e.limiterLinkButton.onClick();});
            fixed("LINK toggle back",[&]{e.limiterLinkButton.onClick();});
            ++cases;
        }
        // Unity or an inactive downward branch must not lock Output itself.
        QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();set(p,"limiterLink",1);
        set(p,"limiterCompressionMode",0);set(p,"limiterProcessingMode",0);set(p,"limiterRatio",1);
        set(p,"limiterOutputDb",8.24f);p.setBoundaryForDomainDb(false,false,0,-18);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
        drag(e.outputGainSlider,10.24);check(std::abs(get(p,"limiterOutputDb")-10.24f)<.001,"Unity Ratio locked Output");
        type(e.outputGainSlider,"6.24");check(std::abs(get(p,"limiterOutputDb")-6.24f)<.001,"Unity Ratio locked typed Output");
        check(std::abs(p.getBoundaryForDomainDb(false,false,0)+18)<.001,"Unity Output fabricated threshold movement");
        std::cout<<"PASS: strict Output whitelist, "<<cases<<" Classic/Super + ST/LR/MS + LINK on/off + Ratio Link on/off cases; "
                 <<unlinkedEdits<<" unrelated control edits left both Output banks/calibration exactly unchanged; unity Output remains usable.\n";
    }
