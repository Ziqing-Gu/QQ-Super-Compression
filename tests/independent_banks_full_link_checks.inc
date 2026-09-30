    // Real JUCE/APVTS/Editor regression; NOT run by the portable mock harness.
    // QQSCLimiterCheck <output-directory> revision129
    static void independentBanksFullLinkChecks(const juce::File& root)
    {
        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
        const auto near=[](double a,double b,double tolerance=.012){return std::abs(a-b)<=tolerance;};
        const auto drag=[&](Knob& slider,double value)
        {
            const auto ev=mouse(slider,slider.getLocalBounds().toFloat().getCentre());
            slider.mouseDown(ev);slider.setValue(value,juce::sendNotificationSync);slider.mouseUp(ev);
        };
        const auto type=[&](Knob& slider,const juce::String& text)
        {
            slider.showTextBox();juce::TextEditor* entry=nullptr;
            for(auto* child:slider.getChildren())
                if(auto* label=dynamic_cast<juce::Label*>(child))
                    if(label->getCurrentTextEditor()!=nullptr)entry=label->getCurrentTextEditor();
            check(entry!=nullptr,"1.2.9 numeric editor missing");
            entry->setText(text);slider.hideTextBox(false);
        };
        const auto normalBank=[](auto& p)
        {
            std::vector<float> v;
            for(const auto* id:qqsc::params::normalSoundIds)v.push_back(get(p,id));
            for(const auto* id:qqsc::params::normalModeIds)v.push_back(get(p,id));
            v.push_back(get(p,"outputGainDb"));return v;
        };
        const auto limiterBank=[](auto& p)
        {
            std::vector<float> v;
            for(const auto* id:qqsc::params::limiterSoundIds)v.push_back(get(p,id));
            for(const auto* id:qqsc::params::limiterModeIds)v.push_back(get(p,id));
            for(const auto* id:{"limiterOutputDb","limiterCalibrationDb","ceilingDb"})v.push_back(get(p,id));
            return v;
        };
        // Fresh bank, repeated visits, first-entry Undo/Redo, independent saved A/B.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);
            set(p,"makeupGainDb",27);set(p,"mix",31);set(p,"outputGainDb",17);
            set(p,"processingMode",2);set(p,"compressionMode",1);set(p,"algorithmMode",1);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            const auto normal=normalBank(p),factory=limiterBank(p);
            (void)p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
            e.limiterButton.onClick();sync(p,e);
            check(p.isLimiterMode()&&limiterBank(p)==factory,"First Limiter entry copied Normal");
            check(normalBank(p)==normal,"First Limiter entry changed Normal");
            check(near(get(p,"limiterMakeupGainDb"),0)&&near(get(p,"limiterMix"),100)
                &&near(get(p,"limiterOutputDb"),0)&&near(p.getBoundaryForDomainDb(false,false,0),0),"Limiter independent defaults");
            p.getUndoManager().beginNewTransaction();check(p.getUndoManager().undo(),"First-entry Undo missing");sync(p,e);
            check(!p.isLimiterMode()&&normalBank(p)==normal,"First-entry Undo changed Normal");
            check(p.getUndoManager().redo(),"First-entry Redo missing");sync(p,e);
            check(p.isLimiterMode()&&limiterBank(p)==factory,"First-entry Redo cloned settings");
            set(p,"limiterLink",0);sync(p,e);drag(e.makeupSTSlider,84);drag(e.mixSlider,64);set(p,"limiterOutputDb",-19);
            const auto configured=limiterBank(p);
            for(int i=0;i<4;++i){e.limiterButton.onClick();sync(p,e);check(normalBank(p)==normal,"Normal memory lost");e.limiterButton.onClick();sync(p,e);check(limiterBank(p)==configured,"Limiter memory reset");}
            p.copyAToB();p.selectABSlot(1);p.leaveLimiterMode();set(p,"makeupGainDb",-12);set(p,"mix",88);
            p.enterLimiterMode();set(p,"limiterMakeupGainDb",-96);set(p,"limiterMix",18);set(p,"limiterOutputDb",7);
            juce::MemoryBlock saved;p.getStateInformation(saved);
            QQSuperCompressionAudioProcessor q;q.setStateInformation(saved.getData(),int(saved.getSize()));
            q.selectABSlot(0);check(near(get(q,"limiterMakeupGainDb"),84)&&near(get(q,"limiterMix"),64),"Saved A Limiter bank lost");
            q.leaveLimiterMode();check(near(get(q,"makeupGainDb"),27)&&near(get(q,"mix"),31),"Saved A Normal bank lost");
            q.selectABSlot(1);q.enterLimiterMode();check(near(get(q,"limiterMakeupGainDb"),-96)&&near(get(q,"limiterMix"),18),"Saved B Limiter bank lost");
            q.leaveLimiterMode();check(near(get(q,"makeupGainDb"),-12)&&near(get(q,"mix"),88),"Saved B Normal bank lost");
        }
        // All five actual host ranges, current-bank typed entry and readout.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);QQSuperCompressionAudioProcessorEditor e(p,settings(root));
            const std::array<Knob*,5> knobs{&e.makeupSTSlider,&e.makeupLSlider,&e.makeupRSlider,&e.makeupMSlider,&e.makeupSSlider};
            for(bool limiter:{false,true})
            {
                if(limiter)p.enterLimiterMode();else p.leaveLimiterMode();
                set(p,p.soundParameterID("domainLink"),0);set(p,"limiterLink",0);
                for(size_t d=0;d<5;++d)
                {
                    set(p,p.soundParameterID("processingMode"),d==0?0.f:d<3?2.f:1.f);sync(p,e);
                    auto* parameter=p.getAPVTS().getParameter(p.soundParameterID(qqsc::params::normalModeIds[1+d]));
                    const float limit=limiter?120.f:30.f;
                    check(near(parameter->convertFrom0to1(0),-limit)&&near(parameter->convertFrom0to1(1),limit),"Wrong host Makeup endpoints");
                    check(near(knobs[d]->getMinimum(),-limit)&&near(knobs[d]->getMaximum(),limit),"Wrong bound slider range");
                    type(*knobs[d],"110");check(near(parameter->convertFrom0to1(parameter->getValue()),limiter?110:30),"Positive Makeup typing clipped in wrong bank");
                    check(near(knobs[d]->getTextFromValue(knobs[d]->getValue()).getDoubleValue(),limiter?110:30),"Makeup text formatter captured old bank");
                    type(*knobs[d],"-110");check(near(parameter->convertFrom0to1(parameter->getValue()),limiter?-110:-30),"Negative Makeup typing clipped in wrong bank");
                }
            }
        }
        // 1.2.8 project physical dB, including inactive A/B, survives inside
        // the new range and clamps outside it. Limiter +/-120 is not rescaled.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();p.copyAToB();
            juce::MemoryBlock saved;p.getStateInformation(saved);
            auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));auto state=juce::ValueTree::fromXml(*xml);
            state.setProperty("qqscStateSchemaVersion",22,nullptr);state.setProperty("qqscLimiterBankInitialised",true,nullptr);
            for(size_t d=0;d<5;++d)
            {
                state.getChildWithProperty("id",qqsc::params::normalModeIds[1+d]).setProperty("value",d%2?-75.f:26.f,nullptr);
                state.getChildWithProperty("id",qqsc::params::limiterModeIds[1+d]).setProperty("value",d%2?-110.f:110.f,nullptr);
            }
            for(const auto* slot:{"A_","B_"})
            {
                for(const auto* key:{"makeupST","makeupL","makeupR","makeupM","makeupS"})
                    state.setProperty("qqscAB_"+juce::String(slot)+key,-75.f,nullptr);
                for(size_t d=0;d<5;++d)state.setProperty("qqscAB_"+juce::String(slot)+qqsc::params::limiterModeIds[1+d],110.f,nullptr);
            }
            juce::AudioProcessor::copyXmlToBinary(*state.createXml(),saved);
            QQSuperCompressionAudioProcessor q;q.setStateInformation(saved.getData(),int(saved.getSize()));
            for(size_t d=0;d<5;++d){check(near(get(q,qqsc::params::normalModeIds[1+d]),d%2?-30:26),"Normal saved dB migration");check(near(get(q,qqsc::params::limiterModeIds[1+d]),d%2?-110:110),"Limiter saved dB changed");}
            q.selectABSlot(1);check(near(get(q,"makeupGainDb"),-30)&&near(get(q,"limiterMakeupGainDb"),110),"Inactive A/B range migration");
            // Only a never-used pre-1.2.9 placeholder is replaced by 0 dB.
            state.setProperty("qqscLimiterBankInitialised",false,nullptr);
            state.getChildWithProperty("id","limiterMode").setProperty("value",0.f,nullptr);
            for(const auto* id:qqsc::params::limiterThresholdIds)state.getChildWithProperty("id",id).setProperty("value",-120.f,nullptr);
            juce::AudioProcessor::copyXmlToBinary(*state.createXml(),saved);q.setStateInformation(saved.getData(),int(saved.getSize()));q.enterLimiterMode();
            check(near(q.getBoundaryForDomainDb(false,false,0),0),"Unused old Limiter placeholder not migrated");
        }
        int edits=0;
        for(int dual:{0,1})for(int mode:{0,1,2})for(int algorithm:{0,1})for(int link:{0,1})
        for(int domainLink:{0,1})for(int ratioLink:{0,1})for(float mix:{0.f,37.f,100.f})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            set(p,"limiterCompressionMode",float(dual));set(p,"limiterProcessingMode",float(mode));
            set(p,"limiterAlgorithmMode",float(algorithm));set(p,"limiterDownAlgorithmMode",float(algorithm));set(p,"limiterUpAlgorithmMode",float(1-algorithm));
            set(p,"limiterLink",float(link));set(p,"limiterDomainLink",float(domainLink));set(p,"limiterDualRatioLink",float(ratioLink));
            set(p,"limiterOutputDb",11.73f);set(p,"limiterCalibrationDb",2);set(p,"ceilingDb",-.9f);
            for(size_t d=0;d<5;++d)
            {
                set(p,qqsc::params::limiterRatioIds[d],400+float(d)*10);set(p,qqsc::params::limiterDownRatioIds[d],400+float(d)*10);set(p,qqsc::params::limiterUpRatioIds[d],.5f);
                p.setBoundaryForDomainDb(false,true,int(d),1);p.setBoundaryForDomainDb(false,false,int(d),-24.3f-float(d));
                p.setBoundaryForDomainDb(true,false,int(d),-70);p.setBoundaryForDomainDb(true,true,int(d),-24.3f-float(d));
                set(p,qqsc::params::limiterModeIds[1+d],float(d)-2);set(p,qqsc::params::limiterModeIds[6+d],mix);
            }
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            const size_t d=mode==0?0:mode==2?1:3;const auto normal=normalBank(p);
            const std::array<Knob*,5> makeups{&e.makeupSTSlider,&e.makeupLSlider,&e.makeupRSlider,&e.makeupMSlider,&e.makeupSSlider};
            const std::array<Knob*,5> mixes{&e.mixSlider,&e.mixLSlider,&e.mixRSlider,&e.mixMSlider,&e.mixSSlider};
            const auto edit=[&](auto action)
            {
                const auto oldOutput=get(p,"limiterOutputDb");const auto total=oldOutput+p.getLimiterReferencePeakDb();
                action();sync(p,e);
                check(link?near(total,get(p,"limiterOutputDb")+p.getLimiterReferencePeakDb(),.02):near(get(p,"limiterOutputDb"),oldOutput,.0001),"Full-reference GUI link mismatch");
                check(normalBank(p)==normal&&near(get(p,"limiterCalibrationDb"),2)&&near(get(p,"ceilingDb"),-.9),"Link changed other bank/calibration/ceiling");++edits;
            };
            if(dual)
            {
                edit([&]{drag(*e.downRatioSliders[d],620);});
                const auto before=get(p,"limiterOutputDb");edit([&]{drag(*e.mainRatioControls()[d],.4);});
                if(!ratioLink)check(near(get(p,"limiterOutputDb"),before,.0001),"UP-only should require zero compensation");
                edit([&]{type(*e.downRatioSliders[d],"710:1");});
            }
            else
            {
                edit([&]{drag(*e.mainRatioControls()[d],620);});
                edit([&]{type(*e.mainRatioControls()[d],"710:1");});
            }
            edit([&]{drag(*makeups[d],6);});edit([&]{type(*makeups[d],"-4");});
            edit([&]{drag(*mixes[d],50);});edit([&]{type(*mixes[d],"37");});
            edit([&]{drag(*mixes[d],0);});edit([&]{drag(*makeups[d],90);});edit([&]{drag(*mixes[d],100);});
            edit([&]{drag(*makeups[d],-4);});edit([&]{drag(*mixes[d],mix);});
            const auto oldOutput=get(p,"limiterOutputDb");set(p,qqsc::params::limiterModeIds[6+d],42);sync(p,e);
            check(near(get(p,"limiterOutputDb"),oldOutput,.0001),"Host playback double-compensated Mix");
        }
        // One real Undo/Redo restores the whole typed Dual group plus Output.
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();set(p,"limiterCompressionMode",1);set(p,"limiterDualRatioLink",1);
            set(p,"limiterUpRatio",.5);set(p,"limiterDownRatio",400);set(p,"limiterMix",37);set(p,"limiterOutputDb",12);
            p.setBoundaryForDomainDb(true,false,0,-70);p.setBoundaryForDomainDb(true,true,0,-24.3f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            (void)p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();const auto before=limiterBank(p);
            type(*e.downRatioSliders[0],"800:1");sync(p,e);const auto after=limiterBank(p);
            check(before!=after,"Typed Dual edit missing");p.getUndoManager().beginNewTransaction();
            check(p.getUndoManager().undo(),"Typed Dual Undo missing");sync(p,e);
            auto nearBank=[&](const auto& a,const auto& b){check(a.size()==b.size(),"bank size");for(size_t i=0;i<a.size();++i)check(near(a[i],b[i],.001),"Undo/Redo incomplete or double-compensated");};
            nearBank(limiterBank(p),before);check(p.getUndoManager().redo(),"Typed Dual Redo missing");sync(p,e);nearBank(limiterBank(p),after);
            const auto total=p.getLimiterReferencePeakDb()+get(p,"limiterOutputDb");
            e.mixSlider.keyPressed(juce::KeyPress(juce::KeyPress::leftKey));sync(p,e);
            check(near(total,p.getLimiterReferencePeakDb()+get(p,"limiterOutputDb"),.02),"Keyboard Mix link mismatch");
        }
        std::cout<<"PASS: 1.2.9 independent first entry/recall, ranges/text, saved A/B clamps and "<<edits
                 <<" real-editor mixed-reference edits; typed Dual Undo/Redo and passive host protection.\n";
    }
