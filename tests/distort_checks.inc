    static void distortChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p; p.setEcoMode(false);
        set(p,"lookaheadMs",26);set(p,"oversampling",0);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.setSize(1200,800);sync(p,e);
        check(get(p,"distort")==0 && p.readSoundParameter("detectorWindowMs")==26,"Distort default");
        check(e.detectorWindowValue.getText()=="0.00 %","Distort percentage display");
        check(e.detectorModeButton.getButtonText()=="SAFE" && !e.detectorModeButton.getToggleState(),"SAFE unlit means Normal");
        e.detectorModeButton.onClick();check(e.detectorModeButton.getButtonText()=="SAFE" && e.detectorModeButton.getToggleState(),"SAFE lit means Safe");
        check(e.detectorWindowValue.getText()=="26.00 ms" && e.detectorWindowLabel.getText()=="WINDOW","Safe window units/default");
        e.detectorWindowValue.setText("13 ms",juce::sendNotificationSync);
        check(std::abs(get(p,"distort")-50)<.001f,"Safe ms conversion");
        e.detectorModeButton.onClick();check(e.detectorWindowValue.getText()=="50.00 %","Mode switch retains window");
        e.detectorWindowValue.setText("50 %",juce::sendNotificationSync);
        check(get(p,"distort")==50 && p.readSoundParameter("detectorWindowMs")==13,"50 percent mapping");
        e.detectorWindowValue.start();e.detectorWindowValue.move(10,false);e.detectorWindowValue.move(10,true);e.detectorWindowValue.finish();
        check(std::abs(get(p,"distort")-51.1f)<.001f,"Distort drag/fine");
        e.detectorWindowValue.setText("200 %",juce::sendNotificationSync);check(get(p,"distort")==100,"Distort upper bound");
        e.detectorWindowValue.reset();check(get(p,"distort")==0,"Distort Alt reset");
        set(p,"distort",63); (void)p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
        p.setLookaheadFromEditor(40);check(get(p,"distort")==0 && p.readSoundParameter("detectorWindowMs")==40,"Lookahead reset");
        (void)p.getAPVTS().copyState();p.getUndoManager().undo();sync(p,e);
        check(get(p,"lookaheadMs")==26 && get(p,"distort")==63,"Lookahead/Distort undo");
        p.getUndoManager().redo();sync(p,e);check(get(p,"lookaheadMs")==40 && get(p,"distort")==0,"Lookahead/Distort redo");
        for(int detector:{0,1})
        {
            set(p,"detectorMode",float(detector));set(p,"distort",70);set(p,"lookaheadMs",80);
            check(p.readSoundParameter("distort")==0,"Host Lookahead reset delayed until UI tick");
            set(p,"distort",31);sync(p,e);check(get(p,"distort")==31,"Later host Distort lost to pending reset");
            set(p,"lookaheadMs",26);sync(p,e);check(get(p,"distort")==0,"Normal/Safe reset");
        }
        set(p,"distort",18);set(p,"detectorMode",0);p.copyAToB();
        set(p,"lookaheadMs",80);set(p,"distort",74);set(p,"oversampling",3);set(p,"detectorMode",1);
        p.selectABSlot(1);check(get(p,"lookaheadMs")==26 && get(p,"distort")==18 && get(p,"detectorMode")==0,"A/B custom B lost");
        p.selectABSlot(0);check(get(p,"lookaheadMs")==80 && get(p,"distort")==74 && get(p,"oversampling")==3,"A/B custom A lost");
        juce::MemoryBlock saved;p.getStateInformation(saved);
        QQSuperCompressionAudioProcessor q;q.setStateInformation(saved.getData(),int(saved.getSize()));
        check(get(q,"distort")==74,"State percentage restore");q.selectABSlot(1);check(get(q,"distort")==18,"State inactive B restore");
        auto tree=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize())));
        tree.removeChild(tree.getChildWithProperty("id","distort"),nullptr);
        tree.removeProperty("qqscAB_A_distort",nullptr);tree.removeProperty("qqscAB_B_distort",nullptr);
        tree.getChildWithProperty("id","detectorWindowMs").setProperty("value",20.f,nullptr);
        tree.setProperty("qqscAB_A_detectorWindowMs",20.f,nullptr);tree.setProperty("qqscAB_B_detectorWindowMs",13.f,nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),saved);q.setStateInformation(saved.getData(),int(saved.getSize()));
        check(std::abs(get(q,"distort")-75)<.001f,"Legacy ms migration active");
        q.selectABSlot(1);check(std::abs(get(q,"distort")-50)<.001f,"Legacy ms migration inactive B");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        for(int limiter:{0,1})for(int mode:{0,1,2})for(int look:{0,26})
        {
            set(p,"limiterMode",float(limiter));set(p,p.soundParameterID("processingMode"),float(mode));
            set(p,p.soundParameterID("lookaheadMs"),float(look));e.theme=theme;e.applyTheme();sync(p,e);e.timerCallback();e.resized();
            check(e.detectorWindowValue.isEnabled()==(look>0),"Zero Lookahead Distort disabled");
            check(e.oversamplingButton.isVisible() && !e.ceilingOversamplingButton.isVisible(),"Single OS selector");
            check(e.detectorWindowValue.getWidth()==e.lookaheadCombo.getWidth(),"Distort field width");
            check(e.detectorModeButton.getX()>e.limiterButton.getRight(),"Detector position");
            const auto img=e.createComponentSnapshot(e.getLocalBounds());
            auto out=root.getChildFile("distort-"+juce::String(int(theme))+"-"+juce::String(limiter)+"-"+juce::String(mode)+"-"+juce::String(look)+".png").createOutputStream();
            check(out && out->setPosition(0) && out->truncate().wasOk() && juce::PNGImageFormat().writeImageToStream(img,*out),"UI image write");
        }
        set(p,"limiterMode",0);set(p,"processingMode",0);set(p,"lookaheadMs",26);
        set(p,"detectorMode",0);set(p,"distort",0);e.theme=qqsc::ui::Theme::dark;e.applyTheme();sync(p,e);e.timerCallback();e.resized();
        {
            auto out=root.getChildFile("normal-distort.png").createOutputStream();
            const auto img=e.createComponentSnapshot(e.getLocalBounds());
            check(out && out->setPosition(0) && out->truncate().wasOk() && juce::PNGImageFormat().writeImageToStream(img,*out),"Normal UI image write");
        }
        std::cout<<"PASS: percentage UI, reset/undo, host edits, A/B/state/legacy migration and 36 layouts plus Normal field.\n";

        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
        for(int mode:{0,1,2})for(int algorithm:{0,1})
        {
            QQSuperCompressionAudioProcessor lp;lp.enterLimiterMode();
            set(lp,"limiterCompressionMode",1);set(lp,"limiterProcessingMode",float(mode));
            set(lp,"limiterUpAlgorithmMode",float(algorithm));set(lp,"limiterLink",1);set(lp,"limiterDomainLink",0);
            const int d=mode==0?0:mode==2?1:3;
            set(lp,qqsc::params::limiterUpRatioIds[size_t(d)],.5f);set(lp,qqsc::params::limiterDownRatioIds[size_t(d)],20.f);
            lp.setBoundaryForDomainDb(true,false,d,-60);lp.setBoundaryForDomainDb(true,true,d,-15);
            QQSuperCompressionAudioProcessorEditor le(lp,settings(root));sync(lp,le);
            auto* up=le.lowerBoundaryControls()[size_t(d)];
            check(up->getMinimum()==(algorithm==0?-90:-120),"Super -inf / Classic -90 minimum");
            const float before=get(lp,"limiterOutputDb");
            le.beginBoundaryGesture(d,false);up->setValue(-50,juce::sendNotificationSync);le.endLinkedGesture();
            check(get(lp,"limiterOutputDb")==before,"UP moved Output");
            auto* makeup=std::array<Knob*,5>{&le.makeupSTSlider,&le.makeupLSlider,&le.makeupRSlider,&le.makeupMSlider,&le.makeupSSlider}[size_t(d)];
            const auto ev=mouse(*makeup,makeup->getLocalBounds().toFloat().getCentre());
            makeup->mouseDown(ev);makeup->setValue(2,juce::sendNotificationSync);makeup->mouseUp(ev);
            check(std::abs(get(lp,"limiterOutputDb")-(before-2))<.02f,"Makeup link lost");
            check(lp.getBoundaryForDomainDb(true,false,d)==-50,"Makeup moved UP");
            le.beginLimiterOutputGesture();const double out=le.applyLimiterOutputChange(get(lp,"limiterOutputDb")+3);
            set(lp,"limiterOutputDb",float(out));le.endLinkedGesture();
            check(lp.getBoundaryForDomainDb(true,false,d)==-50,"Output moved UP");
            check(std::abs(lp.getBoundaryForDomainDb(true,true,d)+18)<.02f,"Output/DOWN link lost");
            if(algorithm==1){lp.setBoundaryForDomainDb(true,false,d,-120);sync(lp,le);check(le.lowerBoundaryValues[size_t(d)].getText().contains("-inf"),"Super UP -inf readout");}
        }
        std::cout<<"PASS: Limiter UP independence, Super -inf / Classic -90, DOWN/Makeup links in ST/LR/MS.\n";

        int cases=0;float worst=0;
        for(double rate:{44100.,48000.,96000.})for(float look:{26.f,40.f,80.f,100.f})
        for(int detector:{0,1})for(float colour:{0.f,75.f,100.f})
        {
            QQSuperCompressionAudioProcessor x;x.setEcoMode(false);
            set(x,"algorithmMode",1);set(x,"ratio",5);set(x,"lookaheadMs",look);set(x,"oversampling",0);
            set(x,"detectorMode",float(detector));set(x,"distort",colour);
            x.setRateAndBufferSizeDetails(rate,257);x.prepareToPlay(rate,257);
            const int delay=int(std::round(rate*look*.001)),window=int(std::round(rate*look*.001*(1-colour/100)));
            check(x.getLatencySamples()==delay,"Distort changed Lookahead PDC");
            const int rise=int(rate*.15),fall=int(rate*.30),total=int(rate*.45)+delay;juce::MidiBuffer midi;
            for(int offset=0;offset<total;offset+=257)
            {
                const int count=std::min(257,total-offset);juce::AudioBuffer<float> a(2,count);
                for(int i=0;i<count;++i)for(int ch=0;ch<2;++ch)a.setSample(ch,i,offset+i>=rise&&offset+i<fall?.8f:.1f);
                x.processBlock(a,midi);
                for(int i=0;i<count;++i)
                {
                    const int t=offset+i-delay;float expected=0;
                    if(t>=0){const float carrier=t>=rise&&t<fall?.8f:.1f;
                        const float past=t>=rise&&t-window<fall?.8f:.1f,future=t+window>=rise&&t<fall?.8f:.1f;
                        expected=carrier/(1+4*(detector?std::max(past,future):std::min(past,future)));}
                    worst=std::max(worst,std::abs(a.getSample(0,i)-expected));
                }
            }
            check(worst<2e-6f,"Normal/Safe time window or static law changed");++cases;
        }
        std::cout<<"PASS: "<<cases<<" independent step-response oracles, worst="<<worst<<".\n";
    }
