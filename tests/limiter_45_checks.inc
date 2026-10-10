
    static void limiter45Checks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;p.setEcoMode(false);
        set(p,"thresholdLDb",-72);set(p,"rangeLDb",-6);set(p,"upThresholdLDb",-100);
        p.enterLimiterMode();set(p,"limiterLink",0);set(p,"limiterKeySource",0);set(p,"limiterInputGainDb",0);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.setSize(1200,800);
        for(float algorithm:{0.f,1.f})for(int dual:{0,1})
        {
            set(p,"limiterAlgorithmMode",algorithm);set(p,"limiterUpAlgorithmMode",algorithm);set(p,"limiterDownAlgorithmMode",algorithm);
            p.setCompressionModeFromEditor(dual);
            for(int d=0;d<5;++d)
            {
                p.setBoundaryForDomainDb(bool(dual),false,d,-120);
                p.setBoundaryForDomainDb(bool(dual),true,d,dual ? -90.f : -6.f);
                check(p.getBoundaryForDomainDb(bool(dual),false,d)==-45,"Limiter lower clamped");
                check(p.getBoundaryForDomainDb(bool(dual),true,d)==(dual?-45.f:1.f),"Limiter upper clamped / Range inactive");
                p.setBoundaryForDomainDb(bool(dual),true,d,dual ? -10.f : 1.f);
            }
            sync(p,e);e.timerCallback();e.resized();
            check(p.boundaryMinimumDb(false)==-45 && p.boundaryMinimumDb(true)==-45,"Both algorithms floor");
            check(e.thresholdLSlider.getMinimum()==-45 && e.thresholdLSlider.getMaximum()==0,"Threshold UI range");
            check(e.upperBoundarySliders[1]->isVisible()==bool(dual),"Range control removed only in Single Limiter");
            check(e.upperBoundaryNames[1].isVisible()==bool(dual) && e.upperBoundaryValues[1].isVisible()==bool(dual),"Range labels removed");
            if(dual)check(e.upperBoundarySliders[1]->getMaximum()==0,"DOWN upper bound");
            auto plot=e.display.getBoundaryPlotForDomain(1);
            for(float db:{0.f,-22.5f,-45.f})
            {
                const float y=plot.getY()+plot.getHeight()*(-db/45.f);
                check(std::abs(e.display.getBoundaryYForDomainDb(1,db,false)-y)<.01f,"Display -45 scale");
                check(std::abs(e.display.getBoundaryDbForY(1,y,false)-db)<.01f,"Display drag inverse");
            }
            for(float input:{-12.f,12.f})
            {
                set(p,"limiterInputGainDb",input);
                const auto y=e.display.getBoundaryYForDomainDb(1,-22.5f,false);
                check(std::abs(e.display.getBoundaryDbForY(1,y,false)+22.5f)<.01f,"Input gain reference alignment");
            }
            set(p,"limiterInputGainDb",0);
        }
        p.setCompressionModeFromEditor(0);set(p,"limiterThresholdLDb",-12);set(p,"limiterRatioL",8);
        const float expected=p.getDynamicsGainForDomain(.8f,1);
        for(float range:{-120.f,-45.f,-6.f,0.f,1.f})
        {
            set(p,"limiterRangeLDb",range);
            check(p.getBoundaryForDomainDb(false,false,1)==-12,"Legacy Range cannot push threshold");
            check(p.readSoundParameter("rangeLDb")==1,"Range read neutral");
            check(p.getDynamicsGainForDomain(.8f,1)==expected,"Range automation cannot change gain");
        }
        p.copyAToB();set(p,"limiterThresholdLDb",-90);p.selectABSlot(1);
        check(p.getBoundaryForDomainDb(false,false,1)==-12,"B restores finite threshold");
        p.selectABSlot(0);check(p.getBoundaryForDomainDb(false,false,1)==-45,"A clamps old threshold");
        juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));
        check(restored.getBoundaryForDomainDb(false,false,1)==-45,"Project recall floor");
        check(restored.getBoundaryForDomainDb(false,true,1)==1,"Project Range inactive");
        set(p,"limiterLink",1);set(p,"limiterOutputDb",0);set(p,"limiterThresholdLDb",-40);sync(p,e);e.timerCallback();
        e.outputGainSlider.onGestureStart();e.outputGainSlider.setValue(20,juce::sendNotificationSync);e.outputGainSlider.onGestureEnd();sync(p,e);
        check(p.getBoundaryForDomainDb(false,false,1)>=-45,"Output LINK respects threshold floor");
        p.setCompressionModeFromEditor(1);set(p,"limiterUpThresholdLDb",-35);set(p,"limiterDownThresholdLDb",-12);sync(p,e);e.timerCallback();
        e.outputGainSlider.onGestureStart();e.outputGainSlider.setValue(10,juce::sendNotificationSync);e.outputGainSlider.onGestureEnd();sync(p,e);
        check(p.getBoundaryForDomainDb(true,false,1)==-35,"Output LINK leaves UP independent");
        p.leaveLimiterMode();set(p,"processingMode",2);set(p,"compressionMode",0);sync(p,e);e.timerCallback();
        check(get(p,"thresholdLDb")==-72 && get(p,"rangeLDb")==-6 && get(p,"upThresholdLDb")==-100,"Normal banks preserved");
        check(e.upperBoundarySliders[1]->isVisible(),"Normal Range remains");
        const auto plot=e.display.getBoundaryPlotForDomain(1);
        check(std::abs(e.display.getBoundaryDbForY(1,plot.getCentreY(),false)+45)<.01f,"Normal -90 Display retained");
        std::cout<<"PASS: -45..0 thresholds, both algorithms, Single/Dual, Display mapping, Range automation neutral, A/B/project, Output LINK and Normal preservation.\n";

        // Real processing comparison: legacy finite Range must sound exactly
        // like OFF, including oversampling and the active Ceiling stage.
        for(int os:{0,1,2,3})
        {
            QQSuperCompressionAudioProcessor a,b;
            for(auto* x:{&a,&b})
            {
                x->setEcoMode(false);x->enterLimiterMode();set(*x,"limiterCompressionMode",0);
                set(*x,"limiterOversampling",float(os));set(*x,"limiterLookaheadMs",26);
                set(*x,"limiterAlgorithmMode",0);set(*x,"limiterThresholdLDb",-18);set(*x,"limiterRatioL",8);
                set(*x,"limiterMakeupGainLDb",12);set(*x,"limiterMixL",100);set(*x,"limiterOutputDb",0);
                x->setRateAndBufferSizeDetails(48000,256);x->prepareToPlay(48000,256);
            }
            juce::MidiBuffer midi;float maxDelta=0;
            for(int n=0;n<8192;n+=256)
            {
                set(a,"limiterRangeLDb",n%512 ? -6.f : -90.f);set(b,"limiterRangeLDb",1);
                juce::AudioBuffer<float> x(2,256),y(2,256);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)
                    x.setSample(ch,i,.7f*std::sin(float(n+i)*(.031f+ch*.019f)));
                y.makeCopyOf(x);a.processBlock(x,midi);b.processBlock(y,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)
                    maxDelta=std::max(maxDelta,std::abs(x.getSample(ch,i)-y.getSample(ch,i)));
            }
            check(maxDelta==0,"Legacy Range changed real Limiter output");
            std::cout<<"RANGE_AUDIO os="<<os<<" max_delta="<<maxDelta<<"\n";
        }
    }
