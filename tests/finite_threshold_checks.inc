
    static void finiteThresholdChecks(const juce::File& root)
    {
        auto near=[](float actual,float expected,const char* message)
        {
            if(std::abs(actual-expected)>.002f)
                std::cerr<<message<<": actual="<<actual<<", expected="<<expected<<"\n";
            check(std::abs(actual-expected)<.002f,message);
        };
        QQSuperCompressionAudioProcessor p;
        for(const auto* ids:{&qqsc::params::normalSoundIds,&qqsc::params::limiterSoundIds,&qqsc::params::clipperSoundIds})
            for(size_t i=15;i<35;++i)
            {
                auto* parameter=p.getAPVTS().getParameter((*ids)[i]);
                const bool range=i>=20 && i<25;
                const float expected=range?qqsc::params::rangeOffDb:(i>=30 || (ids!=&qqsc::params::normalSoundIds && i<20))?0.f:-90.f;
                near(parameter->convertFrom0to1(parameter->getDefaultValue()),expected,"Factory boundary default");
                // Extended host range; state migration is tested separately.
                const float normalised=range ? .98f*.5f : .5f;
                near(parameter->convertFrom0to1(normalised),-48.f,"Extended host normalisation");
                parameter->setValueNotifyingHost(0.f);
                near(parameter->convertFrom0to1(parameter->getValue()),range?-90.f:-120.f,"Host endpoint / finite Range");
                check(parameter->getCurrentValueAsText().containsIgnoreCase("inf")==!range,"Host -inf text / finite Range");
            }
        for(int family:{0,1,2})
        {
            p.setProcessorModeFromEditor(family);
            set(p,p.soundParameterID("algorithmMode"),1);
            set(p,p.soundParameterID("upAlgorithmMode"),1);
            set(p,p.soundParameterID("downAlgorithmMode"),1);
            set(p,p.soundParameterID("limiterLink"),0);
            set(p,p.soundParameterID("domainLink"),0); // Test each independent reset; LINK has its own clamp contract.
            for(bool dual:{false,true})
            {
                if(family==1 && dual)continue;
                p.setCompressionModeFromEditor(dual?1:0);
                for(int d=0;d<5;++d)
                {
                    p.setBoundaryForDomainDb(dual,false,d,-120);
                    p.setBoundaryForDomainDb(dual,true,d,dual?0.f:qqsc::params::rangeOffDb);
                }
                QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
                std::cout<<"CHECK finite boundary family="<<family<<" dual="<<dual<<"\n";
                for(size_t d=0;d<5;++d)
                {
                    p.setBoundaryForDomainDb(dual,false,int(d),-120);sync(p,e);
                    auto* lower=e.lowerBoundaryControls()[d];
                    near(float(lower->getMinimum()),-120.f,"Slider -inf endpoint");
                    near(float(lower->getValue()),-120.f,"Slider endpoint");
                    check(e.lowerBoundaryValues[d].getText().containsIgnoreCase("inf"),"Readout missing -inf");
                    check(!e.lowerBoundaryValues[d].getText().containsIgnoreCase("off"),"Threshold OFF remains");
                    lower->setValue(-40,juce::sendNotificationSync);
                    const auto alt=mouse(*lower,lower->getLocalBounds().toFloat().getCentre(),true);
                    lower->mouseDown(alt);lower->mouseUp(alt);sync(p,e);
                    near(float(lower->getValue()),-90.f,"Alt threshold reset");
                }
                e.lowerBoundaryValues[family==0?0:1].showEditor();
                auto* entry=e.lowerBoundaryValues[family==0?0:1].getCurrentTextEditor();
                check(entry!=nullptr,"Boundary editor missing");
                entry->setText("-inf");e.lowerBoundaryValues[family==0?0:1].hideEditor(false);sync(p,e);
                near(p.getBoundaryForDomainDb(dual,false,family==0?0:1),-120.f,"Typed -inf restore");
                if(dual)e.upAlgorithmButton.onClick();else e.algorithmButton.onClick();
                sync(p,e);
                near(float(e.lowerBoundaryControls()[family==0?0:1]->getValue()),-120.f,"Algorithm switch lost -inf");
            }
        }
        // Inject genuine old project data, including both inactive A/B banks,
        // then recall every family and both slots.
        p.copyAToB();
        juce::MemoryBlock state;p.getStateInformation(state);
        auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);
        for(const auto* ids:{&qqsc::params::normalSoundIds,&qqsc::params::limiterSoundIds,&qqsc::params::clipperSoundIds})
            for(size_t i=15;i<35;++i)
            {
                tree.getChildWithProperty("id",(*ids)[i]).setProperty("value",-120.f,nullptr);
                for(const char* slot:{"A_","B_"})
                    tree.setProperty("qqscAB_"+juce::String(slot)+(*ids)[i],-120.f,nullptr);
            }
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),state);
        p.setStateInformation(state.getData(),int(state.getSize()));
        for(int slot:{0,1})
        {
            p.selectABSlot(slot);
            for(int family:{0,1,2})
            {
                p.setProcessorModeFromEditor(family);
                QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
                for(bool dual:{false,true})for(int d=0;d<5;++d)
                    near(p.getBoundaryForDomainDb(dual,false,d),-120.f,"Project/A-B -inf restore");
            }
        }
        p.getStateInformation(state);
        xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        tree=juce::ValueTree::fromXml(*xml);
        for(const auto* ids:{&qqsc::params::normalSoundIds,&qqsc::params::limiterSoundIds,&qqsc::params::clipperSoundIds})
            for(size_t i=15;i<35;++i)
            {
                check(float(tree.getChildWithProperty("id",(*ids)[i]).getProperty("value"))>=-120.f,"Saved parameter below endpoint");
                for(const char* slot:{"A_","B_"})
                    check(float(tree.getProperty("qqscAB_"+juce::String(slot)+(*ids)[i]))>=-120.f,"Saved A/B below endpoint");
            }
        // Validate the mathematical limiting law, not only endpoint text.
        p.setProcessorModeFromEditor(0);p.setCompressionModeFromEditor(0);
        p.setBoundaryForDomainDb(false,false,0,-120);p.setBoundaryForDomainDb(false,true,0,qqsc::params::rangeOffDb);
        for(int algorithm:{0,1})
        {
            set(p,"algorithmMode",float(algorithm));set(p,"ratio",8);
            near(p.getDynamicsGainForDomain(.1f,0),algorithm==0?0.f:1.f/1.7f,"Down -inf transfer law");
            set(p,"ratio",1);near(p.getDynamicsGainForDomain(.1f,0),1,"Unity at -inf");
            set(p,"ratio",.125f);
            const float expected=algorithm==0?std::pow(10.f,.875f):1.f/(.125f+.875f*.1f);
            near(p.getDynamicsGainForDomain(.1f,0),expected,"Up -inf retains selected law");
            check(std::isfinite(p.getDynamicsGainForDomain(0,0)),"Silence at -inf produced NaN/inf");
            set(p,"ratio",2);p.setBoundaryForDomainDb(false,false,0,-100);
            near(p.getDynamicsGainForDomain(.001f,0),algorithm==0?.1f:(1.f+1.e-5f)/1.001f,"Finite threshold below -90");
            p.setBoundaryForDomainDb(false,false,0,-120);
        }
        // Real processing, not just the transfer helper: sample-wise -inf law.
        int audioCases=0;
        for(int family:{0,1,2})for(int algorithm:{0,1})
        {
            QQSuperCompressionAudioProcessor q;q.setProcessorModeFromEditor(family);q.setEcoMode(false);
            auto put=[&](const char* id,float value){set(q,q.soundParameterID(id),value);};
            put("compressionMode",0);put("algorithmMode",float(algorithm));put("lookaheadMs",0);put("oversampling",0);
            put("keySource",0);put("keyHpfHz",0);put("outputGainDb",0);put("limiterStereoLink",0);
            set(q,"ceilingEnabled",0);q.setUnityMonitorEnabled(false);
            for(auto id:qqsc::params::ratioIds)put(id,10);
            for(int domain=0;domain<5;++domain)
            {q.setBoundaryForDomainDb(false,false,domain,-120);q.setBoundaryForDomainDb(false,true,domain,qqsc::params::rangeOffDb);}
            for(auto id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})put(id,0);
            q.setRateAndBufferSizeDetails(48000,128);q.prepareToPlay(48000,128);
            juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,128);float worst=0;
            for(int block=0;block<48;++block)
            {
                for(int i=0;i<128;++i){const float x=.2f*std::sin(float(block*128+i)*.13f);audio.setSample(0,i,x);audio.setSample(1,i,x);}
                q.processBlock(audio,midi);
                if(block>8)for(int i=0;i<128;++i)
                {const float x=.2f*std::sin(float(block*128+i)*.13f);const float expected=algorithm?x/(1+9*std::abs(x)):0;
                 worst=std::max(worst,std::abs(audio.getSample(0,i)-expected));}
            }
            if(worst>=.00001f)std::cerr<<"-inf audio family="<<family<<" algorithm="<<algorithm<<" error="<<worst<<"\n";
            check(worst<.00001f,"Actual -inf output differs from analytic transfer law");++audioCases;
        }
        std::cout<<"PASS: "<<audioCases<<" actual -inf audio transfer cases.\n";
        near(qqsc::params::rangeFromText("OFF"),qqsc::params::rangeOffDb,"Range OFF lost");
        near(qqsc::params::clampRangeDb(-120),-90,"Finite Range minimum changed");
        std::cout<<"PASS: -inf host/UI/input/algorithm/project/A-B and real transfer laws; original -90/0 defaults, finite Range and Alt reset retained.\n";
    }
