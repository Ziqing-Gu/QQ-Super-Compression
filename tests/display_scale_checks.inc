    static void displayScaleChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;p.setEcoMode(false);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));
        e.stopTimer();e.display.stopTimer();e.setSize(1200,800);
        check(p.getDisplayScaleDb()==90,"Default Scale is -90 dB");
        int cases=0;
        for(bool limiter:{false,true})for(int mode:{0,1,2})for(int dual:{0,1})for(int algorithm:{0,1})
        {
            if(limiter)p.enterLimiterMode();else p.leaveLimiterMode();
            const auto id=[&](const char* value){return p.soundParameterID(value);};
            set(p,id("processingMode"),float(mode));set(p,id("inputGainDb"),0);
            set(p,id("keySource"),0);set(p,id("algorithmMode"),float(algorithm));
            set(p,id("upAlgorithmMode"),float(algorithm));set(p,id("downAlgorithmMode"),float(algorithm));
            p.setCompressionModeFromEditor(dual);sync(p,e);e.timerCallback();e.resized();
            const int domain=limiter ? 1 : mode==0 ? 0 : mode==1 ? 3 : 1;
            p.setBoundaryForDomainDb(bool(dual),false,domain,-75);
            p.setBoundaryForDomainDb(bool(dual),true,domain,dual ? -12.f : 1.f);sync(p,e);
            const auto stored=p.getBoundaryForDomainDb(bool(dual),false,domain);
            check(std::abs(stored+75.f)<.00002f,"Below -45 threshold supported");
            const auto gain=p.getDynamicsGainForDomain(.02f,domain);
            for(int scale:{30,60,90})
            {
                p.setDisplayScaleDb(scale);e.display.refreshScale();e.timerCallback();
                check(p.getBoundaryForDomainDb(bool(dual),false,domain)==stored,"Scale rewrote threshold");
                check(p.getDynamicsGainForDomain(.02f,domain)==gain,"Scale changed gain");
                auto plot=e.display.getBoundaryPlotForDomain(domain);
                for(float t:{0.f,.5f,1.f})
                {
                    const float db=-float(scale)*t,y=plot.getY()+plot.getHeight()*t;
                    check(std::abs(e.display.getBoundaryYForDomainDb(domain,db,false)-y)<.015f,"Scale forward map");
                    check(std::abs(e.display.getBoundaryDbForY(domain,y,false)-db)<.015f,"Scale inverse map");
                }
                check(e.lowerBoundaryValues[size_t(domain)].getText().contains("*")==(scale<75),"Out-of-view actual value marker");
                for(float input:{-12.f,12.f})
                {
                    set(p,id("inputGainDb"),input);
                    const float db=-scale*.5f+input;
                    const float y=e.display.getBoundaryYForDomainDb(domain,db,false);
                    check(std::abs(e.display.getBoundaryDbForY(domain,y,false)-db)<.015f,"Input-reference alignment");
                }
                set(p,id("inputGainDb"),0);
                for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
                {
                    e.theme=theme;e.applyTheme();e.resized();e.display.refreshScale();
                    check(e.display.getLocalBounds().contains(e.display.scaleButton.getBounds()),"Scale button bounds");
                }
                ++cases;
            }
            if(limiter)check(e.upperBoundarySliders[1]->isVisible()==bool(dual),"Limiter Range remains removed");
        }
        p.setDisplayScaleDb(60);e.display.refreshScale();e.display.scaleButton.onClick();
        check(p.getDisplayScaleDb()==90,"Button 60 -> 90");e.display.scaleButton.onClick();
        check(p.getDisplayScaleDb()==30,"Button 90 -> 30");e.display.scaleButton.onClick();
        check(p.getDisplayScaleDb()==60,"Button 30 -> 60");
        p.copyAToB();p.selectABSlot(1);check(p.getDisplayScaleDb()==60,"A/B must retain common Scale");
        p.leaveLimiterMode();check(p.getDisplayScaleDb()==60,"Leaving Limiter changed Scale");
        p.enterLimiterMode();check(p.getDisplayScaleDb()==60,"Entering Limiter changed Scale");
        juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));check(restored.getDisplayScaleDb()==60,"Project Scale restore");
        auto legacy=p.getAPVTS().copyState();auto xml=legacy.createXml();juce::MemoryBlock old;
        juce::AudioProcessor::copyXmlToBinary(*xml,old);
        restored.setStateInformation(old.getData(),int(old.getSize()));check(restored.getDisplayScaleDb()==90,"Legacy Scale defaults to 90");
        std::cout<<"PASS: "<<cases<<" Scale cases; both modes, Single/Dual, algorithms, themes, input reference, preserved thresholds, A/B/project.\n";

        // Real drag sensitivity follows the displayed span, including Shift.
        p.leaveLimiterMode();set(p,"processingMode",0);set(p,"compressionMode",0);set(p,"inputOutputLink",0);
        set(p,"algorithmMode",0);set(p,"rangeDb",1);set(p,"inputGainDb",0);sync(p,e);e.timerCallback();
        for(int scale:{30,60,90})for(bool fine:{false,true})
        {
            p.setDisplayScaleDb(scale);e.display.refreshScale();set(p,"thresholdDb",-15);sync(p,e);
            auto& slider=e.thresholdSlider;const auto plot=slider.boundaryPlotBounds();
            const float pixels=plot.getHeight()/float(scale)*(fine?8.f:1.f);
            auto start=juce::Point<float>(slider.getWidth()*.5f,slider.getHeight()*.5f);
            auto event=[&](juce::Point<float> pos)
            {
                auto now=juce::Time::getCurrentTime();return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),pos,
                    juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|(fine?juce::ModifierKeys::shiftModifier:0)),
                    1.f,0.f,0.f,0.f,0.f,&slider,&slider,now,start,now,1,false);
            };
            slider.mouseDown(event(start));slider.mouseDrag(event(start.translated(0,-pixels)));slider.mouseUp(event(start.translated(0,-pixels)));
            check(std::abs(slider.getValue()+14)<.025,"Drag and Shift scale sensitivity");
        }
        for(bool limiter:{false,true})for(int os:{0,1,2,3})
        {
            QQSuperCompressionAudioProcessor a,b;
            for(auto* x:{&a,&b})
            {
                x->setEcoMode(false);if(limiter)x->enterLimiterMode();
                auto value=[&](const char* id,float v){set(*x,x->soundParameterID(id),v);};
                value("oversampling",float(os));value("lookaheadMs",26);value("algorithmMode",0);
                value(limiter?"thresholdLDb":"thresholdDb",-60);value(limiter?"ratioL":"ratio",4);
                value(limiter?"makeupGainLDb":"makeupGainDb",6);value("inputGainDb",0);
                x->setRateAndBufferSizeDetails(48000,256);x->prepareToPlay(48000,256);
            }
            juce::MidiBuffer midi;float delta=0;
            for(int n=0;n<8192;n+=256)
            {
                a.setDisplayScaleDb(30*((n/256)%3+1));
                juce::AudioBuffer<float> x(2,256),y(2,256);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)x.setSample(ch,i,.3f*std::sin(float(n+i)*(.031f+ch*.019f)));
                y.makeCopyOf(x);a.processBlock(x,midi);b.processBlock(y,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)delta=std::max(delta,std::abs(x.getSample(ch,i)-y.getSample(ch,i)));
            }
            check(delta==0,"Scale changed real audio");
            std::cout<<"SCALE_AUDIO limiter="<<limiter<<" os="<<os<<" max_delta="<<delta<<"\n";
        }
        std::cout<<"PASS: real drag + Shift; 8 exact audio-invariance cases.\n";
    }
