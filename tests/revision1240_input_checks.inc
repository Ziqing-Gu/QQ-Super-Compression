static void inputDiagnostics1240()
{
    QQSuperCompressionAudioProcessor p;auto layout=p.getBusesLayout();layout.inputBuses.set(1,juce::AudioChannelSet::stereo());
    check(p.setBusesLayout(layout),"input diagnostics buses");configure(p,0,0,0,false);p.setEcoMode(false);p.setEditorOpen(false);
    Transport host;p.setPlayHead(&host);p.prepareToPlay(48000,1024);
    juce::AudioBuffer<float> b(4,1024);juce::MidiBuffer midi;
    const auto fill=[&](float main,float key){b.clear();b.setSample(0,2,main);b.setSample(1,7,-main);b.setSample(2,13,key);};
    for(int i=0;i<2;++i){fill(1.e-20f,2.e-17f);p.processBlock(b,midi);}
    auto& closed=p.performanceWindows[0];
    check(closed.mainPeakMin.load()==1.e-20f && closed.mainPeakMax.load()==1.e-20f,"raw tiny main peak rounded away");
    check(closed.externalPeakMax.load()==0.f && closed.selectedKey.load()==0,"unused SC reported as selected input");
    check(closed.zeroBlocks.load()==0 && closed.sleepBlocks.load()==0,"measurement changed strict-zero behavior");
    set(p,p.soundParameterID("keySource"),1);fill(1.e-20f,2.e-17f);p.processBlock(b,midi);
    check(closed.externalPeakMin.load()==2.e-17f && closed.externalPeakMax.load()==2.e-17f,"selected SC peak missing");
    check(closed.selectedKey.load()==1 && closed.externalChannels.load()==2,"selected SC configuration missing");
    const auto count=closed.stoppedInputBlocks.load();host.playing=true;fill(.1f,.2f);p.processBlock(b,midi);
    check(closed.stoppedInputBlocks.load()==count && closed.mainPeakMax.load()==1.e-20f,"playback contaminated stopped input statistics");
    const auto text=p.getPerformanceDiagnostics();check(text.contains("1.000000e-20 (-400.0 dBFS)") && text.contains("2.000000e-17"),"tiny values lost in report formatting");
    p.setEditorOpen(true);host.playing=false;fill(0,0);p.processBlock(b,midi);
    check(closed.mainPeakMax.load()==1.e-20f && p.getPerformanceDiagnostics().contains("FULL closed"),"editor erased closed input evidence");
    check(p.performanceWindows[1].mainPeakMax.load()==0 && p.performanceWindows[1].nonFiniteSamples.load()==0,"phase reset or zero measurement failed");
    uint64_t invalid=0;b.clear();b.setSample(0,0,std::numeric_limits<float>::quiet_NaN());b.setSample(1,0,std::numeric_limits<float>::infinity());b.setSample(0,5,.25f);
    check(p.measureInputPeak(b,2,invalid)==.25f && invalid==2,"nonfinite scanner hides invalid input");
    p.setPlayHead(nullptr);
    std::cout<<"PASS stopped raw input peak, INT/EXT isolation, scientific dB display, no playback contamination, phase retention, zero/nonfinite handling; no sample gate introduced\n";
}
