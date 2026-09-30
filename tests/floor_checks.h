namespace
{
void floorAndLinkChecks()
{
    using E=qqsc::StaticCompressionEngine;
    const double floor=std::pow(10.,-90./20);
    for(float r:{qqsc::minimumUpRatio,.125f,1.f,8.f,11.9f,qqsc::maximumDownRatio})
        for(float db:{-126.f,-100.f,-80.f,-20.f,0.f})
    {
        const float p=std::pow(10.f,db/20);
        double expected=1;
        if(p>floor)expected=r>=1?std::pow(floor/p,1-1/double(r)):std::min(1e6,std::pow(1/double(p),1-double(r)));
        const auto actual=E::singleGainForLevel(p,r,0,std::numeric_limits<float>::infinity());
        check(std::abs(actual/expected-1)<2e-6,"Classic endpoint changed to rational law");
        const auto next=E::singleGainForLevel(p,r,std::pow(10.f,-89.99f/20),std::numeric_limits<float>::infinity());
        check(std::abs(20*std::log10(actual/next))<.0102f,"Classic final .01dB step is discontinuous");
    }
    for(int domain:{0,1,2,3})for(float r:{qqsc::minimumUpRatio,.125f,1.f,8.f,qqsc::normalMaximumDownRatio})
    {
        const float amp=.1f;
        QQSuperCompressionAudioProcessor p;setupDb(p,0,domain==0?0:domain==1?2:1,-120,1,r);
        const auto output=render(p,amp,127,false,400,48000,domain==3?-1.f:1.f);
        const double expected=r>=1?std::pow(floor/amp,1-1/double(r)):std::pow(1/double(amp),1-double(r));
        check(std::abs(rmsGain(output,amp)/expected-1)<.00003,"Classic endpoint actual audio mismatch");
        const auto d=domain==3?4:domain==2?3:domain;
        check(std::abs(p.getDynamicsGainForDomain(amp,d)/expected-1)<.00003,"Classic endpoint Display mismatch");
        check(output.latency==1248,"Classic endpoint changed latency");
    }
    for(int domain:{0,1,2,3})
    {
        QQSuperCompressionAudioProcessor p;setupDb(p,1,domain==0?0:domain==1?2:1,-120,-12,.125f);
        constexpr float amp=.001f;
        const auto output=render(p,amp,256,false,400,48000,domain==3?-1.f:1.f);
        check(std::abs(20*std::log10(rmsGain(output,amp))-48*.875)<.005,"Dual Up minimum gate switched algorithms");
    }
    check(E::dualGainForLevel(.1f,.125f,8,0,0)==1,"Coincident minimum Dual thresholds must disable processing");
    check(E::singleGainForLevel(.1f,8,0,0)==1,"Coincident minimum Single boundaries must disable processing");
    QQSuperCompressionAudioProcessor p;setupDb(p,0,0,-89.99f,1,11.9f);p.prepareToPlay(48000,1);
    juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,1);
    auto tick=[&]{b.setSample(0,0,.1f);b.setSample(1,0,.1f);p.processBlock(b,midi);return b.getSample(0,0);};
    float before=0;for(int n=0;n<4000;++n)before=tick();
    p.setBoundaryForDomainDb(false,false,0,-90);const auto after=tick();
    check(after<before && std::abs(20*std::log10(after/before)+.01f*(1-1/11.9f))<.0001f,"Final fader step jumps or reverses compression");
    QQSuperCompressionAudioProcessor link;
    check(get(link,"inputOutputLink")==1,"I/O Link first-use parameter default must be ON");
    link.initialiseInputOutputLinkPreference(false);check(get(link,"inputOutputLink")==0,"Remembered Link OFF not applied");
    link.initialiseInputOutputLinkPreference(true);check(get(link,"inputOutputLink")==0,"Editor reopen overwrote Link");
    std::cout<<"PASS: Classic -90dB independent oracle / actual audio / Display, final -.01dB fader step, Single ST/LR/M/S, Dual Up and collision, latency, I/O Link ON default and one-time preference.\n";
}
}
