    static void displayFreezeChecks(const juce::File& root)
    {
        struct Head final:juce::AudioPlayHead {
            bool playing=true,recording=false,available=true;int64_t sample=0;
            juce::Optional<PositionInfo> getPosition() const override {
                if(!available)return {};PositionInfo p;p.setIsPlaying(playing);
                p.setIsRecording(recording);p.setTimeInSamples(sample);return p;
            }
        };
        int cases=0;float worstAudio=0.f;
        for(int family:{0,1,2})for(int mode:{0,1,2})for(bool eco:{false,true})
        {
            auto dir=root.getChildFile("case-"+juce::String(cases));dir.createDirectory();
            {auto prefs=settings(dir);prefs->setValue("lastDisplayFreezeOnStop",true);prefs->saveIfNeeded();}
            QQSuperCompressionAudioProcessor p(settings(dir)),reference(settings(dir));Head head;
            QQSuperCompressionAudioProcessorEditor e(p,settings(dir)),other(reference,settings(dir));
            e.stopTimer();other.stopTimer();auto& d=e.display;d.stopTimer();other.display.stopTimer();
            e.setVisible(true);e.setSize(1200,800);other.setVisible(true);
            const double rate=mode==1 ? 96000. : 48000.;const int block=int(rate/60.);
            for(auto* q:{&p,&reference}) {
                q->setEcoMode(eco);q->setProcessorModeFromEditor(family);
                set(*q,q->soundParameterID("processingMode"),float(mode));
                set(*q,q->soundParameterID("lookaheadMs"),25.3f);set(*q,q->soundParameterID("oversampling"),0);
                set(*q,q->soundParameterID("algorithmMode"),1);
                set(*q,q->soundParameterID("compressionMode"),0);set(*q,q->soundParameterID("inputOutputLink"),0);
                for(const auto* id:{"ratio","ratioL","ratioR","ratioM","ratioS"})set(*q,q->soundParameterID(id),20);
                for(const auto* id:{"thresholdDb","thresholdLDb","thresholdRDb","thresholdMDb","thresholdSDb"})set(*q,q->soundParameterID(id),-24);
                if(mode==1){auto layout=q->getBusesLayout();layout.inputBuses.set(1,juce::AudioChannelSet::stereo());check(q->setBusesLayout(layout),"EXT freeze buses");set(*q,q->soundParameterID("keySource"),1);}
                q->setEditorOpen(true);q->setPlayHead(&head);q->setRateAndBufferSizeDetails(rate,block);q->prepareToPlay(rate,block);
            }
            reference.setDisplayFreezeOnStop(false);e.updateModeUi();e.resized();
            check(p.isDisplayFreezeOnStop() && d.freezeButton.getToggleState(),"First-run Freeze must be on");
            check(d.getLocalBounds().contains(d.freezeButton.getBounds()) && d.freezeButton.getY()<28,"Freeze header button misplaced");
            // No native window is created by this headless test. Exercise the
            // visible editor's shared ingestion path for UI ticks only; every
            // processBlock still runs in the requested real ECO/FULL mode.
            const auto tick=[&] {
                if(eco)p.setEcoMode(false);
                d.timerCallback();d.refreshRenderCaches(d.lastMode);
                if(eco)p.setEcoMode(true);
            };
            juce::MidiBuffer midi;int64_t signal=0;
            const auto audio=[&](float amplitude=1.f) {
                juce::AudioBuffer<float> a(mode==1?4:2,block),b(mode==1?4:2,block);
                for(int c=0;c<a.getNumChannels();++c)for(int i=0;i<block;++i) {
                    const double t=double(signal+i)/rate;
                    a.setSample(c,i,amplitude*float((.2+.1*std::sin(2*pi*2.7*t))*std::sin(2*pi*(97+c*43)*t)+.06*std::sin(2*pi*2117*t)));
                }
                b.makeCopyOf(a);p.processBlock(a,midi);reference.processBlock(b,midi);
                for(int c=0;c<2;++c)for(int i=0;i<block;++i)worstAudio=std::max(worstAudio,std::abs(a.getSample(c,i)-b.getSample(c,i)));
                signal+=block;if(head.playing)head.sample+=block;tick();
            };
            for(int i=0;i<90;++i)audio();
            auto pos=p.getDisplayKeyHistoryPosition();auto count=d.histories[0].points.size();
            check(pos.counter>0 && count>40,"Missing playback history");
            const auto last=d.histories[0].points.back(),first=d.histories[0].points.front();
            const auto revision=p.getMeterState().displayCaptureRevision.load();
            QQSuperCompressionAudioProcessor::DisplayKeyHistorySnapshot rawBefore,rawAfter;
            check(p.copyDisplayKeyHistory(pos.generation,0,pos.counter,rawBefore),"Missing raw playback evidence");
            head.playing=false;
            // Longer than the ten-second raw ring for one fixture in each family.
            for(int i=0;i<(mode==0&&!eco?720:60);++i)audio(i%2?1.f:0.f);
            const auto frozen=p.getDisplayKeyHistoryPosition();
            std::cout<<"FREEZE family="<<family<<" mode="<<mode<<" eco="<<eco<<" counter="<<pos.counter<<" -> "<<frozen.counter<<std::endl;
            check(frozen.counter==pos.counter && frozen.generation==pos.generation,"Stopped Display keeps scrolling instead of retaining history");
            check(p.getMeterState().displayCaptureRevision.load()==revision,"Stopped meter frame overwritten");
            check(d.histories[0].points.size()==count && d.histories[0].points.front().captureCounter==first.captureCounter && d.histories[0].points.back().captureCounter==last.captureCounter,"Frozen displayed points were erased");
            check(d.renderReferenceCounter==pos.counter,"Frozen timeline moved");
            check(p.copyDisplayKeyHistory(pos.generation,0,pos.counter,rawAfter) && rawAfter.left==rawBefore.left && rawAfter.right==rawBefore.right,"Frozen raw evidence was overwritten");
            // Regression: stopped Ctrl-wheel must keep the time under the pointer.
            const auto zoomPlot=d.getBoundaryPlotForDomain(family?1:mode==0?0:mode==1?3:1);
            const auto savedCounter=d.renderReferenceCounter;
            const auto savedRate=d.renderReferenceSampleRate;
            d.renderReferenceCounter=480000;d.renderReferenceSampleRate=48000.;
            const auto wheelAt=[&](float x,float delta,int modifiers) {
                auto at=zoomPlot.getRelativePoint(x,.5f);auto now=juce::Time::getCurrentTime();
                juce::MouseEvent ev(juce::Desktop::getInstance().getMainMouseSource(),at,
                    juce::ModifierKeys(modifiers),1.f,0.f,0.f,0.f,0.f,&d,&d,now,at,now,0,false);
                juce::MouseWheelDetails details {};details.deltaY=delta;
                d.mouseWheelMove(ev,details);
            };
            for(float fraction:{.1f,.5f,.9f}) {
                d.resetZoom();const auto capture=uint64_t(std::llround(480000.-(1.-fraction)*8.*48000.));
                const auto anchor=d.historyXForCounter(capture,zoomPlot);
                for(int i=0;i<4;++i)wheelAt(fraction,1.f/3.f,juce::ModifierKeys::ctrlModifier);
                const auto zoom=p.getDisplayZoom();
                std::cout<<"STOPPED CTRL ZOOM fraction="<<fraction<<" seconds="<<zoom[2]
                    <<" anchor_error="<<d.historyXForCounter(capture,zoomPlot)-anchor<<std::endl;
                check(std::abs(zoom[2]-4.f)<.001f && zoom[0]==-90.f && zoom[1]==0.f,
                    "Ctrl wheel must change time only");
                check(std::abs(d.historyXForCounter(capture,zoomPlot)-anchor)<.02f,
                    "Stopped horizontal zoom is not centred on the mouse");
                check(zoom[3]==0.f,"Stopped browsing offset leaked into persistent project zoom");
                for(int i=0;i<4;++i)wheelAt(fraction,-1.f/3.f,juce::ModifierKeys::ctrlModifier);
                check(p.getDisplayZoom()[2]==8.f && std::abs(d.historyXForCounter(480000,zoomPlot)-zoomPlot.getRight())<.02f,
                    "Zoom out failed to restore the full stopped history");
            }
            d.resetZoom();wheelAt(.5f,1.f/3.f,juce::ModifierKeys::shiftModifier);
            check(p.getDisplayZoom()[2]==8.f && d.getVisibleDbSpan()<90.f,"Shift still selects horizontal zoom");
            d.resetZoom();wheelAt(.5f,1.f/3.f,juce::ModifierKeys::ctrlModifier);
            d.renderReferenceCounter=savedCounter;d.renderReferenceSampleRate=savedRate;
            const auto outputBefore=family?d.stereoLimiterCache.projected.output:d.renderCaches[0].projected.output;
            for(auto* q:{&p,&reference})for(const auto* id:{"thresholdDb","thresholdLDb","thresholdRDb","thresholdMDb","thresholdSDb"})set(*q,q->soundParameterID(id),-32);
            tick();
            const auto& projected=family?d.stereoLimiterCache.projected:d.renderCaches[0].projected;
            bool changed=false;for(size_t i=0;i<projected.size;++i)changed=changed || std::abs(projected.output[i]-outputBefore[i])>.1f;
            check(changed,"Freeze blocked retrospective parameter projection");
            p.setDisplayZoom(-60,-6,2,0);d.refreshScale();d.refreshRenderCaches(d.lastMode);
            check(d.renderReferenceCounter==pos.counter && d.getVisibleDbSpan()==54.f,"Frozen zoom moved time or failed");
            if(cases==0) {
                for(auto theme:{qqsc::ui::Theme::dark,qqsc::ui::Theme::light,qqsc::ui::Theme::classic}) {
                    e.theme=theme;e.applyTheme();e.resized();
                    auto shot=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;
                    juce::FileOutputStream out(root.getChildFile("freeze-"+juce::String(int(theme))+".png"));png.writeImageToStream(shot,out);
                }
                root.getChildFile("freeze-tooltip.txt").replaceWithText(d.freezeButton.getTooltip());
                root.getChildFile("display-tooltip.txt").replaceWithText(d.getTooltip());
                // Run the actual asynchronous Window/HPF worker while stopped.
               #if JUCE_WINDOWS
                const auto applied=d.hpfReplayAppliedCount;
                for(auto* q:{&p,&reference}){set(*q,q->soundParameterID("keyHpfHz"),180);set(*q,q->soundParameterID("distort"),70);}
                const auto deadline=juce::Time::getMillisecondCounterHiRes()+15000;
                do {d.timerCallback();MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}juce::Thread::sleep(2);}
                while((d.hpfReplayAppliedCount==applied || d.hpfReplayBusy || d.hpfRefreshPending) && juce::Time::getMillisecondCounterHiRes()<deadline);
                check(d.hpfReplayAppliedCount>applied && !d.hpfReplayBusy,"Frozen Window/HPF replay failed");
                check(p.getDisplayKeyHistoryPosition().counter==pos.counter && d.histories[0].points.size()==count,"Frozen replay advanced history");
               #endif
            }
            // Disable while stopped: old continuous capture returns immediately.
            d.freezeButton.setToggleState(false,juce::dontSendNotification);d.freezeButton.onClick();audio();
            check(!p.isDisplayFreezeOnStop() && p.getDisplayKeyHistoryPosition().counter>pos.counter,"Freeze off did not resume stopped capture");
            d.freezeButton.setToggleState(true,juce::dontSendNotification);d.freezeButton.onClick();
            pos=p.getDisplayKeyHistoryPosition();audio();check(p.getDisplayKeyHistoryPosition().counter==pos.counter,"Freeze re-enable failed");
            // Recording, export and transport-unknown hosts must not freeze.
            for(int exception=0;exception<4;++exception) {
                head.playing=false;head.recording=false;head.available=true;
                p.setNonRealtime(false);reference.setNonRealtime(false);audio();
                d.resetZoom();wheelAt(.5f,1.f/3.f,juce::ModifierKeys::ctrlModifier);
                check(d.stoppedViewEndAgeSeconds>0.f,"Missing stopped anchor before resume");
                pos=p.getDisplayKeyHistoryPosition();head.recording=exception==0;head.available=exception!=2;head.playing=exception==3;
                p.setNonRealtime(exception==1);reference.setNonRealtime(exception==1);audio();
                check(p.getDisplayKeyHistoryPosition().counter>pos.counter,"Active/record/offline/unknown Display was frozen");
                const auto plot=d.getBoundaryPlotForDomain(family?1:mode==0?0:mode==1?3:1);
                check(std::abs(d.historyXForCounter(d.renderReferenceCounter,plot)-plot.getRight())<.01f,"Resume introduced a historical lag");
            }
            p.setPlayHead(nullptr);reference.setPlayHead(nullptr);++cases;
        }
        // Isolated user preferences: fresh default, manual choice, new instance,
        // instance independence, project restore and old-state fallback.
        auto prefsDir=root.getChildFile("preferences");prefsDir.createDirectory();
        {auto prefs=settings(prefsDir);prefs->removeValue("lastDisplayFreezeOnStop");prefs->saveIfNeeded();}
        QQSuperCompressionAudioProcessor p(settings(prefsDir)),existing(settings(prefsDir));
        check(p.isDisplayFreezeOnStop(),"No-preference default is not on");
        juce::MemoryBlock state;
        {
            QQSuperCompressionAudioProcessorEditor e(p,settings(prefsDir));e.stopTimer();e.display.stopTimer();
            e.display.freezeButton.setToggleState(false,juce::dontSendNotification);e.display.freezeButton.onClick();
            QQSuperCompressionAudioProcessor next(settings(prefsDir));check(!next.isDisplayFreezeOnStop() && existing.isDisplayFreezeOnStop(),"Last-choice default or instance independence failed");
            p.getStateInformation(state);e.display.freezeButton.setToggleState(true,juce::dontSendNotification);e.display.freezeButton.onClick();
            p.setStateInformation(state.getData(),int(state.getSize()));e.display.timerCallback();
            check(!p.isDisplayFreezeOnStop() && !e.display.freezeButton.getToggleState(),"Project Freeze restore/UI failed");
            check(settings(prefsDir)->getBoolValue("lastDisplayFreezeOnStop",false),"Project restore overwrote manual default");
            p.copyAToB();p.selectABSlot(1);check(!p.isDisplayFreezeOnStop(),"A/B changed Freeze preference");
        }
        {QQSuperCompressionAudioProcessorEditor reopened(p,settings(prefsDir));check(!reopened.display.freezeButton.getToggleState(),"Editor reopen lost instance Freeze");}
        auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));auto old=juce::ValueTree::fromXml(*xml);old.removeProperty("qqscDisplayFreezeOnStop",nullptr);
        juce::AudioProcessor::copyXmlToBinary(*old.createXml(),state);QQSuperCompressionAudioProcessor legacy(settings(prefsDir));legacy.setStateInformation(state.getData(),int(state.getSize()));
        check(legacy.isDisplayFreezeOnStop(),"Legacy project ignored new-instance default");
        check(worstAudio==0.f,"Display Freeze changed actual audio");
        std::cout<<"PASS: "<<cases<<" Freeze transport/mode/ECO/FULL fixtures, long retained raw history, retrospective parameters and asynchronous Window/HPF, zoom, record/offline/unknown/resume, preferences/project/A-B/reopen; audio_delta="<<worstAudio<<"\n";
    }
