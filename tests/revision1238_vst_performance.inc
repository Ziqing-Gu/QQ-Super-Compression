static int compareEcoVst(const char* oldPath,const char* newPath)
{
    juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> oldDesc,newDesc;
    format.findAllTypesForFile(oldDesc,juce::String::fromUTF8(oldPath));format.findAllTypesForFile(newDesc,juce::String::fromUTF8(newPath));
    require(oldDesc.size()==1 && newDesc.size()==1,"VST3 comparison descriptions");
    std::cout<<"Actual VST3 comparison: "<<oldDesc[0]->version<<" -> "<<newDesc[0]->version<<"; rate=48000; editor closed; audible signal; transport playing\n";
    struct C{const char* name;bool limiter;float look;int core,ceil;};
    const C configs[]={{"Normal0-1",false,0,0,3},{"Normal0-4",false,0,1,3},{"Normal0-8",false,0,2,3},{"Normal0-16",false,0,3,3},
        {"Normal26",false,26,0,3},{"Limiter26-16",true,26,0,3},{"Limiter0-16shared",true,0,3,3},{"Limiter0-16ceil4",true,0,3,1}};
    std::cout<<"config,block,eco,old_us,new_us,reduction_percent,max_audio_delta,latency\n";
    double allWorst=0;
    for(const auto& c:configs)for(int size:{256,2048})for(bool eco:{false,true})
    {
        juce::String error;auto old=format.createInstanceFromDescription(*oldDesc[0],48000,size,error);require(old!=nullptr,error);
        auto next=format.createInstanceFromDescription(*newDesc[0],48000,size,error);require(next!=nullptr,error);
        SilenceHost host;host.playing=true;
        for(auto* q:{old.get(),next.get()})
        {
            editState(*q,[&](auto& tree)
            {
                const auto put=[&](const juce::String& id,float value){auto v=tree.getChildWithProperty("id",id);require(v.isValid(),id);v.setProperty("value",value,nullptr);};
                for(const juce::String prefix:{juce::String(),juce::String("limiter")})
                {
                    const auto id=[&](const char* s){juce::String n(s);return prefix.isEmpty()?n:prefix+n.substring(0,1).toUpperCase()+n.substring(1);};
                    put(id("lookaheadMs"),c.look);put(id("oversampling"),float(c.core));put(id("keySource"),0);put(id("keyHpfHz"),0);
                    put(id("processingMode"),0);put(id("algorithmMode"),1);put(id("compressionMode"),0);
                    put(id("inputGainDb"),0);put(id("makeupGainDb"),c.limiter?1.36f:23.82f);put(id("mix"),100);
                    put(id("ratio"),52.8f);put(id("thresholdDb"),c.limiter?-16.27f:-120.f);put(id("rangeDb"),1);
                }
                put("limiterMode",c.limiter?1.f:0.f);put("ceilingOversampling",float(c.ceil));put("truePeakLimiting",1);put("tpRecoveryMode",2);
                put("ceilingDb",-.3f);put("limiterOutputDb",14.91f);put("outputGainDb",0);put("bypass",0);
                tree.setProperty("qqscPerformanceEco",eco,nullptr);tree.setProperty("qqscLimiterUnityMonitor",false,nullptr);
                tree.setProperty("qqscLimiterBankInitialised",true,nullptr);
            });
            q->setPlayHead(&host);q->setRateAndBufferSizeDetails(48000,size);q->prepareToPlay(48000,size);
        }
        juce::AudioBuffer<float> source(juce::jmax(2,old->getTotalNumInputChannels()),size),a(source.getNumChannels(),size),b(source.getNumChannels(),size);
        juce::MidiBuffer midi;double oldUs=0,newUs=0,worst=0;
        for(int block=0;block<160;++block)
        {
            source.clear();for(int ch=0;ch<2;++ch)for(int i=0;i<size;++i)
            {
                const double t=double(host.sample+i)/48000.;
                source.setSample(ch,i,float((.5+.49*std::sin(2*juce::MathConstants<double>::pi*.73*t))*std::sin(2*juce::MathConstants<double>::pi*(ch?371:997)*t)));
            }
            a.makeCopyOf(source,true);b.makeCopyOf(source,true);
            const auto run=[&](juce::AudioPluginInstance& q,juce::AudioBuffer<float>& output){const auto t=std::chrono::steady_clock::now();q.processBlock(output,midi);return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t).count();};
            double ta,tb;if(block%2){tb=run(*next,b);ta=run(*old,a);}else{ta=run(*old,a);tb=run(*next,b);}
            if(block>=32){oldUs+=ta;newUs+=tb;}
            for(int ch=0;ch<2;++ch)for(int i=0;i<size;++i){require(std::isfinite(b.getSample(ch,i)),"VST3 nonfinite");worst=std::max(worst,double(std::abs(a.getSample(ch,i)-b.getSample(ch,i))));}
            require(old->getLatencySamples()==next->getLatencySamples(),"VST3 PDC changed");host.sample+=size;
        }
        allWorst=std::max(allWorst,worst);
        std::cout<<c.name<<','<<size<<','<<eco<<','<<oldUs/128<<','<<newUs/128<<','<<100*(1-newUs/oldUs)<<','<<worst<<','<<next->getLatencySamples()<<'\n';
        require(worst<=2.e-6,"1.2.37/1.2.38 VST3 audible output changed");
        for(auto* q:{old.get(),next.get()}){q->releaseResources();q->setPlayHead(nullptr);}
    }
    std::cout<<"PASS actual old/new VST3 output/PDC; max_delta="<<allWorst<<". Plugin CPU timings are not Cubase ASIO-Guard percentages.\n";
    return 0;
}
