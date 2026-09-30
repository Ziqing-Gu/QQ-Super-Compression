// End-to-end checks: the real processor, final L/R output, independent 16x meter.
void peakGuardProcessorChecks()
{
    int cases=0;
    double worstTp=-200,largestIntersample=0;
    for(int algo:{0,1})for(int mode:{0,1,2})for(bool tp:{false,true})for(float ceiling:{0.f,-24.f})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);
        set(p,"algorithmMode",float(algo));set(p,"processingMode",float(mode));
        set(p,"ratio",1);set(p,"ratioL",1);set(p,"ratioR",1);set(p,"ratioM",1);set(p,"ratioS",1);
        p.enterLimiterMode();set(p,"truePeakLimiting",tp?1.f:0.f);
        set(p,"ceilingDb",ceiling);set(p,"limiterOutputDb",24);set(p,"limiterCalibrationDb",0);
        // Quarter-rate sine has peaks between samples. Limiter must run after
        // Makeup/Mix/Output and M/S decoding, not on a theoretical reference.
        const auto audio=render(p,4,127,false,12000,48000,.7f);
        juce::AudioBuffer<float> b(2,int(audio.output.size()));
        double sp=0;
        for(int i=0;i<b.getNumSamples();++i)
        {
            b.setSample(0,i,audio.output[size_t(i)]);b.setSample(1,i,audio.right[size_t(i)]);
            for(int ch=0;ch<2;++ch)sp=std::max(sp,std::abs(double(b.getSample(ch,i))));
        }
        juce::dsp::Oversampling<float> meter(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false);
        meter.initProcessing(size_t(b.getNumSamples()));
        const auto up=meter.processSamplesUp(juce::dsp::AudioBlock<float>(b));
        double peak=sp;
        for(int ch=0;ch<2;++ch)for(int i=0;i<int(up.getNumSamples());++i)peak=std::max(peak,std::abs(double(up.getSample(ch,i))));
        const double spDb=20*std::log10(sp),tpDb=20*std::log10(peak);
        check(spDb<=ceiling+.002,"Final processor exceeded sample ceiling");
        if(tp){check(tpDb<=ceiling+.01,"Final processor exceeded true-peak ceiling");worstTp=std::max(worstTp,tpDb-ceiling);}
        else largestIntersample=std::max(largestIntersample,tpDb-spDb);
        check(std::abs(p.getMeterState().truePeakHoldDb.load()-tpDb)<.2,"TP display must meter actual final L/R");
        ++cases;
    }
    std::cout<<"PASS: "<<cases<<" actual processor ceiling cases (Classic/Super,ST/LR/MS,TP off/on,0/-24); worst TP relative to ceiling="<<worstTp<<" dB; sample-only intersample excess="<<largestIntersample<<" dB.\n";
    for(bool tp:{false,true})for(bool hostBypass:{false,true})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
        set(p,"ceilingDb",-24);set(p,"truePeakLimiting",tp?1.f:0.f);
        if(!hostBypass)set(p,"bypass",1);
        const auto audio=render(p,1.5f,17,hostBypass,400);
        for(size_t i=0;i<audio.output.size();++i)
        {
            const float expected=i<size_t(audio.latency)?0:1.5f*float(std::sin(2*pi*400*double(i-size_t(audio.latency))/48000));
            check(audio.output[i]==expected && audio.right[i]==expected,"Host/plugin bypass is not an exact latency-aligned copy");
        }
    }
    std::cout<<"PASS: host and plug-in bypass bit-exact from first sample with Ceiling -24 and TP off/on.\n";
    for(bool tp:{false,true})for(double frequency:{20.,1000.})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"ratio",1);p.enterLimiterMode();
        set(p,"truePeakLimiting",tp?1.f:0.f);set(p,"ceilingDb",-1);
        const auto audio=render(p,4,256,false,frequency);
        const auto thd=harmonicDb(audio,frequency);
        std::cout<<"Steady sine "<<frequency<<" Hz TP="<<tp<<" THD(2..7)="<<thd<<" dB\n";
        check(thd < -85,"Unexpected steady-sine harmonic distortion from final guard");
    }
    // OutputCeiling is Limiter-only in 1.2.28. Exercise only the switches that
    // remain internal to the prepared Ceiling pipeline: TP and Ceiling value.
    // Limiter ON/OFF itself is covered above through the real processor, where
    // changing mode is allowed to rebuild host PDC.
    qqsc::OutputCeiling guard;guard.prepare(48000.0,false,-1.0f);
    float last=0;double maxDelta=0;
    const auto begin=juce::Time::getMillisecondCounterHiRes();
    for(int i=0;i<48000*5;++i)
    {
        if(i==48000)guard.set(true,-1.0f);
        if(i==96000)guard.set(false,-1.0f);
        if(i==144000)guard.set(true,-24.0f);
        if(i==192000)guard.set(true,-1.0f);
        const float y=guard.process(2,1)[0];
        if(i>4800)maxDelta=std::max(maxDelta,std::abs(double(y-last)));
        last=y;
    }
    const auto ms=juce::Time::getMillisecondCounterHiRes()-begin;
    check(maxDelta<.015,"TP/Ceiling switching made an abrupt step");
    std::cout<<"PASS: TP/Ceiling transitions inside the fixed Limiter pipeline, max adjacent DC delta="<<maxDelta<<"; 5s stereo guard CPU wall="<<ms<<"ms.\n";
}
