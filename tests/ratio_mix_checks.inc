    static void ratioMixChecks(const juce::File& root)
    {
        // Exercise actual host parameters. Limiter SINGLE is continuous around
        // unity: Upward 1:8..1:1 and Downward 1:1..1000:1. DUAL DOWN is also
        // 1:1..1000:1. Ceiling/TP, not a forced 200:1 compressor ratio, is the
        // final limiter stage.
        QQSuperCompressionAudioProcessor p;
        auto near=[](double a,double b){return std::abs(a-b)<1.e-5*std::max(1.,std::abs(b));};
        for(size_t d=0;d<5;++d)
        {
            auto range=[&](const char* id,float lo,float hi)
            {
                auto* param=p.getAPVTS().getParameter(id);
                check(near(param->convertFrom0to1(0),lo)&&near(param->convertFrom0to1(1),hi),"Host Ratio endpoints");
                for(float value:{lo,1.f,hi})
                {
                    value=juce::jlimit(lo,hi,value);set(p,id,value);
                    check(near(get(p,id),value),"Ratio endpoint round trip");
                }
            };
            range(qqsc::params::ratioIds[d],.005f,200);
            range(qqsc::params::upRatioIds[d],.005f,1);
            range(qqsc::params::downRatioIds[d],1,200);
            range(qqsc::params::limiterUpRatioIds[d],.125f,1);
            range(qqsc::params::limiterDownRatioIds[d],1,1000);
            range(qqsc::params::limiterRatioIds[d],.125f,1000);
            for(float value:{.125f,.5f,1.f,2.f,20.f,200.f,1000.f})
            {
                set(p,qqsc::params::limiterRatioIds[d],value);
                check(near(get(p,qqsc::params::limiterRatioIds[d]),value),"Single Limiter continuous ratio round trip");
            }
            auto* param=p.getAPVTS().getParameter(qqsc::params::limiterRatioIds[d]);
            check(near(param->convertFrom0to1(.5f),1.0),"Limiter Single unity is not the centre detent");
            check(near(param->convertFrom0to1(param->getDefaultValue()),1.0),"Limiter Single default is not 1:1");
            float previous=0.0f;
            for(int n=0;n<=10000;++n)
            {
                param->setValueNotifyingHost(float(n)/10000);
                const auto v=get(p,qqsc::params::limiterRatioIds[d]);
                check(v+1.0e-6f>=previous,"Limiter Single automation mapping is not monotonic");
                previous=v;
            }
            auto* downParam=p.getAPVTS().getParameter(qqsc::params::limiterDownRatioIds[d]);
            check(near(downParam->convertFrom0to1(downParam->getDefaultValue()),1.0),"Limiter Dual DOWN default is not 1:1");
        }
        // Mimic a previous project's physical values, rather than using the
        // new parameter setter (which would already clamp the test input).
        juce::MemoryBlock state;p.getStateInformation(state);
        auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);
        for(size_t d=0;d<5;++d)
            for(auto item:std::array<std::pair<const char*,float>,6>{{
                {qqsc::params::ratioIds[d],1000},{qqsc::params::upRatioIds[d],.001f},
                {qqsc::params::downRatioIds[d],1000},{qqsc::params::limiterRatioIds[d],20},
                {qqsc::params::limiterUpRatioIds[d],.001f},{qqsc::params::limiterDownRatioIds[d],20}}})
                tree.getChildWithProperty("id",item.first).setProperty("value",item.second,nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),state);
        p.setStateInformation(state.getData(),int(state.getSize()));
        for(size_t d=0;d<5;++d)
        {
            check(near(get(p,qqsc::params::ratioIds[d]),200)&&near(get(p,qqsc::params::upRatioIds[d]),.005),"Normal legacy ratios not clamped");
            check(near(get(p,qqsc::params::downRatioIds[d]),200)&&near(get(p,qqsc::params::limiterRatioIds[d]),20),"Limiter Single valid old down ratio was not preserved");
            check(near(get(p,qqsc::params::limiterUpRatioIds[d]),.125)&&near(get(p,qqsc::params::limiterDownRatioIds[d]),20),"Limiter legacy ratios not clamped/preserved");
        }
        // A pre-1.2.23 Limiter Single upward value below 1:8 must migrate to the new floor.
        {
            juce::MemoryBlock upwardState; p.getStateInformation(upwardState);
            auto upwardXml=juce::AudioProcessor::getXmlFromBinary(upwardState.getData(),int(upwardState.getSize()));
            auto upwardTree=juce::ValueTree::fromXml(*upwardXml);
            for(size_t d=0;d<5;++d)
                upwardTree.getChildWithProperty("id",qqsc::params::limiterRatioIds[d]).setProperty("value",.001f,nullptr);
            juce::AudioProcessor::copyXmlToBinary(*upwardTree.createXml(),upwardState);
            p.setStateInformation(upwardState.getData(),int(upwardState.getSize()));
            for(size_t d=0;d<5;++d)
                check(near(get(p,qqsc::params::limiterRatioIds[d]),.125),"Limiter Single legacy UP ratio not clamped to 1:8");
        }
        std::cout<<"PASS: all 30 Ratio parameter ranges, 50,005 continuous Single-Limiter automation positions, unity defaults/centre and legacy-state clamps including the 1:8 UP floor.\n";

        // 1.2.25: Limiter Single upward and Limiter Dual UP both use 1:8..1:1; DOWN starts at unity.
        // Normal Single/Dual keep their existing 1:200 upward range.
        {
            QQSuperCompressionAudioProcessor u; baseline(u);
            u.enterLimiterMode(); set(u,u.soundParameterID("compressionMode"),1);
            // The absolute UI range is tested with Ratio LINK off. With Dual Ratio LINK
            // on, the existing product-preserving coupling may legitimately stop the
            // UP control above 1:8 when DOWN reaches its 1000:1 boundary.
            // Test the absolute Limiter-Dual UP range with the ACTIVE Limiter bank's
            // Ratio LINK explicitly off. dualRatioLink is mode-banked, so once Limiter
            // mode is active the editor reads limiterDualRatioLink via soundParameterID().
            // Do not configure only the Normal-bank preference here.
            QQSuperCompressionAudioProcessorEditor e(u,settings(root)); sync(u,e);
            set(u,u.soundParameterID(qqsc::params::dualRatioLink),0.0f); sync(u,e);
            // The editor button owns the live gesture coupling flag; force the
            // test fixture visibly OFF as well as the active Limiter parameter.
            e.dualRatioLinkButton.setToggleState(false,juce::dontSendNotification);
            check(u.readSoundParameter(qqsc::params::dualRatioLink)<0.5f && !e.dualRatioLinkButton.getToggleState(),
                  "Limiter Dual Ratio LINK-off test fixture was not established");
            check(near(e.ratioSlider.getMinimum(),.125)&&near(e.ratioSlider.getMaximum(),1),"Limiter Dual UP editor range");
            e.ratioSlider.setValue(e.ratioSlider.getValueFromText("1:32"),juce::sendNotificationSync);
            check(near(get(u,"limiterUpRatio"),.125),"Limiter Dual UP numeric entry escaped 1:8 floor");
            set(u,u.soundParameterID("compressionMode"),0); sync(u,e);
            check(near(e.ratioSlider.getMinimum(),.125)&&near(e.ratioSlider.getMaximum(),1000),"Limiter Single UP range");
            e.ratioSlider.setValue(e.ratioSlider.getValueFromText("1:32"),juce::sendNotificationSync);
            check(near(get(u,"limiterRatio"),.125),"Limiter Single numeric entry escaped 1:8 floor");

            // 1.2.25: Limiter compression is born at unity. Alt-click restores
            // SINGLE, DUAL UP and DUAL DOWN to 1:1; Ceiling/TP remains the final limiter.
            set(u,"limiterRatio",800.0f); sync(u,e);
            { const auto alt=mouse(e.ratioSlider,e.ratioSlider.getLocalBounds().toFloat().getCentre(),true);
              e.ratioSlider.mouseDown(alt); e.ratioSlider.mouseUp(alt); }
            check(near(get(u,"limiterRatio"),1.0),"Limiter Single Alt-reset did not return to 1:1");

            set(u,u.soundParameterID("compressionMode"),1); sync(u,e);
            set(u,"limiterUpRatio",0.5f); sync(u,e);
            { const auto alt=mouse(e.ratioSlider,e.ratioSlider.getLocalBounds().toFloat().getCentre(),true);
              e.ratioSlider.mouseDown(alt); e.ratioSlider.mouseUp(alt); }
            check(near(get(u,"limiterUpRatio"),1.0),"Limiter Dual UP Alt-reset changed from 1:1");
            set(u,"limiterDownRatio",800.0f); sync(u,e);
            { const auto alt=mouse(*e.downRatioSliders[0],e.downRatioSliders[0]->getLocalBounds().toFloat().getCentre(),true);
              e.downRatioSliders[0]->mouseDown(alt); e.downRatioSliders[0]->mouseUp(alt); }
            check(near(get(u,"limiterDownRatio"),1.0),"Limiter Dual DOWN Alt-reset did not return to 1:1");

            u.leaveLimiterMode(); set(u,u.soundParameterID("compressionMode"),1); sync(u,e);
            check(near(e.ratioSlider.getMinimum(),.005)&&near(e.ratioSlider.getMaximum(),1),"Normal Dual UP range changed");
            set(u,u.soundParameterID("compressionMode"),0); sync(u,e);
            check(near(e.ratioSlider.getMinimum(),.005)&&near(e.ratioSlider.getMaximum(),200),"Normal Single range changed");
            set(u,"ratio",8.0f); sync(u,e);
            { const auto alt=mouse(e.ratioSlider,e.ratioSlider.getLocalBounds().toFloat().getCentre(),true);
              e.ratioSlider.mouseDown(alt); e.ratioSlider.mouseUp(alt); }
            check(near(get(u,"ratio"),1.0),"Normal Single Alt-reset changed from 1:1");
        }

        int mixCases=0;
        for(int algorithm:{0,1})for(int mode:{0,1,2})for(int dual:{0,1})for(bool monitor:{false,true})
        {
            QQSuperCompressionAudioProcessor q;baseline(q);
            set(q,"algorithmMode",float(algorithm));set(q,"processingMode",float(mode));set(q,"compressionMode",float(dual));
            for(size_t d=0;d<5;++d)
            {set(q,qqsc::params::ratioIds[d],8);set(q,qqsc::params::downRatioIds[d],8);set(q,qqsc::params::rangeIds[d],1);}
            q.enterLimiterMode();q.setUnityMonitorEnabled(monitor);
            QQSuperCompressionAudioProcessorEditor e(q,settings(root));sync(q,e);
            const std::array<const char*,5> mixIds{"mix","mixL","mixR","mixM","mixS"};
            const std::array<QQSuperCompressionAudioProcessorEditor::FineKnob*,5> mixControls{&e.mixSlider,&e.mixLSlider,&e.mixRSlider,&e.mixMSlider,&e.mixSSlider};
            const auto output=get(q,"limiterOutputDb");
            std::array<float,35> before;
            for(size_t i=0;i<35;++i)before[i]=get(q,qqsc::params::limiterSoundIds[i]);
            for(size_t d=0;d<5;++d)
            {
                auto& slider=*mixControls[d];
                const auto gesture=mouse(slider,slider.getLocalBounds().toFloat().getCentre());
                slider.mouseDown(gesture);slider.setValue(35,juce::sendNotificationSync);slider.mouseUp(gesture);
                slider.setValue(slider.getValueFromText("65"),juce::sendNotificationSync);
                set(q,q.soundParameterID(mixIds[d]),10);sync(q,e); // Host automation / refresh.
                check(near(get(q,"limiterOutputDb"),output),"Mix changed Output");
                for(size_t i=0;i<35;++i)check(near(get(q,qqsc::params::limiterSoundIds[i]),before[i]),"Mix changed Threshold or Ratio");
                ++mixCases;
            }
            // Relative Ratio LINK keeps the existing product, including an
            // asymmetric pair, and stops together at the new range bounds.
            set(q,q.soundParameterID("compressionMode"),1);set(q,q.soundParameterID("dualRatioLink"),1);sync(q,e);
            set(q,"limiterUpRatio",.5f);set(q,"limiterDownRatio",400);sync(q,e);
            e.beginDualRatioGesture(0,false);e.applyDualRatioChange(800,true);e.endLinkedGesture();
            check(near(get(q,"limiterUpRatio"),.25)&&near(get(q,"limiterDownRatio"),800),"Relative Ratio LINK changed");
            e.beginDualRatioGesture(0,false);e.applyDualRatioChange(2000,true);e.endLinkedGesture();
            check(near(get(q,"limiterUpRatio"),.2)&&near(get(q,"limiterDownRatio"),1000),"Ratio LINK upper endpoint");
            e.beginDualRatioGesture(0,true);e.applyDualRatioChange(1,true);e.endLinkedGesture();
            check(near(get(q,"limiterUpRatio"),1.0)&&near(get(q,"limiterDownRatio"),200),"Ratio LINK lower endpoint");
        }
        std::cout<<"PASS: "<<mixCases<<" Mix drag/text/automation cases leave Output and Threshold unchanged; both algorithms, Single/Dual, ST/LR/MS, monitor on/off; relative Ratio LINK endpoints.\n";
    }
