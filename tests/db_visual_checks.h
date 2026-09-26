void QQSCVisualCheck::dbComparisonChecks(QQSuperCompressionAudioProcessorEditor& e,
                                       QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    parameter(p,"processingMode",0);parameter(p,"compressionMode",0);
    parameter(p,"inputGainDb",0);parameter(p,"outputGainDb",0);parameter(p,"makeupGainDb",0);
    parameter(p,"mix",100);parameter(p,"inputOutputLink",0);parameter(p,"dualRatioLink",0);
    parameter(p,"keySource",0);parameter(p,"keyHpfHz",0);parameter(p,"bypass",0);
    parameter(p,"lookaheadMs",26);
    p.setBoundaryForDomainDb(false,false,0,-10);p.setBoundaryForDomainDb(false,true,0,1);
    parameter(p,"ratio",5);e.timerCallback();
    const auto checkPoint=[&](float input,float wantedOutput)
    {
        auto& points=e.display.histories[0].points;points.clear();
        DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=input;points.push_back(point);
        e.display.refreshRenderCaches(0);
        const auto& projected=e.display.renderCaches[0].projected;
        if(projected.size!=1 || std::abs(projected.output[0]-wantedOutput)>.002f)
            throw std::runtime_error("Fixed-dB Display does not match audible transfer");
    };
    checkPoint(0,-8);checkPoint(-12,-12);
    // A finite deep gate must preserve the new dB boost above the old 60dB cap.
    p.setBoundaryForDomainDb(false,false,0,-110);parameter(p,"ratio",.125f);e.timerCallback();
    checkPoint(-100,-12.5f);
    parameter(p,"ratio",1000);e.timerCallback();checkPoint(0,-109.89f);
    for(int dual:{0,1})
    {
        parameter(p,"compressionMode",static_cast<float>(dual));
        p.setBoundaryForDomainDb(false,false,0,-24);p.setBoundaryForDomainDb(false,true,0,1);
        p.setBoundaryForDomainDb(true,false,0,-48);p.setBoundaryForDomainDb(true,true,0,-18);
        parameter(p,"ratio",5);parameter(p,"upRatio",.125f);parameter(p,"downRatio",8);
        parameter(p,"upEnabled",1);parameter(p,"downEnabled",1);e.timerCallback();
        auto& points=e.display.histories[0].points;points.clear();
        for(int i=0;i<480;++i)
        {
            DynamicDisplay::HistoryPoint point;
            point.inputDb=point.detectorDb=-27+22*std::sin(i*.031f);points.push_back(point);
        }
        e.display.refreshRenderCaches(0);e.setSize(1008,672);
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();
            const juce::String skin=theme==qqsc::ui::Theme::light?"light":theme==qqsc::ui::Theme::dark?"dark":"classic";
            snapshot(e,dir.getChildFile("db-"+skin+"-"+(dual?"dual":"single")+".png"));
        }
    }
    std::cout<<"PASS: fixed-dB Display 0->-8dB at5:1/-10dB, sub-threshold unity, +87.5dB deep Up; formal-name three-theme Single/Dual snapshots.\n";
}
