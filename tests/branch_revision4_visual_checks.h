void QQSCVisualCheck::revisionFourChecks(QQSuperCompressionAudioProcessorEditor& e,
                                       QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    const auto require=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
    parameter(p,"compressionMode",1);parameter(p,"domainLink",0);parameter(p,"dualRatioLink",1);
    parameter(p,"keySource",0);parameter(p,"inputGainDb",0);parameter(p,"outputGainDb",0);
    parameter(p,"bypass",0);
    const char* mixes[]={"mix","mixL","mixR","mixM","mixS"};
    const char* makeups[]={"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
    for(int d=0;d<5;++d)
    {
        const int mode=d==0?0:d<3?2:1;
        parameter(p,"processingMode",static_cast<float>(mode));
        parameter(p,qqsc::params::upRatioIds[d],0.125f);parameter(p,qqsc::params::downRatioIds[d],8);
        parameter(p,qqsc::params::upEnabledIds[d],1);parameter(p,qqsc::params::downEnabledIds[d],1);
        parameter(p,mixes[d],100);parameter(p,makeups[d],0);
        p.setBoundaryForDomainDb(true,false,d,-50);p.setBoundaryForDomainDb(true,true,d,-12);
        e.timerCallback();
        auto& up=e.upEnabledButtons[d];auto& down=e.downEnabledButtons[d];
        require(up.isVisible() && down.isVisible() && up.getToggleState() && down.getToggleState(),"Enable buttons missing in domain");
        require(!up.getBounds().intersects(e.mainRatioControls()[d]->getBounds())
                && !down.getBounds().intersects(e.downRatioSliders[d]->getBounds()),"Enable overlaps Ratio control");
        require(!up.getBounds().intersects(e.dualRatioLinkButton.getBounds())
                && !down.getBounds().intersects(e.dualRatioLinkButton.getBounds()),"Enable overlaps Ratio LINK");
        const auto u=p.getAPVTS().getRawParameterValue(qqsc::params::upRatioIds[d])->load();
        const auto dn=p.getAPVTS().getRawParameterValue(qqsc::params::downRatioIds[d])->load();
        const int historyDomain=(d==2 || d==4)?1:0;
        auto& points=e.display.histories[static_cast<size_t>(historyDomain)].points;points.clear();
        for(float level:{-26.0f,-4.0f})
        {DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=level;points.push_back(point);}
        const auto project=[&]()
        {e.display.refreshRenderCaches(mode);return e.display.renderCaches[static_cast<size_t>(historyDomain)].projected;};
        auto both=project();require(both.size==2 && both.effectiveGainReduction[0]<0 && both.effectiveGainReduction[1]>0,"Display fixture did not exercise both branches");
        {
            GestureProbe probe(*p.getAPVTS().getParameter(qqsc::params::upEnabledIds[d]));
            up.onClick();require(probe.events==std::vector<bool>{true,false},"Up enable did not notify a host gesture");
        }
        auto upOff=project();require(up.getButtonText()=="OFF" && std::abs(upOff.effectiveGainReduction[0])<1e-6f
            && upOff.effectiveGainReduction[1]==both.effectiveGainReduction[1],"Up button did not reproject only Boost");
        down.onClick();auto allOff=project();
        require(std::abs(allOff.effectiveGainReduction[1])<1e-6f && allOff.output[0]==allOff.input[0]
                && allOff.output[1]==allOff.input[1],"Both OFF must flatten dynamic gain and align Output with Dry");
        require(p.getAPVTS().getRawParameterValue(qqsc::params::upRatioIds[d])->load()==u
                && p.getAPVTS().getRawParameterValue(qqsc::params::downRatioIds[d])->load()==dn,
                "Enable buttons changed Ratio under LINK");
        up.onClick();down.onClick();
    }
    for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
    {
        e.theme=theme;e.applyTheme();
        const juce::String skin=theme==qqsc::ui::Theme::light?"light":theme==qqsc::ui::Theme::dark?"dark":"classic";
        for(int mode:{0,1,2})
        {
            parameter(p,"processingMode",static_cast<float>(mode));e.timerCallback();
            parameter(p,qqsc::params::upEnabledIds[mode==0?0:mode==1?3:1],0);e.timerCallback();
            e.setSize(1200,800);snapshot(e,dir.getChildFile("rev4-"+skin+"-dual-"+juce::String(mode)+".png"));
            e.setSize(1008,672);snapshot(e,dir.getChildFile("rev4-"+skin+"-compact-"+juce::String(mode)+".png"));
        }
    }
    parameter(p,"compressionMode",0);e.timerCallback();
    for(size_t d=0;d<5;++d)
        require(!e.upEnabledButtons[d].isVisible() && !e.downEnabledButtons[d].isVisible()
                && e.mainRatioControls()[d]->getAlpha()==1.0f,"Single retained Dual enable UI/disabled appearance");
    std::cout<<"PASS: real branch buttons/host gestures in all five domains, independent Display reprojection with LINK, Single visibility, three-theme full/minimum renders.\n";
}
