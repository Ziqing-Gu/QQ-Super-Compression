void QQSCVisualCheck::revisionThreeChecks(QQSuperCompressionAudioProcessorEditor& editor,
                                         QQSuperCompressionAudioProcessor& processor, const juce::File& dir)
{
    const auto require=[](bool ok,const char* message) { if(!ok) throw std::runtime_error(message); };
    const auto raw=[&](const char* id) { return processor.getAPVTS().getRawParameterValue(id)->load(); };
    const auto dragTo=[&](auto& slider,double value)
    {
        const auto event=mouseEvent(slider); slider.mouseDown(event);
        slider.setValue(value,juce::sendNotificationSync); slider.mouseUp(event);
    };
    const auto enterText=[&](auto& slider,const char* value)
    {
        slider.showTextBox();
        for(auto* child:slider.getChildren())
            if(auto* label=dynamic_cast<juce::Label*>(child))
                if(auto* input=label->getCurrentTextEditor())
                { input->setText(value); slider.hideTextBox(false); return; }
        throw std::runtime_error("Ratio text box did not open");
    };
    parameter(processor,"compressionMode",1); parameter(processor,"domainLink",0);
    parameter(processor,"dualRatioLink",1); editor.timerCallback();
    for(int d=0;d<5;++d)
    {
        parameter(processor,"processingMode",static_cast<float>(d==0?0:d<3?2:1)); editor.timerCallback();
        const auto i=static_cast<size_t>(d);
        const auto* upId=qqsc::params::upRatioIds[i]; const auto* downId=qqsc::params::downRatioIds[i];
        auto& up=*editor.mainRatioControls()[i]; auto& down=*editor.downRatioSliders[i];
        parameter(processor,upId,1); parameter(processor,downId,1);
        {
            GestureProbe upGesture(*processor.getAPVTS().getParameter(upId));
            GestureProbe downGesture(*processor.getAPVTS().getParameter(downId));
            dragTo(down,2);
            require(std::abs(raw(upId)-0.5f)<1e-6 && std::abs(raw(downId)-2)<1e-5,"Default linked Down2 must produce Up1/2");
            require(upGesture.events==std::vector<bool>{true,false} && downGesture.events==std::vector<bool>{true,false},"Linked Ratio drag must record both host gestures");
        }
        parameter(processor,upId,0.5f); parameter(processor,downId,4);
        dragTo(down,8);
        require(std::abs(raw(upId)-0.25f)<1e-6 && std::abs(raw(downId)-8)<1e-5,"Relative LINK must keep product2, not force reciprocals");
        dragTo(up,0.5);
        require(std::abs(raw(downId)-4)<1e-5,"Reverse UP edit must preserve the initial pair relationship");
        enterText(up,"1:4");
        require(std::abs(raw(upId)-0.25f)<1e-6 && std::abs(raw(downId)-8)<1e-5,"Numeric UP commit must apply inverse relative LINK");
        parameter(processor,upId,0.5f); parameter(processor,downId,1.5f);
        dragTo(up,1);
        require(std::abs(raw(upId)-0.75f)<1e-6 && std::abs(raw(downId)-1)<1e-6,"Shared range limit must stop UP at .75 when DOWN reaches1");
        parameter(processor,upId,1.0f/32); parameter(processor,downId,2);
        dragTo(down,32);
        require(std::abs(raw(upId)-1.0f/32)<1e-6 && std::abs(raw(downId)-2)<1e-5,"Range limit must stop both linked ratios");
        dragTo(down,1);
        require(std::abs(raw(upId)-1.0f/16)<1e-6,"Reversing a clamped linked gesture must reopen travel");
        const auto beforeUp=raw(upId), beforeDown=raw(downId);
        editor.dualRatioLinkButton.onClick();
        require(raw("dualRatioLink")==0 && raw(upId)==beforeUp && raw(downId)==beforeDown,"Disabling LINK must not change ratios");
        dragTo(down,4);
        require(std::abs(raw(upId)-beforeUp)<1e-6,"Unlinked DOWN edit changed UP");
        editor.dualRatioLinkButton.onClick();
        require(raw("dualRatioLink")==1 && std::abs(raw(upId)-beforeUp)<1e-6 && std::abs(raw(downId)-4)<1e-5,"Enabling LINK must not snap to reciprocal values");
        parameter(processor,upId,0.25f); parameter(processor,downId,4);
        for(auto* slider : {&up,&down})
        { const auto event=mouseEvent(*slider,true); slider->mouseDown(event); slider->mouseUp(event); }
        require(std::abs(raw(upId)-1)<1e-6 && std::abs(raw(downId)-1)<1e-6,"Reciprocal pair Alt reset must restore both1:1 defaults");
    }
    // The existing LR/MS domain link retains an additive delta for the driven
    // branch; each UP/DOWN pair independently preserves its original product.
    parameter(processor,"processingMode",2); parameter(processor,"domainLink",1); editor.timerCallback();
    parameter(processor,"upRatioL",0.5f); parameter(processor,"downRatioL",4);
    parameter(processor,"upRatioR",0.25f); parameter(processor,"downRatioR",8);
    dragTo(*editor.downRatioSliders[1],8);
    require(std::abs(raw("downRatioR")-12)<1e-5 && std::abs(raw("upRatioR")-1.0f/6)<1e-6
            && std::abs(raw("upRatioL")-0.25f)<1e-6,"Domain LINK and UP/DOWN LINK conflict");
    std::cout<<"PASS: real UP/DOWN LINK across five domains: reciprocal and offset pairs, either direction, numeric input, Alt reset, shared limits, no toggle jumps, domain LINK interaction.\n";

    // Settings injection uses a D-drive test file, never the user's preferences.
    const auto settingsFile=dir.getNonexistentChildFile("ratio-link-preference-test",".settings");
    const auto settings=[&]
    {
        juce::PropertiesFile::Options options; options.storageFormat=juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile>(settingsFile,options);
    };
    juce::MemoryBlock savedOff;
    {
        QQSuperCompressionAudioProcessor p;
        QQSuperCompressionAudioProcessorEditor e(p,settings());
        require(p.getAPVTS().getRawParameterValue("dualRatioLink")->load()==1,"First-use LINK should default ON");
        e.dualRatioLinkButton.onClick(); p.getStateInformation(savedOff);
    }
    {
        QQSuperCompressionAudioProcessor p;
        QQSuperCompressionAudioProcessorEditor e(p,settings());
        require(p.getAPVTS().getRawParameterValue("dualRatioLink")->load()==0,"New instance forgot the last LINK choice");
        e.dualRatioLinkButton.onClick();
    }
    {
        QQSuperCompressionAudioProcessor p;
        p.setStateInformation(savedOff.getData(),static_cast<int>(savedOff.getSize()));
        for(int reopen=0;reopen<2;++reopen)
        {
            QQSuperCompressionAudioProcessorEditor e(p,settings());
            require(p.getAPVTS().getRawParameterValue("dualRatioLink")->load()==0,"Global LINK preference overrode restored project/reopened editor");
        }
    }
    std::cout<<"PASS: LINK default ON, last user choice across new instances, project-state precedence and editor reopen (isolated settings file).\n";

    parameter(processor,"domainLink",0); parameter(processor,"dualRatioLink",1);
    parameter(processor,"processingMode",0); editor.timerCallback();
    for(auto theme : {qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
    {
        editor.theme=theme; editor.applyTheme(); editor.setSize(1200,800);
        const juce::String skin=theme==qqsc::ui::Theme::light?"light":theme==qqsc::ui::Theme::dark?"dark":"classic";
        parameter(processor,"compressionMode",0); editor.timerCallback();
        const std::array<juce::Slider*,5> dials {&editor.inputGainSlider,&editor.ratioSlider,&editor.makeupSTSlider,&editor.mixSlider,&editor.outputGainSlider};
        for(size_t i=0;i<dials.size();++i)
        {
            const auto drawn=dials[i]->getLookAndFeel().getSliderLayout(*dials[i]).sliderBounds;
            require(juce::jmin(drawn.getWidth(),drawn.getHeight())==94,"Five primary dials must share exactly94px drawing size");
            require(dials[i]->getBounds().getCentreX()==80+static_cast<int>(i)*216 && dials[i]->getY()==648,"Primary dial centres/height are not uniformly spaced");
        }
        for(float ratio : {1.0f/32,0.25f,1.0f,4.0f,32.0f})
        {
            parameter(processor,"ratio",ratio); editor.timerCallback();
            require(qqsc::knob_light::origin(editor.ratioSlider)==0.5f,"Single Ratio light origin must be centre");
            snapshot(editor,dir.getChildFile(skin+"-rev3-single-ratio-"+juce::String(ratio,5)+".png"));
        }
        parameter(processor,"compressionMode",1); editor.timerCallback();
        for(int mode : {0,2,1})
        {
            parameter(processor,"processingMode",static_cast<float>(mode)); editor.timerCallback();
            require(editor.dualRatioLinkButton.getWidth()==(mode==0?36:30)
                    && editor.dualRatioLinkButton.getHeight()==(mode==0?17:14),"Ratio LINK must shrink for LR/MS");
            require(editor.dualRatioLinkButton.getBounds().getCentreX()==296,"LINK must sit between the UP and DOWN columns");
            for(size_t d=0;d<5;++d)
            {
                parameter(processor,qqsc::params::upRatioIds[d],0.125f); parameter(processor,qqsc::params::downRatioIds[d],8);
                require(qqsc::knob_light::origin(*editor.mainRatioControls()[d])==1 && qqsc::knob_light::origin(*editor.downRatioSliders[d])==0,"Dual Ratio light directions are reversed");
            }
            snapshot(editor,dir.getChildFile(skin+"-rev3-dual-"+qqsc::params::modeName(mode)+".png"));
            editor.setSize(1008,672); snapshot(editor,dir.getChildFile(skin+"-rev3-dual-"+qqsc::params::modeName(mode)+"-minimum.png"));
            editor.setSize(1200,800);
        }
        parameter(processor,"processingMode",0);
    }
    std::cout<<"PASS: Light/Dark/Classic94px primary dials,216px centre spacing, Ratio light origins and adaptive LINK geometry; actual production snapshots.\n";

    qqsc::warm_asset::Material warm; qqsc::dark_refined::Material dark;
    for(bool darkTheme : {false,true})
    for(float origin : {0.0f,0.5f,1.0f})
    for(float value : {0.0f,0.25f,0.5f,0.75f,1.0f})
    {
        const auto frame=darkTheme?dark.render(value,400,origin):warm.render(value,400,origin);
        const auto neutral=darkTheme?dark.render(value,400,value):warm.render(value,400,value);
        for(float position : {0.12f,0.37f,0.63f,0.88f})
        {
            const auto angle=darkTheme?qqsc::dark_refined::Material::angleForValue(position):juce::degreesToRadians(qqsc::warm_asset::Material::angleForValue(position));
            const auto cx=200.0f, cy=darkTheme?184.0f:199.0f, radius=darkTheme?134.8f:144.0f;
            const int x=juce::roundToInt(cx+radius*std::sin(angle)), y=juce::roundToInt(cy-radius*std::cos(angle));
            const auto a=frame.getPixelAt(x,y), b=neutral.getPixelAt(x,y);
            const int difference=std::abs(a.getRed()-b.getRed())+std::abs(a.getGreen()-b.getGreen())+std::abs(a.getBlue()-b.getBlue());
            const bool lit=position>juce::jmin(value,origin) && position<juce::jmax(value,origin);
            require(lit?difference>25:difference<8,"Rendered Ratio light mask illuminates the wrong side of unity");
        }
    }
    std::cout<<"PASS: actual warm/dark material pixels light only the span between neutral and current Ratio in both directions.\n";
}
