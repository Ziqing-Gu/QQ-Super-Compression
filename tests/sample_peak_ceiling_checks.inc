    static void samplePeakCeilingChecks(const juce::File& root,bool baseline=false)
    {
        // Capture the unchanged TP path before and after the SP implementation.
        juce::MemoryBlock tpAudio;
        for(double rate:{44100.,48000.,96000.})for(int os=0;os<4;++os)for(int recovery=0;recovery<3;++recovery)
        {
            qqsc::OutputCeiling tp;tp.prepare(rate,true,-1.f,recovery,os);tp.setStereoLink(.35f);
            for(int i=0;i<int(rate*.15);++i)
            {
                float t=float(i/rate),amp=i<int(rate*.05)?2.f:.2f;
                auto out=tp.process(amp*std::sin(t*6283.1853f)+.7f*std::sin(t*91721.f),amp*std::sin(t*3511.f));
                tpAudio.append(out.data(),sizeof(float)*2);
            }
        }
        check(root.getChildFile("tp-reference.bin").replaceWithData(tpAudio.getData(),tpAudio.getSize()),"Cannot save TP evidence");
        if(baseline){std::cout<<"PASS: captured TP baseline.\n";return;}

        // The expanded 1x stress check exposed TP overshoot at 44.1 kHz.
        // Keep the exact offending Nyquist-near tone/noise fixtures in the gate.
        for(int recovery=0;recovery<3;++recovery)for(int shape:{4,7})
        {
            constexpr double rate=44100;const int count=int(rate*.8);
            qqsc::OutputCeiling cap;cap.prepare(rate,true,-1.f,recovery,0);
            juce::AudioBuffer<float> audio(2,count);
            std::mt19937 rng(137);std::uniform_real_distribution<float> random(-1,1);
            for(int i=0;i<count;++i)
            {
                const double t=i/rate;
                const float x=shape==4 ? float(std::sin(juce::MathConstants<double>::twoPi*t*rate*.49+.785398)) : random(rng);
                const float amp=t<.15?.03f:t<.45?4.f:.05f;
                auto y=cap.process(amp*x,amp*x*(i%37<18?-.8f:.6f));
                for(int ch=0;ch<2;++ch)audio.setSample(ch,i,y[size_t(ch)]);
            }
            juce::dsp::Oversampling<float> meter(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false);
            meter.initProcessing(size_t(count));auto up=meter.processSamplesUp(juce::dsp::AudioBlock<float>(audio));
            float peak=0;for(int ch=0;ch<2;++ch)for(int i=0;i<int(up.getNumSamples());++i)peak=std::max(peak,std::abs(up.getSample(ch,i)));
            check(juce::Decibels::gainToDecibels(peak)<=-.999f,"Native TP final reconstruction exceeds Ceiling");
        }
        std::cout<<"PASS: 6 native TP final-reconstruction guards across all recovery modes.\n";

        std::array<float,3> tailByMode {};
        for(int recovery=0;recovery<3;++recovery)
        {
            qqsc::OutputCeiling cap;cap.prepare(48000,false,0.f,recovery,0);
            for(int i=0;i<12000;++i)
            {
                const float v=i<1920?2.f:.25f;auto out=cap.process(v,v);
                if(i==4800+cap.latencySamples())tailByMode[size_t(recovery)]=out[0];
            }
        }
        check(tailByMode[qqsc::params::tpTight]>tailByMode[qqsc::params::tpAuto]+.005f
              && tailByMode[qqsc::params::tpAuto]>tailByMode[qqsc::params::tpSmooth]+.005f,
              "SP Tight/Auto/Smooth do not produce distinct recovery");

        int cases=0;juce::String csv="rate,os,recovery,link,max_sample,tail_gain\n";
        for(double rate:{44100.,48000.,96000.})for(int os=0;os<4;++os)for(int recovery=0;recovery<3;++recovery)for(float link:{0.f,1.f})
        {
            qqsc::OutputCeiling sp;sp.prepare(rate,false,-1.f,recovery,os);sp.setStereoLink(link);
            float peak=0,tail=0;const int tailAt=int(rate*.05)+sp.latencySamples();
            for(int i=0;i<int(rate*.20);++i)
            {
                float left=i<int(rate*.04)?2.f:.25f;
                float right=.2f*std::sin(float(i/rate)*6031.86f);
                if(i==int(rate*.12))left=-4.f;
                auto out=sp.process(left,right);
                peak=std::max(peak,std::max(std::abs(out[0]),std::abs(out[1])));
                if(i==tailAt)tail=out[0]/.25f;
                check(std::isfinite(out[0]) && std::isfinite(out[1]),"SP output is not finite");
            }
            check(peak<=juce::Decibels::decibelsToGain(-1.f)+.00001f,"Sample Peak Limiter exceeded Ceiling");
            check(tail>.05f && tail<.95f,"TP OFF still hard-clips: no gain recovery after overload");
            csv+=juce::String(rate)+","+juce::String(os)+","+juce::String(recovery)+","+juce::String(link)+","+juce::String(peak,8)+","+juce::String(tail,8)+"\n";
            ++cases;
        }
        // The native samples are 0.8, but their reconstructed sine peak is 1.131.
        // SP must leave them alone; TP must reduce the inter-sample overshoot.
        for(int recovery=0;recovery<3;++recovery)
        {
            qqsc::OutputCeiling sp,tp;sp.prepare(48000,false,0.f,recovery,0);tp.prepare(48000,true,0.f,recovery,0);
            float spPeak=0,tpPeak=0;
            for(int i=0;i<24000;++i)
            {float v=(i%4<2?.8f:-.8f);auto a=sp.process(v,v),b=tp.process(v,v);
             if(i>12000){spPeak=std::max(spPeak,std::abs(a[0]));tpPeak=std::max(tpPeak,std::abs(b[0]));}}
            check(std::abs(spPeak-.8f)<.00001f,"SP is incorrectly using reconstructed true peaks");
            check(tpPeak<.72f && tpPeak>.65f,"TP no longer limits the known inter-sample overshoot");
        }
        QQSuperCompressionAudioProcessor p;p.enterLimiterMode();set(p,"truePeakLimiting",0);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.stopTimer();
        for(int recovery:{qqsc::params::tpAuto,qqsc::params::tpTight,qqsc::params::tpSmooth})
        {
            set(p,"tpRecoveryMode",float(recovery));sync(p,e);
            check(e.tpRecoveryButton.isVisible() && e.tpRecoveryButton.isEnabled(),"Recovery selector unavailable with TP OFF");
            auto name=recovery==qqsc::params::tpAuto?"AUTO":recovery==qqsc::params::tpTight?"TIGHT":"SMOOTH";
            check(e.tpRecoveryButton.getButtonText()==name,"SP recovery label incorrect");
            juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
            restored.setStateInformation(state.getData(),int(state.getSize()));
            check(!restored.isTruePeakSelected() && restored.getTpRecoveryMode()==recovery,"SP/recovery did not survive project save");
        }
        auto put=[&](const char* id,float v){set(p,p.soundParameterID(id),v);};
        put("limiterLink",0);put("inputGainDb",0);put("outputGainDb",0);put("mixL",0);put("limiterStereoLink",0);
        set(p,"ceilingDb",0);e.display.clearHistories();
        DynamicDisplay::HistoryPoint hit;hit.inputDb=3;hit.detectorDb=3;hit.truePeakExcessDb=4;
        auto tail=hit;tail.inputDb=-12;tail.detectorDb=-12;tail.truePeakExcessDb=0;
        e.display.histories[0].points={hit,tail,tail};
        std::array<float,3> displayTail {};
        for(int recovery=0;recovery<3;++recovery)
        {
            set(p,"tpRecoveryMode",float(recovery));e.display.refreshRenderCaches(2);
            const auto& view=e.display.renderCaches[0].projected;
            check(view.size==3 && std::abs(view.output[0])<.002f,"SP Display uses true-peak excess or misses sample ceiling");
            displayTail[size_t(recovery)]=view.output[1];
            check(view.output[1]<-12.05f,"SP Display lost recovery after hit");
            e.truePeakButton.onClick();sync(p,e);
            check(p.isTruePeakSelected() && p.getTpRecoveryMode()==recovery,"Enabling TP resets recovery");
            e.display.refreshRenderCaches(2);
            check(e.display.renderCaches[0].projected.output[0]<-3.9f,"TP Display ignores intersample evidence");
            e.truePeakButton.onClick();sync(p,e);
            check(!p.isTruePeakSelected() && p.getTpRecoveryMode()==recovery,"Disabling TP resets recovery");
        }
        check(displayTail[0]>displayTail[1]+.05f && displayTail[1]>displayTail[2]+.05f,"SP Display recovery ordering");
        image(e,root.getChildFile("sample-peak-smooth.png"));
        root.getChildFile("sample-peak.csv").replaceWithText(csv);
        std::cout<<"PASS: "<<cases<<" SP peak/recovery cases, 3 analytic SP-vs-TP cases, TP OFF recovery controls and state.\n";
    }
