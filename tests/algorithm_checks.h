namespace
{
void algorithmStateChecks()
{
    QQSuperCompressionAudioProcessor p;
    check(get(p,"algorithmMode")==0,"New instances must default to Classic");
    auto* last=dynamic_cast<juce::AudioProcessorParameterWithID*>(p.getParameters().getLast());
    check(last && last->paramID=="detectorWindowMs","Window must be appended to host contract");
    check(p.getParameters().indexOf(p.getAPVTS().getParameter("detectorWindowMs"))
        ==p.getParameters().indexOf(p.getAPVTS().getParameter("detectorMode"))+1,
        "Detector host index changed");
    check(p.getParameters().indexOf(p.getAPVTS().getParameter("detectorMode"))
        ==p.getParameters().indexOf(p.getAPVTS().getParameter("ceilingOversampling"))+1,
        "Ceiling OS host index changed");
    check(p.getParameters().indexOf(p.getAPVTS().getParameter("algorithmMode"))
        ==p.getParameters().indexOf(p.getAPVTS().getParameter("inputOutputLink"))+1,"Algorithm legacy index changed");
    check(p.getParameters().indexOf(p.getAPVTS().getParameter("truePeakLimiting"))
        ==p.getParameters().indexOf(p.getAPVTS().getParameter("limiterDownThresholdSDb"))+1,"Legacy Limiter parameter suffix changed");
    set(p,"algorithmMode",1);p.copyAToB();set(p,"algorithmMode",0);
    p.selectABSlot(1);check(get(p,"algorithmMode")==1,"B did not retain Super");
    p.selectABSlot(0);check(get(p,"algorithmMode")==0,"A did not retain Classic");
    juce::MemoryBlock data;p.getStateInformation(data);
    QQSuperCompressionAudioProcessor q;q.setStateInformation(data.getData(),int(data.getSize()));
    check(get(q,"algorithmMode")==0,"Project lost Classic");
    q.selectABSlot(1);check(get(q,"algorithmMode")==1,"Project B lost Super");
    q.getStateInformation(data);auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));
    auto tree=juce::ValueTree::fromXml(*xml);
    tree.removeChild(tree.getChildWithProperty("id","algorithmMode"),nullptr);
    tree.removeProperty("qqscAB_A_algorithmMode",nullptr);tree.removeProperty("qqscAB_B_algorithmMode",nullptr);
    for(int schema:{16,17})
    {
        tree.setProperty("qqscStateSchemaVersion",schema,nullptr);
        tree.removeProperty("qqscCurveVariant",nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);
        q.setStateInformation(data.getData(),int(data.getSize()));
        for(int slot:{0,1})
        {
            q.selectABSlot(slot);
            if(get(q,"algorithmMode")!=float(schema==16))
                std::cerr<<"Migration schema="<<schema<<" slot="<<slot<<" algorithm="<<get(q,"algorithmMode")<<'\n';
            check(get(q,"algorithmMode")==float(schema==16),"Legacy project / AB picked wrong law");
        }
    }
    std::cout<<"PASS: Classic default, appended parameter, A/B, project roundtrip, schema16 Super / schema17 Classic migration.\n";
}

// Independent old rational-law oracle, expressed in double precision.
double superLaw(double p,double r,double lower,double upper,bool dual)
{
    const auto blend=[](double t){t=std::clamp(t,0.0,1.0);return t*t*(3-2*t);};
    if(lower>=upper || p<=lower)return 1;
    if(dual && p>upper)return (1+(std::max(r,1.0)-1)*upper)/(1+(std::max(r,1.0)-1)*p);
    if(p>=upper)return 1;
    if(r>=1)
    {
        if(dual)return 1;
        double g=(1+(r-1)*lower)/(1+(r-1)*p);
        if(std::isfinite(upper)){double w=blend((upper-p)/std::min(upper*.5,(upper-lower)*.5));g=1-w+g*w;}
        return g;
    }
    const auto anchor=std::min(1.0,upper);
    double g=1/(r+(1-r)*p/anchor);
    if(lower>0)g=1+(g-1)*blend((p-lower)/std::min(lower,(anchor-lower)*.5));
    return g;
}
void superAudioChecks()
{
    for(int dual:{0,1})for(int domain:{0,1,2,3})for(float ratio:{qqsc::minimumUpRatio,.125f,1.0f,8.0f,qqsc::normalMaximumDownRatio})
    {
        const float amp=ratio<1?.08f:.65f;
        QQSuperCompressionAudioProcessor p;setupDb(p,dual,domain==0?0:domain==1?2:1,-40,dual?-12.0f:1.0f,ratio);
        set(p,"algorithmMode",1);
        if(dual){set(p,"upAlgorithmMode",1);set(p,"downAlgorithmMode",1);}
        const auto out=render(p,amp,127,false,400,48000,domain==3?-1.0f:1.0f);
        const double expected=superLaw(amp,ratio,.01,dual?std::pow(10.,-12./20):std::numeric_limits<double>::infinity(),dual!=0);
        if(std::abs(rmsGain(out,amp)/expected-1)>=1e-5)std::cout<<"SUPER mismatch dual="<<dual<<" domain="<<domain<<" ratio="<<ratio<<" actual="<<rmsGain(out,amp)<<" expected="<<expected<<"\n";
        check(std::abs(rmsGain(out,amp)/expected-1)<1e-5,"Super audio differs from old law");
        check(std::abs(p.getDynamicsGainForDomain(amp,domain==3?4:domain==2?3:domain)/expected-1)<1e-5,"Super display law differs from audio");
        check(out.latency==1248,"Super changed lookahead latency");
    }
    std::cout<<"PASS: old Super law independent oracle, actual Single/Dual ST/LR/M/S audio and Display, ratios1/200..200.\n";
}
void algorithmFadeChecks()
{
    for(double rate:{44100.,48000.,96000.})for(int os:{-1,0,1,2})for(int dual:{0,1})for(bool up:{false,true})
    {
        QQSuperCompressionAudioProcessor p;
        setupDb(p,dual,0,-40,dual?-12.f:1.f,up?.125f:8.f);
        set(p,"lookaheadMs",os==-1?26.f:0.f);set(p,"oversampling",float(std::max(os,0)));
        if(os==-1)
        {
            p.copyAToB();p.selectABSlot(1);set(p,"algorithmMode",1);
            if(dual){set(p,"upAlgorithmMode",1);set(p,"downAlgorithmMode",1);}
            p.selectABSlot(0);
        }
        const auto selectAlgorithm=[&](float value)
        {
            if(os==-1)p.selectABSlot(value>=.5f?1:0);
            else if(dual){set(p,"upAlgorithmMode",value);set(p,"downAlgorithmMode",value);}
            else set(p,"algorithmMode",value);
        };
        p.setRateAndBufferSizeDetails(rate,64);p.prepareToPlay(rate,64);
        const float x=up?.05f:.6f;
        juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,1);
        auto tick=[&](){b.setSample(0,0,x);b.setSample(1,0,x);p.processBlock(b,midi);return b.getSample(0,0);};
        for(int n=0;n<int(rate*.08);++n)tick();
        const float classic=tick();selectAlgorithm(1);
        for(int n=0;n<int(rate*.03);++n)tick();
        const float super=tick();
        check(std::abs(super-classic)>1e-3,"Fade test lacks contrasting algorithms");
        const int length=int(std::floor(rate*(os==-1?.020:.010)));
        double weight=1,target=1,step=0;int remaining=0;
        float previous=super;double largestStep=0,exactError=0;
        // Start fully Super; reverse again only 3 ms into Classic.
        for(int n=0;n<int(rate*.065);++n)
        {
            if(n==0 || n==int(rate*.003) || n==int(rate*.025))
            {
                target=n==int(rate*.003)?1:0;
                if(os==-1 && n==0){selectAlgorithm(0);selectAlgorithm(1);}
                selectAlgorithm(float(target));
                step=(target-weight)/length;remaining=length;
            }
            if(remaining>0){weight+=step;if(--remaining==0)weight=target;}
            const float value=tick();check(std::isfinite(value),"Algorithm fade non-finite");
            largestStep=std::max(largestStep,double(std::abs(value-previous)));previous=value;
            const double blend=os==-1?weight*weight*(3-2*weight):weight;
            if(os<=0)exactError=std::max(exactError,std::abs(value-((1-blend)*classic+blend*super)));
        }
        if(exactError>=3e-5)std::cerr<<"fade error="<<exactError<<" rate="<<rate<<" os="<<os<<" dual="<<dual<<" up="<<up<<'\n';
        check(exactError<3e-5,"Crossfade not the aligned output blend / reversal discontinuity");
        check(largestStep<std::abs(super-classic)*.02,"Algorithm switch contains abrupt audio jump");
        check(std::abs(previous-classic)<1e-5,"Algorithm fade failed to settle");
        const int latency=p.getLatencySamples();
        set(p,"algorithmMode",1);tick();check(p.getLatencySamples()==latency,"Algorithm changed host PDC");
        p.releaseResources();
    }
    // Both curves are identical at unity: linear crossfade must not boost it.
    QQSuperCompressionAudioProcessor p;setupDb(p,0,0,-40,1,1);p.prepareToPlay(48000,1);
    juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,1);
    for(int n=0;n<6000;++n)
    {
        if(n%73==0)set(p,"algorithmMode",float((n/73)%2));
        b.setSample(0,0,.25f);b.setSample(1,0,.25f);p.processBlock(b,midi);
        if(n>1500)check(std::abs(b.getSample(0,0)-.25f)<1e-7,"Equal-signal algorithm crossfade changes level");
    }
    std::cout<<"PASS: actual 10ms direct algorithm / 20ms complete A/B crossfade, both directions / mid-fade reversal, 44.1/48/96kHz, 26ms and0ms1x/8x/16x, Up/Down Single/Dual, unity and latency.\n";
}
}
