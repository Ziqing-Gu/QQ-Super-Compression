void QQSCVisualCheck::up1000Checks(QQSuperCompressionAudioProcessorEditor& e,
                                  QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    constexpr float minimum=1.0f/1000;
    const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto raw=[&](const char* id){return p.getAPVTS().getRawParameterValue(id)->load();};
    const auto enter=[&](auto& slider,const char* text)
    {
        slider.showTextBox();
        for(auto* child:slider.getChildren())if(auto* label=dynamic_cast<juce::Label*>(child))
            if(auto* input=label->getCurrentTextEditor())
            {input->setText(text);slider.hideTextBox(false);return;}
        throw std::runtime_error("Ratio numeric editor missing");
    };
    parameter(p,"domainLink",0);parameter(p,"dualRatioLink",0);
    for(int dual:{0,1})for(size_t d=0;d<5;++d)
    {
        parameter(p,"compressionMode",static_cast<float>(dual));
        parameter(p,"processingMode",d==0?0.0f:d<3?2.0f:1.0f);e.timerCallback();
        const auto id=(dual?qqsc::params::upRatioIds:qqsc::params::ratioIds)[d];
        auto& knob=*e.mainRatioControls()[d];
        enter(knob,"1:1000");
        require(std::abs(raw(id)-minimum)<1e-9f && knob.getTextFromValue(knob.getValue())=="1:1000",
                "Real Ratio numeric entry/label does not reach 1:1000");
        enter(knob,"1:8");
        const auto event=mouseEvent(knob);
        knob.mouseDown(event);knob.mouseDrag(event.withNewPosition(event.position+juce::Point<float>(0,300)));knob.mouseUp(event);
        require(std::abs(raw(id)-minimum)<1e-9f,"Native rotary drag does not reach 1:1000");
        if(!dual)require(knob.valueToProportionOfLength(1)==0.5,"Single unity marker moved off centre");
    }
    parameter(p,"compressionMode",1);parameter(p,"processingMode",0);parameter(p,"dualRatioLink",1);
    parameter(p,"upRatio",1);parameter(p,"downRatio",1);e.timerCallback();
    enter(e.ratioSlider,"1:1000");
    require(std::abs(raw("upRatio")-minimum)<1e-9f && std::abs(raw("downRatio")-1000)<0.001f,
            "Original Ratio LINK must reach 1/1000 and 1000 together from unity");
    require(e.dualRatioLinkButton.getButtonText()=="LINK" && e.dualRatioLinkButton.getToggleState(),
            "Finite endpoint must not pause the restored Ratio LINK");
    enter(*e.downRatioSliders[0],"500");
    require(std::abs(raw("upRatio")-0.002f)<1e-8f,"LINK failed when reversing from the high endpoint");
    parameter(p,"upRatio",0.25f);parameter(p,"downRatio",8);enter(*e.downRatioSliders[0],"16");
    require(std::abs(raw("upRatio")-0.125f)<1e-6f,"Restored LINK forced an offset pair to reciprocal values");
    parameter(p,"dualRatioLink",0);e.timerCallback();enter(e.ratioSlider,"1:1000");
    require(std::abs(raw("upRatio")-minimum)<1e-9f,"Unlinking did not release the full Up range");
    parameter(p,"dualRatioLink",1);e.timerCallback();enter(*e.downRatioSliders[0],"8");
    require(std::abs(raw("upRatio")-0.002f)<1e-8f,"Relative LINK lost offset pair near extended minimum");

    parameter(p,"dualRatioLink",0);parameter(p,"keySource",0);parameter(p,"inputGainDb",0);
    parameter(p,"outputGainDb",0);parameter(p,"bypass",0);
    const char* mixes[]={"mix","mixL","mixR","mixM","mixS"};
    const char* makeups[]={"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
    for(size_t d=0;d<5;++d)
    {
        parameter(p,qqsc::params::ratioIds[d],minimum);parameter(p,qqsc::params::upRatioIds[d],minimum);
        parameter(p,qqsc::params::downRatioIds[d],8);parameter(p,mixes[d],100);parameter(p,makeups[d],0);
        parameter(p,qqsc::params::upEnabledIds[d],1);parameter(p,qqsc::params::downEnabledIds[d],1);
        p.setBoundaryForDomainDb(false,false,static_cast<int>(d),-120);p.setBoundaryForDomainDb(false,true,static_cast<int>(d),1);
        p.setBoundaryForDomainDb(true,false,static_cast<int>(d),-120);p.setBoundaryForDomainDb(true,true,static_cast<int>(d),0);
    }
    for(int dual:{0,1})
    {
        parameter(p,"compressionMode",static_cast<float>(dual));parameter(p,"processingMode",0);e.timerCallback();
        auto& points=e.display.histories[0].points;points.clear();
        DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=-100;points.push_back(point);
        e.display.refreshRenderCaches(0);
        const auto& projected=e.display.renderCaches[0].projected;
        const auto expected=20*std::log10(1/(0.001f+0.999f*0.00001f));
        require(projected.size==1 && std::abs(projected.effectiveGainReduction[0]+expected)<0.002
                && std::abs(projected.output[0]-(-100+expected))<0.002,"Display clips high upward gain");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();
            const juce::String skin=theme==qqsc::ui::Theme::light?"light":theme==qqsc::ui::Theme::dark?"dark":"classic";
            for(int mode:{0,1,2})
            {
                parameter(p,"processingMode",static_cast<float>(mode));e.timerCallback();
                for(auto& history:e.display.histories)
                {
                    history.points.clear();
                    for(int i=0;i<480;++i)
                    {DynamicDisplay::HistoryPoint item;item.inputDb=item.detectorDb=-78+35*std::sin(i*0.025f);history.points.push_back(item);}
                }
                e.display.refreshRenderCaches(mode);
                e.setSize(1008,672);snapshot(e,dir.getChildFile("up1000-"+skin+"-"+juce::String(dual)+"-"+juce::String(mode)+".png"));
            }
        }
    }
    std::cout<<"PASS: real 1:1000 numeric/native-drag endpoints across Single/Dual ST/LR/MS; centred unity; relative LINK limits; high-gain Display; 18 compact theme screenshots.\n";
}
