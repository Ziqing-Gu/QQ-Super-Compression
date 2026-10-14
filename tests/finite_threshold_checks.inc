
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
                const float expected=range?1.f:(i>=30 || (ids!=&qqsc::params::normalSoundIds && i<20))?0.f:-90.f;
                near(parameter->convertFrom0to1(parameter->getDefaultValue()),expected,"Factory boundary default");
                // Preserve every legal old automation point; only the removed
                // region below -90 saturates at the new finite floor.
                const float normalised=range ? .98f*.5f : .5f;
                near(parameter->convertFrom0to1(normalised),-60.f,"Legacy automation mapping");
                parameter->setValueNotifyingHost(0.f);
                near(parameter->convertFrom0to1(parameter->getValue()),-90.f,"Host endpoint must be finite");
                check(!parameter->getCurrentValueAsText().containsIgnoreCase("inf"),"Host text still offers -inf");
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
                    p.setBoundaryForDomainDb(dual,true,d,dual?0.f:1.f);
                }
                QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
                std::cout<<"CHECK finite boundary family="<<family<<" dual="<<dual<<"\n";
                for(size_t d=0;d<5;++d)
                {
                    auto* lower=e.lowerBoundaryControls()[d];
                    near(float(lower->getMinimum()),-90.f,"Slider floor");
                    near(float(lower->getValue()),-90.f,"Slider endpoint");
                    check(!e.lowerBoundaryValues[d].getText().containsIgnoreCase("inf"),"Readout -inf");
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
                near(p.getBoundaryForDomainDb(dual,false,family==0?0:1),-90.f,"Legacy typed -inf clamp");
                if(dual)e.upAlgorithmButton.onClick();else e.algorithmButton.onClick();
                sync(p,e);
                near(float(e.lowerBoundaryControls()[0]->getValue()),-90.f,"Algorithm switch restored -inf");
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
                    near(p.getBoundaryForDomainDb(dual,false,d),-90.f,"Project/A-B lower restore");
            }
        }
        p.getStateInformation(state);
        xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        tree=juce::ValueTree::fromXml(*xml);
        for(const auto* ids:{&qqsc::params::normalSoundIds,&qqsc::params::limiterSoundIds,&qqsc::params::clipperSoundIds})
            for(size_t i=15;i<35;++i)
            {
                check(float(tree.getChildWithProperty("id",(*ids)[i]).getProperty("value"))>=-90.f,"Saved parameter below floor");
                for(const char* slot:{"A_","B_"})
                    check(float(tree.getProperty("qqscAB_"+juce::String(slot)+(*ids)[i]))>=-90.f,"Saved A/B below floor");
            }
        // The processor must use the finite gate, not merely display -90.
        p.setProcessorModeFromEditor(0);p.setCompressionModeFromEditor(0);
        set(p,"algorithmMode",1);set(p,"ratio",8);
        p.setBoundaryForDomainDb(false,false,0,-120);p.setBoundaryForDomainDb(false,true,0,1);
        near(p.getDynamicsGainForDomain(1.e-5f,0),1.f,"Super below -90 must bypass dynamics");
        set(p,"ratio",.125f);
        near(p.getDynamicsGainForDomain(1.e-5f,0),1.f,"Super upward below -90 must bypass dynamics");
        near(qqsc::params::rangeFromText("OFF"),1.f,"Range OFF lost");
        std::cout<<"PASS: finite -90 defaults/host automation/UI/Alt/input/algorithm/project/A-B/audio gate; Range OFF preserved.\n";
    }
