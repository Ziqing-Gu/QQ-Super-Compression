    static void displayAdjustChecks(const juce::File& root)
    {
        int cases=0;
        for(int family:{0,1,2})for(int mode:{0,1,2})
        {
            if(family && mode!=2)continue;
            QQSuperCompressionAudioProcessor p;
            p.setProcessorModeFromEditor(family);set(p,p.soundParameterID("processingMode"),float(mode));
            set(p,p.soundParameterID("compressionMode"),0);set(p,p.soundParameterID("limiterLink"),0);
            set(p,p.soundParameterID("inputGainDb"),12);set(p,"inputOutputLink",1);
            const int domain=family?1:mode==0?0:mode==1?3:1;
            p.setBoundaryForDomainDb(false,false,domain,-30);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.stopTimer();e.display.stopTimer();sync(p,e);
            auto& d=e.display;p.setDisplayZoom(-60,0,4,0);d.refreshScale();
            check(e.inputGainLabel.getText()=="ADJUST","Adjust label missing");
            check(!e.inputOutputLinkButton.isVisible(),"Retired Input / Output LINK visible");
            check(p.readSoundParameter("inputGainDb")==0,"Legacy Input still active");
            const auto base=p.getDisplayZoom();const float output=p.readSoundParameter("outputGainDb");
            const auto plot=d.getBoundaryPlotForDomain(domain);const auto y0=d.getBoundaryYForDomainDb(domain,-30);
            const auto thumb=[&] {return d.getBoundaryDbForY(domain,d.getBoundaryYForDomainDb(domain,-30));};
            for(float adjust:{-18.f,0.f,18.f})
            {
                e.inputGainSlider.setValue(adjust,juce::sendNotificationSync);d.refreshScale();
                check(p.getDisplayZoom()==base,"Adjust changed zoom/time scale");
                check(std::abs(d.view[1]+adjust)<.001f && std::abs(d.view[0]+60+adjust)<.001f,"Display axis did not translate");
                check(std::abs(d.getBoundaryYForDomainDb(domain,-30)-(y0-adjust/60*plot.getHeight()))<.01f,"Threshold line did not follow Adjust");
                check(std::abs(thumb()+30)<.001f,"Adjusted threshold drag inverse differs from actual dB");
                check(std::abs(d.dbToY(-30,plot)-d.getBoundaryYForDomainDb(domain,-30))<.001f,"Waveform and threshold coordinates differ");
                check(p.getBoundaryForDomainDb(false,false,domain)==-30 && p.readSoundParameter("outputGainDb")==output,"Adjust changed sound controls");
                juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
                restored.setStateInformation(state.getData(),int(state.getSize()));
                check(restored.getDisplayAdjustDb()==adjust && restored.readSoundParameter("inputGainDb")==0,"Project lost Adjust / revived Input");
                p.copyAToB();p.selectABSlot(1);check(p.getDisplayAdjustDb()==adjust,"A/B changed display-only Adjust");p.selectABSlot(0);
                ++cases;
            }
            // View controls must preserve the offset, time anchor and scale semantics.
            const auto wheel=[&](bool ctrl,bool shift)
            {
                auto now=juce::Time::getCurrentTime();auto pos=plot.getCentre();
                juce::MouseEvent ev(juce::Desktop::getInstance().getMainMouseSource(),pos,
                    juce::ModifierKeys(ctrl?juce::ModifierKeys::ctrlModifier:shift?juce::ModifierKeys::shiftModifier:0),
                    1.f,0.f,0.f,0.f,0.f,&d,&d,now,pos,now,0,false);
                juce::MouseWheelDetails w{};w.deltaY=.2f;d.mouseWheelMove(ev,w);
            };
            const float seconds=d.view[2],span=d.getVisibleDbSpan();wheel(true,false);
            check(d.view[2]<seconds && d.getVisibleDbSpan()==span && p.getDisplayAdjustDb()==18,"Ctrl zoom changed Adjust/level scale");
            const float time=d.view[2];wheel(false,true);
            check(d.view[2]==time && std::abs(d.getVisibleDbSpan()-span)<.001f
                  && std::abs(p.getDisplayAdjustDb()-(18.f+span*.03f))<.001f,"Shift wheel must edit Adjust itself");
            check(std::abs(float(e.inputGainSlider.getValue())-p.getDisplayAdjustDb())<.051f,"Shift wheel did not synchronise Adjust knob");
            e.inputGainSlider.setValue(18,juce::sendNotificationSync);
            const float centre=.5f*(d.view[0]+d.view[1]);wheel(false,false);
            check(std::abs(.5f*(d.view[0]+d.view[1])-centre)<.001f && p.getDisplayAdjustDb()==18,"Pointer zoom lost Adjust centre");
            p.setDisplayZoom(-60,0,4,0);d.refreshScale();
            // Finite zoom with Adjust retains the selected rectangle's actual dB bounds.
            const auto selection=plot.reduced(plot.getWidth()*.25f,plot.getHeight()*.25f);
            const float selectedTop=d.view[1]-(d.view[1]-d.view[0])*.25f;
            d.zoomToSelection(selection,plot);
            check(std::abs(d.view[1]-selectedTop)<.01f && p.getDisplayAdjustDb()==18,"Rectangle zoom double-counted Adjust");
            const auto alt=mouse(e.inputGainSlider,e.inputGainSlider.getLocalBounds().toFloat().getCentre(),true);
            e.inputGainSlider.mouseDown(alt);e.inputGainSlider.mouseUp(alt);
            check(p.getDisplayAdjustDb()==0,"Adjust Alt reset failed");
            p.setDisplayAdjustDb(-12);d.resetZoom();
            check(p.getDisplayAdjustDb()==0 && p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Right click must restore complete display");
            // Silence has no finite dB coordinate, even when viewing below -120.
            p.setDisplayAdjustDb(90);d.refreshScale();juce::Path silent;
            std::array<float,DynamicDisplay::historyLength> levels{};levels.fill(-120);
            std::array<uint64_t,DynamicDisplay::historyLength> counters{};counters[1]=800;
            d.updatePath(silent,levels,counters,2,plot);
            check(std::abs(silent.getBounds().getBottom()-plot.getBottom())<.01f,"Adjust raised the silence baseline");
            d.resetZoom();
            if(family==2)
            {
                p.setDisplayZoom(-60,0,8,0);e.inputGainSlider.setValue(12,juce::sendNotificationSync);
                for(int ch=0;ch<2;++ch)for(int i=0;i<120;++i)
                {DynamicDisplay::HistoryPoint point;point.inputDb=-15-8*std::sin(float(i)*.14f);point.detectorDb=point.inputDb;
                 point.captureCounter=uint64_t(i+1)*800;point.captureGeneration=1;d.histories[size_t(ch)].points.push_back(point);}
                d.renderReferenceCounter=96000;d.renderReferenceSampleRate=48000;
                e.theme=qqsc::ui::Theme::dark;e.applyTheme();e.resized();d.refreshScale();d.refreshRenderCaches(mode);
                image(e,root.getChildFile("adjust-preview.png"));
            }
        }
        std::cout<<"PASS: "<<cases<<" Adjust geometry, waveform/threshold coordinate, state, A/B, zoom/reset and UI cases.\n";
    }
