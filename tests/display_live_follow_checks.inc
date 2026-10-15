    static void displayLiveFollowChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p(settings(root));
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));
        e.stopTimer();auto& d=e.display;d.stopTimer();e.setSize(1200,800);
        int cases=0;
        for(int family:{0,1,2})for(int mode:{0,1,2})
        {
            p.setProcessorModeFromEditor(family);
            set(p,p.soundParameterID("processingMode"),float(mode));
            e.updateModeUi();e.resized();d.resetZoom();
            const int domain=family?1:mode==0?0:mode==1?3:1;
            const auto plot=d.getBoundaryPlotForDomain(domain);
            for(int repeat=0;repeat<3;++repeat)
            {
                const auto old=p.getDisplayZoom();
                const auto pos=plot.getRelativePoint(.25f,.5f);
                const auto time=juce::Time::getCurrentTime();
                const juce::MouseEvent wheelEvent(juce::Desktop::getInstance().getMainMouseSource(),pos,
                    juce::ModifierKeys(juce::ModifierKeys::ctrlModifier),1.f,0.f,0.f,0.f,0.f,&d,&d,time,pos,time,0,false);
                juce::MouseWheelDetails wheel {};wheel.deltaY=1.f/3.f;
                if(repeat==1)
                {
                    const auto start=plot.getRelativePoint(.25f,.1f),end=plot.getRelativePoint(.75f,.9f);
                    const juce::MouseEvent boxEvent(juce::Desktop::getInstance().getMainMouseSource(),start,
                        juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1.f,0.f,0.f,0.f,0.f,&d,&d,time,start,time,1,false);
                    d.mouseDown(boxEvent);d.mouseDrag(boxEvent.withNewPosition(end));d.mouseUp(boxEvent.withNewPosition(end));
                }
                else for(int step=0;step<4;++step) d.mouseWheelMove(wheelEvent,wheel);
                const auto zoom=p.getDisplayZoom();
                d.renderReferenceSampleRate=48000;d.renderReferenceCounter=48000*60;
                const auto latestX=d.historyXForCounter(d.renderReferenceCounter,plot);
                if(std::abs(latestX-plot.getRight())>.01f)
                    std::cout<<"Observed Display delay="<<zoom[3]<<" s after combined zoom, latest x="<<latestX<<", right edge="<<plot.getRight()<<std::endl;
                check(std::abs(latestX-plot.getRight())<.01f,"Combined zoom hides current audio beyond the right edge");
                check(zoom[3]==0.f,"Live zoom retained a historical time offset");
                check(std::abs(zoom[2]-old[2]*.5f)<.001f,"Live follow changed the selected time scale");
                for(double rate:{44100.,48000.})for(int elapsed:{1,60,600,3600})
                {
                    d.renderReferenceSampleRate=rate;d.renderReferenceCounter=uint64_t(rate*elapsed);
                    check(std::abs(d.historyXForCounter(d.renderReferenceCounter,plot)-plot.getRight())<.01f,"Latest audio drifted after continued playback");
                    const auto age=uint64_t(rate*zoom[2]*.5);
                    if(d.renderReferenceCounter>=age)
                        check(std::abs(d.historyXForCounter(d.renderReferenceCounter-age,plot)-plot.getCentreX())<.1f,"Zoomed timeline scale changed");
                }
                ++cases;
            }
        }
        // Older projects may have saved a multi-second zoom offset. Preserve
        // their vertical range and duration, but restore live following.
        juce::MemoryBlock state;p.getStateInformation(state);
        auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
        check(xml!=nullptr,"State XML missing");auto tree=juce::ValueTree::fromXml(*xml);
        tree.setProperty("qqscDisplayZoom0",-42.f,nullptr);tree.setProperty("qqscDisplayZoom1",-12.f,nullptr);
        tree.setProperty("qqscDisplayZoom2",1.5f,nullptr);tree.setProperty("qqscDisplayZoom3",6.f,nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),state);
        QQSuperCompressionAudioProcessor restored(settings(root));
        restored.setStateInformation(state.getData(),int(state.getSize()));
        check(restored.getDisplayZoom()==std::array<float,4>{-42,-12,1.5f,0},"Legacy zoom offset was restored as display lag");
        restored.getStateInformation(state);QQSuperCompressionAudioProcessor again(settings(root));
        again.setStateInformation(state.getData(),int(state.getSize()));
        check(again.getDisplayZoom()==restored.getDisplayZoom(),"Live zoom did not survive state round trip");
        std::cout<<"PASS: "<<cases<<" repeated mixed wheel/rectangle live-zoom cases; timeline coordinates simulated through one hour at 44.1/48k; legacy offset migration and state round trip\n";
    }
