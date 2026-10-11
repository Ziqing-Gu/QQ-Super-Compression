    static void carrierDisplayChecks(const juce::File& root)
    {
        int cases=0;
        for(int mode:{0,1,2})for(bool limiter:{false,true})
        {
            if(limiter && mode!=2)continue;
            QQSuperCompressionAudioProcessor p;p.setEcoMode(false);if(limiter)p.enterLimiterMode();
            auto value=[&](const char* id,float v){set(p,p.soundParameterID(id),v);};
            value("processingMode",float(mode));value("keySource",0);value("compressionMode",0);
            value("inputGainDb",0);value("outputGainDb",0);set(p,"limiterOutputDb",0);
            for(auto id:qqsc::params::thresholdIds)value(id,-30);
            for(auto id:qqsc::params::rangeIds)value(id,qqsc::params::rangeOffDb);
            for(auto id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})
                if(p.getAPVTS().getParameter(p.soundParameterID(id)))value(id,0);
            set(p,"truePeakLimiting",0);set(p,"ceilingDb",0);value("limiterStereoLink",0);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.stopTimer();e.display.stopTimer();
            auto& d=e.display;
            for(int ch=0;ch<2;++ch)for(float input:{-18.f,-65.f,-32.f})
            {
                DynamicDisplay::HistoryPoint point;point.inputDb=input;point.detectorDb=-9;
                point.captureCounter=1000+uint64_t(d.histories[size_t(ch)].points.size())*800;
                d.histories[size_t(ch)].points.push_back(point);
            }
            for(int algorithm:{0,1})for(float ratio:{1.f,4.67f,19.1f})for(float mix:{0.f,74.3f,100.f})
            {
                value("algorithmMode",float(algorithm));for(auto id:qqsc::params::ratioIds)value(id,ratio);
                for(auto id:{"mix","mixL","mixR","mixM","mixS"})value(id,mix);
                d.refreshRenderCaches(mode);
                for(int ch=0;ch<(mode==0?1:2);++ch)
                {
                    const auto& v=d.renderCaches[size_t(ch)].projected;
                    check(v.size==3,"Carrier projection point count");
                    for(size_t i=0;i<v.size;++i)
                    {
                        check(v.output[i]<=v.input[i]+.002f,"Downward compression falsely displays louder than input");
                        check(v.gainReductionBoundary[i]<=v.input[i]+.002f,"Blue trace falsely fills an input valley");
                        if(ratio==1 || mix==0)check(std::abs(v.output[i]-v.input[i])<.002f,"Unity/dry curve no longer coincides with input");
                    }
                    check(std::abs((v.output[0]-v.output[1])-47.f)<.002f,"Output valley depth replaced by detector envelope");
                }
                ++cases;
            }
            // A real fixed boost must remain visible; never fix this by simply
            // clipping all output curves to the white input line.
            for(auto id:qqsc::params::ratioIds)value(id,1);
            for(auto id:{"mix","mixL","mixR","mixM","mixS"})value(id,100);
            value("outputGainDb",6);if(limiter)set(p,"limiterOutputDb",6);
            d.refreshRenderCaches(mode);
            check(std::abs(d.renderCaches[0].projected.output[1]-(-59.f))<.002f,"Real output gain hidden by display fix");
        }
        std::cout<<"PASS: "<<cases<<" carrier projection cases: Classic/Super, Ratio/Mix, ST/LR/MS/Limiter; valleys preserved, unity coincident, real fixed gain retained.\n";
    }
