    static void detectorWindowChecks (const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;
        set(p,"lookaheadMs",26); set(p,"detectorMode",1);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));
        e.setSize(1200,800); sync(p,e);
        check(e.detectorWindowValue.getWidth()==e.lookaheadCombo.getWidth(),"WINDOW width differs from Lookahead");
        check(e.detectorWindowValue.getX()>e.lookaheadCombo.getRight(),"WINDOW not to right of Lookahead");
        check(e.detectorWindowValue.getY()==e.lookaheadCombo.getY(),"WINDOW row mismatch");
        check(e.detectorWindowValue.getText()=="26.00 ms","Legacy window default changed");
        e.detectorWindowValue.showEditor();
        check(e.detectorWindowValue.getCurrentTextEditor()!=nullptr,"WINDOW inline editor missing");
        e.detectorWindowValue.getCurrentTextEditor()->setText("8.25 ms");
        e.detectorWindowValue.hideEditor(false); sync(p,e);
        check(std::abs(get(p,"detectorWindowMs")-8.25f)<.001f,"WINDOW text commit");
        e.detectorWindowValue.start(); e.detectorWindowValue.move(10,false); e.detectorWindowValue.move(10,true); e.detectorWindowValue.finish();
        check(std::abs(get(p,"detectorWindowMs")-9.35f)<.001f,"WINDOW vertical/Shift drag");
        (void)p.getAPVTS().copyState(); p.getUndoManager().clearUndoHistory();
        const auto alt=mouse(e.detectorWindowValue,e.detectorWindowValue.getLocalBounds().toFloat().getCentre(),true);
        e.detectorWindowValue.mouseDown(alt);e.detectorWindowValue.mouseUp(alt);
        check(get(p,"detectorWindowMs")==0,"WINDOW Alt reset");
        (void)p.getAPVTS().copyState();p.getUndoManager().undo();sync(p,e);
        check(std::abs(get(p,"detectorWindowMs")-9.35f)<.001f,"WINDOW reset undo");
        e.detectorWindowValue.setText("100 ms",juce::sendNotificationSync);
        check(get(p,"detectorWindowMs")==26,"WINDOW typed upper bound");
        e.detectorWindowValue.setText("-1 ms",juce::sendNotificationSync);
        check(get(p,"detectorWindowMs")==0,"WINDOW typed lower bound");
        set(p,"detectorWindowMs",8);p.copyAToB();set(p,"detectorWindowMs",2);set(p,"detectorMode",0);
        p.selectABSlot(1);check(get(p,"detectorWindowMs")==8 && get(p,"detectorMode")==1,"A/B lost B WINDOW/Det");
        p.selectABSlot(0);check(get(p,"detectorWindowMs")==2 && get(p,"detectorMode")==0,"A/B lost A WINDOW/Det");
        juce::MemoryBlock data;p.getStateInformation(data);
        QQSuperCompressionAudioProcessor q;q.setStateInformation(data.getData(),int(data.getSize()));
        check(get(q,"detectorWindowMs")==2,"Project WINDOW roundtrip");
        q.selectABSlot(1);check(get(q,"detectorWindowMs")==8 && get(q,"detectorMode")==1,"Project B WINDOW roundtrip");
        auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);
        tree.removeChild(tree.getChildWithProperty("id","detectorWindowMs"),nullptr);
        tree.removeProperty("qqscAB_A_detectorWindowMs",nullptr);tree.removeProperty("qqscAB_B_detectorWindowMs",nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);
        q.setStateInformation(data.getData(),int(data.getSize()));
        check(get(q,"detectorWindowMs")==100,"Old project retained prior WINDOW edit");
        q.selectABSlot(1);check(get(q,"detectorWindowMs")==100,"Old project B WINDOW fallback");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();
            for(int mode:{0,1,2})for(int look:{0,26})
            {
                set(p,"processingMode",float(mode));set(p,"lookaheadMs",float(look));sync(p,e);e.timerCallback();e.resized();
                check(e.detectorWindowValue.isEnabled()==(look>0),"0 ms WINDOW must be disabled");
                check(e.oversamplingButton.isVisible()==(look==0),"0 ms OS lost");
                if(look==0)
                {
                    check(e.detectorWindowValue.getText()=="0.00 ms","0 ms WINDOW display not clamped");
                    for(auto* button:{&e.monitorAllButton,&e.monitorFirstButton,&e.monitorSecondButton})
                        if(button->isVisible())check(!e.oversamplingButton.getBounds().intersects(button->getBounds()),"OS/monitor overlap");
                }
                const auto image=e.createComponentSnapshot(e.getLocalBounds());
                auto stream=root.getChildFile("window-"+juce::String(int(theme))+"-"+juce::String(mode)+"-"+juce::String(look)+".png").createOutputStream();
                check(stream!=nullptr && stream->setPosition(0) && stream->truncate().wasOk(),"WINDOW snapshot truncate");
                check(stream!=nullptr && juce::PNGImageFormat().writeImageToStream(image,*stream),"WINDOW snapshot write");
            }
        }
        std::cout<<"PASS: WINDOW editing, drag/Shift, Alt reset/undo, bounds, A/B, old/new state and 18 layouts.\n";

        int cases=0;float worst=0;
        for(double rate:{44100.0,48000.0,96000.0})for(float look:{10.f,26.f,40.f,80.f,100.f})
        for(int detector:{0,1})for(float fraction:{0.f,.25f,1.f})
        {
            QQSuperCompressionAudioProcessor engine;engine.setEcoMode(false);
            set(engine,"algorithmMode",1);set(engine,"ratio",5);set(engine,"thresholdDb",-120);set(engine,"rangeDb",1);
            set(engine,"lookaheadMs",look);set(engine,"detectorWindowMs",look*fraction);set(engine,"detectorMode",float(detector));
            const int block=cases%3==0?17:cases%3==1?257:1024;
            engine.setRateAndBufferSizeDetails(rate,block);engine.prepareToPlay(rate,block);
            const int latency=int(std::round(rate*look*.001)),window=int(std::round(rate*look*fraction*.001));
            check(engine.getLatencySamples()==latency,"WINDOW changed PDC");
            const int rise=int(rate*.15),fall=int(rate*.35),total=int(rate*.50)+latency;
            juce::MidiBuffer midi;
            for(int offset=0;offset<total;offset+=block)
            {
                const int count=juce::jmin(block,total-offset);juce::AudioBuffer<float> audio(2,count);
                for(int i=0;i<count;++i){const int t=offset+i;const float x=t>=rise&&t<fall?.8f:.1f;audio.setSample(0,i,x);audio.setSample(1,i,x);}
                engine.processBlock(audio,midi);
                for(int i=0;i<count;++i)
                {
                    const int t=offset+i-latency;
                    float expected=0;
                    if(t>=0)
                    {
                        const float carrier=t>=rise&&t<fall?.8f:.1f;
                        const float past=t>=rise&&t-window<fall?.8f:.1f;
                        const float future=t+window>=rise&&t<fall?.8f:.1f;
                        const float peak=detector?std::max(past,future):std::min(past,future);
                        expected=carrier/(1+4*peak);
                    }
                    for(int ch=0;ch<2;++ch)worst=std::max(worst,std::abs(audio.getSample(ch,i)-expected));
                }
            }
            check(worst<2e-6f,"WINDOW step span/alignment/static gain oracle failed");
            engine.releaseResources();++cases;
        }
        std::cout<<"PASS: "<<cases<<" fixed-PDC step oracles, both detectors, 3 rates, 5 Lookaheads, 0/quarter/full windows, max delta="<<worst<<".\n";
        // Reconfigure the window in flight; steady key history must not be
        // cleared into unity or silence. Exercise the same path host automation uses.
        QQSuperCompressionAudioProcessor changing;changing.setEcoMode(false);
        set(changing,"algorithmMode",1);set(changing,"ratio",5);set(changing,"thresholdDb",-120);set(changing,"rangeDb",1);
        set(changing,"detectorMode",1);set(changing,"lookaheadMs",26);
        changing.setRateAndBufferSizeDetails(48000,257);changing.prepareToPlay(48000,257);
        juce::MidiBuffer midi;
        for(int block=0;block<160;++block)
        {
            if(block>=50)set(changing,"detectorWindowMs",float((block*7)%27));
            juce::AudioBuffer<float> audio(2,257);for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i)audio.setSample(ch,i,.2f);
            changing.processBlock(audio,midi);check(changing.getLatencySamples()==1248,"Automated WINDOW changed PDC");
            if(block>=50)for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i)
                check(std::abs(audio.getSample(ch,i)-(.2f/1.8f))<2e-6f,"WINDOW drag cleared steady detector history");
        }
        std::cout<<"PASS: 110 automated WINDOW changes retain steady output and PDC.\n";
    }
