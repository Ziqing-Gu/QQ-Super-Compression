
    static void displayReplayDragChecks(const juce::File& root)
    {
       #if JUCE_WINDOWS
        auto pump=[](int ms) {
            const auto until=juce::Time::getMillisecondCounterHiRes()+ms;
            do { MSG message; while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE))
                {TranslateMessage(&message);DispatchMessageW(&message);}
                juce::Thread::sleep(1);
            } while(juce::Time::getMillisecondCounterHiRes()<until);
        };
        struct ReplayThread final:juce::Thread {
            ReplayThread():juce::Thread("replay oracle"){} void run() override {}
        } oracleThread;
        {
            QQSuperCompressionAudioProcessor oracleProcessor;
            QQSuperCompressionAudioProcessorEditor oracleEditor(oracleProcessor,settings(root));
            oracleEditor.stopTimer();oracleEditor.display.stopTimer();
            replayReferenceChecks(oracleEditor,oracleProcessor);
        }
        int cases=0;float worstAudio=0;double worstTimerMs=0;
        for(bool limiter:{false,true})for(int os:{0,1,2,3})for(bool filter:{false,true})
        {
            if(limiter && os!=0 && os!=3)continue;
            QQSuperCompressionAudioProcessor p,reference;
            for(auto* x:{&p,&reference}) {
                x->setEcoMode(false);if(limiter)x->enterLimiterMode();
                auto value=[&](const char* id,float v){set(*x,x->soundParameterID(id),v);};
                value("lookaheadMs",25.3f);value("oversampling",float(os));value("keyHpfHz",20);
                value("algorithmMode",1);value("ratio",5);value("ratioL",5);value("ratioR",5);
                value("thresholdDb",-24);value("thresholdLDb",-24);value("thresholdRDb",-24);
                value("rangeDb",qqsc::params::rangeOffDb);set(*x,"distort",0);set(*x,"truePeakLimiting",0);
                if(!limiter){set(*x,"stLRLink",35);set(*x,"stMSLink",65);}
                x->setRateAndBufferSizeDetails(48000,800);x->prepareToPlay(48000,800);
            }
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.stopTimer();
            auto& d=e.display;d.stopTimer();p.setEditorOpen(true);
            juce::MidiBuffer midi;int64_t counter=0;
            auto audio=[&]() {
                juce::AudioBuffer<float> a(2,800),b(2,800);
                for(int i=0;i<800;++i)for(int ch=0;ch<2;++ch) {
                    const double t=double(counter+i)/48000;
                    const float x=float((.3+.1*std::sin(2*pi*3.7*t)) * std::sin(2*pi*(ch?107:83)*t)
                                       +.09*std::sin(2*pi*3119*t));a.setSample(ch,i,x);
                }
                b.makeCopyOf(a);p.processBlock(a,midi);reference.processBlock(b,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<800;++i)
                    worstAudio=std::max(worstAudio,std::abs(a.getSample(ch,i)-b.getSample(ch,i)));
                counter+=800;
            };
            for(int i=0;i<492;++i){audio();d.timerCallback();}
            // Discard warm-up work; the measured gesture begins with eight
            // seconds of real retained audio and no outstanding replay.
            ++*d.replayRequestGeneration;d.hpfReplayBusy=false;d.hpfRefreshPending=false;
            d.hpfReplayRetryPending=false;d.hpfFinalRefreshPending=false;d.hpfNextReplayMs=0;
            pump(5);d.hpfReplayAppliedCount=0;d.hpfReplaySubmittedCount=0;
            const auto start=juce::Time::getMillisecondCounterHiRes();
            if(filter)e.keyHpfSlider.onGestureStart();else e.detectorWindowValue.start();
            for(int i=0;i<108;++i) {
                if(filter) {
                    const float hz=20.f+float(i)*5.f;
                    e.keyHpfSlider.setValue(hz,juce::sendNotificationSync);
                    set(reference,reference.soundParameterID("keyHpfHz"),hz);
                } else {
                    e.detectorWindowValue.move(-1,false);
                    set(reference,"distort",p.readSoundParameter("distort"));
                }
                audio();const auto tick=juce::Time::getMillisecondCounterHiRes();d.timerCallback();d.refreshRenderCaches(d.lastMode);
                worstTimerMs=std::max(worstTimerMs,juce::Time::getMillisecondCounterHiRes()-tick);
                if(d.hpfReplayBusy) {
                    const auto before=d.hpfReplaySubmittedCount;
                    check(!d.requestHpfHistoryRefresh(),"Replay queued while another job is in flight");
                    check(d.hpfReplaySubmittedCount==before,"Replay submission grew during in-flight work");
                }
                pump(16);
            }
            const auto dragMs=juce::Time::getMillisecondCounterHiRes()-start;
            const auto during=d.hpfReplayAppliedCount,submitted=d.hpfReplaySubmittedCount;
            check(during>0,"Continuous drag starved Display updates until mouse-up");
            check(double(submitted)<=std::ceil(dragMs/100.0)+1,"Replay exceeded 10 Hz submission limit");
            if(filter)e.keyHpfSlider.onGestureEnd();else e.detectorWindowValue.finish();
            const auto deadline=juce::Time::getMillisecondCounterHiRes()+15000;
            do {d.timerCallback();pump(10);}
            while((d.hpfReplayBusy||d.hpfRefreshPending||d.hpfReplayRetryPending)
                   && juce::Time::getMillisecondCounterHiRes()<deadline);
            check(!d.hpfReplayBusy&&!d.hpfRefreshPending&&!d.hpfReplayRetryPending,"Final replay did not settle");
            check(d.hpfReplayAppliedCount>during,"Final mouse-up replay missing");

            const double completedWorkMs=d.hpfLastReplayWorkMs;
            DynamicDisplay::ReplayRequest request;
            request.requestGeneration=d.replayRequestGeneration->load();request.captureGeneration=d.lastCaptureGeneration;
            request.mode=d.lastMode;request.keySource=d.lastKeySource;request.lookaheadMs=25.3f;
            request.detectorWindowMs=p.readSoundParameter("detectorWindowMs");
            request.detectorMode=0;request.oversampling=os;request.hpfHz=p.readSoundParameter("keyHpfHz");
            request.abSlot=p.getActiveABSlot();request.limiter=limiter;
            for(const auto& point:d.histories[0].points) {request.markers.push_back(point.captureCounter);request.carrierDelays.push_back(point.carrierDelaySeconds);request.captureBlockSamples.push_back(point.captureBlockSamples);}
            request.requestedStartCounter=request.markers.front()>48000?request.markers.front()-48000:0;
            DynamicDisplay::ReplayResult expected;
            check(d.buildHpfReplay(request,expected,oracleThread),"Final-value replay oracle failed");
            size_t compared=0;
            for(size_t i=0;i<expected.detectorDb0.size();++i)if(std::isfinite(expected.detectorDb0[i])) {
                if(!d.histories[0].points[i].hasReplayedDetector)
                {
                    const auto& captured=d.histories[0].points[i].capturedDetectorSettings;
                    check(std::abs(captured[0]-request.lookaheadMs)<.001f && std::abs(captured[1]-request.detectorWindowMs)<.001f
                        && captured[2]==float(os) && std::abs(captured[3]-request.hpfHz)<.001f && captured[4]==float(request.detectorMode),
                        "Unreplayed point must have exact original detection settings");
                    ++compared;continue;
                }
                check(std::abs(d.histories[0].points[i].replayedDetectorDb-expected.detectorDb0[i])<.0001f,"Final history differs from exact settled parameters");++compared;
                if(!limiter)
                {
                    check(d.histories[0].points[i].hasSTReplay,"Missing LR/MS detector replay in ST");
                    for(size_t channel=0;channel<4;++channel)
                        check(std::abs(d.histories[0].points[i].stReplayedDetectorDb[channel]-expected.stDetectorDb[channel][i])<.0001f,"ST LR/MS detector replay did not follow final Window/HPF");
                }
            }
            check(compared>400,"Too little final history checked");
            // Even while a gesture is active, never paint another A/B slot,
            // detector mode, OS, channel mode, or capture generation's result.
            d.beginHistoryGesture();
            for(int mismatch=0;mismatch<6;++mismatch) {
                auto stale=expected;stale.request=request;
                if(mismatch==0)stale.request.abSlot=1-request.abSlot;
                if(mismatch==1)stale.request.detectorMode=1;
                if(mismatch==2)stale.request.oversampling=(os+1)%4;
                if(mismatch==3)stale.request.mode=(request.mode+1)%3;
                if(mismatch==4)++stale.request.captureGeneration;
                if(mismatch==5)stale.request.limiter=!limiter;
                const auto before=d.hpfReplayAppliedCount;d.applyHpfReplay(std::move(stale));
                check(d.hpfReplayAppliedCount==before,"Incompatible history result painted during drag");
            }
            d.historyGestureActive=false;d.hpfRefreshPending=false;
            const auto workMs=expected.workMilliseconds;juce::ignoreUnused(workMs);
            // A direct oracle does not time itself; worker completion records it.
            std::cout<<"REPLAY limiter="<<limiter<<" os="<<qqsc::params::oversamplingFactorForChoiceIndex(os)
                <<" control="<<(filter?"HPF":"Window")<<" drag_ms="<<dragMs
                <<" updates_during_drag="<<during<<" submitted="<<submitted
                <<" worker_ms="<<completedWorkMs<<" exact_points="<<compared<<" audio_delta="<<worstAudio<<"\n";
            ++cases;
        }
        check(worstAudio==0,"Display replay changed actual audio");
        std::cout<<"PASS: "<<cases<<" real Window/HPF drag cases, bounded work, exact final history, incompatible-result rejection; audio_delta="
            <<worstAudio<<" max_hidden_timer_ms="<<worstTimerMs<<"\n";
       #else
        juce::ignoreUnused(root);
        throw std::runtime_error("Display gesture message-pump profile currently targets Windows");
       #endif
    }
