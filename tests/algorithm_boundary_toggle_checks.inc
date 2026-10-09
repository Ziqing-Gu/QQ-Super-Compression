    static void algorithmBoundaryToggleChecks(const juce::File& root)
    {
        int cases=0;
        for(int limiter:{0,1})for(int mode:{0,1,2})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);
            if(limiter)p.enterLimiterMode();
            set(p,p.soundParameterID("compressionMode"),1);
            set(p,p.soundParameterID("processingMode"),float(mode));
            set(p,p.soundParameterID("upAlgorithmMode"),0);
            set(p,p.soundParameterID("downAlgorithmMode"),0);
            for(int d=0;d<5;++d)
            {
                p.setBoundaryForDomainDb(true,false,d,-120);
                p.setBoundaryForDomainDb(true,true,d,0);
            }
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            const auto verify=[&]
            {
                const bool classic=p.isClassicBoundary(false);
                const float expected=limiter||classic?-90.f:-120.f;
                for(int d=0;d<5;++d)
                {
                    const auto* slider=e.lowerBoundaryControls()[size_t(d)];
                    check(std::abs(slider->getMinimum()-expected)<.001,"UP slider floor changed with wrong algorithm/bank");
                    check(std::abs(slider->getValue()-expected)<.001,"UP endpoint display differs from selected floor");
                    check(std::abs(p.getBoundaryForDomainDb(true,false,d)-(limiter?-90.f:-120.f))<.001,"Algorithm click changed stored UP boundary");
                    check(e.lowerBoundaryValues[size_t(d)].getText().containsIgnoreCase("inf")==(!limiter&&!classic),"UP text inconsistent with floor");
                }
            };
            verify();
            for(int repeat=0;repeat<4;++repeat)
            {
                e.upAlgorithmButton.onClick();verify();
                e.downAlgorithmButton.onClick();verify();
                e.updateAlgorithmUi();verify();
            }
            if(limiter)
            {
                for(size_t d=0;d<5;++d)
                {
                    auto* param=p.getAPVTS().getParameter(qqsc::params::limiterUpThresholdIds[d]);
                    check(std::abs(param->convertTo0to1(-60.f)-.5f)<1.e-6f,"Existing host automation mapping changed");
                    set(p,qqsc::params::limiterUpThresholdIds[d],-120);
                    check(std::abs(p.getBoundaryForDomainDb(true,false,int(d))+90)<.001,"Host automation crossed Limiter UP floor");
                }
                p.copyAToB();p.setBoundaryForDomainDb(true,false,0,-45);p.selectABSlot(1);sync(p,e);verify();
                juce::MemoryBlock state;p.getStateInformation(state);
                auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
                check(xml!=nullptr,"Missing project state");auto tree=juce::ValueTree::fromXml(*xml);
                for(const auto* id:qqsc::params::limiterUpThresholdIds)
                    tree.getChildWithProperty("id",id).setProperty("value",-120.f,nullptr);
                juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),state);
                p.setStateInformation(state.getData(),int(state.getSize()));sync(p,e);verify();
                check(p.readSoundParameter("upThresholdDb")==-90.f,"Legacy -inf state was not migrated to -90 dB");
                // Below the finite UP gate, the real dynamics law must leave
                // the sample alone even in Super (not just display -90).
                set(p,"limiterUpAlgorithmMode",1);set(p,"limiterUpRatio",.5f);
                check(std::abs(p.getDynamicsGainForDomain(1.e-5f,0)-1.f)<1.e-6f,"Super still boosts below the -90 dB UP gate");
                e.updateAlgorithmUi();verify();
            }
            ++cases;
        }
        std::cout<<"PASS: "<<cases<<" bank/domain cases; independent algorithm toggles, finite Limiter UP floor, Normal -inf preserved, legacy state, A/B, host automation mapping and actual gain below gate.\n";
    }
