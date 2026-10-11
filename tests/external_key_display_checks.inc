    static void externalKeyDisplayChecks(const juce::File& root)
    {
        struct Worker final:juce::Thread { Worker():juce::Thread("key signal oracle"){} void run() override{} } worker;
        struct Case { int mode; bool stereoKey,limiter; int os; };
        const Case cases[]={{0,false,false,0},{0,true,false,0},{2,true,false,0},
                            {1,true,false,0},{1,false,false,0},{2,true,true,0},
                            {0,true,false,3},{2,true,true,3}};
        constexpr int n=800,blocks=96;constexpr double sr=48000;
        int replayCases=0,livePoints=0;float maxAudioDelta=0,maxLiveError=0,maxOracleError=0;
        for(const auto c:cases)
        {
            QQSuperCompressionAudioProcessor p,reference;
            auto setSound=[](auto& q,const char* id,float value){set(q,q.soundParameterID(id),value);};
            for(auto* q:{&p,&reference})
            {
                auto layout=q->getBusesLayout();
                layout.inputBuses.set(1,c.stereoKey?juce::AudioChannelSet::stereo():juce::AudioChannelSet::mono());
                check(q->setBusesLayout(layout),"External curve test bus layout");
                q->setEcoMode(false);if(c.limiter)q->enterLimiterMode();
                setSound(*q,"processingMode",float(c.mode));setSound(*q,"keySource",1);
                setSound(*q,"lookaheadMs",26);setSound(*q,"detectorWindowMs",26);
                setSound(*q,"detectorMode",0);setSound(*q,"oversampling",float(c.os));
                setSound(*q,"keyHpfHz",qqsc::params::keyHpfOffHz);setSound(*q,"keyGainDb",0);
                for(auto id:qqsc::params::ratioIds)setSound(*q,id,4);
                for(auto id:qqsc::params::thresholdIds)setSound(*q,id,-30);
                q->setRateAndBufferSizeDetails(sr,n);q->prepareToPlay(sr,n);
            }
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.stopTimer();
            auto& d=e.display;d.stopTimer();p.setEditorOpen(true);
            std::vector<std::array<float,2>> raw;
            juce::MidiBuffer midi;
            for(int b=0;b<blocks;++b)
            {
                const float window=float((b/16)%3==0?0:(b/16)%3==1?8:26);
                const float safe=float((b/8)%2);
                for(auto* q:{&p,&reference}){setSound(*q,"detectorWindowMs",window);setSound(*q,"detectorMode",safe);}
                juce::AudioBuffer<float> audio(c.stereoKey?4:3,n),other(c.stereoKey?4:3,n);
                for(int i=0;i<n;++i)
                {
                    const int at=b*n+i;const double t=double(at)/sr;
                    const float burst=at%4700<170?.2f:0.f;
                    const float l=.025f+.035f*std::sin(float(2*pi*31*t))+burst*std::sin(float(2*pi*997*t));
                    const float r=c.stereoKey ? -.018f+.025f*std::sin(float(2*pi*53*t))+.7f*burst*std::cos(float(2*pi*733*t)) : l;
                    raw.push_back({l,r});
                    audio.setSample(0,i,.2f*std::sin(float(2*pi*271*t)));
                    audio.setSample(1,i,.15f*std::sin(float(2*pi*419*t)));
                    audio.setSample(2,i,l);if(c.stereoKey)audio.setSample(3,i,r);
                }
                other.makeCopyOf(audio);p.processBlock(audio,midi);reference.processBlock(other,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<n;++i)
                    maxAudioDelta=std::max(maxAudioDelta,std::abs(audio.getSample(ch,i)-other.getSample(ch,i)));
                const int delay=int(std::llround(p.getDisplayCoreDelaySeconds()*sr));
                std::array<float,2> peaks{};
                for(int at=std::max(0,b*n-delay);at<(b+1)*n-delay;++at)
                {
                    auto x=raw[size_t(at)];
                    if(c.mode==1)x=c.stereoKey ? std::array<float,2>{.5f*(x[0]+x[1]),.5f*(x[0]-x[1])} : std::array<float,2>{x[0],x[0]};
                    if(c.mode==0)x[0]=std::max(std::abs(x[0]),std::abs(x[1]));
                    for(int ch=0;ch<2;++ch)peaks[size_t(ch)]=std::max(peaks[size_t(ch)],std::abs(x[size_t(ch)]));
                }
                const float dbs[]={p.getMeterState().displayExternalKeyDb0.load(),p.getMeterState().displayExternalKeyDb1.load()};
                for(int ch=0;ch<(c.mode==0?1:2);++ch)
                    maxLiveError=std::max(maxLiveError,std::abs(dbs[ch]-juce::Decibels::gainToDecibels(peaks[size_t(ch)],-120.f)));
                // Capture through the real timer; defer asynchronous replay so
                // the live waveform can be checked independently of the worker.
                d.hpfNextReplayMs=std::numeric_limits<double>::max();d.timerCallback();
                d.refreshRenderCaches(c.mode);
                check(std::abs(d.renderCaches[0].projected.externalKey[d.renderCaches[0].projected.size-1]-dbs[0])<.002f,
                      "Live External key curve still uses detector evidence");
                ++livePoints;
            }
            check(maxLiveError<.005f,"Live external curve differs from aligned signal during Window/Safe changes");
            DynamicDisplay::ReplayRequest request;
            request.requestGeneration=d.replayRequestGeneration->load();request.captureGeneration=d.lastCaptureGeneration;
            request.mode=c.mode;request.keySource=1;request.lookaheadMs=26;request.oversampling=c.os;
            request.hpfHz=qqsc::params::keyHpfOffHz;request.abSlot=p.getActiveABSlot();request.limiter=c.limiter;
            for(const auto& point:d.histories[0].points){request.markers.push_back(point.captureCounter);request.carrierDelays.push_back(point.carrierDelaySeconds);}
            request.requestedStartCounter=0;
            DynamicDisplay::ReplayResult baseline;
            float maxDetectorDifference=0,maxOutputDifference=0;
            std::vector<float> baselineOutput;
            for(float window:{0.f,8.f,26.f})for(int safe:{0,1})
            {
                setSound(p,"detectorWindowMs",window);setSound(p,"detectorMode",float(safe));
                request.detectorWindowMs=window;request.detectorMode=window==0?0:safe;
                DynamicDisplay::ReplayResult result;check(d.buildHpfReplay(request,result,worker),"External signal replay failed");
                if(baseline.externalKeyDb0.empty())baseline=result;
                for(size_t i=0;i<result.externalKeyDb0.size();++i)
                {
                    check(result.externalKeyDb0[i]==baseline.externalKeyDb0[i] && result.externalKeyDb1[i]==baseline.externalKeyDb1[i],
                          "Window/Safe changed the external signal waveform");
                    if(std::isfinite(result.detectorDb0[i]) && std::isfinite(baseline.detectorDb0[i]))
                        maxDetectorDifference=std::max(maxDetectorDifference,std::abs(result.detectorDb0[i]-baseline.detectorDb0[i]));
                }
                d.applyHpfReplay(result);d.refreshRenderCaches(c.mode);
                auto& projected=d.renderCaches[0].projected;
                check(projected.size==result.externalKeyDb0.size(),"External replay marker count mismatch");
                for(size_t i=0;i<projected.size;++i)
                    check(std::abs(projected.externalKey[i]-result.externalKeyDb0[i])<.002f,"Replayed External curve still uses detector");
                if(baselineOutput.empty())baselineOutput.assign(projected.output.begin(),projected.output.begin()+projected.size);
                else for(size_t i=0;i<projected.size;++i)maxOutputDifference=std::max(maxOutputDifference,std::abs(projected.output[i]-baselineOutput[i]));
                ++replayCases;
            }
            check(maxDetectorDifference>.1f,"Test failed to exercise distinct Window/Safe detector envelopes");
            check(maxOutputDifference>.01f,"Window/Safe stopped changing the projected compressed result");
            // Neither a new Lookahead nor a different detector OS may move or
            // reshape a retained source waveform. Capture-time delay wins.
            for(float look:{0.f,40.f,100.f})for(int os:{0,3})
            {
                auto changed=request;changed.lookaheadMs=look;changed.detectorWindowMs=0;changed.detectorMode=0;changed.oversampling=os;
                DynamicDisplay::ReplayResult result;check(d.buildHpfReplay(changed,result,worker),"Lookahead/OS key replay failed");
                check(result.externalKeyDb0==baseline.externalKeyDb0 && result.externalKeyDb1==baseline.externalKeyDb1,
                      "Lookahead/OS moved or reshaped retained sidechain signal");++replayCases;
            }
            // Independent JUCE filter oracle on raw host-rate key samples.
            auto coefficients=juce::dsp::IIR::Coefficients<float>::makeHighPass(sr,500.f);
            juce::dsp::IIR::Filter<float> filterL(coefficients),filterR(coefficients);
            std::vector<std::array<float,2>> filtered;filtered.reserve(raw.size());
            for(auto x:raw)
            {
                x[0]=filterL.processSample(x[0]);x[1]=c.stereoKey?filterR.processSample(x[1]):x[0];
                if(c.mode==1)x=c.stereoKey ? std::array<float,2>{.5f*(x[0]+x[1]),.5f*(x[0]-x[1])} : std::array<float,2>{x[0],x[0]};
                filtered.push_back(x);
            }
            setSound(p,"keyHpfHz",500);request.hpfHz=500;
            DynamicDisplay::ReplayResult filteredResult;check(d.buildHpfReplay(request,filteredResult,worker),"Filter replay failed");
            float filterDifference=0;
            for(size_t i=0;i<request.markers.size();++i)
            {
                const int end=int(request.markers[i])-int(std::llround(request.carrierDelays[i]*sr));
                std::array<float,2> peak{};
                for(int at=std::max(0,end-n);at<end;++at)for(int ch=0;ch<2;++ch)
                    peak[size_t(ch)]=std::max(peak[size_t(ch)],std::abs(filtered[size_t(at)][size_t(ch)]));
                if(c.mode==0)peak[0]=std::max(peak[0],peak[1]);
                const float actual[]={filteredResult.externalKeyDb0[i],filteredResult.externalKeyDb1[i]};
                for(int ch=0;ch<(c.mode==0?1:2);++ch)
                    maxOracleError=std::max(maxOracleError,std::abs(actual[ch]-juce::Decibels::gainToDecibels(peak[size_t(ch)],-120.f)));
                filterDifference=std::max(filterDifference,std::abs(actual[0]-baseline.externalKeyDb0[i]));
            }
            check(filterDifference>3.f,"Filter no longer changes external waveform");
            check(maxOracleError<.02f,"Post-Filter waveform does not match independent aligned oracle");
            d.applyHpfReplay(filteredResult);setSound(p,"keyGainDb",6);d.refreshRenderCaches(c.mode);
            for(size_t i=0;i<filteredResult.externalKeyDb0.size();++i)
                check(std::abs(d.renderCaches[0].projected.externalKey[i]-juce::jlimit(-120.f,0.f,filteredResult.externalKeyDb0[i]+6))<.002f,
                      "Key Gain no longer reprojects the external waveform");
            // Stale source/A-B/Safe results must not replace either evidence type.
            for(int wrong=0;wrong<3;++wrong)
            {
                auto stale=filteredResult;if(wrong==0)++stale.request.captureGeneration;
                if(wrong==1)stale.request.abSlot=1-request.abSlot;if(wrong==2)stale.request.detectorMode=1-request.detectorMode;
                const auto before=d.hpfReplayAppliedCount;d.applyHpfReplay(stale);
                check(d.hpfReplayAppliedCount==before,"Stale external replay accepted");
            }
            std::cout<<"KEY mode="<<c.mode<<" stereo="<<c.stereoKey<<" limiter="<<c.limiter<<" os="<<c.os
                     <<" points="<<request.markers.size()<<" detector_delta="<<maxDetectorDifference<<" output_delta="<<maxOutputDifference
                     <<" filter_delta="<<filterDifference<<"\n";
        }
        check(maxAudioDelta==0,"External key display changed audio samples");
        std::cout<<"PASS: external signal display: fixtures=8 live_points="<<livePoints<<" replay_cases="<<replayCases
                 <<" max_live_db_error="<<maxLiveError<<" max_filter_oracle_db_error="<<maxOracleError<<" audio_delta="<<maxAudioDelta<<"\n";
    }
