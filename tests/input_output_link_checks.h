void QQSCVisualCheck::inputOutputLinkChecks(QQSuperCompressionAudioProcessorEditor& e,
                                          QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    const auto require=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto raw=[&](const char* id){return p.getAPVTS().getRawParameterValue(id)->load();};
    const auto pair=[&](float input,float output)
    {require(std::abs(raw("inputGainDb")-input)<0.011f && std::abs(raw("outputGainDb")-output)<0.011f,
             "Input/Output equal-opposite dB link or relative offset is wrong");};
    const auto drag=[&](auto& slider,double value)
    {const auto event=mouseEvent(slider);slider.mouseDown(event);slider.setValue(value,juce::sendNotificationSync);slider.mouseUp(event);};
    const auto enter=[&](auto& slider,const char* value)
    {
        slider.showTextBox();
        for(auto* child:slider.getChildren())if(auto* label=dynamic_cast<juce::Label*>(child))
            if(auto* input=label->getCurrentTextEditor())
            {input->setText(value);slider.hideTextBox(false);return;}
        throw std::runtime_error("Gain text editor missing");
    };
    parameter(p,"inputOutputLink",0);e.timerCallback(); // Isolate toggle/drag tests from remembered preference.
    parameter(p,"processingMode",0);parameter(p,"inputGainDb",2);parameter(p,"outputGainDb",-4);e.timerCallback();
    {
        GestureProbe touch(*p.getAPVTS().getParameter("inputOutputLink"));e.inputOutputLinkButton.onClick();
        require(raw("inputOutputLink")==1 && touch.events==std::vector<bool>{true,false},"I/O LINK toggle host gesture missing");
        pair(2,-4);
    }
    {
        GestureProbe input(*p.getAPVTS().getParameter("inputGainDb"));
        GestureProbe output(*p.getAPVTS().getParameter("outputGainDb"));
        drag(e.inputGainSlider,5);pair(5,-7);
        require(input.events==std::vector<bool>{true,false} && output.events==std::vector<bool>{true,false},"I/O linked drag host gestures unbalanced");
    }
    drag(e.outputGainSlider,-10);pair(8,-10);enter(e.inputGainSlider,"11 dB");pair(11,-13);
    enter(e.outputGainSlider,"-9 dB");pair(7,-9);
    const auto alt=mouseEvent(e.inputGainSlider,true);e.inputGainSlider.mouseDown(alt);e.inputGainSlider.mouseUp(alt);pair(0,-2);
    parameter(p,"inputGainDb",23);parameter(p,"outputGainDb",-23.75f);
    drag(e.inputGainSlider,24);pair(23.25f,-24);drag(e.inputGainSlider,22);pair(22,-22.75f);
    enter(e.outputGainSlider,"-24");pair(23.25f,-24);
    e.inputOutputLinkButton.onClick();drag(e.inputGainSlider,3);pair(3,-24);
    e.inputOutputLinkButton.onClick();pair(3,-24);
    // Automation/state recall must not cause an editor-dependent companion write.
    parameter(p,"inputGainDb",6);pair(6,-24);
    for(int mode:{0,1,2})
    {
        parameter(p,"processingMode",static_cast<float>(mode));parameter(p,"domainLink",1);parameter(p,"dualRatioLink",1);e.timerCallback();
        parameter(p,"inputGainDb",2);parameter(p,"outputGainDb",-4);drag(e.outputGainSlider,-5);pair(3,-5);
        require(e.inputOutputLinkButton.getWidth()==(mode==0?36:30) && e.inputOutputLinkButton.getHeight()==(mode==0?17:14),"I/O LINK compact size incorrect");
        require(!e.inputOutputLinkButton.getBounds().intersects(e.inputGainLabel.getBounds())
                && !e.inputOutputLinkButton.getBounds().intersects(e.inputGainSlider.getBounds()),"I/O LINK overlaps Input control");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();e.setSize(1008,672);
            const juce::String skin=theme==qqsc::ui::Theme::light?"light":theme==qqsc::ui::Theme::dark?"dark":"classic";
            snapshot(e,dir.getChildFile("io-link-"+skin+"-"+juce::String(mode)+".png"));
        }
    }
    // Closing/rebinding during a drag releases both host touches.
    {
        GestureProbe input(*p.getAPVTS().getParameter("inputGainDb"));
        GestureProbe output(*p.getAPVTS().getParameter("outputGainDb"));
        e.inputGainSlider.mouseDown(mouseEvent(e.inputGainSlider));e.finishCompressionControlGestures();
        require(input.events==std::vector<bool>{true,false} && output.events==std::vector<bool>{true,false},"I/O unfinished gesture leaked host touches");
    }
    juce::MemoryBlock saved;p.getStateInformation(saved);
    parameter(p,"inputOutputLink",0);p.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));e.timerCallback();
    require(e.inputOutputLinkButton.getToggleState(),"I/O LINK did not restore into visible control");
    p.copyAToB();p.selectABSlot(1);require(raw("inputOutputLink")==1,"A/B changed shared I/O LINK");
    auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()));
    require(xml!=nullptr,"Cannot decode state");auto legacy=juce::ValueTree::fromXml(*xml);
    legacy.setProperty("qqscStateSchemaVersion",14,nullptr);
    for(int i=legacy.getNumChildren();--i>=0;)if(legacy.getChild(i).getProperty("id")==juce::var("inputOutputLink"))legacy.removeChild(i,nullptr);
    xml=legacy.createXml();juce::AudioProcessor::copyXmlToBinary(*xml,saved);
    p.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));e.timerCallback();
    require(raw("inputOutputLink")==0 && !e.inputOutputLinkButton.getToggleState(),"Legacy project inherited stale I/O LINK");
    std::cout<<"PASS: Input/Output LINK opposite dB, relative offsets, both directions, text/Alt reset, shared limits, balanced host gestures, no toggle jumps, automation independence, ST/LR/MS, project/A-B/legacy state and compact theme snapshots.\n";
}
