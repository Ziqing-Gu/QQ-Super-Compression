    // Full JUCE / actual-editor integration tests. Run:
    // QQSCLimiterCheck <output-directory> continuity
    static void limiterModeContinuityChecks(const juce::File& root)
    {
        const auto close=[](float a,float b){return std::abs(a-b)<0.005f;};
        const auto captureNormal=[](auto& p)
        {
            std::vector<float> values;
            for(int bank=0;bank<2;++bank)for(int d=0;d<5;++d)for(bool upper:{false,true})
                values.push_back(p.getBoundaryForBankDb(bank,upper,d));
            return values;
        };
        const auto captureUnchanged=[](auto& p)
        {
            std::vector<float> values;
            // All Limiter settings except the Single/Dual selector itself.
            for(size_t i=0;i<qqsc::params::limiterModeIds.size();++i)
                if(i!=12) values.push_back(get(p,qqsc::params::limiterModeIds[i]));
            for(size_t i=0;i<15;++i) values.push_back(get(p,qqsc::params::limiterSoundIds[i]));
            for(const auto* id:{"limiterOutputDb","limiterCalibrationDb","ceilingDb","limiterLink","outputGainDb"})
                values.push_back(get(p,id));
            return values;
        };
        const auto seed=[&](auto& p,int algorithm,int channelMode,int link,float mix)
        {
            baseline(p);
            set(p,"compressionMode",0);set(p,"algorithmMode",float(algorithm));
            for(int d=0;d<5;++d)
            {
                p.setBoundaryForDomainDb(false,true,d,1);
                p.setBoundaryForDomainDb(false,false,d,-45.f-float(d));
                p.setBoundaryForDomainDb(true,false,d,-75.f-float(d));
                p.setBoundaryForDomainDb(true,true,d,-8.f-float(d));
            }
            p.enterLimiterMode();set(p,"limiterCompressionMode",0);
            set(p,"limiterProcessingMode",float(channelMode));
            set(p,"limiterAlgorithmMode",float(algorithm));
            set(p,"limiterUpAlgorithmMode",float(algorithm));
            set(p,"limiterDownAlgorithmMode",float(algorithm));
            set(p,"limiterLink",float(link));
            set(p,"limiterDomainLink",float(link));set(p,"limiterDualRatioLink",float(1-link));
            set(p,"limiterOutputDb",-10);set(p,"limiterCalibrationDb",1.25f);set(p,"ceilingDb",-.9f);
            for(int d=0;d<5;++d)
            {
                set(p,qqsc::params::limiterRatioIds[size_t(d)],400);
                set(p,qqsc::params::limiterDownRatioIds[size_t(d)],600);
                set(p,qqsc::params::limiterUpRatioIds[size_t(d)],.5f);
                p.setBoundaryForDomainDb(false,true,d,1);
                p.setBoundaryForDomainDb(false,false,d,-12.f-float(d));
                p.setBoundaryForDomainDb(true,false,d,-70.f-float(d));
                p.setBoundaryForDomainDb(true,true,d,0);
                set(p,qqsc::params::limiterModeIds[size_t(1+d)],2.5f+float(d));
                set(p,qqsc::params::limiterModeIds[size_t(6+d)],mix);
            }
        };
        const auto requireThreshold=[&](auto& p,bool dual,float first,const char* message)
        {
            for(int d=0;d<5;++d)
                check(close(p.getBoundaryForDomainDb(dual,dual,d),first-float(d)),message);
        };
        int headlessCases=0,editorCases=0;
        for(int algorithm:{0,1})for(int channelMode:{0,1,2})for(int link:{0,1})for(float mix:{0.f,37.f,100.f})
        {
            QQSuperCompressionAudioProcessor p;seed(p,algorithm,channelMode,link,mix);
            const auto normal=captureNormal(p),unchanged=captureUnchanged(p);
            // Real APVTS listener path, without creating an editor or waiting
            // for a timer: the next DSP read must already see the carried value.
            set(p,"limiterCompressionMode",1);
            requireThreshold(p,true,-12,"Headless Single->Dual lost current threshold");
            check(p.readSoundParameter("compressionMode")==1,"Headless audio mode publication");
            check(captureUnchanged(p)==unchanged,"Headless switch changed gain/Mix/ratio/algorithm/Link");
            // Save immediately, while the raw companion parameter may not yet
            // have been published by the processor's message-thread timer.
            juce::MemoryBlock saved;p.getStateInformation(saved);
            QQSuperCompressionAudioProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
            requireThreshold(restored,true,-12,"Immediate save/load lost canonical threshold");
            set(restored,"limiterCompressionMode",0);
            requireThreshold(restored,false,-12,"Post-reload Dual->Single mismatch");
            for(int d=0;d<5;++d)p.setBoundaryForDomainDb(true,true,d,-32.f-float(d));
            set(p,"limiterCompressionMode",0);
            requireThreshold(p,false,-32,"Headless Dual->Single recalled old threshold");
            check(captureUnchanged(p)==unchanged && captureNormal(p)==normal,"Headless switch leaked to unrelated settings");
            ++headlessCases;

            QQSuperCompressionAudioProcessor u;seed(u,algorithm,channelMode,link,mix);
            QQSuperCompressionAudioProcessorEditor editor(u,settings(root));sync(u,editor);
            const auto uiNormal=captureNormal(u),uiUnchanged=captureUnchanged(u);
            const auto singleBefore=u.getBoundaryForDomainDb(false,false,0);
            const auto dualBefore=u.getBoundaryForDomainDb(true,true,0);
            u.getUndoManager().clearUndoHistory();(void)u.getAPVTS().copyState();
            editor.compressionModeButton.onClick();sync(u,editor);
            requireThreshold(u,true,-12,"Editor Single->Dual threshold mismatch");
            for(int d=0;d<5;++d)
            {
                check(close(float(editor.upperBoundarySliders[size_t(d)]->getValue()),-12.f-float(d)),"Editor rebound to stale DOWN parameter");
                check(close(get(u,qqsc::params::limiterDownThresholdIds[size_t(d)]),-12.f-float(d)),"Editor did not flush raw destination threshold");
            }
            check(captureUnchanged(u)==uiUnchanged,"Editor switch recalculated an unrelated setting");
            u.getUndoManager().beginNewTransaction();
            check(u.getUndoManager().undo(),"Mode switch Undo missing");sync(u,editor);
            check(u.readSoundParameter("compressionMode")==0,"Mode switch Undo selected wrong mode");
            check(close(u.getBoundaryForDomainDb(false,false,0),singleBefore),"Undo changed source threshold");
            check(close(u.getBoundaryForDomainDb(true,true,0),dualBefore),"Undo failed to restore destination bank");
            check(captureUnchanged(u)==uiUnchanged,"Undo changed gains");
            check(u.getUndoManager().redo(),"Mode switch Redo missing");sync(u,editor);
            requireThreshold(u,true,-12,"Mode switch Redo lost threshold");
            for(int d=0;d<5;++d)u.setBoundaryForDomainDb(true,true,d,-32.f-float(d));
            sync(u,editor);editor.compressionModeButton.onClick();sync(u,editor);
            requireThreshold(u,false,-32,"Editor Dual->Single recalled old threshold");
            for(int i=0;i<12;++i){editor.compressionModeButton.onClick();sync(u,editor);}
            requireThreshold(u,false,-32,"Editor round-trip threshold drift");
            check(captureUnchanged(u)==uiUnchanged && captureNormal(u)==uiNormal,"Editor switch changed unrelated bank/gain");

            // Normal-mode independent memories must survive all those Limiter
            // switches, and keep their original per-Single/Dual values.
            editor.limiterButton.onClick();sync(u,editor);
            check(!u.isLimiterMode(),"Failed to return to Normal mode");
            for(int i=0;i<12;++i){editor.compressionModeButton.onClick();sync(u,editor);}
            check(captureNormal(u)==uiNormal,"Normal Single/Dual memories were merged");
            editor.limiterButton.onClick();sync(u,editor);
            requireThreshold(u,false,-32,"Limiter re-entry reseeded current threshold");
            check(captureUnchanged(u)==uiUnchanged,"Limiter re-entry lost shared gains");
            ++editorCases;
        }
        std::cout<<"PASS: "<<headlessCases<<" actual-APVTS headless cases and "<<editorCases
                 <<" real-editor cases; all domains, both algorithms, LINK off/on, Mix 0/37/100, both directions, Undo/Redo, immediate save/load and Normal independent memories.\n";

        // A/B remains a separate sound comparison. Do not mistake a whole-bank
        // restore for a request to carry a threshold out of the previous slot.
        QQSuperCompressionAudioProcessor ab;seed(ab,0,0,0,37);
        ab.copyAToB();ab.selectABSlot(1);
        ab.setCompressionModeFromEditor(1);
        for(int d=0;d<5;++d)ab.setBoundaryForDomainDb(true,true,d,-35.f-float(d));
        ab.selectABSlot(0);requireThreshold(ab,false,-12,"A/B restore copied threshold from B into A");
        ab.selectABSlot(1);requireThreshold(ab,true,-35,"A/B restore copied threshold from A into B");
        ab.setCompressionModeFromEditor(0);requireThreshold(ab,false,-35,"Post-A/B mode switch did not carry B threshold");

        // Collision behavior stays the pre-existing push/equality rule. Range
        // OFF must survive; no extra ceiling, gate or ratio policy is introduced.
        QQSuperCompressionAudioProcessor collisions;seed(collisions,1,0,1,100);
        collisions.setBoundaryForDomainDb(true,false,0,-5);
        collisions.setCompressionModeFromEditor(1);
        check(close(collisions.getBoundaryForDomainDb(true,false,0),-12)
           && close(collisions.getBoundaryForDomainDb(true,true,0),-12),"Dual gate collision did not preserve carried DOWN");
        collisions.setBoundaryForDomainDb(false,true,0,-8);
        collisions.setBoundaryForDomainDb(true,true,0,-2);
        collisions.setCompressionModeFromEditor(0);
        check(close(collisions.getBoundaryForDomainDb(false,false,0),-2)
           && close(collisions.getBoundaryForDomainDb(false,true,0),-2),"Single Range collision did not preserve carried Threshold");
        std::cout<<"PASS: A/B slot isolation, subsequent mode continuity and existing boundary collision rules.\n";
    }
