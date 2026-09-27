    static void ratioMixChecks(const juce::File& root)
    {
        // Exercise actual host parameters, including the discontinuous Single
        // Limiter range. In particular, 200 must not round-trip to unity.
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
            range(qqsc::params::limiterUpRatioIds[d],.005f,1);
            range(qqsc::params::limiterDownRatioIds[d],200,1000);
            range(qqsc::params::limiterRatioIds[d],.005f,1000);
            for(float value:{1.f,200.f,400.f,1000.f})
            {
                set(p,qqsc::params::limiterRatioIds[d],value);
                check(near(get(p,qqsc::params::limiterRatioIds[d]),value),"Single Limiter unity/down detents");
            }
            auto* param=p.getAPVTS().getParameter(qqsc::params::limiterRatioIds[d]);
            for(int n=0;n<=10000;++n)
            {
                param->setValueNotifyingHost(float(n)/10000);
                const auto v=get(p,qqsc::params::limiterRatioIds[d]);
                check(v<=1.000001f || v>=199.999f,"Automation entered forbidden Limiter down range");
            }
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
            check(near(get(p,qqsc::params::downRatioIds[d]),200)&&near(get(p,qqsc::params::limiterRatioIds[d]),200),"Legacy down/single migration");
            check(near(get(p,qqsc::params::limiterUpRatioIds[d]),.005)&&near(get(p,qqsc::params::limiterDownRatioIds[d]),200),"Limiter legacy ratios not clamped");
        }
        std::cout<<"PASS: all 30 Ratio parameter ranges, 50,005 Single-Limiter automation positions, unity/200 endpoints and legacy-state clamps.\n";

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
            set(q,"limiterUpRatio",.125f);set(q,"limiterDownRatio",400);sync(q,e);
            e.beginDualRatioGesture(0,false);e.applyDualRatioChange(800,true);e.endLinkedGesture();
            check(near(get(q,"limiterUpRatio"),.0625)&&near(get(q,"limiterDownRatio"),800),"Relative Ratio LINK changed");
            e.beginDualRatioGesture(0,false);e.applyDualRatioChange(2000,true);e.endLinkedGesture();
            check(near(get(q,"limiterUpRatio"),.05)&&near(get(q,"limiterDownRatio"),1000),"Ratio LINK upper endpoint");
            e.beginDualRatioGesture(0,true);e.applyDualRatioChange(1,true);e.endLinkedGesture();
            check(near(get(q,"limiterUpRatio"),.25)&&near(get(q,"limiterDownRatio"),200),"Ratio LINK lower endpoint");
        }
        std::cout<<"PASS: "<<mixCases<<" Mix drag/text/automation cases leave Output and Threshold unchanged; both algorithms, Single/Dual, ST/LR/MS, monitor on/off; relative Ratio LINK endpoints.\n";
    }
