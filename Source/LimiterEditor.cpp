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
    for (auto* button : { &limiterButton,&limiterLinkButton,&unityMonitorButton,&truePeakButton,&tpRecoveryButton })
    {
        configureActionButton (*button); contentRoot.addAndMakeVisible (*button); registerKeyboardListener (*button);
    }
    limiterButton.setComponentID ("limiterMode");
    limiterLinkButton.getProperties().set ("qqscSmallLink",true);
    limiterButton.setTooltip ("Classic/Super dynamics followed by final Ceiling protection. Down Ratio 1:1-1000:1; Up Ratio 1:8-1:1. The compression stage shapes character; Ceiling/TP performs final limiting.");
    truePeakButton.setComponentID("truePeakLimiting");
    truePeakButton.getProperties().set("qqscSmallLink",true);
    truePeakButton.setTooltip("True Peak: lit = reconstructed true-peak Ceiling; unlit = Hard Clip at the selected Ceiling OS. TP supports 4x, 8x or 16x; enabling TP from 1x promotes Ceiling OS to 8x. TP switching at a fixed OS keeps the same PDC.");
    truePeakButton.onClick=[this]
    {
        beginUndoTransaction("True Peak Limiting");
        const bool enabling=!processor.isTruePeakSelected();
        // Keep the stored TP-OFF quality intact. The processor promotes a stored
        // 1x Ceiling to an effective 8x only while TP is active, so disabling TP
        // restores the user's previous native Hard-Clip choice automatically.
        setChoiceParameter(qqsc::params::truePeakLimiting,enabling ? 1 : 0);
        processor.notifyHostProcessingLatency();
        updateLimiterUi();
        updateCeilingOversamplingUi();
    };
    tpRecoveryButton.setComponentID("tpRecoveryMode");
    tpRecoveryButton.getProperties().set("qqscSmallLink",true);
    tpRecoveryButton.setTooltip("TP Recovery: TIGHT = fastest recovery / maximum loudness; AUTO = adaptive 1.2.17 timing (default); SMOOTH = slower, calmer gain recovery. True-Peak Ceiling protection remains active in all modes.");
    tpRecoveryButton.onClick=[this]
    {
        beginUndoTransaction("TP Recovery");
        const auto current=processor.getTpRecoveryMode();
        const auto next=current==qqsc::params::tpAuto ? qqsc::params::tpTight
            : current==qqsc::params::tpTight ? qqsc::params::tpSmooth : qqsc::params::tpAuto;
        setChoiceParameter(qqsc::params::tpRecoveryMode,next);
        updateLimiterUi();
    };
    unityMonitorButton.setComponentID("limiterUnityMonitor");
    unityMonitorButton.setTitle("1:1 Monitor");
    unityMonitorButton.getProperties().set("qqscSmallLink",true);
    unityMonitorButton.getProperties().set("qqscHeadphones",true);
    unityMonitorButton.setTooltip("1:1 Monitor: cancel Output Gain after the complete limiter and Ceiling. Active UP/DOWN Threshold and Makeup 1:1 links remain active. Use MATCH, then Bypass for a loudness comparison. Saved with this project, independent of A/B.");
    unityMonitorButton.onClick=[this]
    {
        processor.setUnityMonitorEnabled(!processor.isUnityMonitorEnabled());
        updateLimiterUi();
    };
    limiterLinkButton.setTooltip ("Strict 1:1 dB Link: the active UP or DOWN Threshold and Makeup move Output Gain by the same dB amount in the opposite direction; editing Output moves every active non-unity Threshold oppositely 1:1. Ratio, Mix, Input and algorithms never adjust Output.");
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
        finishCompressionControlGestures(); beginUndoTransaction ("Limiter Output Link");
        setChoiceParameter (qqsc::params::limiterLink,processor.isLimiterLinked() ? 0 : 1);
        // Enabling LINK preserves the current pair, including saved/manual offsets.
        updateLimiterUi();
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
    ceilingValue.setTooltip ("Final ceiling, -24 to 0 dB, default 0. TP on: true peak at 4x/8x/16x; TP off: Hard Clip at the selected 1x/4x/8x/16x Ceiling OS. MON shows the converted 1:1 listening ceiling. Alt-click resets to 0. Double-click to type; drag vertically; Shift = 10x finer. Ctrl+Z / Ctrl+Shift+Z.");
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
    // These formatters also survive rebinding. A formatter capturing the
    // original Normal parameter would display Limiter values above 30 as 30.
    for (auto* slider : { &makeupSTSlider,&makeupLSlider,&makeupRSlider,&makeupMSlider,&makeupSSlider })
        slider->textFromValueFunction = [] (double value) { return juce::String(value, 2); };
    // Limiter LINK keeps the musical compression controls coherent in either
    // direction: Makeup and every active non-unity UP/DOWN Threshold are paired
    // with Output in exact opposite dB deltas. Ratio/Mix/algorithms themselves
    // never move Output; they only decide which threshold branch is active.
    for (auto* slider : { &makeupSTSlider,&makeupLSlider,&makeupRSlider,&makeupMSlider,&makeupSSlider })
        linkLimiterReferenceControl(*slider);
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
    const auto recovery=processor.getTpRecoveryMode();
    tpRecoveryButton.setButtonText(recovery==qqsc::params::tpTight ? "TIGHT"
        : recovery==qqsc::params::tpSmooth ? "SMOOTH" : "AUTO");
    tpRecoveryButton.setVisible(limiter && processor.isTruePeakSelected());
    monitorCeilingLabel.setVisible(processor.isUnityMonitorActive());
    inputOutputLinkButton.setVisible(!limiter);
    ceilingSlider.setVisible(false);
    limiterLinkButton.setVisible(limiter); ceilingLabel.setVisible(limiter); ceilingValue.setVisible(limiter);
    ceilingPanel.setVisible(limiter);ceilingPanel.repaint();
    updateCeilingOversamplingUi();
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
        down.setTooltip(limiter ? "Downward Ratio: 1:1 to 1000:1" : "Downward Ratio: 1:1 to 200:1");
        const int kind=(dual ? 2 : 0)+(limiter ? 1 : 0);
        if(int(main[d]->getProperties().getWithDefault("limiterRangeKind",-1))!=kind)
        {
            if(kind==1) main[d]->setNormalisableRange(qqsc::params::limiterSingleRange());
            else
            {
                const auto r=qqsc::params::dynamicsRatioRange(limiter ? qqsc::limiterDualMinimumUpRatio : qqsc::minimumUpRatio,
                    dual ? 1.0f : qqsc::normalMaximumDownRatio);
                main[d]->setNormalisableRange({double(r.start),double(r.end),
                    [r](double,double,double n){return double(r.convertFrom0to1(float(n)));},
                    [r](double,double,double v){return double(r.convertTo0to1(float(v)));}});
            }
            main[d]->getProperties().set("limiterRangeKind",kind);
        }
        main[d]->setValue(dual ? qqsc::params::upwardRatio(processor.readSoundParameter(qqsc::params::upRatioIds[d]),limiter)
                              : processor.effectiveSingleRatio(d),juce::dontSendNotification);
        // Limiter compression defaults to unity in both directions. Ceiling/TP
        // is the final limiter; Alt-click therefore restores the compression
        // stage to 1:1 instead of forcing a high downward ratio.
        main[d]->setResetValue(1.0);
        main[d]->setTooltip(dual
            ? (limiter ? "Upward Ratio: 1:8 to 1:1" : "Upward Ratio: 1:200 to 1:1")
            : (limiter ? "Up 1:8 to 1:1; Down 1:1 to 1000:1. Alt-click resets to 1:1."
                       : "Ratio: 1:200 to 200:1. Below 1:1 boosts; above 1:1 reduces."));
    }
    ceilingValue.setColour(juce::Label::textColourId,qqsc::ui::text());
    ceilingValue.setColour(juce::Label::backgroundColourId,qqsc::ui::panelAlt());
    ceilingValue.setColour(juce::Label::outlineColourId,qqsc::ui::border());
    monitorCeilingLabel.setColour(juce::Label::textColourId,qqsc::ui::cyanAccent());
    refreshCeilingValue();
}

void QQSuperCompressionAudioProcessorEditor::linkLimiterReferenceControl(FineKnob& slider)
{
    const auto originalBefore = slider.onBeforeValueEdit;
    const auto originalStart = slider.onGestureStart;
    const auto originalEnd = slider.onGestureEnd;
    const auto originalChange = slider.onValueChange;
    slider.onBeforeValueEdit = [this, &slider, originalBefore]
    {
        if (originalBefore) originalBefore();
        if (!limiterUpdating && limiterControlsReady && !limiterReferenceEditInProgress)
            captureLimiterLinkAnchor(slider);
    };
    slider.onGestureStart = [this, &slider, originalStart]
    {
        if (originalStart) originalStart();
        captureLimiterLinkAnchor(slider);
        limiterReferenceGestureSource = &slider;
        if (limiterLinkAnchorValid)
        {
            auto* output = processor.getAPVTS().getParameter(qqsc::params::limiterOutputDb);
            if (std::find(companionGestureParameters.begin(), companionGestureParameters.end(), output)
                == companionGestureParameters.end())
            {
                output->beginChangeGesture();
                companionGestureParameters.push_back(output);
            }
        }
    };
    slider.onGestureEnd = [this, originalEnd]
    {
        if (originalEnd) originalEnd();
        endLinkedGesture();
    };
    slider.onValueChange = [this, &slider, originalChange]
    {
        if (limiterReferenceEditInProgress || linkedValueUpdateInProgress || dualRatioValueUpdateInProgress)
            return;
        if (limiterUpdating || !limiterControlsReady || processor.isRestoringSoundState()
            || processor.getUndoManager().isPerformingUndoRedo())
            return;
        const bool explicitEdit = slider.hasActiveNativeGesture() || activeLinkSource == &slider
            || limiterReferenceGestureSource == &slider;
        if (!explicitEdit || !processor.isLimiterLinked() || !limiterLinkAnchorValid)
        {
            if (originalChange) originalChange();
            return;
        }

        const juce::ScopedValueSetter<bool> editGuard(limiterReferenceEditInProgress, true);
        // Keep the source and Output inside their parameter ranges while
        // preserving the exact opposite dB delta. Clamp the shared delta, not
        // just one member of the pair.
        const auto requestedDelta = slider.getValue() - double(limiterLinkReference);
        const auto minDelta = juce::jmax(slider.getMinimum() - double(limiterLinkReference),
                                         double(limiterLinkOutput) - 120.0);
        const auto maxDelta = juce::jmin(slider.getMaximum() - double(limiterLinkReference),
                                         double(limiterLinkOutput) + 120.0);
        const auto appliedDelta = juce::jlimit(minDelta, maxDelta, requestedDelta);
        const auto sourceValue = double(limiterLinkReference) + appliedDelta;
        if (std::abs(slider.getValue() - sourceValue) > 1.0e-9)
            setLinkedControlValue(slider, sourceValue);

        // Preserve the existing LR/MS relative-link callback. It now sees the
        // already-clamped source value, so partner range limits may reduce the
        // final shared delta before Output is reconciled.
        if (originalChange) originalChange();
        reconcileLimiterOutput(slider);
    };
}

void QQSuperCompressionAudioProcessorEditor::captureLimiterLinkAnchor(FineKnob& source)
{
    limiterLinkAnchorValid=processor.isLimiterLinked() && !limiterUpdating
        && !processor.isRestoringSoundState() && !processor.getUndoManager().isPerformingUndoRedo();
    limiterReferenceGestureSource=&source;
    if(!limiterLinkAnchorValid) return;
    limiterLinkReference=float(source.getValue());
    limiterLinkOutput=processor.readSoundParameter(qqsc::params::outputGainDb);
}

void QQSuperCompressionAudioProcessorEditor::reconcileLimiterOutput(FineKnob& source)
{
    if(limiterUpdating || !processor.isLimiterLinked() || !limiterLinkAnchorValid
        || processor.isRestoringSoundState() || processor.getUndoManager().isPerformingUndoRedo()) return;
    const juce::ScopedValueSetter<bool> guard(limiterUpdating,true);
    const auto delta=source.getValue()-double(limiterLinkReference);
    const auto next=juce::jlimit(-120.0,120.0,double(limiterLinkOutput)-delta);
    auto* p=processor.getAPVTS().getParameter(qqsc::params::limiterOutputDb);
    const auto gestureOpen=std::find(companionGestureParameters.begin(),companionGestureParameters.end(),p)!=companionGestureParameters.end();
    if(!gestureOpen) p->beginChangeGesture();
    setLinkedControlValue(outputGainSlider,next);
    if(!gestureOpen) p->endChangeGesture();
}

void QQSuperCompressionAudioProcessorEditor::beginLimiterOutputGesture()
{
    endLinkedGesture(); beginUndoTransaction("Output / Active Threshold 1:1");
    limiterOutputGesture=true; limiterStartOutput=float(outputGainSlider.getValue());
    const auto mode=juce::roundToInt(processor.readSoundParameter(qqsc::params::processingMode));
    const bool dual=attachedCompressionMode==1;
    for(size_t d=0; d<5; ++d)
    {
        limiterStartLowerThresholds[d]=processor.getBoundaryForDomainDb(dual,false,int(d));
        limiterStartUpperThresholds[d]=processor.getBoundaryForDomainDb(dual,true,int(d));
        const bool active=mode==0 ? d==0 : mode==2 ? d==1||d==2 : d==3||d==4;
        if(!active) continue;

        const auto openGesture=[&](const char* normalID)
        {
            auto* p=processor.getAPVTS().getParameter(processor.soundParameterID(normalID));
            if(p!=nullptr && std::find(companionGestureParameters.begin(),companionGestureParameters.end(),p)==companionGestureParameters.end())
            { p->beginChangeGesture(); companionGestureParameters.push_back(p); }
        };

        if(!dual)
        {
            if(std::abs(processor.effectiveSingleRatio(d)-1.0f)>1.0e-6f)
                openGesture(qqsc::params::thresholdIds[d]);
            continue;
        }

        const auto upRatio=qqsc::params::upwardRatio(processor.readSoundParameter(qqsc::params::upRatioIds[d]),true);
        const auto downRatio=qqsc::params::limiterRatio(processor.readSoundParameter(qqsc::params::downRatioIds[d]),true,true);
        const bool upActive=processor.readSoundParameter(qqsc::params::upEnabledIds[d])>=0.5f && upRatio<1.0f-1.0e-6f;
        const bool downActive=processor.readSoundParameter(qqsc::params::downEnabledIds[d])>=0.5f && downRatio>1.0f+1.0e-6f;
        if(upActive) openGesture(qqsc::params::upThresholdIds[d]);
        if(downActive) openGesture(qqsc::params::downThresholdIds[d]);
    }
}

double QQSuperCompressionAudioProcessorEditor::applyLimiterOutputChange (double requested)
{
    const auto requestedDelta=float(requested-limiterStartOutput);
    if(std::abs(requestedDelta)<1.e-5f) return limiterStartOutput;

    // Output and every active non-unity Threshold move by the same absolute dB
    // amount in opposite directions. When both Dual branches are active, both
    // boundaries move together so their width is preserved. A unity branch is
    // intentionally ignored: it produces no dynamics and must not lock Output.
    float minDelta=-120.0f-limiterStartOutput;
    float maxDelta= 120.0f-limiterStartOutput;
    const auto mode=juce::roundToInt(processor.readSoundParameter(qqsc::params::processingMode));
    const bool dual=attachedCompressionMode==1;
    std::array<bool,5> moveLower {}, moveUpper {};
    for(int d=0; d<5; ++d)
    {
        const bool active=mode==0 ? d==0 : mode==2 ? d==1||d==2 : d==3||d==4;
        if(!active) continue;

        if(!dual)
        {
            if(std::abs(processor.effectiveSingleRatio(size_t(d))-1.0f)<=1.0e-6f) continue;
            moveLower[size_t(d)]=true;
            const auto floor=processor.isClassicBoundary(false) ? -90.0f : -120.0f;
            const auto minimum=floor;
            const auto maximum=juce::jmin(0.0f,limiterStartUpperThresholds[size_t(d)]);
            const auto start=limiterStartLowerThresholds[size_t(d)];
            minDelta=juce::jmax(minDelta,start-maximum);
            maxDelta=juce::jmin(maxDelta,start-minimum);
            continue;
        }

        const auto upRatio=qqsc::params::upwardRatio(processor.readSoundParameter(qqsc::params::upRatioIds[size_t(d)]),true);
        const auto downRatio=qqsc::params::limiterRatio(processor.readSoundParameter(qqsc::params::downRatioIds[size_t(d)]),true,true);
        const bool upActive=processor.readSoundParameter(qqsc::params::upEnabledIds[size_t(d)])>=0.5f && upRatio<1.0f-1.0e-6f;
        const bool downActive=processor.readSoundParameter(qqsc::params::downEnabledIds[size_t(d)])>=0.5f && downRatio>1.0f+1.0e-6f;
        moveLower[size_t(d)]=upActive; moveUpper[size_t(d)]=downActive;

        const auto lowerFloor=processor.isClassicBoundary(false) ? -90.0f : -120.0f;
        const auto upperFloor=processor.isClassicBoundary(true) ? -90.0f : -120.0f;
        if(upActive)
        {
            const auto minimum=lowerFloor;
            const auto maximum=downActive ? 0.0f : juce::jmin(0.0f,limiterStartUpperThresholds[size_t(d)]-0.01f);
            const auto start=limiterStartLowerThresholds[size_t(d)];
            minDelta=juce::jmax(minDelta,start-maximum);
            maxDelta=juce::jmin(maxDelta,start-minimum);
        }
        if(downActive)
        {
            const auto minimum=upActive ? upperFloor : juce::jmax(upperFloor,juce::jmin(0.0f,limiterStartLowerThresholds[size_t(d)]+0.01f));
            const auto maximum=0.0f;
            const auto start=limiterStartUpperThresholds[size_t(d)];
            minDelta=juce::jmax(minDelta,start-maximum);
            maxDelta=juce::jmin(maxDelta,start-minimum);
        }
    }
    const auto appliedDelta=juce::jlimit(minDelta,maxDelta,requestedDelta);
    for(int d=0; d<5; ++d)
    {
        if(moveLower[size_t(d)]) processor.setBoundaryForDomainDb(dual,false,d,limiterStartLowerThresholds[size_t(d)]-appliedDelta);
        if(moveUpper[size_t(d)]) processor.setBoundaryForDomainDb(dual,true,d,limiterStartUpperThresholds[size_t(d)]-appliedDelta);
    }
    return double(limiterStartOutput+appliedDelta);
}
