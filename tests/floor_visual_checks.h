void QQSCVisualCheck::floorChecks(QQSuperCompressionAudioProcessorEditor& e,
                                  QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    parameter(p,"inputGainDb",0);parameter(p,"outputGainDb",0);parameter(p,"makeupGainDb",0);
    parameter(p,"mix",100);parameter(p,"ratio",11.9f);parameter(p,"processingMode",0);parameter(p,"compressionMode",0);
    p.setBoundaryForDomainDb(false,false,0,-120);p.setBoundaryForDomainDb(false,true,0,1);
    for(int algorithm:{0,1})
    {
        parameter(p,"algorithmMode",float(algorithm));e.timerCallback();
        for(size_t d=0;d<5;++d)
        {
            require(e.lowerBoundaryControls()[d]->getMinimum()==(algorithm==0?-90:-120),"Threshold fader minimum does not follow algorithm");
            require(e.upperBoundarySliders[d]->getMinimum()==(algorithm==0?-90:-120),"Upper fader minimum does not follow algorithm");
        }
        require(p.getBoundaryForDomainDb(false,false,0)==-120,"Algorithm switch rewrote stored Super -inf threshold");
        auto& points=e.display.histories[0].points;points.clear();
        DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=-20;points.push_back(point);
        e.display.refreshRenderCaches(0);
        const auto expected=algorithm==0?-90+70/11.9:-20-20*std::log10(1+10.9*.1);
        require(std::abs(e.display.renderCaches[0].projected.output[0]-expected)<.002,"Display reverted to Super at Classic floor");
        require(e.lowerBoundaryValues[0].getText()==(algorithm==0?"-90.00 dB":"OFF"),"Visible threshold floor label wrong");
        for(auto ids:{qqsc::params::thresholdIds,qqsc::params::upThresholdIds,qqsc::params::downThresholdIds})
            for(auto id:ids)
                require(p.getAPVTS().getParameter(id)->getText(0,128).contains(algorithm==0?"-90":"-inf"),"Host floor text wrong");
        require(p.getAPVTS().getParameter("rangeDb")->getText(1,128)=="OFF","Range OFF changed");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();e.setSize(1008,672);
            snapshot(e,dir.getChildFile("Floor-"+juce::String(int(theme))+(algorithm?"-Super.png":"-Classic.png")));
        }
    }
    parameter(p,"algorithmMode",0);parameter(p,"processingMode",2);parameter(p,"domainLink",1);e.timerCallback();
    p.setBoundaryForDomainDb(false,false,1,-120);p.setBoundaryForDomainDb(false,false,2,-80);e.timerCallback();
    e.thresholdLSlider.mouseDown(mouseEvent(e.thresholdLSlider));
    e.thresholdLSlider.setValue(-89,juce::sendNotificationSync);e.thresholdLSlider.mouseUp(mouseEvent(e.thresholdLSlider));
    require(std::abs(p.getBoundaryForDomainDb(false,false,2)+79)<.02,"Classic minimum still behaves like unlinked -inf sentinel");
    parameter(p,"compressionMode",1);e.timerCallback();
    for(int d=0;d<5;++d){p.setBoundaryForDomainDb(true,false,d,-120);p.setBoundaryForDomainDb(true,true,d,0);}
    e.timerCallback();
    for(int d=0;d<5;++d)require(e.lowerBoundaryValues[size_t(d)].getText().contains("-90"),"Dual Classic gate minimum readout wrong");
    std::cout<<"PASS: Classic -90dB labels and projected audio, Super original endpoint, Range OFF, all host domains, Classic LR relative Link from minimum, Dual gates, three skins.\n";

    // Exercise persistent settings with a private file, including reopen and
    // project precedence. Never write the user's application preferences.
    const auto file=dir.getNonexistentChildFile("input-output-link-preference-test",".settings");
    const auto settings=[&]{juce::PropertiesFile::Options options;options.storageFormat=juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile>(file,options);};
    juce::MemoryBlock savedOff;
    {
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor editor(fresh,settings());
        require(editor.inputOutputLinkButton.getToggleState(),"First-use I/O Link must be ON");
        editor.inputOutputLinkButton.onClick();fresh.getStateInformation(savedOff);
        require(!settings()->getBoolValue("inputOutputLink",true),"Link OFF not persisted");
    }
    {
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor editor(fresh,settings());
        require(!editor.inputOutputLinkButton.getToggleState(),"New instance lost last OFF choice");
        editor.inputOutputLinkButton.onClick();
        require(settings()->getBoolValue("inputOutputLink",false),"Link ON not persisted");
    }
    {
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor editor(fresh,settings());
        require(editor.inputOutputLinkButton.getToggleState(),"New instance lost last ON choice");
    }
    {
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(savedOff.getData(),int(savedOff.getSize()));
        for(int reopen=0;reopen<2;++reopen)
        {
            QQSuperCompressionAudioProcessorEditor editor(restored,settings());
            require(!editor.inputOutputLinkButton.getToggleState(),"Remembered Link overwrote project or reopened editor");
            require(settings()->getBoolValue("inputOutputLink",false),"Project recall overwrote global Link choice");
        }
    }
    std::cout<<"PASS: I/O Link persistent first-use ON, OFF/ON across new instances, project priority, editor reopen, independent stored preference.\n";
    parameter(p,"algorithmMode",0);parameter(p,"processingMode",0);parameter(p,"compressionMode",0);
    parameter(p,"ratio",1000);parameter(p,"makeupGainDb",69.93f);
    p.setBoundaryForDomainDb(false,false,0,-120);p.setBoundaryForDomainDb(false,true,0,1);e.timerCallback();
    auto& points=e.display.histories[0].points;points.clear();
    DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=-20;points.push_back(point);
    e.display.refreshRenderCaches(0);
    require(std::abs(e.display.renderCaches[0].projected.output[0]+20)<.002,"Deep MATCH output projection wrong");
    require(std::abs(e.makeupSTSlider.getValue()-69.93)<.002 && e.makeupSTSlider.getMaximum()==120,"Extended Makeup knob attachment wrong");
    for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
    {e.theme=theme;e.applyTheme();e.setSize(1008,672);snapshot(e,dir.getChildFile("Deep-Match-"+juce::String(int(theme))+".png"));}
    std::cout<<"PASS: +69.93dB Makeup actual knob and Display restore -20dB reference, three skins.\n";

    const auto algoFile=dir.getNonexistentChildFile("algorithm-preference-test",".settings");
    const auto algoSettings=[&]{juce::PropertiesFile::Options options;options.storageFormat=juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile>(algoFile,options);};
    juce::MemoryBlock savedSuper;
    {
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor editor(fresh,algoSettings());
        require(fresh.isClassicAlgorithm(),"First-use algorithm must be Classic");
        editor.algorithmButton.onClick();fresh.getStateInformation(savedSuper);
        require(!fresh.isClassicAlgorithm() && algoSettings()->getIntValue("lastAlgorithmMode",-1)==1,"Super choice not persisted");
    }
    {
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor editor(fresh,algoSettings());
        require(!fresh.isClassicAlgorithm(),"New instance lost remembered Super");
        fresh.selectABSlot(1);require(!fresh.isClassicAlgorithm(),"Fresh B bank did not inherit remembered algorithm");
        editor.algorithmButton.onClick();
        require(fresh.isClassicAlgorithm() && algoSettings()->getIntValue("lastAlgorithmMode",-1)==0,"Classic choice not persisted");
    }
    {
        QQSuperCompressionAudioProcessor fresh;QQSuperCompressionAudioProcessorEditor editor(fresh,algoSettings());
        require(fresh.isClassicAlgorithm(),"New instance lost remembered Classic");
    }
    {
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(savedSuper.getData(),int(savedSuper.getSize()));
        for(int reopen=0;reopen<2;++reopen)
        {
            QQSuperCompressionAudioProcessorEditor editor(restored,algoSettings());
            require(!restored.isClassicAlgorithm(),"Global algorithm overwrote saved project / reopened editor");
            restored.selectABSlot(1);require(restored.isClassicAlgorithm(),"Remembered algorithm overwrote B bank");
            restored.selectABSlot(0);require(!restored.isClassicAlgorithm(),"Remembered algorithm overwrote A bank");
            require(algoSettings()->getIntValue("lastAlgorithmMode",-1)==0,"Project or A/B recall changed global algorithm preference");
        }
    }
    std::cout<<"PASS: remembered Classic/Super across new instances, first-use Classic, fresh A/B inheritance, saved-project and A/B priority, editor reopen, no recall writes to global preference.\n";
}
