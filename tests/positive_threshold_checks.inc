    static void positiveThresholdChecks(const juce::File& root)
    {
        auto near=[](float actual,float expected,const char* message)
        { if(std::abs(actual-expected)>.005f)std::cerr<<message<<": "<<actual<<" != "<<expected<<"\n";
          check(std::abs(actual-expected)<.005f,message); };
        QQSuperCompressionAudioProcessor p;
        for(const auto* ids:{&qqsc::params::normalSoundIds,&qqsc::params::limiterSoundIds,&qqsc::params::clipperSoundIds})
            for(size_t i=15;i<35;++i)
            {
                auto* param=p.getAPVTS().getParameter((*ids)[i]);
                const bool range=i>=20 && i<25;
                near(param->convertFrom0to1(param->getValueForText("24 dB")),24,"Positive boundary text is clamped");
                near(param->convertFrom0to1(param->convertTo0to1(6)),6,"Positive boundary host roundtrip");
                if(range)check(param->getText(param->getValueForText("OFF"),64)=="OFF","Range OFF text");
            }

        int controls=0,audioCases=0;
        for(int family:{0,1,2})for(bool dual:{false,true})
        {
            if(family==1 && dual)continue;
            p.setProcessorModeFromEditor(family);p.setCompressionModeFromEditor(dual?1:0);
            set(p,p.soundParameterID("limiterLink"),0);set(p,p.soundParameterID("domainLink"),0);
            for(int d=0;d<5;++d)
            {p.setBoundaryForDomainDb(dual,true,d,dual?18.f:qqsc::params::rangeOffDb);
             p.setBoundaryForDomainDb(dual,false,d,6);}
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            p.setDisplayAdjustDb(-24);e.display.refreshView();
            for(int d=0;d<5;++d)
            {
                p.setBoundaryForDomainDb(dual,false,d,6);sync(p,e);
                auto* slider=e.lowerBoundaryControls()[size_t(d)];
                near(float(slider->getMaximum()),24,"Positive slider maximum");
                near(p.getBoundaryForDomainDb(dual,false,d),6,"Positive lower boundary");
                auto& display=e.display;const auto plot=display.getBoundaryPlotForDomain(d);
                const float y=display.getBoundaryYForDomainDb(d,6);
                check(y>plot.getY()+1 && y<plot.getBottom()-1,"Positive boundary incorrectly pinned to plot edge");
                near(y,display.dbToY(6,plot),"Positive boundary / axis alignment");
                near(display.getBoundaryDbForY(d,y,false),6,"Positive boundary drag coordinate");
                auto& label=e.lowerBoundaryValues[size_t(d)];label.showEditor();
                check(label.getCurrentTextEditor()!=nullptr,"Boundary text editor");
                label.getCurrentTextEditor()->setText("+12 dB");label.hideEditor(false);sync(p,e);
                near(p.getBoundaryForDomainDb(dual,false,d),12,"Positive numeric edit");
                ++controls;
            }
            const int visible=family?1:0;
            auto dragDb=[&](auto& slider,float delta)
            {
                const auto start=slider.getLocalBounds().toFloat().getCentre();
                const float pixels=slider.boundaryPlotBounds().getHeight()/slider.boundaryDbSpan()*delta;
                auto ev=mouse(slider,start);slider.mouseDown(ev);slider.mouseDrag(ev.withNewPosition(start.translated(0,-pixels)));slider.mouseUp(ev.withNewPosition(start.translated(0,-pixels)));sync(p,e);
            };
            p.setBoundaryForDomainDb(dual,false,visible,6);sync(p,e);
            dragDb(*e.lowerBoundaryControls()[size_t(visible)],1);
            near(p.getBoundaryForDomainDb(dual,false,visible),7,"Positive fader up jumped to maximum");
            dragDb(*e.lowerBoundaryControls()[size_t(visible)],-2);
            near(p.getBoundaryForDomainDb(dual,false,visible),5,"Positive fader down jumped to zero");
            if(family==0 && !dual)
            {
                p.setBoundaryForDomainDb(false,true,0,qqsc::params::rangeOffDb);sync(p,e);
                dragDb(*e.upperBoundarySliders[0],-1);
                near(p.getBoundaryForDomainDb(false,true,0),23,"Range OFF must enter through +24");
                dragDb(*e.upperBoundarySliders[0],2);
                near(p.getBoundaryForDomainDb(false,true,0),qqsc::params::rangeOffDb,"Range top detent must remain OFF");
            }
            if(family==0 && !dual)
            {
                p.setBoundaryForDomainDb(false,true,0,24);
                p.setBoundaryForDomainDb(false,false,0,24);sync(p,e);
                near(p.getBoundaryForDomainDb(false,true,0),24,"Finite Range collision");
                near(p.getDynamicsGainForDomain(30,0),1,"Equal boundaries must remain bypassed");
            }
        }
        // State stores absolute dB: new +1 dB is finite, old +1 was OFF.
        for(int family:{0,1,2})
        {
            p.setProcessorModeFromEditor(family);
            for(int d=0;d<5;++d)
            {p.setBoundaryForDomainDb(false,false,d,-12);p.setBoundaryForDomainDb(false,true,d,1);
             p.setBoundaryForDomainDb(true,true,d,18);p.setBoundaryForDomainDb(true,false,d,6);}
        }
        p.copyAToB();juce::MemoryBlock state;p.getStateInformation(state);
        auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
        for(int slot:{0,1})for(int family:{0,1,2})
        {
            restored.selectABSlot(slot);restored.setProcessorModeFromEditor(family);
            for(int d=0;d<5;++d)
            {near(restored.getBoundaryForDomainDb(true,false,d),6,"Saved positive UP");
             near(restored.getBoundaryForDomainDb(true,true,d),18,"Saved positive DOWN");
             near(restored.getBoundaryForDomainDb(false,true,d),family?qqsc::params::rangeOffDb:1,"New finite +1 Range / OFF roundtrip");}
        }
        tree.setProperty("qqscStateSchemaVersion",32,nullptr);
        for(const auto* ids:{&qqsc::params::normalSoundIds,&qqsc::params::limiterSoundIds,&qqsc::params::clipperSoundIds})
            for(size_t i=20;i<25;++i)
            {
                const float oldValue=i==21?0.f:i==22?-12.f:1.f;
                tree.getChildWithProperty("id",(*ids)[i]).setProperty("value",oldValue,nullptr);
                for(const auto* slot:{"A_","B_"})tree.setProperty("qqscAB_"+juce::String(slot)+(*ids)[i],oldValue,nullptr);
            }
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),state);
        restored.setStateInformation(state.getData(),int(state.getSize()));
        for(int slot:{0,1})
        {restored.selectABSlot(slot);restored.setProcessorModeFromEditor(0);
         for(int d=0;d<5;++d)near(restored.getBoundaryForDomainDb(false,true,d),d==1?0.f:d==2?-12.f:qqsc::params::rangeOffDb,"Legacy Range migration changed OFF/finite");}
        auto amp=[](float db){return juce::Decibels::decibelsToGain(db);};
        auto db=[](float x){return juce::Decibels::gainToDecibels(x,-180.f);};
        for(int family:{0,1,2})for(int algorithm:{0,1})for(bool dual:{false,true})
        for(float look:{0.f,25.3f})for(bool transparent:{false,true})for(float inputDb:{0.f,12.f,24.f})
        {
            if(family==1 && dual)continue;
            QQSuperCompressionAudioProcessor q;q.setProcessorModeFromEditor(family);q.setEcoMode(false);
            auto put=[&](const char* id,float v){set(q,q.soundParameterID(id),v);};
            put("compressionMode",dual?1.f:0.f);put("algorithmMode",float(algorithm));put("upAlgorithmMode",float(algorithm));put("downAlgorithmMode",float(algorithm));
            put("lookaheadMs",look);put("distort",0);put("detectorMode",transparent?1.f:0.f);put("oversampling",0);
            put("keySource",0);put("keyHpfHz",0);put("outputGainDb",0);put("limiterStereoLink",0);put("processingMode",0);
            set(q,"ceilingEnabled",0);q.setUnityMonitorEnabled(false);
            for(int d=0;d<5;++d)
            {
                put(qqsc::params::ratioIds[size_t(d)],10);put(qqsc::params::upRatioIds[size_t(d)],1);put(qqsc::params::downRatioIds[size_t(d)],10);
                put(qqsc::params::downEnabledIds[size_t(d)],1);
                q.setBoundaryForDomainDb(false,true,d,qqsc::params::rangeOffDb);q.setBoundaryForDomainDb(false,false,d,6);
                q.setBoundaryForDomainDb(true,false,d,-90);q.setBoundaryForDomainDb(true,true,d,6);
            }
            for(const char* id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})put(id,0);
            for(const char* id:{"mix","mixL","mixR","mixM","mixS"})put(id,100);
            q.setRateAndBufferSizeDetails(48000,256);q.prepareToPlay(48000,256);
            juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,256);const float x=amp(inputDb),t=amp(6);
            for(int b=0;b<40;++b){for(int c=0;c<2;++c)for(int i=0;i<256;++i)audio.setSample(c,i,x);q.processBlock(audio,midi);}
            const float target=x<=t?x:algorithm?x*(1+9*t)/(1+9*x):amp(6+(inputDb-6)/10);
            near(db(audio.getSample(0,255)),db(target),"Positive threshold actual audio law");
            auto transfer=QQSCReviewCheck::currentTransfer(q);
            const auto matrix=transfer.evaluate({x,x,x,x,0},false,true);
            near(db((matrix[0]+matrix[1])*x),db(target),"Positive A/B transfer law");
            DynamicDisplay display(q);display.stopTimer();display.setSize(1000,500);
            DynamicDisplay::HistoryPoint point;point.inputDb=inputDb;point.detectorDb=inputDb;point.captureCounter=100;point.captureGeneration=1;
            display.pushHistory(display.histories[0],point);display.pushHistory(display.histories[1],point);
            display.refreshRenderCaches(family?2:0);
            near(display.renderCaches[0].projected.output[0],db(target),"Positive Display transfer law");
            ++audioCases;
        }
        // Finite positive Range and dual DOWN are valid upward anchors.
        for(int algorithm:{0,1})
        {
            p.setProcessorModeFromEditor(0);set(p,"algorithmMode",float(algorithm));set(p,"upAlgorithmMode",float(algorithm));
            set(p,"ratio",.125f);set(p,"upRatio",.125f);set(p,"upEnabled",1);
            const float x=amp(12),anchor=amp(24);
            const float gain=algorithm?1.f/(.125f+.875f*x/anchor):std::pow(anchor/x,.875f);
            p.setCompressionModeFromEditor(0);p.setBoundaryForDomainDb(false,true,0,24);p.setBoundaryForDomainDb(false,false,0,0);
            near(p.getDynamicsGainForDomain(x,0),gain,"Upward finite +24 Range anchor");
            near(p.getDynamicsGainForDomain(amp(30),0),1,"Finite Range must stop processing above +24");
            p.setBoundaryForDomainDb(false,true,0,qqsc::params::rangeOffDb);
            near(p.getDynamicsGainForDomain(x,0),1,"Range OFF retains existing 0 dB upward anchor");
            p.setCompressionModeFromEditor(1);p.setBoundaryForDomainDb(true,true,0,24);p.setBoundaryForDomainDb(true,false,0,0);
            near(p.getDynamicsGainForDomain(x,0),gain,"Dual upward +24 DOWN anchor");
        }
        int rangeAudioCases=0;
        for(int algorithm:{0,1})for(float ratio:{.125f,10.f})for(float inputDb:{-6.f,12.f,21.f,30.f})
        for(float look:{0.f,25.3f})for(bool transparent:{false,true})
        {
            QQSuperCompressionAudioProcessor q;q.setEcoMode(false);set(q,"processingMode",0);set(q,"compressionMode",0);
            set(q,"algorithmMode",float(algorithm));set(q,"lookaheadMs",look);set(q,"distort",0);set(q,"oversampling",0);
            set(q,"detectorMode",transparent?1.f:0.f);set(q,"keySource",0);set(q,"keyHpfHz",0);set(q,"outputGainDb",0);
            set(q,"makeupGainDb",0);set(q,"mix",100);set(q,"ratio",ratio);
            q.setBoundaryForDomainDb(false,true,0,24);q.setBoundaryForDomainDb(false,false,0,0);
            q.setRateAndBufferSizeDetails(48000,256);q.prepareToPlay(48000,256);
            juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;const float x=amp(inputDb),upper=amp(24);
            for(int b=0;b<40;++b){for(int c=0;c<2;++c)for(int i=0;i<256;++i)audio.setSample(c,i,x);q.processBlock(audio,midi);}
            float expectedGain=1;
            if(x>1 && x<upper)
            {
                if(ratio>1)
                {
                    const float raw=algorithm?(1+(ratio-1))/(1+(ratio-1)*x):std::pow(1/x,1-1/ratio);
                    const float t=std::clamp((upper-x)/((upper-1)*.5f),0.f,1.f),blend=t*t*(3-2*t);
                    expectedGain=1-blend+raw*blend;
                }
                else
                {
                    const float raw=algorithm?1/(ratio+(1-ratio)*x/upper):std::pow(upper/x,1-ratio);
                    const float t=std::clamp(x-1,0.f,1.f),blend=t*t*(3-2*t);expectedGain=1+(raw-1)*blend;
                }
            }
            near(db(audio.getSample(0,255)),db(x*expectedGain),"Actual finite positive Range transfer");
            ++rangeAudioCases;
        }
        std::cout<<"PASS: "<<rangeAudioCases<<" finite positive Range audio cases.\n";
        std::cout<<"PASS: positive boundaries, "<<controls<<" editor controls, legacy/new Range migration, "<<audioCases<<" actual audio/Display/A-B cases and positive upward anchors.\n";

    }
