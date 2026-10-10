    static void overallOversamplingChecks(const juce::File&)
    {
        auto render=[](double rate,float look,int os,int detector,int block,bool eco,bool limiter)
        {
            QQSuperCompressionAudioProcessor p;p.setEcoMode(eco);p.setEditorOpen(!eco);
            set(p,"limiterMode",limiter?1.f:0.f);
            auto put=[&](const char* id,float v){set(p,p.soundParameterID(id),v);};
            put("algorithmMode",1);put("ratio",5);put("thresholdDb",-120);put("rangeDb",1);
            if(limiter)for(int d:{1,2}) {set(p,qqsc::params::limiterRatioIds[size_t(d)],5);set(p,qqsc::params::limiterThresholdIds[size_t(d)],-120);set(p,qqsc::params::limiterRangeIds[size_t(d)],1);}
            put("lookaheadMs",look);put("oversampling",float(os));set(p,"distort",100);set(p,"detectorMode",float(detector));
            set(p,"truePeakLimiting",0);
            p.setRateAndBufferSizeDetails(rate,block);p.prepareToPlay(rate,block);
            const int latency=p.getLatencySamples(),length=int(rate*.25)+latency;
            std::vector<float> output(size_t(length)*2);juce::MidiBuffer midi;
            for(int offset=0;offset<length;offset+=block)
            {
                const int n=std::min(block,length-offset);juce::AudioBuffer<float>a(2,n);
                for(int i=0;i<n;++i)
                {
                    const double t=(offset+i)/rate;
                    const float l=float(.31*std::sin(2*juce::MathConstants<double>::pi*713*t)+.2*std::sin(2*juce::MathConstants<double>::pi*9743*t));
                    a.setSample(0,i,l);a.setSample(1,i,-.7f*l);
                }
                p.processBlock(a,midi);
                for(int i=0;i<n;++i)for(int ch=0;ch<2;++ch)
                {const auto y=a.getSample(ch,i);check(std::isfinite(y),"Nonfinite overall OS output");output[size_t((offset+i)*2+ch)]=y;}
            }
            check(p.getEffectiveCeilingOversamplingChoice()==os,"Ceiling not following overall OS");
            return std::make_pair(latency,std::move(output));
        };
        int cases=0;float worst=0;
        for(double rate:{48000.,96000.})for(int os:{0,1,2,3})for(bool limiter:{false,true})
        {
            const auto base=render(rate,0,os,0,257,false,limiter);
            for(float look:{26.f,100.f})
            {
                const auto delayed=render(rate,look,os,1,17,true,limiter);
                const int difference=int(std::round(rate*look*.001));
                check(delayed.first-base.first==difference,"Unified OS Lookahead latency differs from audio delay");
                for(size_t i=0;i<base.second.size();++i)
                    worst=std::max(worst,std::abs(base.second[i]-delayed.second[i+size_t(2*difference)]));
                check(worst<3e-5f,"Zero-window delayed/zero-look, block or ECO audio mismatch");++cases;
            }
        }
        std::cout<<"PASS: "<<cases<<" unified OS fixed-delay/audio checks; 1x/4x/8x/16x, 48/96 kHz, Normal/Limiter, 17/257 blocks, FULL/ECO; worst="<<worst<<".\n";
        // Independent reconstructed peak measurement, including native audio + TP.
        for(double rate:{48000.,96000.})for(int os:{0,1,2,3})
        {
            qqsc::OutputCeiling ceiling;ceiling.prepare(rate,true,-1.f,qqsc::params::tpAuto,os,257);
            const int total=int(rate*.6);juce::AudioBuffer<float> measured(2,total);
            for(int i=0;i<total;++i)
            {
                const double t=i/rate;
                const float carrier=float((i<int(rate*.4)?1.8:0)*std::sin(2*juce::MathConstants<double>::pi*9973*t));
                const auto out=ceiling.process(carrier,-.8f*carrier);
                measured.setSample(0,i,out[0]);measured.setSample(1,i,out[1]);
            }
            juce::dsp::Oversampling<float> analyser(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true);
            analyser.initProcessing(size_t(total));const auto up=analyser.processSamplesUp(juce::dsp::AudioBlock<float>(measured));
            float peak=0;for(int ch=0;ch<2;++ch)for(size_t i=0;i<up.getNumSamples();++i)peak=std::max(peak,std::abs(up.getSample(ch,int(i))));
            const float db=juce::Decibels::gainToDecibels(peak,-120.f);
            std::cout<<"OVERALL_TP rate="<<rate<<" os="<<os<<" peak="<<db<<" dBTP\n";
            check(db<=-.85f,"Unified OS TP exceeded ceiling tolerance");
        }
        std::cout<<"PASS: independent 16x reconstruction confirms TP protection at all four audio OS rates.\n";
    }
