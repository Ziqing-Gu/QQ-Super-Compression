static void diagnostics1239(const juce::File& root)
{
    using P=QQSuperCompressionAudioProcessor;
    juce::PropertiesFile::Options opts;opts.storageFormat=juce::PropertiesFile::storeAsXML;opts.millisecondsBeforeSaving=0;
    const auto file=root.getChildFile("diagnostics-preferences.settings");
    {juce::PropertiesFile f(file,opts);f.setValue("lastPerformanceEco",false);f.saveIfNeeded();}
    const auto prefs=[&]{return std::make_unique<juce::PropertiesFile>(file,opts);};
    P p(prefs()),other(prefs());check(!p.isEcoMode() && !other.isEcoMode(),"initial FULL preference");
    configure(p,0,0,0,false);Transport host;p.setPlayHead(&host);p.prepareToPlay(48000,512);
    juce::MidiBuffer midi;juce::AudioBuffer<float> block(2,512);
    for(int i=0;i<120;++i){block.clear();p.processBlock(block,midi);}
    const auto closed=p.performanceWindows[0].blocks.load();check(closed>0,"closed snapshot missing");
    juce::MemoryBlock savedFull;p.getStateInformation(savedFull);
    {
        QQSuperCompressionAudioProcessorEditor editor(p,prefs());editor.setSize(1200,800);
        check(editor.tooltipWindow.getParentComponent()==&editor,"missing editor-owned tooltip window");
        const auto tip=editor.performanceModeButton.getTooltip();
        editor.updatePerformanceModeUi();block.clear();p.processBlock(block,midi);editor.updatePerformanceModeUi();
        check(tip==editor.performanceModeButton.getTooltip() && tip.contains("Right-click"),"tooltip changes during audio processing");
        const auto time=juce::Time::getCurrentTime();auto* button=&editor.performanceModeButton;
        const juce::MouseEvent right(juce::Desktop::getInstance().getMainMouseSource(),{8.f,8.f},juce::ModifierKeys(juce::ModifierKeys::rightButtonModifier),
            1.f,0.f,0.f,0.f,0.f,button,button,time,{8.f,8.f},time,1,false);
        button->mouseDown(right);button->mouseUp(right);
        check(!p.isEcoMode(),"right click changed FULL/ECO");
        check(editor.performanceDiagnosticsPanel && editor.performanceDiagnosticsPanel->isVisible(),"right click did not open panel");
        check(editor.performanceDiagnosticsPanel->snapshot().contains("FULL closed")
            && editor.performanceDiagnosticsPanel->snapshot().contains("sleeping"),"closed idle reason not displayed");
        const auto report=editor.performanceDiagnosticsPanel->snapshot();
        block.clear();p.processBlock(block,midi);editor.updatePerformanceModeUi();
        check(editor.performanceDiagnosticsPanel->snapshot()==report,"snapshot unexpectedly changes while reading");
        check(p.performanceWindows[0].blocks.load()==closed,"opening panel erased closed measurements");
        for(int width:{1008,1200,1800})
        {
            editor.setSize(width,width*2/3);
            check(editor.contentRoot.getLocalBounds().contains(editor.performanceDiagnosticsPanel->getBounds()),"diagnostic panel escapes layout");
            const auto snapshot=editor.createComponentSnapshot(editor.getLocalBounds());
            const auto png=root.getChildFile("diagnostics-"+juce::String(width)+".png");
            auto stream=png.createOutputStream();check(stream!=nullptr,"snapshot file");stream->setPosition(0);stream->truncate();juce::PNGImageFormat format;
            check(format.writeImageToStream(snapshot,*stream),"snapshot encode");
        }
        editor.setSize(1200,800);
        editor.performanceDiagnosticsPanel->setVisible(false);
        editor.tooltipWindow.displayTip(button->localPointToGlobal(button->getLocalBounds().getCentre()),tip);
        check(editor.tooltipWindow.isVisible(),"tooltip cannot display");editor.tooltipWindow.hideTip();
        // Explicit choice saves a default; other existing instances stay FULL.
        editor.performanceModeButton.onClick();check(p.isEcoMode() && !other.isEcoMode(),"instance mode isolation");
        {auto f=prefs();check(f->getBoolValue("lastPerformanceEco"),"explicit ECO not persisted");}
        P newEco(prefs());check(newEco.isEcoMode(),"new instance failed to inherit ECO before opening editor");
        newEco.setStateInformation(savedFull.getData(),int(savedFull.getSize()));
        check(!newEco.isEcoMode(),"saved FULL project did not override ECO default");
        {auto f=prefs();check(f->getBoolValue("lastPerformanceEco"),"project restore overwrote user default");}
        juce::MemoryBlock savedEco;p.getStateInformation(savedEco);
        p.copyAToB();p.selectABSlot(1);check(p.isEcoMode(),"A/B changed workflow preference");
        {
            QQSuperCompressionAudioProcessorEditor second(other,prefs());
            second.performanceModeButton.onClick();second.performanceModeButton.onClick();
            check(!other.isEcoMode() && p.isEcoMode(),"new last FULL choice affected existing ECO instance");
            editor.toggleTheme(); // stale editor must reload before saving unrelated UI preferences
            auto f=prefs();check(!f->getBoolValue("lastPerformanceEco",true),"stale editor overwrote last FULL choice");
        }
        P newFull(prefs());check(!newFull.isEcoMode(),"new instance failed to inherit FULL");
        newFull.setStateInformation(savedEco.getData(),int(savedEco.getSize()));
        check(newFull.isEcoMode(),"saved ECO project did not override FULL default");
    }
    {
        QQSuperCompressionAudioProcessorEditor reopened(p,prefs());
        check(p.isEcoMode() && reopened.performanceModeButton.getButtonText()=="ECO","reopening editor reapplied last default");
    }
    p.setPlayHead(nullptr);
    std::cout<<"PASS right-click panel, stable editor-owned tooltip, retained closed data, three-size renders, last-choice defaults before UI, existing-instance isolation, project precedence, A/B, reopen, stale preference writers\n";
}
