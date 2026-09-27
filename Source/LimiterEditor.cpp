#include "PluginEditor.h"

void QQSuperCompressionAudioProcessorEditor::setLimiterParameter (const char* id, float value)
{
    if (auto* parameter = processor.getAPVTS().getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void QQSuperCompressionAudioProcessorEditor::refreshCeilingValue()
{
    if (!ceilingValue.isBeingEdited())
        ceilingValue.setText (juce::String (ceilingSlider.getValue(),2)+(processor.isTruePeakSelected() ? " dBTP" : " dBFS"),juce::dontSendNotification);
    monitorCeilingLabel.setText("MON "+juce::String(processor.getUnityMonitorCeilingDb(),2),juce::dontSendNotification);
}

void QQSuperCompressionAudioProcessorEditor::initialiseLimiterControls()
{
    for (auto* button : { &limiterButton,&limiterLinkButton,&unityMonitorButton,&truePeakButton })
    {
        configureActionButton (*button); contentRoot.addAndMakeVisible (*button); registerKeyboardListener (*button);
    }
    limiterButton.setComponentID ("limiterMode");
    limiterLinkButton.getProperties().set ("qqscSmallLink",true);
    limiterButton.setTooltip ("Classic/Super dynamics with final output Ceiling protection. Down Ratio 200:1-1000:1; Up Ratio 1:200-1:1. TP selects true-peak instead of sample-peak limiting.");
    truePeakButton.setComponentID("truePeakLimiting");
    truePeakButton.getProperties().set("qqscSmallLink",true);
    truePeakButton.setTooltip("True Peak (default on): lit = true-peak ceiling; unlit = sample-peak ceiling. Effective only in Limiter mode. Switching crossfades over 10 ms.");
    truePeakButton.onClick=[this]
    {
        beginUndoTransaction("True Peak Limiting");
        setChoiceParameter(qqsc::params::truePeakLimiting,processor.isTruePeakSelected() ? 0 : 1);
        updateLimiterUi();
    };
    unityMonitorButton.setComponentID("limiterUnityMonitor");
    unityMonitorButton.setTitle("1:1 Monitor");
    unityMonitorButton.getProperties().set("qqscSmallLink",true);
    unityMonitorButton.getProperties().set("qqscHeadphones",true);
    unityMonitorButton.setTooltip("1:1 Monitor: cancel Output Gain after the complete limiter and Ceiling. Threshold/Makeup links remain active. Use MATCH, then Bypass for a loudness comparison. Saved with this project, independent of A/B.");
    unityMonitorButton.onClick=[this]
    {
        processor.setUnityMonitorEnabled(!processor.isUnityMonitorEnabled());
        updateLimiterUi();
    };
    limiterLinkButton.setTooltip ("Link only DOWN Threshold and Makeup to Output in opposite directions. Ratio, Mix, Input, algorithm, channel mode and branch switches never adjust Output. UP gate is not linked. GUI edits record both parameters; automate both recorded lanes.");
    limiterButton.onClick = [this]
    {
        finishCompressionControlGestures(); beginUndoTransaction ("Limiter Mode");
        const bool enabled=!processor.isLimiterMode();
        if(enabled) processor.enterLimiterMode();
        else processor.leaveLimiterMode();
        updateCompressionUi();
    };
    limiterLinkButton.onClick = [this]
    {
        finishCompressionControlGestures(); beginUndoTransaction ("Down Threshold / Output Link");
        setChoiceParameter (qqsc::params::limiterLink,processor.isLimiterLinked() ? 0 : 1);
        // Enabling LINK preserves the current pair, including saved/manual offsets.
        updateLimiterUi(); captureLimiterLinkAnchor();
    };
    contentRoot.addAndMakeVisible(ceilingPanel);
    ceilingPanel.toBack();
    ceilingPanel.setInterceptsMouseClicks(false,false);
    configureLabel (ceilingLabel,"CEILING");
    ceilingLabel.setFont(juce::Font(juce::FontOptions(8.0f,juce::Font::bold)));
    configureLabel(monitorCeilingLabel,{});
    monitorCeilingLabel.setFont(juce::Font(juce::FontOptions(8.0f)));
    monitorCeilingLabel.setTooltip("Ceiling at the 1:1 monitoring level: Ceiling minus Output Gain. The normal Ceiling setting and TP/Peak selection are unchanged.");
    contentRoot.addAndMakeVisible(monitorCeilingLabel);
    configureLabel (ceilingValue,{});
    ceilingValue.setFont (juce::Font (juce::FontOptions (11.0f)));
    ceilingValue.setEditable (false,true,false);
    ceilingValue.setComponentID ("ceilingValue");
    ceilingValue.setTooltip ("Final ceiling, -24 to 0 dB, default 0. TP on: true peak; TP off: sample peak. MON shows the converted 1:1 listening ceiling. Alt-click resets to 0. Double-click to type; drag vertically; Shift = 10x finer. Ctrl+Z / Ctrl+Shift+Z.");
    for (auto* c : { static_cast<juce::Component*>(&ceilingLabel),static_cast<juce::Component*>(&ceilingValue) })
    { contentRoot.addAndMakeVisible (*c); registerKeyboardListener (*c); }
    // Hidden attachment target: the only visible Ceiling control is the number.
    configureKnob (ceilingSlider," dBFS");
    ceilingSlider.setVisible(false);
    ceilingAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.getAPVTS(),qqsc::params::ceilingDb,ceilingSlider);
    ceilingSlider.onValueChange=[this] { refreshCeilingValue(); };
    ceilingValue.start=[this]
    {
        beginUndoTransaction ("Ceiling");
        ceilingDragValue=ceilingSlider.getValue();
        processor.getAPVTS().getParameter(qqsc::params::ceilingDb)->beginChangeGesture();
    };
    ceilingValue.move=[this] (float delta,bool fine)
    {
        // Accumulate unquantized movement; Shift changes sensitivity without
        // a new anchor or jump. Audio uses the existing 10 ms gain smoothing.
        ceilingDragValue=juce::jlimit(-24.0,0.0,ceilingDragValue+double(delta)*(fine ? 0.005 : 0.05));
        ceilingSlider.setValue (ceilingDragValue,juce::sendNotificationSync);
    };
    ceilingValue.finish=[this] { processor.getAPVTS().getParameter(qqsc::params::ceilingDb)->endChangeGesture(); };
    ceilingValue.reset=[this]
    {
        beginUndoTransaction ("Reset Ceiling");
        auto* p=processor.getAPVTS().getParameter(qqsc::params::ceilingDb);
        p->beginChangeGesture(); ceilingSlider.setValue(0.0,juce::sendNotificationSync); p->endChangeGesture();
        refreshCeilingValue();
    };
    ceilingValue.onEditorShow=[this]
    {
        if (auto* e=ceilingValue.getCurrentTextEditor())
        { e->setInputRestrictions (16,"0123456789+-. dBFSTPdbfstp"); registerKeyboardListener (*e); }
    };
    ceilingValue.onTextChange=[this]
    {
        const auto text=ceilingValue.getText().trim();
        if (!text.containsAnyOf("0123456789")) { refreshCeilingValue(); return; }
        const auto value=text.getDoubleValue();
        if (!std::isfinite(value)) { refreshCeilingValue(); return; }
        beginUndoTransaction ("Ceiling");
        auto* p=processor.getAPVTS().getParameter(qqsc::params::ceilingDb);
        p->beginChangeGesture(); ceilingSlider.setValue(value,juce::sendNotificationSync); p->endChangeGesture();
        refreshCeilingValue();
    };

    const auto normalStart=outputGainSlider.onGestureStart, normalEnd=outputGainSlider.onGestureEnd;
    const auto normalChange=outputGainSlider.onValueChange;
    const auto normalText=outputGainSlider.valueFromTextFunction;
    outputGainSlider.onGestureStart=[this,normalStart]
    { if(processor.isLimiterLinked()) beginLimiterOutputGesture(); else if(normalStart) normalStart(); };
    outputGainSlider.onGestureEnd=[this,normalEnd]
    { limiterOutputGesture=false; if(normalEnd) normalEnd(); };
    outputGainSlider.onValueChange=[this,normalChange]
    {
        if(limiterUpdating) return;
        if(processor.isLimiterLinked() && limiterOutputGesture)
        {
            const juce::ScopedValueSetter<bool> guard(limiterUpdating,true);
            const auto value=applyLimiterOutputChange(outputGainSlider.getValue());
            setLinkedControlValue(outputGainSlider,value);
        }
        else if(normalChange) normalChange();
    };
    limiterOutputText=[this,normalText] (const juce::String& text)
    {
        if(!processor.isLimiterMode()) return normalText(text);
        beginUndoTransaction("Limiter Output");
        if(!processor.isLimiterLinked()) return juce::jlimit(-120.0,120.0,text.getDoubleValue());
        beginLimiterOutputGesture();
        const juce::ScopedValueSetter<bool> guard(limiterUpdating,true);
        const auto result=applyLimiterOutputChange(text.getDoubleValue());
        endLinkedGesture(); limiterOutputGesture=false;
        return result;
    };
    // Output has an explicit edit whitelist: DOWN Threshold (in
    // handleBoundaryChange) and Makeup here. Never reconcile after a Ratio,
    // algorithm, channel/mode, branch-enable or Mix edit.
    const auto linkMakeup=[this] (FineKnob& slider)
    {
        slider.onBeforeValueEdit=[this]()
        {
            if(limiterUpdating || !limiterControlsReady) return;
            captureLimiterLinkAnchor();
        };
        auto original=slider.onValueChange;
        slider.onValueChange=[this,&slider,original]
        {
            if(original) original();
            if(!limiterUpdating && (slider.hasActiveNativeGesture() || activeLinkSource==&slider))
                reconcileLimiterOutput();
        };
    };
    for(auto* slider : { &makeupSTSlider,&makeupLSlider,&makeupRSlider,&makeupMSlider,&makeupSSlider }) linkMakeup(*slider);
    // MATCH owns its mixed-signal correction inside the processor. Do not
    // follow it with an unrelated control-reference reconciliation.
    limiterControlsReady=true;
    updateLimiterUi(); refreshCeilingValue();
}

void QQSuperCompressionAudioProcessorEditor::updateLimiterUi()
{
    if(!limiterControlsReady) return;
    const bool limiter=processor.isLimiterMode(), dual=attachedCompressionMode==1;
    const juce::ScopedValueSetter<bool> guard(limiterUpdating,true);
    limiterButton.setToggleState(limiter,juce::dontSendNotification);
    limiterLinkButton.setToggleState(processor.isLimiterLinked(),juce::dontSendNotification);
    unityMonitorButton.setToggleState(processor.isUnityMonitorEnabled(),juce::dontSendNotification);
    unityMonitorButton.setVisible(limiter);
    truePeakButton.setToggleState(processor.isTruePeakSelected(),juce::dontSendNotification);
    truePeakButton.setVisible(limiter);
    monitorCeilingLabel.setVisible(processor.isUnityMonitorActive());
    inputOutputLinkButton.setVisible(!limiter);
    ceilingSlider.setVisible(false);
    limiterLinkButton.setVisible(limiter); ceilingLabel.setVisible(limiter); ceilingValue.setVisible(limiter);
    ceilingPanel.setVisible(limiter);ceilingPanel.repaint();
    if(attachedLimiter!=int(limiter))
    {
        finishCompressionControlGestures();
        // Rebind every common control to its mode bank, keeping its text/gesture
        // callbacks (in particular linked edits and numeric entry) intact.
        const auto rebind=[this](auto& attachment,FineKnob& slider,const char* normalID)
        {
            const auto text=slider.textFromValueFunction;
            const auto parse=slider.valueFromTextFunction;
            attachment.reset();
            const auto* id=processor.soundParameterID(normalID);
            slider.getProperties().set("qqscParameterID",id);
            attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.getAPVTS(),id,slider);
            slider.textFromValueFunction=text;slider.valueFromTextFunction=parse;
        };
        rebind(inputGainAttachment,inputGainSlider,qqsc::params::inputGainDb);
        rebind(makeupSTAttachment,makeupSTSlider,qqsc::params::makeupGainDb);
        rebind(makeupLAttachment,makeupLSlider,qqsc::params::makeupGainLDb);
        rebind(makeupRAttachment,makeupRSlider,qqsc::params::makeupGainRDb);
        rebind(makeupMAttachment,makeupMSlider,qqsc::params::makeupGainMDb);
        rebind(makeupSAttachment,makeupSSlider,qqsc::params::makeupGainSDb);
        rebind(mixAttachment,mixSlider,qqsc::params::mix);
        rebind(mixLAttachment,mixLSlider,qqsc::params::mixL);
        rebind(mixRAttachment,mixRSlider,qqsc::params::mixR);
        rebind(mixMAttachment,mixMSlider,qqsc::params::mixM);
        rebind(mixSAttachment,mixSSlider,qqsc::params::mixS);
        rebind(keyGainAttachment,keyGainSlider,qqsc::params::keyGainDb);
        rebind(keyHpfAttachment,keyHpfSlider,qqsc::params::keyHpfHz);
        linkAttachment.reset();
        linkAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.getAPVTS(),processor.soundParameterID(qqsc::params::domainLink),linkButton);
        outputGainAttachment.reset();
        const auto* id=limiter ? qqsc::params::limiterOutputDb : qqsc::params::outputGainDb;
        outputGainSlider.getProperties().set("qqscParameterID",id);
        outputGainAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.getAPVTS(),id,outputGainSlider);
        outputGainSlider.valueFromTextFunction=limiterOutputText;
        attachedLimiter=int(limiter);
        reattachCompressionControls(dual);
        resized();
    }
    auto main=mainRatioControls();
    for(size_t d=0; d<5; ++d)
    {
        auto& down=*downRatioSliders[d];
        const auto min=limiter ? double(qqsc::limiterMinimumDownRatio) : 1.0;
        const auto max=double(limiter ? qqsc::maximumDownRatio : qqsc::normalMaximumDownRatio);
        if(down.getMinimum()!=min || down.getMaximum()!=max)
        {
            const auto r=qqsc::params::dynamicsRatioRange(float(min),float(max));
            down.setNormalisableRange({min,max,
                [r](double,double,double n){return double(r.convertFrom0to1(float(n)));},
                [r](double,double,double v){return double(r.convertTo0to1(float(v)));}});
        }
        down.setValue(qqsc::params::limiterRatio(processor.readSoundParameter(qqsc::params::downRatioIds[d]),limiter,true),juce::dontSendNotification);
        down.setResetValue(min);
        down.setTooltip(limiter ? "Downward Ratio: 200:1 to 1000:1" : "Downward Ratio: 1:1 to 200:1");
        const int kind=(dual ? 2 : 0)+(limiter ? 1 : 0);
        if(int(main[d]->getProperties().getWithDefault("limiterRangeKind",-1))!=kind)
        {
            if(kind==1) main[d]->setNormalisableRange(qqsc::params::limiterSingleRange());
            else
            {
                const auto r=qqsc::params::dynamicsRatioRange(limiter ? qqsc::limiterMinimumUpRatio : qqsc::minimumUpRatio,
                    dual ? 1.0f : qqsc::normalMaximumDownRatio);
                main[d]->setNormalisableRange({double(r.start),double(r.end),
                    [r](double,double,double n){return double(r.convertFrom0to1(float(n)));},
                    [r](double,double,double v){return double(r.convertTo0to1(float(v)));}});
            }
            main[d]->getProperties().set("limiterRangeKind",kind);
        }
        main[d]->setValue(dual ? qqsc::params::upwardRatio(processor.readSoundParameter(qqsc::params::upRatioIds[d]),limiter)
                              : processor.effectiveSingleRatio(d),juce::dontSendNotification);
        main[d]->setTooltip(dual
            ? "Upward Ratio: 1:200 to 1:1"
            : (limiter ? "Up 1:200 to 1:1; Down 200:1 to 1000:1. Unity is the centre detent."
                       : "Ratio: 1:200 to 200:1. Below 1:1 boosts; above 1:1 reduces."));
    }
    ceilingValue.setColour(juce::Label::textColourId,qqsc::ui::text());
    ceilingValue.setColour(juce::Label::backgroundColourId,qqsc::ui::panelAlt());
    ceilingValue.setColour(juce::Label::outlineColourId,qqsc::ui::border());
    monitorCeilingLabel.setColour(juce::Label::textColourId,qqsc::ui::cyanAccent());
    refreshCeilingValue();
}

void QQSuperCompressionAudioProcessorEditor::captureLimiterLinkAnchor()
{
    limiterLinkAnchorValid=processor.isLimiterLinked() && !limiterUpdating;
    if(!limiterLinkAnchorValid) return;
    limiterLinkReference=processor.getLimiterLinkReferencePeakDb();
    limiterLinkOutput=processor.readSoundParameter(qqsc::params::outputGainDb);
}

void QQSuperCompressionAudioProcessorEditor::reconcileLimiterOutput()
{
    if(limiterUpdating || !processor.isLimiterLinked() || !limiterLinkAnchorValid) return;
    const juce::ScopedValueSetter<bool> guard(limiterUpdating,true);
    const auto next=limiterLinkOutput+limiterLinkReference-processor.getLimiterLinkReferencePeakDb();
    auto* p=processor.getAPVTS().getParameter(qqsc::params::limiterOutputDb);
    const auto gestureOpen=std::find(companionGestureParameters.begin(),companionGestureParameters.end(),p)!=companionGestureParameters.end();
    if(!gestureOpen) p->beginChangeGesture();
    setLinkedControlValue(outputGainSlider,next);
    if(!gestureOpen) p->endChangeGesture();
}

void QQSuperCompressionAudioProcessorEditor::beginLimiterOutputGesture()
{
    endLinkedGesture(); beginUndoTransaction("Output / Down Threshold");
    limiterOutputGesture=true; limiterStartOutput=float(outputGainSlider.getValue());
    const auto mode=juce::roundToInt(processor.readSoundParameter(qqsc::params::processingMode));
    const bool dual=attachedCompressionMode==1;
    for(size_t d=0; d<5; ++d)
    {
        const bool active=mode==0 ? d==0 : mode==2 ? d==1||d==2 : d==3||d==4;
        if(!active || (!dual && processor.effectiveSingleRatio(d)<=1.0f)
            || (dual && processor.readSoundParameter(qqsc::params::downEnabledIds[d])<0.5f)) continue;
        auto* p=processor.getAPVTS().getParameter(processor.soundParameterID((dual ? qqsc::params::downThresholdIds : qqsc::params::thresholdIds)[d]));
        p->beginChangeGesture(); companionGestureParameters.push_back(p);
    }
}

double QQSuperCompressionAudioProcessorEditor::applyLimiterOutputChange (double requested)
{
    const auto before=processor.getLimiterLinkReferencePeakDb();
    const auto requestedDelta=float(requested-limiterStartOutput);
    if(std::abs(requestedDelta)<1.e-5f) return limiterStartOutput;
    const auto target=before-requestedDelta;
    float lo=-120,hi=120;
    if(std::abs(processor.getLimiterLinkReferencePeakDb(hi)-processor.getLimiterLinkReferencePeakDb(lo))<1.e-6f)
    {
        // A unity/upward Ratio, disabled Down branch or finite Range may
        // leave no downward threshold response. Output remains a usable gain
        // control; do not freeze it or move thresholds to fabricate a link.
        limiterStartOutput=float(juce::jlimit(-120.0,120.0,requested));
        return limiterStartOutput;
    }
    for(int iteration=0; iteration<40; ++iteration)
    {
        const auto mid=(lo+hi)*0.5f;
        if(processor.getLimiterLinkReferencePeakDb(mid)<target) lo=mid; else hi=mid;
    }
    const auto delta=(lo+hi)*0.5f;
    const auto mode=juce::roundToInt(processor.readSoundParameter(qqsc::params::processingMode));
    const bool dual=attachedCompressionMode==1;
    const auto floor=processor.isClassicBoundary(dual) ? -90.0f : -120.0f;
    for(int d=0; d<5; ++d)
    {
        const bool active=mode==0 ? d==0 : mode==2 ? d==1||d==2 : d==3||d==4;
        if(!active || (!dual && processor.effectiveSingleRatio(size_t(d))<=1.0f)
            || (dual && processor.readSoundParameter(qqsc::params::downEnabledIds[size_t(d)])<0.5f)) continue;
        const auto current=processor.getBoundaryForDomainDb(dual,dual,d);
        const auto minimum=dual ? juce::jmax(floor,juce::jmin(0.0f,processor.getBoundaryForDomainDb(true,false,d)+0.01f)) : floor;
        const auto maximum=dual ? 0.0f : juce::jmin(0.0f,processor.getBoundaryForDomainDb(false,true,d));
        processor.setBoundaryForDomainDb(dual,dual,d,juce::jlimit(minimum,maximum,current+delta));
    }
    const auto result=limiterStartOutput+before-processor.getLimiterLinkReferencePeakDb();
    limiterStartOutput=result;
    return result;
}
