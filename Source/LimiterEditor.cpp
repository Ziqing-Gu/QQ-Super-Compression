#include "PluginEditor.h"

void QQSuperCompressionAudioProcessorEditor::refreshDetectorWindowControl()
{
    const auto maximum = qqsc::params::snapLookaheadMs (processor.readSoundParameter (qqsc::params::lookaheadMs));
    const bool safe = processor.readSoundParameter(qqsc::params::detectorMode) >= 0.5f;
    const auto value = processor.readSoundParameter(qqsc::params::detectorWindowMs);
    const bool safeAvailable = maximum > 0.0f && value > 0.0f;
    detectorWindowLabel.setText("WINDOW",juce::dontSendNotification);
    detectorModeButton.setToggleState(safe && safeAvailable,juce::dontSendNotification);
    detectorModeButton.setEnabled(safeAvailable);
    detectorModeButton.setAlpha(safeAvailable ? 1.0f : 0.35f);
    detectorWindowValue.setTooltip(safe
        ? juce::String::fromUTF8 (u8"Window：0 ms 至当前 Lookahead。保持延迟不变，调整实际检测时长，便于连续比较声音。Transparent 开启时，Window 会影响瞬态与细节的表现；较大的值有助于减少噼啪声，也可能压低峰值前后的声音。双击输入毫秒数；上下拖动调整，Shift 微调；Alt 单击（Mac：Option）恢复为 Lookahead 的数值。")
        : juce::String::fromUTF8 (u8"Window：0 ms 至当前 Lookahead。保持延迟不变，调整实际检测时长，便于连续比较声音。较小的 Window 可能增加染色或噼啪声，尤其低于 10 ms 时可考虑开启 OS 过采样。双击输入毫秒数；上下拖动调整，Shift 微调；Alt 单击（Mac：Option）恢复为 Lookahead 的数值。"));
    detectorWindowValue.setEnabled (maximum > 0.0f);
    detectorWindowLabel.setAlpha (maximum > 0.0f ? 1.0f : 0.45f);
    if (! detectorWindowValue.isBeingEdited())
        detectorWindowValue.setText (juce::String (value, 2) + " ms", juce::dontSendNotification);
}

void QQSuperCompressionAudioProcessorEditor::setDetectorWindowValue (double value)
{
    const auto maximum = qqsc::params::snapLookaheadMs (processor.readSoundParameter (qqsc::params::lookaheadMs));
    if (std::isfinite (value))
    {
        auto* parameter = processor.getAPVTS().getParameter (qqsc::params::distort);
        const auto percent=qqsc::params::distortForWindowMs(maximum,float(juce::jlimit(0.0,double(maximum),value)));
        parameter->setValueNotifyingHost (parameter->convertTo0to1(percent));
    }
    refreshDetectorWindowControl();
}

void QQSuperCompressionAudioProcessorEditor::initialiseDetectorWindowControl()
{
    configureLabel (detectorWindowLabel, "WINDOW");
    configureLabel (detectorWindowValue, {});
    detectorWindowValue.setFont (juce::Font (juce::FontOptions (11.0f)));
    detectorWindowValue.setEditable (false, true, false);
    detectorWindowValue.setComponentID (qqsc::params::distort);
    for (auto* component : { &detectorWindowLabel, static_cast<juce::Label*>(&detectorWindowValue) })
    {
        contentRoot.addAndMakeVisible (*component);
        registerKeyboardListener (*component);
    }
    detectorWindowValue.start = [this]
    {
        beginUndoTransaction ("Window");
        display.beginHistoryGesture();
        detectorWindowDragValue = processor.readSoundParameter(qqsc::params::detectorWindowMs);
        processor.getAPVTS().getParameter (qqsc::params::distort)->beginChangeGesture();
    };
    detectorWindowValue.move = [this] (float delta, bool fine)
    {
        const auto maximum=qqsc::params::snapLookaheadMs(processor.readSoundParameter(qqsc::params::lookaheadMs));
        detectorWindowDragValue = juce::jlimit (0.0, double(maximum), detectorWindowDragValue + double(delta) * (fine ? 0.01 : 0.1));
        setDetectorWindowValue (detectorWindowDragValue);
    };
    detectorWindowValue.finish = [this] { processor.getAPVTS().getParameter (qqsc::params::distort)->endChangeGesture(); display.endHistoryGesture(); };
    detectorWindowValue.reset = [this]
    {
        beginUndoTransaction ("Reset Window");
        auto* parameter = processor.getAPVTS().getParameter (qqsc::params::distort);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(0.0f));
        parameter->endChangeGesture(); refreshDetectorWindowControl();
    };
    detectorWindowValue.onEditorShow = [this]
    {
        if (auto* editor = detectorWindowValue.getCurrentTextEditor())
        { editor->setInputRestrictions (16, "0123456789+-. msMS"); registerKeyboardListener (*editor); }
    };
    detectorWindowValue.onTextChange = [this]
    {
        const auto text = detectorWindowValue.getText().trim();
        if (! text.containsAnyOf ("0123456789")) { refreshDetectorWindowControl(); return; }
        beginUndoTransaction ("Window");
        auto* parameter = processor.getAPVTS().getParameter (qqsc::params::distort);
        parameter->beginChangeGesture(); setDetectorWindowValue (text.getDoubleValue()); parameter->endChangeGesture();
    };
    refreshDetectorWindowControl();
}

void QQSuperCompressionAudioProcessorEditor::setLimiterParameter (const char* id, float value)
{
    if (auto* parameter = processor.getAPVTS().getParameter (processor.soundParameterID(id)))
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
    initialiseDetectorWindowControl();
    initialiseLimiterStereoLinkControl();
    initialiseSTLinkControls();
    configureActionButton (detectorModeButton);
    contentRoot.addAndMakeVisible (detectorModeButton);
    registerKeyboardListener (detectorModeButton);
    detectorModeButton.setComponentID (qqsc::params::detectorMode);
    detectorModeButton.setTooltip (juce::String::fromUTF8 (u8"Transparent（透明）：开启后使用前后两侧较大的检测峰值。可尝试开启以保留瞬态与细节，同时减少噼啪声；实际听感取决于素材和设置。Window 较大时，峰值前后的声音也可能被压低。使用 Range 出现噼啪声时也可尝试开启。默认关闭。Lookahead 或 Window 为 0 ms 时变灰、不可用；恢复非零值后保留之前的开关状态。不额外加入 Attack / Release。"));
    detectorModeButton.onClick = [this]
    {
        beginUndoTransaction ("Detector Mode");
        if (!detectorModeButton.isEnabled()) return;
        const auto next = processor.readSoundParameter (qqsc::params::detectorMode) >= 0.5f ? 0 : 1;
        setChoiceParameter (qqsc::params::detectorMode, next);
        updateLimiterUi();
    };
    for (auto* button : { &limiterButton,&limiterLinkButton,&unityMonitorButton,&truePeakButton,&tpRecoveryButton,&ceilingEnabledButton })
    {
        configureActionButton (*button); contentRoot.addAndMakeVisible (*button); registerKeyboardListener (*button);
    }
    limiterButton.setComponentID ("limiterMode");
    ceilingEnabledButton.setComponentID(qqsc::params::ceilingEnabled);
    ceilingEnabledButton.getProperties().set("qqscSmallLink",true);
    ceilingEnabledButton.setTooltip(juce::String::fromUTF8(u8"Ceiling 开关：默认开启。关闭后停止最终峰值限制（含真峰值与采样峰值限制），不再应用 Ceiling 电平调整及附加延迟；压缩、Output LINK 保持有效。原设置保留，开关随工程、预置和 A/B 保存。"));
    ceilingEnabledButton.onClick=[this]
    {
        beginUndoTransaction("Ceiling On / Off");
        setChoiceParameter(qqsc::params::ceilingEnabled,processor.isCeilingEnabled() ? 0 : 1);
        processor.notifyHostProcessingLatency();
        updateLimiterUi();
    };
    limiterLinkButton.getProperties().set ("qqscSmallLink",true);
    limiterButton.setTooltip (juce::String::fromUTF8 (u8"处理模式：单击在 COMPRESSOR、CLIPPER、LIMITER 之间切换，各自保存参数。Compressor 为常规压缩；Clipper 仅下压，Ratio 为 10:1 至 1000:1，使用单组立体声控制和 L/R Link，没有双压和 Ceiling；Limiter 支持单压、双压，并带有 Ceiling 最终峰值限制。Clipper 的 Output 联动默认关闭，可用 LINK 开启。"));
    truePeakButton.setComponentID("truePeakLimiting");
    truePeakButton.getProperties().set("qqscSmallLink",true);
    truePeakButton.setTooltip(juce::String::fromUTF8 (u8"TP：点亮时启用真峰值限制；关闭时限制普通采样峰值。与压缩共用 OS 过采样设置，开启 TP 不会自动改变 OS 倍率。"));
    truePeakButton.onClick=[this]
    {
        beginUndoTransaction("True Peak Limiting");
        const bool enabling=!processor.isTruePeakSelected();
        // TP changes the peak reference; OS and recovery remain user choices.
        setChoiceParameter(qqsc::params::truePeakLimiting,enabling ? 1 : 0);
        processor.notifyHostProcessingLatency();
        updateLimiterUi();
        updateCeilingOversamplingUi();
    };
    tpRecoveryButton.setComponentID("tpRecoveryMode");
    tpRecoveryButton.getProperties().set("qqscSmallLink",true);
    tpRecoveryButton.setTooltip(juce::String::fromUTF8 (u8"Ceiling 限制器的恢复方式：TIGHT 恢复较快，便于提升响度；AUTO 自动适应信号，为默认选项；SMOOTH 恢复较慢，变化更平缓。TP 开关两种状态均可选择。"));
    tpRecoveryButton.onClick=[this]
    {
        beginUndoTransaction("Ceiling Recovery");
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
    unityMonitorButton.setTooltip(juce::String::fromUTF8 (u8"耳机监听：在 Limiter 和已开启的 Ceiling 处理后抵消 Output Gain，便于比较处理前后的声音。保留 Threshold、Ratio、Makeup 和 Mix 的处理结果；Adjust 只移动图表，不改变监听音量。开关随工程保存，不随 A/B 切换。"));
    unityMonitorButton.onClick=[this]
    {
        processor.setUnityMonitorEnabled(!processor.isUnityMonitorEnabled());
        updateLimiterUi();
    };
    limiterLinkButton.setTooltip (juce::String::fromUTF8 (u8"Output 联动：调整下压 Threshold 或 Makeup 时，Output Gain 按相同 dB 数反向变化；调整 Output 时，下压 Threshold 按相同 dB 数反向变化。联动只由此 LINK 开关控制，Ratio 为 1:1 或下压关闭时仍然有效，不受其他参数影响。双压的上压阈值独立。"));
    limiterButton.onClick = [this]
    {
        finishCompressionControlGestures(); beginUndoTransaction ("Processor Mode");
        processor.setProcessorModeFromEditor((processor.getProcessorMode()+1)%3);
        updateModeUi();
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
    configureLabel(monitorCeilingLabel,{});
    monitorCeilingLabel.setFont(juce::Font(juce::FontOptions(8.0f)));
    monitorCeilingLabel.setTooltip(juce::String::fromUTF8 (u8"MON：等音量监听时的峰值上限，等于 Ceiling 减去 Output Gain。不会改写 Ceiling 数值或 TP 开关。"));
    contentRoot.addAndMakeVisible(monitorCeilingLabel);
    configureLabel (ceilingValue,{});
    ceilingValue.setFont (juce::Font (juce::FontOptions (11.0f)));
    ceilingValue.setEditable (false,true,false);
    ceilingValue.setComponentID ("ceilingValue");
    ceilingValue.setTooltip (juce::String::fromUTF8 (u8"Ceiling：最终峰值上限，范围 -24 至 0 dB，默认为 0。TP 开启时限制真峰值，关闭时限制采样峰值；两者均使用统一的 OS 设置。MON 显示耳机监听时的峰值上限。双击输入，上下拖动调整，Shift 微调；Alt 单击（Mac：Option）恢复为 0 dB。"));
    for (auto* c : { static_cast<juce::Component*>(&ceilingValue) })
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
        if(!processor.isMasteringMode()) return normalText(text);
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
    // LINK is a control relationship, independent of whether dynamics are
    // active: Makeup and Single Threshold / Dual DOWN drive Output by inverse
    // 1:1 dB deltas. Ratio, enables, Mix and algorithms never gate this link.
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
    const bool limiter=processor.isMasteringMode(), dual=attachedCompressionMode==1;
    compressionModeButton.setVisible(!processor.isClipperMode());
    const juce::ScopedValueSetter<bool> guard(limiterUpdating,true);
    const bool bilateral = processor.readSoundParameter (qqsc::params::detectorMode) >= 0.5f;
    detectorModeButton.setToggleState (bilateral, juce::dontSendNotification);
    refreshDetectorWindowControl();
    limiterButton.setButtonText(processor.isLimiterMode() ? "LIMITER" : processor.isClipperMode() ? "CLIPPER" : "COMPRESSOR");
    limiterButton.setToggleState(limiter,juce::dontSendNotification);
    limiterLinkButton.setToggleState(processor.isLimiterLinked(),juce::dontSendNotification);
    refreshLimiterStereoLinkControl();
    unityMonitorButton.setToggleState(processor.isUnityMonitorEnabled(),juce::dontSendNotification);
    unityMonitorButton.setVisible(processor.isLimiterMode());
    truePeakButton.setToggleState(processor.isTruePeakSelected(),juce::dontSendNotification);
    truePeakButton.setVisible(processor.isLimiterMode());
    ceilingEnabledButton.setVisible(processor.isLimiterMode());
    ceilingEnabledButton.setToggleState(processor.isCeilingEnabled(),juce::dontSendNotification);
    const bool ceilingActive=processor.isCeilingActive();
    for(auto* c : {static_cast<juce::Component*>(&ceilingValue),static_cast<juce::Component*>(&truePeakButton),static_cast<juce::Component*>(&tpRecoveryButton)})
    { c->setEnabled(ceilingActive);c->setAlpha(ceilingActive ? 1.f : .35f); }
    const auto recovery=processor.getTpRecoveryMode();
    tpRecoveryButton.setButtonText(recovery==qqsc::params::tpTight ? "TIGHT"
        : recovery==qqsc::params::tpSmooth ? "SMOOTH" : "AUTO");
    tpRecoveryButton.setVisible(processor.isLimiterMode());
    monitorCeilingLabel.setVisible(processor.isUnityMonitorActive() && ceilingActive);
    inputOutputLinkButton.setVisible(false);
    ceilingSlider.setVisible(false);
    limiterLinkButton.setVisible(limiter); ceilingValue.setVisible(processor.isLimiterMode());
    ceilingPanel.setVisible(processor.isLimiterMode());ceilingPanel.repaint();
    updateCeilingOversamplingUi();
    if(attachedLimiter!=processor.getProcessorMode())
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
        inputGainSlider.setValue(processor.getDisplayAdjustDb(),juce::dontSendNotification);
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
        const auto* id=processor.soundParameterID(qqsc::params::outputGainDb);
        outputGainSlider.getProperties().set("qqscParameterID",id);
        outputGainAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.getAPVTS(),id,outputGainSlider);
        outputGainSlider.valueFromTextFunction=limiterOutputText;
        attachedLimiter=processor.getProcessorMode();
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
        down.setTooltip(limiter ? juce::String::fromUTF8 (u8"下压 Ratio：1:1 至 1000:1。数值越大，下压越强；1:1 不下压。") : juce::String::fromUTF8 (u8"下压 Ratio：1:1 至 200:1。数值越大，下压越强；1:1 不下压。"));
        const int kind=processor.isClipperMode() ? 4 : (dual ? 2 : 0)+(limiter ? 1 : 0);
        if(int(main[d]->getProperties().getWithDefault("limiterRangeKind",-1))!=kind)
        {
            if(kind==4)
            {
                const auto r=qqsc::params::dynamicsRatioRange(10.f,1000.f);
                main[d]->setNormalisableRange({10.0,1000.0,
                    [r](double,double,double n){return double(r.convertFrom0to1(float(n)));},
                    [r](double,double,double v){return double(r.convertTo0to1(float(v)));}});
            }
            else if(kind==1) main[d]->setNormalisableRange(qqsc::params::limiterSingleRange());
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
        main[d]->setResetValue(processor.isClipperMode() ? 10.0 : 1.0);
        main[d]->setTooltip(processor.isClipperMode()
            ? juce::String::fromUTF8(u8"Clipper 下压 Ratio：10:1 至 1000:1，仅保留下压，没有双压和 Ceiling。Alt 单击（Mac：Option）恢复为 10:1。")
            : dual
            ? (limiter ? juce::String::fromUTF8 (u8"上压 Ratio：1:8 至 1:1。数值越接近 1:8，提升越强；1:1 不提升。") : juce::String::fromUTF8 (u8"上压 Ratio：1:200 至 1:1。数值越接近 1:200，提升越强；1:1 不提升。"))
            : (limiter ? juce::String::fromUTF8 (u8"Ratio：上压为 1:8 至 1:1，下压为 1:1 至 1000:1。Alt 单击（Mac：Option）恢复为 1:1，不作动态压缩。")
                       : juce::String::fromUTF8 (u8"Ratio：1:200 至 200:1。小于 1:1 为上压，大于 1:1 为下压，1:1 不作动态压缩。")));
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
            auto* output = processor.getAPVTS().getParameter(processor.soundParameterID(qqsc::params::limiterOutputDb));
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
    auto* p=processor.getAPVTS().getParameter(processor.soundParameterID(qqsc::params::limiterOutputDb));
    const auto gestureOpen=std::find(companionGestureParameters.begin(),companionGestureParameters.end(),p)!=companionGestureParameters.end();
    if(!gestureOpen) p->beginChangeGesture();
    setLinkedControlValue(outputGainSlider,next);
    if(!gestureOpen) p->endChangeGesture();
}

void QQSuperCompressionAudioProcessorEditor::beginLimiterOutputGesture()
{
    endLinkedGesture(); beginUndoTransaction("Output / Threshold 1:1");
    limiterOutputGesture=true; limiterStartOutput=float(outputGainSlider.getValue());
    const auto mode=juce::roundToInt(processor.readSoundParameter(qqsc::params::processingMode));
    const bool dual=attachedCompressionMode==1;
    for(size_t d=0; d<5; ++d)
    {
        limiterStartLowerThresholds[d]=processor.getBoundaryForDomainDb(dual,false,int(d));
        limiterStartUpperThresholds[d]=processor.getBoundaryForDomainDb(dual,true,int(d));
        const bool active=processor.isMasteringMode() ? d==1 : mode==0 ? d==0 : mode==2 ? d==1||d==2 : d==3||d==4;
        if(!active) continue;

        const auto openGesture=[&](const char* normalID)
        {
            auto* p=processor.getAPVTS().getParameter(processor.soundParameterID(normalID));
            if(p!=nullptr && std::find(companionGestureParameters.begin(),companionGestureParameters.end(),p)==companionGestureParameters.end())
            { p->beginChangeGesture(); companionGestureParameters.push_back(p); }
        };

        if(!dual)
        {
            openGesture(qqsc::params::thresholdIds[d]);
            continue;
        }

        openGesture(qqsc::params::downThresholdIds[d]);
    }
}

double QQSuperCompressionAudioProcessorEditor::applyLimiterOutputChange (double requested)
{
    const auto requestedDelta=float(requested-limiterStartOutput);
    if(std::abs(requestedDelta)<1.e-5f) return limiterStartOutput;

    // Output and the selected Threshold move by equal opposite dB amounts,
    // including Ratio 1:1 and DOWN OFF. Dual links DOWN only; UP is independent.
    // Parameter bounds limit the shared delta, never the dynamics settings.
    float minDelta=-120.0f-limiterStartOutput;
    float maxDelta= 120.0f-limiterStartOutput;
    const auto mode=juce::roundToInt(processor.readSoundParameter(qqsc::params::processingMode));
    const bool dual=attachedCompressionMode==1;
    std::array<bool,5> moveLower {}, moveUpper {};
    for(int d=0; d<5; ++d)
    {
        const bool active=processor.isMasteringMode() ? d==1 : mode==0 ? d==0 : mode==2 ? d==1||d==2 : d==3||d==4;
        if(!active) continue;

        if(!dual)
        {
            moveLower[size_t(d)]=true;
            const auto floor=processor.boundaryMinimumDb(false);
            const auto minimum=floor;
            const auto maximum=qqsc::params::thresholdMaximumDb;
            const auto start=limiterStartLowerThresholds[size_t(d)];
            minDelta=juce::jmax(minDelta,start-maximum);
            maxDelta=juce::jmin(maxDelta,start-minimum);
            continue;
        }

        moveLower[size_t(d)]=false; moveUpper[size_t(d)]=true;

        const auto upperFloor=processor.boundaryMinimumDb(true);
        {
            const auto minimum=juce::jmax(upperFloor,limiterStartLowerThresholds[size_t(d)]);
            const auto maximum=qqsc::params::thresholdMaximumDb;
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

void QQSuperCompressionAudioProcessorEditor::refreshLimiterStereoLinkControl()
{
    refreshSTLinkControls();
    limiterStereoLinkValue.setVisible(processor.isMasteringMode());
    if (!limiterStereoLinkValue.isBeingEdited())
        limiterStereoLinkValue.setText(juce::String(processor.readSoundParameter(qqsc::params::limiterStereoLink),2)+" %",juce::dontSendNotification);
}

void QQSuperCompressionAudioProcessorEditor::initialiseLimiterStereoLinkControl()
{
    configureLabel(limiterStereoLinkValue,{});
    limiterStereoLinkValue.setFont(juce::Font(juce::FontOptions(11.0f)));
    limiterStereoLinkValue.setEditable(false,true,false);
    limiterStereoLinkValue.setComponentID(qqsc::params::limiterStereoLink);
    limiterStereoLinkValue.setTooltip(juce::String::fromUTF8 (u8"L/R Link：左右声道限制量的相互影响。0% 各自独立，100% 共同采用较强的衰减量；Limiter 中 Ceiling 也参与联动。不会混合左右声道的音频。追求响度可从 0% 开始，保留声像细节可适当调高。双击输入百分比，上下拖动调整，Shift 微调；Alt 单击（Mac：Option）恢复为 0%。"));
    contentRoot.addAndMakeVisible(limiterStereoLinkValue);
    registerKeyboardListener(limiterStereoLinkValue);
    limiterStereoLinkValue.start=[this] {
        beginUndoTransaction("Limiter L/R Link");
        limiterStereoLinkDragValue=processor.readSoundParameter(qqsc::params::limiterStereoLink);
        processor.getAPVTS().getParameter(processor.soundParameterID(qqsc::params::limiterStereoLink))->beginChangeGesture();
    };
    limiterStereoLinkValue.move=[this](float delta,bool fine) {
        limiterStereoLinkDragValue=juce::jlimit(0.0,100.0,limiterStereoLinkDragValue+delta*(fine ? .01 : .1));
        setLimiterParameter(qqsc::params::limiterStereoLink,float(limiterStereoLinkDragValue));
        refreshLimiterStereoLinkControl();
    };
    limiterStereoLinkValue.finish=[this] { processor.getAPVTS().getParameter(processor.soundParameterID(qqsc::params::limiterStereoLink))->endChangeGesture(); };
    limiterStereoLinkValue.reset=[this] {
        limiterStereoLinkValue.start();setLimiterParameter(qqsc::params::limiterStereoLink,0);
        limiterStereoLinkValue.finish();refreshLimiterStereoLinkControl();
    };
    limiterStereoLinkValue.onEditorShow=[this] {
        if(auto* ed=limiterStereoLinkValue.getCurrentTextEditor())
        {ed->setInputRestrictions(16,"0123456789+-. %");registerKeyboardListener(*ed);}
    };
    limiterStereoLinkValue.onTextChange=[this] {
        const auto text=limiterStereoLinkValue.getText().trim();
        if(text.containsAnyOf("0123456789")) {
            limiterStereoLinkValue.start();
            setLimiterParameter(qqsc::params::limiterStereoLink,juce::jlimit(0.f,100.f,text.getFloatValue()));
            limiterStereoLinkValue.finish();
        }
        refreshLimiterStereoLinkControl();
    };
}

void QQSuperCompressionAudioProcessorEditor::refreshSTLinkControls()
{
    const bool visible=!processor.isMasteringMode() && processor.readSoundParameter(qqsc::params::processingMode)<.5f;
    for(size_t i=0;i<2;++i)
    {
        stLinkValues[i].setVisible(visible);stLinkLabels[i].setVisible(visible);
        if(!stLinkValues[i].isBeingEdited())stLinkValues[i].setText(
            juce::String(processor.readSoundParameter(i==0 ? "stLRLink" : "stMSLink"),1)+" %",juce::dontSendNotification);
    }
}
void QQSuperCompressionAudioProcessorEditor::initialiseSTLinkControls()
{
    for(size_t i=0;i<2;++i)
    {
        const auto* id=i==0 ? "stLRLink" : "stMSLink";
        auto& value=stLinkValues[i];auto& label=stLinkLabels[i];
        configureLabel(label,i==0 ? "L/R LINK" : "M/S LINK");configureLabel(value,{});
        label.setFont(juce::Font(juce::FontOptions(8.f)));value.setFont(juce::Font(juce::FontOptions(10.f)));
        value.setEditable(false,true,false);value.setComponentID(id);
        value.setTooltip(juce::String::fromUTF8(u8"Comp/ST 联动：两项均为 100% 时保持原 ST 声音。降低 L/R Link，逐渐加入左右独立压缩；降低 M/S Link，逐渐加入中侧独立压缩。两项均为 0% 时两种结果各占一半，仍共用一组压缩参数。双击输入，上下拖动，Shift 微调；Alt 单击（Mac：Option）恢复 100%。"));
        label.setTooltip(value.getTooltip());
        contentRoot.addAndMakeVisible(label);contentRoot.addAndMakeVisible(value);registerKeyboardListener(value);
        value.start=[this,i,id]{beginUndoTransaction("ST Link");stLinkDragValues[i]=processor.readSoundParameter(id);processor.getAPVTS().getParameter(id)->beginChangeGesture();};
        value.move=[this,i,id](float delta,bool fine){stLinkDragValues[i]=juce::jlimit(0.,100.,stLinkDragValues[i]+delta*(fine?.01:.1));setLimiterParameter(id,float(stLinkDragValues[i]));refreshSTLinkControls();};
        value.finish=[this,id]{processor.getAPVTS().getParameter(id)->endChangeGesture();};
        value.reset=[this,i,id]{stLinkValues[i].start();setLimiterParameter(id,100);stLinkValues[i].finish();refreshSTLinkControls();};
        value.onEditorShow=[this,i]{if(auto* ed=stLinkValues[i].getCurrentTextEditor()){ed->setInputRestrictions(16,"0123456789+-. %");registerKeyboardListener(*ed);}};
        value.onTextChange=[this,i,id]{const auto text=stLinkValues[i].getText();if(text.containsAnyOf("0123456789")){stLinkValues[i].start();setLimiterParameter(id,juce::jlimit(0.f,100.f,text.getFloatValue()));stLinkValues[i].finish();}refreshSTLinkControls();};
    }
}
