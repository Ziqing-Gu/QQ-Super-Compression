    static void limiterUnityAndBidirectionalLinkChecks(const juce::File& root)
    {
        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
        auto near=[](double a,double b,double eps=.02){return std::abs(a-b)<eps;};
        auto drag=[&](Knob& slider,double value)
        {
            const auto ev=mouse(slider,slider.getLocalBounds().toFloat().getCentre());
            slider.mouseDown(ev); slider.setValue(value,juce::sendNotificationSync); slider.mouseUp(ev);
        };
        auto boundary=[&](QQSuperCompressionAudioProcessorEditor& e,int domain,bool upper,double value)
        {
            auto* slider=upper ? e.upperBoundarySliders[size_t(domain)].get() : e.lowerBoundaryControls()[size_t(domain)];
            e.beginBoundaryGesture(domain,upper); slider->setValue(value,juce::sendNotificationSync); e.endLinkedGesture();
        };

        // SINGLE upward: Threshold <-> Output must be the same strict inverse 1:1
        // relationship already used by downward compression.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",0);set(p,"limiterProcessingMode",0);set(p,"limiterLink",1);
            set(p,"limiterRatio",.5f);set(p,"limiterOutputDb",5.0f);p.setBoundaryForDomainDb(false,false,0,-30);p.setBoundaryForDomainDb(false,true,0,0);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            boundary(e,0,false,-27.0); check(near(get(p,"limiterOutputDb"),2.0),"Single UP Threshold did not move Output inverse 1:1");
            const auto beforeRange=p.getBoundaryForDomainDb(false,true,0);
            drag(e.outputGainSlider,4.0); check(near(p.getBoundaryForDomainDb(false,false,0),-29.0),"Single UP Output did not move Threshold inverse 1:1");
            check(near(p.getBoundaryForDomainDb(false,true,0),beforeRange,.001),"Single UP Output moved Range");
        }

        // SINGLE downward keeps the same contract, now with ordinary ratios from
        // unity upward rather than a forced 200:1 floor.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",0);set(p,"limiterProcessingMode",0);set(p,"limiterLink",1);
            set(p,"limiterRatio",4.0f);set(p,"limiterOutputDb",5.0f);p.setBoundaryForDomainDb(false,false,0,-18);p.setBoundaryForDomainDb(false,true,0,0);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            boundary(e,0,false,-21.0); check(near(get(p,"limiterOutputDb"),8.0),"Single DOWN Threshold did not move Output inverse 1:1");
            drag(e.outputGainSlider,10.0); check(near(p.getBoundaryForDomainDb(false,false,0),-23.0),"Single DOWN Output did not move Threshold inverse 1:1");
        }

        // At unity there is no compression stage to preserve, so LINK must not
        // fabricate Threshold movement or lock the Output control.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",0);set(p,"limiterProcessingMode",0);set(p,"limiterLink",1);
            set(p,"limiterRatio",1.0f);set(p,"limiterOutputDb",3.0f);p.setBoundaryForDomainDb(false,false,0,-18);p.setBoundaryForDomainDb(false,true,0,0);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            drag(e.outputGainSlider,7.0);check(near(get(p,"limiterOutputDb"),7.0,.001),"Unity Single Ratio locked Output");
            check(near(p.getBoundaryForDomainDb(false,false,0),-18.0,.001),"Unity Single Ratio fabricated Threshold movement");
        }

        // DUAL UP-only and DOWN-only: each active non-unity branch independently
        // participates in the same Threshold/Makeup/Output LINK.
        for(bool upward:{true,false})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",1);set(p,"limiterProcessingMode",0);set(p,"limiterLink",1);
            set(p,"limiterUpEnabled",1);set(p,"limiterDownEnabled",1);
            set(p,"limiterUpRatio",upward ? .5f : 1.0f);set(p,"limiterDownRatio",upward ? 1.0f : 4.0f);
            set(p,"limiterOutputDb",0);p.setBoundaryForDomainDb(true,false,0,-50);p.setBoundaryForDomainDb(true,true,0,-10);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            if(upward)
            {
                boundary(e,0,false,-47.0);check(near(get(p,"limiterOutputDb"),-3.0),"Dual UP Threshold did not move Output inverse 1:1");
                const auto upper=p.getBoundaryForDomainDb(true,true,0);drag(e.outputGainSlider,-1.0);
                check(near(p.getBoundaryForDomainDb(true,false,0),-49.0),"Dual UP Output did not move UP Threshold inverse 1:1");
                check(near(p.getBoundaryForDomainDb(true,true,0),upper,.001),"Dual UP-only Output moved unity DOWN Threshold");
            }
            else
            {
                boundary(e,0,true,-13.0);check(near(get(p,"limiterOutputDb"),3.0),"Dual DOWN Threshold did not move Output inverse 1:1");
                const auto lower=p.getBoundaryForDomainDb(true,false,0);drag(e.outputGainSlider,5.0);
                check(near(p.getBoundaryForDomainDb(true,true,0),-15.0),"Dual DOWN Output did not move DOWN Threshold inverse 1:1");
                check(near(p.getBoundaryForDomainDb(true,false,0),lower,.001),"Dual DOWN-only Output moved unity UP Threshold");
            }
        }

        // DUAL both branches active: Output moves both thresholds together, keeping
        // the window width unchanged. Makeup remains an inverse 1:1 Output source.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",1);set(p,"limiterProcessingMode",0);set(p,"limiterLink",1);
            set(p,"limiterUpEnabled",1);set(p,"limiterDownEnabled",1);set(p,"limiterUpRatio",.5f);set(p,"limiterDownRatio",4.0f);
            set(p,"limiterOutputDb",0);p.setBoundaryForDomainDb(true,false,0,-50);p.setBoundaryForDomainDb(true,true,0,-10);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            drag(e.outputGainSlider,3.0);
            check(near(p.getBoundaryForDomainDb(true,false,0),-53.0),"Dual both-active Output did not move UP Threshold");
            check(near(p.getBoundaryForDomainDb(true,true,0),-13.0),"Dual both-active Output did not move DOWN Threshold");
            check(near((p.getBoundaryForDomainDb(true,true,0)-p.getBoundaryForDomainDb(true,false,0)),40.0,.001),"Dual both-active Output changed threshold width");
            const auto out=get(p,"limiterOutputDb");drag(e.makeupSTSlider,e.makeupSTSlider.getValue()+2.0);
            check(near(get(p,"limiterOutputDb"),out-2.0),"Limiter Makeup did not move Output inverse 1:1");
        }

        // LINK OFF disables the bidirectional threshold relationship as before.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",0);set(p,"limiterProcessingMode",0);set(p,"limiterLink",0);
            set(p,"limiterRatio",.5f);set(p,"limiterOutputDb",2);p.setBoundaryForDomainDb(false,false,0,-24);p.setBoundaryForDomainDb(false,true,0,0);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            boundary(e,0,false,-20);check(near(get(p,"limiterOutputDb"),2.0,.001),"LINK OFF allowed UP Threshold to move Output");
        }

        std::cout<<"PASS: 1.2.25 Limiter unity-default ratio model and bidirectional UP/DOWN Threshold + Makeup + Output strict 1:1 LINK.\n";
    }
