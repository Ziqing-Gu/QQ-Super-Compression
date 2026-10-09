#include "PluginEditor.h"
#include <iostream>
#include <fstream>

struct QQSCVisualCheck
{
    static void floorChecks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    static void algorithmChecks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    static void unityDisplayCheck(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&);
    static void zeroLookaheadDetectorCheck(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&);
    static void windowReplayCheck(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&);
    static void revisionThreeChecks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    static void revisionFourChecks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    static void up1000Checks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    static void inputOutputLinkChecks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    static void dbComparisonChecks(QQSuperCompressionAudioProcessorEditor&, QQSuperCompressionAudioProcessor&, const juce::File&);
    struct GestureProbe final : juce::AudioProcessorParameter::Listener
    {
        explicit GestureProbe(juce::AudioProcessorParameter& parameterIn) : watched(parameterIn) { watched.addListener(this); }
        ~GestureProbe() override { watched.removeListener(this); }
        void parameterValueChanged(int,float) override {}
        void parameterGestureChanged(int,bool starting) override { events.push_back(starting); }
        juce::AudioProcessorParameter& watched;
        std::vector<bool> events;
    };
    static juce::MouseEvent mouseEvent(juce::Component& component, bool alt=false)
    {
        const auto position=component.getLocalBounds().toFloat().getCentre();
        const auto now=juce::Time::getCurrentTime();
        return {juce::Desktop::getInstance().getMainMouseSource(),position,
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | (alt ? juce::ModifierKeys::altModifier : 0)),
                1.0f,0.0f,0.0f,0.0f,0.0f,&component,&component,now,position,now,1,false};
    }
    static std::unique_ptr<juce::FileOutputStream> output (const juce::File& file)
    {
        auto stream = file.createOutputStream();
        if (stream == nullptr || ! stream->setPosition (0) || stream->truncate().failed())
            throw std::runtime_error ("Cannot create test output");
        return stream;
    }
    static void parameter (QQSuperCompressionAudioProcessor& p, const char* id, float value)
    {
        auto* parameter = p.getAPVTS().getParameter (id);
        if (parameter == nullptr) throw std::runtime_error (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    static void snapshot (QQSuperCompressionAudioProcessorEditor& editor, const juce::File& file, float scale = 1.0f)
    {
        if (editor.getWidth() * 2 != editor.getHeight() * 3
            || editor.getConstrainer()->getFixedAspectRatio() != 1.5)
            throw std::runtime_error ("Landscape aspect ratio changed");
        for (auto* child : editor.contentRoot.getChildren())
            if (child->isVisible() && ! editor.contentRoot.getLocalBounds().contains (child->getBounds()))
                throw std::runtime_error ("Visible control exceeds landscape bounds");
        if (editor.display.getBounds() != juce::Rectangle<int> (22, 80, 866, 530)
            || editor.meters.getBounds() != juce::Rectangle<int> (978, 80, 200, 530))
            throw std::runtime_error ("Landscape Display/meter geometry changed");
        const auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, scale);
        auto stream = output (file);
        if (stream == nullptr || ! juce::PNGImageFormat().writeImageToStream (image, *stream))
            throw std::runtime_error ("PNG write failed");
        if (editor.theme == qqsc::ui::Theme::light)
        {
            auto regions = output (file.getSiblingFile (file.getFileNameWithoutExtension() + "-rotary-bounds.txt"));
            std::function<void(juce::Component&)> visit = [&] (juce::Component& c)
            {
                if (auto* slider = dynamic_cast<juce::Slider*> (&c))
                {
                    const auto style = slider->getSliderStyle();
                    if (style >= juce::Slider::Rotary && style <= juce::Slider::RotaryHorizontalVerticalDrag)
                    {
                        const auto r = editor.getLocalArea (slider, slider->getLookAndFeel().getSliderLayout (*slider).sliderBounds);
                        regions->writeText (r.toString() + "\n", false, false, "\n");
                    }
                }
                for (auto* child : c.getChildren()) if (child->isVisible()) visit (*child);
            };
            visit (editor);
            if (file.getFileNameWithoutExtension() == "warm-ST")
                for (float pixelScale : {1.0f, 2.0f})
                {
                    const auto crop = editor.createComponentSnapshot ({16, 620, 1168, 162}, true, pixelScale);
                    auto cropStream = output (file.getSiblingFile (pixelScale == 1.0f ? "warm-bottom.png" : "warm-bottom-2x.png"));
                    if (! juce::PNGImageFormat().writeImageToStream (crop, *cropStream))
                        throw std::runtime_error ("Light bottom snapshot failed");
                }
        }
    }

    static void bounds (juce::Component& c, juce::OutputStream& out, const juce::String& path)
    {
        juce::String description = path + "|" + c.getBounds().toString() + "|" + juce::String (c.isVisible() ? 1 : 0);
        if (auto* button = dynamic_cast<juce::TextButton*> (&c)) description += "|" + button->getButtonText();
        out.writeText (description + "\n", false, false, "\n");
        for (int i = 0; i < c.getNumChildComponents(); ++i)
            bounds (*c.getChildComponent (i), out, path + "/" + juce::String (i));
    }

    static void meterGradientCheck (QQSuperCompressionAudioProcessorEditor& editor,
                                    QQSuperCompressionAudioProcessor& processor, const juce::File& dir)
    {
        auto& meters = processor.getMeterState();
        constexpr float level = 0.65f;
        meters.inputDb0.store (-60.0f + 63.0f * level);
        meters.inputDb1.store (-60.0f + 63.0f * level);
        meters.outputDb0.store (-60.0f + 63.0f * level);
        meters.outputDb1.store (-60.0f + 63.0f * level);
        meters.gainReductionDb0.store (36.0f * level);
        meters.gainReductionDb1.store (36.0f * level);
        meters.gainReductionHoldDb0.store (0.0f);
        meters.gainReductionHoldDb1.store (0.0f);
        const auto image = editor.meters.createComponentSnapshot (editor.meters.getLocalBounds());
        auto stream = output (dir.getChildFile ("warm-meter-depth.png"));
        if (! juce::PNGImageFormat().writeImageToStream (image, *stream))
            throw std::runtime_error ("Meter PNG write failed");
        const float groupWidth = (image.getWidth() - 20.0f) / 3.0f;
        const float firstBarCentre = 6.0f + (groupWidth - 2.5f) * 0.25f;
        const float barTop = 6.0f + 24.0f + 18.0f + 3.0f;
        const float barBottom = image.getHeight() - 6.0f - 32.0f - 3.0f;
        const float fillHeight = (barBottom - barTop) * level;
        const auto luminance = [&] (int x, float y)
        {
            float total = 0.0f;
            for (int dy = -5; dy <= 5; ++dy)
            {
                const auto pixel = image.getPixelAt (x, juce::roundToInt (y) + dy);
                total += pixel.getFloatRed() * 0.2126f + pixel.getFloatGreen() * 0.7152f
                         + pixel.getFloatBlue() * 0.0722f;
            }
            return total / 11.0f;
        };
        // Input/Output keep the previous full-height gradient; the dynamics
        // meter now has a signed centre-zero scale and is checked separately.
        for (int group = 0; group < 2; ++group)
        {
            const auto x = juce::roundToInt (firstBarCentre + group * (groupWidth + 4.0f));
            const auto richY = group == 2 ? barTop + fillHeight * 0.10f : barBottom - fillHeight * 0.10f;
            const auto paleY = group == 2 ? barTop + fillHeight * 0.90f : barBottom - fillHeight * 0.90f;
            if (luminance (x, paleY) - luminance (x, richY) < 0.12f)
                throw std::runtime_error ("Meter vertical tonal contrast missing");
        }
        std::cout << "PASS: actual Input/Output meters retain vertical depth.\n";
    }

    static void upDownChecks (QQSuperCompressionAudioProcessorEditor& editor,
                              QQSuperCompressionAudioProcessor& processor, const juce::File& dir)
    {
        // Drive the REAL visible Ratio sliders, then measure actual processBlock
        // output. This catches attachment/parameter/DSP mismatches that setting
        // APVTS parameters directly in a DSP-only test cannot catch.
        parameter(processor,"compressionMode",1); parameter(processor,"processingMode",0);
        parameter(processor,"domainLink",0); parameter(processor,"lookaheadMs",26);
        parameter(processor,"dualRatioLink",0);
        parameter(processor,"inputGainDb",0); parameter(processor,"outputGainDb",0);
        parameter(processor,"makeupGainDb",0); parameter(processor,"mix",100); parameter(processor,"keySource",0);
        processor.setBoundaryForDomainDb(true,false,0,-50);
        processor.setBoundaryForDomainDb(true,true,0,-30);
        editor.timerCallback();
        constexpr double probeRate=48000.0, probeFrequency=400.0, probeAmplitude=0.01;
        int64_t probeSample=0;
        double previousLift=-1;
        for(const double wantedRatio : {1.0,0.125,1.0/32.0})
        {
            editor.ratioSlider.setValue(wantedRatio,juce::sendNotificationSync);
            const auto actualParameter=processor.getAPVTS().getRawParameterValue("upRatio")->load();
            if(std::abs(actualParameter-wantedRatio)>1e-6)
                throw std::runtime_error("UP knob writes the wrong DSP parameter/value");
            double outputPower=0,dryPower=0;
            for(int block=0;block<40;++block)
            {
                juce::AudioBuffer<float> audio(2,800); juce::MidiBuffer midi;
                for(int i=0;i<800;++i)
                {
                    const auto value=static_cast<float>(probeAmplitude*std::sin(juce::MathConstants<double>::twoPi*probeFrequency*(probeSample+i)/probeRate));
                    audio.setSample(0,i,value); audio.setSample(1,i,value);
                }
                processor.processBlock(audio,midi);
                if(block==39)
                    for(int i=0;i<800;++i)
                    {
                        const double dry=probeAmplitude*std::sin(juce::MathConstants<double>::twoPi*probeFrequency*(probeSample+i-processor.getLatencySamples())/probeRate);
                        outputPower+=static_cast<double>(audio.getSample(0,i))*audio.getSample(0,i); dryPower+=dry*dry;
                    }
                probeSample+=800;
            }
            const auto lift=10.0*std::log10(outputPower/dryPower);
            if(lift<=previousLift+0.1 || (wantedRatio==1.0 && std::abs(lift)>0.001))
                throw std::runtime_error("Actual UP knob does not progressively raise eligible audio");
            previousLift=lift;
            const auto expected=10.0*(1.0-wantedRatio); // finite gate: fixed dB slope, 10dB below anchor
            if(std::abs(lift-expected)>0.01) throw std::runtime_error("UP knob output does not match displayed Ratio value");
            std::cout<<"PASS: real UP knob "<<wantedRatio<<" -> upRatio="<<actualParameter<<" -> actual lift "<<lift<<" dB (-40dB signal, UP -50dB, DOWN -30dB).\n";
        }
        const char* upwardIds[]={"upRatio","upRatioL","upRatioR","upRatioM","upRatioS"};
        const char* downwardIds[]={"downRatio","downRatioL","downRatioR","downRatioM","downRatioS"};
        for(int domain=0;domain<5;++domain)
        {
            parameter(processor,"processingMode",static_cast<float>(domain==0 ? 0 : domain<3 ? 2 : 1)); editor.timerCallback();
            editor.mainRatioControls()[static_cast<size_t>(domain)]->setValue(0.25,juce::sendNotificationSync);
            editor.downRatioSliders[static_cast<size_t>(domain)]->setValue(12,juce::sendNotificationSync);
            if(std::abs(processor.getAPVTS().getRawParameterValue(upwardIds[domain])->load()-0.25f)>1e-6f
               || std::abs(processor.getAPVTS().getRawParameterValue(downwardIds[domain])->load()-12.0f)>1e-5f)
                throw std::runtime_error("Domain ratio knob is attached to wrong parameter");
        }
        std::cout<<"PASS: all ST/LR/MS Up/Down knob-to-parameter bindings.\n";
        parameter(processor,"domainLink",0); parameter(processor,"processingMode",0);
        for(int startingMode : {0,1})
        {
            parameter(processor,"compressionMode",static_cast<float>(startingMode)); editor.timerCallback();
            GestureProbe oldParameter(*processor.getAPVTS().getParameter(startingMode==0 ? "ratio" : "upRatio"));
            GestureProbe newParameter(*processor.getAPVTS().getParameter(startingMode==0 ? "upRatio" : "ratio"));
            const auto event=mouseEvent(editor.ratioSlider);
            editor.ratioSlider.mouseDown(event);
            parameter(processor,"compressionMode",static_cast<float>(1-startingMode)); editor.timerCallback();
            editor.ratioSlider.mouseUp(event);
            if(oldParameter.events!=std::vector<bool>{true,false} || !newParameter.events.empty())
                throw std::runtime_error("Host mode change during Ratio drag leaves unmatched parameter gestures");
        }
        parameter(processor,"compressionMode",1); parameter(processor,"processingMode",2);
        parameter(processor,"domainLink",1); editor.timerCallback();
        for(bool upward : {true,false})
        {
            auto* source=upward ? editor.mainRatioControls()[1] : editor.downRatioSliders[1].get();
            const char* sourceId=upward ? "upRatioL" : "downRatioL";
            const char* targetId=upward ? "upRatioR" : "downRatioR";
            parameter(processor,sourceId,upward ? 0.125f : 8.0f);
            parameter(processor,targetId,upward ? 0.25f : 10.0f); editor.timerCallback();
            GestureProbe sourceGesture(*processor.getAPVTS().getParameter(sourceId));
            GestureProbe targetGesture(*processor.getAPVTS().getParameter(targetId));
            bool committed=false;
            for(auto* child : source->getChildren())
                if(auto* label=dynamic_cast<juce::Label*>(child))
                {
                    label->showEditor();
                    if(auto* input=label->getCurrentTextEditor())
                    {
                        input->setText(upward ? "1:4" : "12:1");
                        label->hideEditor(false); committed=true;
                    }
                    break;
                }
            const float expectedTarget=upward ? 0.375f : 14.0f;
            if(!committed || sourceGesture.events!=std::vector<bool>{true,false}
               || targetGesture.events!=std::vector<bool>{true,false}
               || std::abs(processor.getAPVTS().getRawParameterValue(targetId)->load()-expectedTarget)>1e-5f)
                throw std::runtime_error("Linked Ratio text commit failed to record both host gestures/relative values");
        }
        parameter(processor,"domainLink",0);
        std::cout<<"PASS: mode switch during real Ratio drag closes old host gesture; linked Up/Down numeric edits record both parameters.\n";
        // Deliberately leave audio metering in ST, as when playback is stopped.
        // Boundary geometry must immediately follow the requested editor mode.
        processor.getMeterState().processingMode.store(0);
        for(int mode : {0,2,1})
        for(float trim : {0.0f,6.0f,-9.0f})
        {
            parameter(processor,"processingMode",static_cast<float>(mode));
            parameter(processor,"inputGainDb",trim); editor.timerCallback();
            const int first=mode==0 ? 0 : mode==2 ? 1 : 3;
            const int last=mode==0 ? 0 : first+1;
            for(int domain=first;domain<=last;++domain)
            {
                processor.setBoundaryForDomainDb(true,true,domain,-12);
                processor.setBoundaryForDomainDb(true,false,domain,-60);
                editor.timerCallback();
                for(bool upper : {false,true})
                {
                    auto* slider=upper ? editor.upperBoundarySliders[static_cast<size_t>(domain)].get()
                                       : editor.lowerBoundaryControls()[static_cast<size_t>(domain)];
                    const float thumbY=static_cast<float>(slider->getY())+slider->getBoundaryThumbY();
                    const float lineY=static_cast<float>(editor.display.getY())+editor.display.getBoundaryYForDomainDb(domain,static_cast<float>(slider->getValue()));
                    if(std::abs(thumbY-lineY)>0.001f) throw std::runtime_error("Fader thumb and Display line are vertically misaligned");
                    const auto track=slider->boundaryPlotBounds();
                    const auto plot=editor.display.getBoundaryPlotForDomain(domain);
                    if(std::abs(track.getHeight()-plot.getHeight())>0.001f)
                        throw std::runtime_error("Fader rail does not span full Display plot");
                    const auto previousValue=slider->getValue();
                    const auto event=mouseEvent(*slider);
                    slider->mouseDown(event); slider->mouseUp(event);
                    if(slider->getSliderSnapsToMousePosition() || std::abs(slider->getValue()-previousValue)>0.001)
                        throw std::runtime_error("Native mouseDown jumps the aligned boundary fader");
                }
            }
            if(mode!=0 && editor.display.getBoundaryPlotForDomain(first).getY()>=editor.display.getBoundaryPlotForDomain(last).getY())
                throw std::runtime_error("Stopped LR/MS mode retains stale single-plot geometry");
        }
        parameter(processor,"inputGainDb",0);
        if(editor.compressionModeButton.getBounds()!=juce::Rectangle<int>(380,620,60,21))
            throw std::runtime_error("Single/Dual switch not at requested bottom-panel location");
        std::cout<<"PASS: full-length rail/thumb-to-Display alignment in ST/LR/MS with input trim, including stopped mode changes.\n";
        parameter(processor,"domainLink",0);
        const auto boundaryControls=editor.lowerBoundaryControls();
        for(int compression=0;compression<2;++compression)
        {
            parameter(processor,"compressionMode",static_cast<float>(compression)); editor.timerCallback();
            for(int domain=0;domain<5;++domain)
            {
                const int channelMode=domain==0 ? 0 : domain<3 ? 2 : 1;
                parameter(processor,"processingMode",static_cast<float>(channelMode)); editor.timerCallback();
                processor.setBoundaryForDomainDb(compression==1,false,domain,-40);
                processor.setBoundaryForDomainDb(compression==1,true,domain,-20);
                editor.timerCallback();
                editor.beginBoundaryGesture(domain,false);
                boundaryControls[static_cast<size_t>(domain)]->setValue(-10,juce::sendNotificationSync);
                editor.endLinkedGesture();
                if(std::abs(editor.upperBoundarySliders[static_cast<size_t>(domain)]->getValue()+10)>0.01
                   ||std::abs(processor.getBoundaryForDomainDb(compression==1,true,domain)+10)>0.01f)
                    throw std::runtime_error("Actual lower fader did not push upper fader");
                editor.beginBoundaryGesture(domain,false);
                boundaryControls[static_cast<size_t>(domain)]->setValue(-40,juce::sendNotificationSync);
                editor.endLinkedGesture();
                editor.beginBoundaryGesture(domain,true);
                editor.upperBoundarySliders[static_cast<size_t>(domain)]->setValue(-60,juce::sendNotificationSync);
                editor.endLinkedGesture();
                if(std::abs(boundaryControls[static_cast<size_t>(domain)]->getValue()+60)>0.01
                   ||std::abs(processor.getBoundaryForDomainDb(compression==1,false,domain)+60)>0.01f)
                    throw std::runtime_error("Actual upper fader did not push lower fader");
            }
        }
        parameter(processor,"processingMode",0); parameter(processor,"compressionMode",0); editor.timerCallback();
        if(std::abs(editor.ratioSlider.getMinimum()-1.0/1000.0)>1e-6 || editor.ratioSlider.getMaximum()!=1000)
            throw std::runtime_error("Single Ratio must span 1/1000 to 1000");
        if(editor.downRatioSliders[0]->isVisible()) throw std::runtime_error("Single shows two ratios");
        editor.compressionModeButton.onClick(); editor.timerCallback();
        if(std::abs(editor.ratioSlider.getMinimum()-1.0/1000.0)>1e-6 || editor.ratioSlider.getMaximum()!=1
           || editor.downRatioSliders[0]->getMinimum()!=1 || editor.downRatioSliders[0]->getMaximum()!=1000)
            throw std::runtime_error("Dual Ratio ranges must be UP 1/1000..1, DOWN 1..1000");
        if(!editor.downRatioSliders[0]->isVisible() || editor.compressionModeButton.getWidth()>64
           ||editor.ratioSlider.getWidth()>70 || editor.downRatioSliders[0]->getWidth()>70)
            throw std::runtime_error("Dual compact controls contract failed");
        if(editor.upRatioNames[0].getText()!="UP RATIO" || editor.downRatioNames[0].getText()!="DOWN RATIO")
            throw std::runtime_error("Dual ratio labels unclear");
        const auto fractional=editor.ratioSlider.valueFromTextFunction("1/16");
        editor.ratioSlider.setValue(fractional,juce::sendNotificationSync);
        if(std::abs(processor.getAPVTS().getRawParameterValue("upRatio")->load()-0.0625f)>0.0001f)
            throw std::runtime_error("Reciprocal numeric entry failed after mode reattachment");
        editor.compressionModeButton.onClick(); editor.timerCallback();
        if(editor.downRatioSliders[0]->isVisible()) throw std::runtime_error("Dual to Single leaves second ratio visible");
        std::cout<<"PASS: actual ten-pair fader collision gestures, compact toggle, two smaller labelled ratios, reciprocal entry.\n";
        parameter (processor, "inputGainDb", 0.0f);
        parameter (processor, "outputGainDb", 0.0f);
        parameter (processor, "makeupGainDb", 0.0f);
        parameter (processor, "mix", 100.0f);
        const char* thresholds[] {"thresholdDb","thresholdLDb","thresholdRDb","thresholdMDb","thresholdSDb"};
        const char* ranges[] {"rangeDb","rangeLDb","rangeRDb","rangeMDb","rangeSDb"};
        const char* upThresholds[] {"upThresholdDb","upThresholdLDb","upThresholdRDb","upThresholdMDb","upThresholdSDb"};
        const char* downThresholds[] {"downThresholdDb","downThresholdLDb","downThresholdRDb","downThresholdMDb","downThresholdSDb"};
        const char* upRatios[] {"upRatio","upRatioL","upRatioR","upRatioM","upRatioS"};
        const char* downRatios[] {"downRatio","downRatioL","downRatioR","downRatioM","downRatioS"};
        const char* ratios[] {"ratio","ratioL","ratioR","ratioM","ratioS"};
        for (int i=0;i<5;++i)
        {
            parameter(processor, thresholds[i], -45.0f); parameter(processor, ranges[i], -8.0f);
            parameter(processor, upThresholds[i], -24.0f); parameter(processor, downThresholds[i], -12.0f);
            parameter(processor, upRatios[i], 0.125f); parameter(processor, downRatios[i], 8.0f);
            parameter(processor, ratios[i], 0.125f);
        }
        const auto fillSyntheticHistory = [&]
        {
            for (auto& history : editor.display.histories)
            {
                history.points.clear();
                for(int i=0;i<480;++i)
                {
                    DynamicDisplay::HistoryPoint point;
                    point.inputDb = -26.0f + 23.0f*std::sin(i*0.033f) + 2.0f*std::sin(i*0.14f);
                    point.detectorDb = point.inputDb;
                    history.points.push_back(point);
                }
            }
        };
        for (int compression=0;compression<2;++compression)
        {
            parameter(processor,"compressionMode",static_cast<float>(compression));
            for(int domainMode : {0,1,2})
            {
                parameter(processor,"processingMode",static_cast<float>(domainMode));
                processor.getMeterState().processingMode.store(domainMode);
                editor.timerCallback(); editor.display.timerCallback(); fillSyntheticHistory();
                editor.display.refreshRenderCaches(domainMode);
                bool hasBoost=false, hasCut=false;
                for(const auto value : editor.display.renderCaches[0].projected.effectiveGainReduction)
                { hasBoost=hasBoost || value < -0.01f; hasCut=hasCut || value > 0.01f; }
                if(!hasBoost || (compression==0 && hasCut) || (compression==1 && !hasCut))
                    throw std::runtime_error("Wrong sign branches in synthetic Display projection");
                if(editor.display.renderCaches[0].projected.size!=480
                   || (domainMode!=0 && editor.display.renderCaches[1].projected.size!=480))
                    throw std::runtime_error("Display history budget changed");
                for(auto theme : {qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
                {
                    editor.theme=theme; editor.applyTheme(); editor.setSize(1200,800);
                    const juce::String skin=theme==qqsc::ui::Theme::light ? "light" : theme==qqsc::ui::Theme::dark ? "dark" : "classic";
                    const juce::String name=skin+"-"+(compression==0 ? "single-up" : "dual")+"-"+qqsc::params::modeName(domainMode);
                    snapshot(editor,dir.getChildFile(name+".png"));
                    auto out=output(dir.getChildFile(name+"-bounds.txt")); bounds(editor.contentRoot,*out,"root");
                    editor.setSize(1008,672); snapshot(editor,dir.getChildFile(name+"-minimum.png"));
                }
            }
        }
        // Exercise the real cached Display with changing parameters and software
        // rasterization; both modes use the same history and pixel dimensions.
        editor.setSize(1200,800); editor.theme=qqsc::ui::Theme::dark; editor.applyTheme();
        parameter(processor,"processingMode",2); processor.getMeterState().processingMode.store(2); editor.timerCallback();
        juce::Image buffer(juce::Image::RGB,editor.display.getWidth(),editor.display.getHeight(),true,juce::SoftwareImageType());
        auto timings=output(dir.getChildFile("display-performance-1.2.0.txt"));
        for(int compression=0;compression<2;++compression)
        {
            parameter(processor,"compressionMode",static_cast<float>(compression));
            editor.timerCallback(); editor.display.timerCallback(); fillSyntheticHistory();
            std::vector<double> samples;
            for(int frame=0;frame<240;++frame)
            {
                const float db=-40.0f+10.0f*std::sin(frame*0.1f);
                parameter(processor, compression==0 ? "thresholdLDb" : "upThresholdLDb",db);
                const auto start=juce::Time::getMillisecondCounterHiRes();
                editor.display.refreshRenderCaches(2);
                {juce::Graphics g(buffer); editor.display.paint(g);}
                samples.push_back(juce::Time::getMillisecondCounterHiRes()-start);
            }
            std::sort(samples.begin(),samples.end());
            const auto line=juce::String(compression==0 ? "Single" : "Dual")+" LR cache+software-paint, median "+juce::String(samples[120],3)+" ms; p95 "+juce::String(samples[228],3)+" ms\n";
            timings->writeText(line,false,false,"\n"); std::cout<<line;
        }
        parameter(processor,"processingMode",0); parameter(processor,"compressionMode",1);
        processor.getMeterState().processingMode.store(0);
        parameter(processor,"upThresholdDb",-20); parameter(processor,"downThresholdDb",-20);
        editor.timerCallback(); editor.display.timerCallback(); fillSyntheticHistory(); editor.display.refreshRenderCaches(0);
        for(size_t i=0;i<editor.display.renderCaches[0].projected.size;++i)
            if(std::abs(editor.display.renderCaches[0].projected.effectiveGainReduction[i])>1e-5f)
                throw std::runtime_error("Collapsed dual Display still shows dynamic gain");
        snapshot(editor,dir.getChildFile("dark-dual-collapsed.png"));
        auto& meters=processor.getMeterState();
        meters.gainReductionDb0.store(-12); meters.gainReductionDb1.store(12);
        meters.gainReductionHoldDb0.store(-16); meters.gainReductionHoldDb1.store(16);
        snapshot(editor,dir.getChildFile("dark-signed-meter.png"));
        editor.theme=qqsc::ui::Theme::light; editor.applyTheme();
        for(bool dual : {true,false})
        {
            parameter(processor,"compressionMode",dual ? 1.0f : 0.0f); editor.timerCallback();
            processor.setBoundaryForDomainDb(dual,false,0,-30);
            processor.setBoundaryForDomainDb(dual,true,0,-6); editor.timerCallback();
            for(auto* slider : {editor.lowerBoundaryControls()[0],editor.upperBoundarySliders[0].get()})
            {
                const auto alt=mouseEvent(*slider,true); slider->mouseDown(alt); slider->mouseUp(alt);
            }
            editor.timerCallback(); editor.display.timerCallback(); fillSyntheticHistory(); editor.display.refreshRenderCaches(0);
            if(processor.getBoundaryForDomainDb(dual,false,0)!=(processor.isClassicAlgorithm()?qqsc::classicThresholdMinimumDb:qqsc::params::thresholdOffDb)
               || processor.getBoundaryForDomainDb(dual,true,0)!=(dual ? 0.0f : qqsc::params::rangeOffDb))
                throw std::runtime_error("Alt reset does not restore algorithm minimum/0 or Single RangeOFF");
            snapshot(editor,dir.getChildFile(dual ? "light-dual-default-boundaries.png" : "light-single-range-off.png"));
        }
        std::cout<<"PASS: actual Alt resets restore algorithm minimum/0 and Single RangeOFF.\n";
        std::cout<<"PASS: single/dual three themes, ST/MS/LR and minimum size; collapsed projection and signed meter snapshots.\n";
    }

    static void darkCheck (QQSuperCompressionAudioProcessorEditor& editor,
                            QQSuperCompressionAudioProcessor& processor, const juce::File& dir)
    {
        editor.theme = qqsc::ui::Theme::dark;
        editor.applyTheme();
        snapshot (editor, dir.getChildFile ("dark-cache-prime.png"));
        auto& renderer = *editor.utf8LookAndFeel.darkKnobs;
        juce::Slider control;
        juce::Image buffer (juce::Image::ARGB, 160, 160, true);
        juce::Graphics g (buffer);
        renderer.draw (g, { 0, 0, 100, 100 }, 0.5f, control);
        const auto count = renderer.getRenderCount();
        for (int i = 0; i < 200; ++i) renderer.draw (g, { 0, 0, 100, 100 }, 0.5f, control);
        if (renderer.getRenderCount() != count) throw std::runtime_error ("Dark cache missed unchanged knob");
        renderer.draw (g, { 0, 0, 100, 100 }, 0.6f, control);
        if (renderer.getRenderCount() != count + 1) throw std::runtime_error ("Dark cache failed to refresh");

        qqsc::dark_refined::Material material;
        const auto zero = material.render (0.0f, 400);
        for (float value : { 0.1f, 0.5f, 1.0f })
        {
            const auto frame = material.render (value, 400);
            if (frame.getPixelAt (200, 180) != zero.getPixelAt (200, 180)
                || frame.getPixelAt (200, 357) != zero.getPixelAt (200, 357))
                throw std::runtime_error ("Dark fixed body/shadow changed");
            for (float fraction : { 0.04f, 0.20f, 0.40f, 0.65f, 0.90f })
            {
                const auto angle = qqsc::dark_refined::Material::angleForValue (fraction);
                const auto x = juce::roundToInt ((50.0f + 33.7f * std::sin (angle)) * 4.0f);
                const auto y = juce::roundToInt ((46.0f - 33.7f * std::cos (angle)) * 4.0f);
                const auto a = zero.getPixelAt (x, y);
                const auto b = frame.getPixelAt (x, y);
                const int blueIncrease = b.getBlue() - a.getBlue();
                if (fraction < value && (blueIncrease < 35 || b.getBlue() - b.getRed() < 12))
                    throw std::runtime_error ("Dark active arc is not blue and lit");
                if (fraction > value && std::abs (blueIncrease) > 3)
                    throw std::runtime_error ("Dark inactive arc emits light");
            }
        }
        std::vector<double> timings;
        material.render (0.0f, 160);
        for (int i = 0; i < 40; ++i)
        {
            const auto begin = juce::Time::getMillisecondCounterHiRes();
            material.render (i / 39.0f, 160);
            timings.push_back (juce::Time::getMillisecondCounterHiRes() - begin);
        }
        std::sort (timings.begin(), timings.end());
        std::cout << "PASS: Dark zero/progressive blue light, fixed metal/shadow, 200 cached repaints; 160px median "
                  << timings[20] << " ms, p95 " << timings[38] << " ms.\n";
        juce::Image details (juce::Image::RGB, 1020, 400, true);
        {
            juce::Graphics graphics (details);
            graphics.fillAll (qqsc::ui::canvas());
            const float positions[] { 0.0f, 0.1f, 0.5f, 0.75f, 1.0f };
            for (int i = 0; i < 5; ++i)
            {
                editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 2, 4, 200, 200,
                    positions[i], qqsc::dark::Material::start, qqsc::dark::Material::end, control);
                graphics.setColour (qqsc::ui::text());
                graphics.setFont (16.0f);
                graphics.drawText (juce::String (juce::roundToInt (positions[i] * 100.0f)) + "%",
                                   i * 204, 204, 204, 24, juce::Justification::centred);
                editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 62, 252, 80, 90,
                    positions[i], qqsc::dark::Material::start, qqsc::dark::Material::end, control);
            }
        }
        auto detailStream = output (dir.getChildFile ("dark-control-detail.png"));
        if (! juce::PNGImageFormat().writeImageToStream (details, *detailStream))
            throw std::runtime_error ("Dark detail PNG failed");

        // Production bottom finish is cached and independent of parameter motion.
        juce::Image panelImage (juce::Image::RGB, 988, 162, true);
        const auto panelCount = editor.darkBottomPanel.getGenerationCount();
        for (int i = 0; i < 200; ++i)
        {
            juce::Graphics panelGraphics (panelImage);
            panelGraphics.fillAll (qqsc::ui::canvas());
            editor.darkBottomPanel.draw (panelGraphics, {0, 0, 988, 162}, 15.0f);
        }
        if (editor.darkBottomPanel.getGenerationCount() != panelCount || panelCount != 1)
            throw std::runtime_error ("Bottom panel finish was regenerated on repaint");
        qqsc::dark::BottomPanelMaterial independentPanel;
        juce::Image secondPanel (juce::Image::RGB, 988, 162, true);
        {
            juce::Graphics panelGraphics (secondPanel);
            panelGraphics.fillAll (qqsc::ui::canvas());
            independentPanel.draw (panelGraphics, {0, 0, 988, 162}, 15.0f);
        }
        for (int y = 0; y < 162; ++y)
            for (int x = 0; x < 988; ++x)
                if (panelImage.getPixelAt (x, y) != secondPanel.getPixelAt (x, y))
                    throw std::runtime_error ("Bottom grain is not deterministic");
        auto panelStream = output (dir.getChildFile ("dark-panel-material.png"));
        if (! juce::PNGImageFormat().writeImageToStream (panelImage, *panelStream))
            throw std::runtime_error ("Panel finish PNG failed");
        for (float pixelScale : {1.0f, 2.0f})
        {
            const auto crop = editor.createComponentSnapshot ({16, 620, 1168, 162}, true, pixelScale);
            auto stream = output (dir.getChildFile (pixelScale == 1.0f ? "dark-bottom.png" : "dark-bottom-2x.png"));
            if (! juce::PNGImageFormat().writeImageToStream (crop, *stream))
                throw std::runtime_error ("Actual bottom panel PNG failed");
        }
        std::cout << "PASS: stationary deterministic bottom grain, one generation across 200 paints.\n";

        auto& m = processor.getMeterState();
        m.inputDb0.store (-18.1f); m.inputDb1.store (-18.3f);
        m.outputDb0.store (-17.2f); m.outputDb1.store (-17.4f);
        m.gainReductionDb0.store (1.6f); m.gainReductionDb1.store (1.6f);
        const auto meters = editor.meters.createComponentSnapshot (editor.meters.getLocalBounds());
        const auto meanLuma = [&] (int x, int y)
        {
            float total = 0.0f;
            for (int dy = -6; dy <= 6; ++dy)
            {
                const auto c = meters.getPixelAt (x, y + dy);
                total += c.getFloatRed() * 0.2126f + c.getFloatGreen() * 0.7152f + c.getFloatBlue() * 0.0722f;
            }
            return total / 13.0f;
        };
        const int firstBarX = juce::roundToInt (6.0f + ((meters.getWidth() - 20.0f) / 3.0f - 2.5f) * 0.25f);
        if (meanLuma (firstBarX, meters.getHeight() - 100) - meanLuma (firstBarX, 100) < 0.18f)
            throw std::runtime_error ("Input active/inactive contrast too low");
        snapshot (editor, dir.getChildFile ("dark-meter-reference.png"));
        const auto audioState = processor.getAPVTS().copyState();
        juce::PropertiesFile::Options options;
        options.millisecondsBeforeSaving = -1;
        const auto prefFile = dir.getChildFile ("test-ui-preference.xml");
        editor.uiProperties = std::make_unique<juce::PropertiesFile> (prefFile, options);
        editor.uiProperties->setValue ("classicTheme", true);
        editor.uiProperties->removeValue ("uiTheme");
        if (qqsc::ui::themeFromPreferences (*editor.uiProperties) != qqsc::ui::Theme::classic)
            throw std::runtime_error ("Legacy Classic preference did not migrate to Classic");
        editor.uiProperties->setValue ("classicTheme", false);
        if (qqsc::ui::themeFromPreferences (*editor.uiProperties) != qqsc::ui::Theme::light)
            throw std::runtime_error ("Legacy Light preference migration failed");
        for (auto expected : {qqsc::ui::Theme::classic, qqsc::ui::Theme::light, qqsc::ui::Theme::dark})
        {
            editor.toggleTheme();
            juce::PropertiesFile reopened (prefFile, options);
            if (editor.theme != expected || qqsc::ui::themeFromPreferences (reopened) != expected
                || editor.themeButton.getButtonText() != qqsc::ui::themeLabel (expected))
                throw std::runtime_error ("Three-theme cycle or disk preference restore failed");
        }
        editor.uiProperties.reset();
        if (! audioState.isEquivalentTo (processor.getAPVTS().copyState()))
            throw std::runtime_error ("Theme changed audio parameter state");
        for (int i = 0; i < editor.ratioSlider.getNumChildComponents(); ++i)
            if (auto* label = dynamic_cast<juce::Label*> (editor.ratioSlider.getChildComponent (i)))
            {
                label->showEditor();
                snapshot (editor, dir.getChildFile ("dark-numeric-edit.png"));
                label->hideEditor (true);
                break;
            }
        editor.setSize (1008, 672);
        snapshot (editor, dir.getChildFile ("dark-minimum.png"));
        editor.setSize (1800, 1200);
        snapshot (editor, dir.getChildFile ("dark-maximum.png"));
        editor.setSize (1200, 800);
        std::cout << "PASS: Dark Input active/inactive contrast, theme toggle/unchanged APVTS, numeric entry and sizes.\n";
    }

    static int run (const juce::File& dir, bool displayUnityOnly = false, bool zeroDetectorOnly = false)
    {
        dir.createDirectory();
        QQSuperCompressionAudioProcessor processor;
        processor.setRateAndBufferSizeDetails (48000.0, 800);
        processor.prepareToPlay (48000.0, 800);
        juce::PropertiesFile::Options isolatedOptions;
        isolatedOptions.storageFormat = juce::PropertiesFile::storeAsXML;
        auto isolatedSettings = std::make_unique<juce::PropertiesFile>
            (dir.getNonexistentChildFile ("ui-isolated-preferences", ".settings"), isolatedOptions);
        QQSuperCompressionAudioProcessorEditor editor (processor, std::move (isolatedSettings));
        // Never change the user's remembered size or theme while rendering tests.
        editor.uiProperties.reset();
        editor.stopTimer();
        editor.display.stopTimer();
        editor.setSize (1200, 800);
        if (zeroDetectorOnly)
        {
            zeroLookaheadDetectorCheck (editor, processor);
            windowReplayCheck (editor, processor);
            processor.releaseResources();
            return 0;
        }
        if (displayUnityOnly)
        {
            unityDisplayCheck (editor, processor);
            for (auto skinTheme : {qqsc::ui::Theme::light, qqsc::ui::Theme::dark, qqsc::ui::Theme::classic})
            {
                editor.theme = skinTheme;
                editor.applyTheme();
                const juce::String skin = skinTheme == qqsc::ui::Theme::dark ? "dark"
                    : (skinTheme == qqsc::ui::Theme::classic ? "classic" : "light");
                for (int size : {1008, 1200})
                {
                    editor.setSize (size, size * 2 / 3);
                    for (int detector : {0, 1})
                    {
                        parameter (processor, qqsc::params::detectorMode, float (detector));
                        editor.timerCallback();
                        snapshot (editor, dir.getChildFile ("detector-" + skin + "-" + juce::String (size)
                            + (detector == 0 ? "-Min.png" : "-Max.png")));
                    }
                }
            }
            processor.releaseResources();
            return 0;
        }
        parameter (processor, "ratio", 5.0f);
        parameter (processor, "thresholdDb", -28.0f);
        parameter (processor, "makeupGainDb", 3.4f);
        parameter (processor, "mix", 76.0f);
        parameter (processor, "lookaheadMs", 26.0f);
        juce::AudioBuffer<float> audio (2, 800);
        juce::MidiBuffer midi;
        const auto feedHistory = [&]
        {
        for (int frame = 0; frame < 500; ++frame)
        {
            for (int i = 0; i < 800; ++i)
            {
                const double t = static_cast<double> (frame * 800 + i) / 48000.0;
                const double pulse = std::pow (0.5 + 0.5 * std::sin (t * 6.4), 2.5);
                const double phrase = 0.4 + 0.6 * std::pow (0.5 + 0.5 * std::sin (t * 2.3 + 0.7), 1.7);
                const float envelope = static_cast<float> ((0.035 + 0.21 * pulse) * phrase);
                audio.setSample (0, i, envelope * static_cast<float> (std::sin (t * 1137.0) + 0.3 * std::sin (t * 2763.0)));
                audio.setSample (1, i, envelope * static_cast<float> (std::sin (t * 1137.0 + 0.25) + 0.26 * std::sin (t * 2701.0)));
            }
            processor.processBlock (audio, midi);
            editor.display.timerCallback();
        }
        };
        feedHistory();
        editor.timerCallback();
        // Validate production cache and material, not the separate prototype.
        editor.theme = qqsc::ui::Theme::light;
        editor.applyTheme();
        snapshot (editor, dir.getChildFile ("warm-cache-prime.png"));
        {
            auto& renderer = *editor.utf8LookAndFeel.warmKnobs;
            juce::Image buffer (juce::Image::ARGB, 100, 100, true);
            juce::Graphics g (buffer);
            juce::Slider control;
            renderer.draw (g, { 0, 0, 90, 90 }, 0.5f, control);
            const auto before = renderer.getRenderCount();
            for (int i = 0; i < 200; ++i) renderer.draw (g, { 0, 0, 90, 90 }, 0.5f, control);
            if (renderer.getRenderCount() != before) throw std::runtime_error ("Unchanged knobs recomposed");
            renderer.draw (g, { 0, 0, 90, 90 }, 0.6f, control);
            if (renderer.getRenderCount() != before + 1) throw std::runtime_error ("Changed knob not refreshed");
            std::cout << "PASS: 200 unchanged production knob paints use cached frames; parameter changes refresh once.\n";
        }
        {
            qqsc::warm_asset::Material material;
            const auto zero = material.render (0.0f, 400);
            const auto half = material.render (0.5f, 400);
            const auto full = material.render (1.0f, 400);
            if (zero.getPixelAt (200, 150) != full.getPixelAt (200, 150)
                || zero.getPixelAt (200, 346) != full.getPixelAt (200, 346))
                throw std::runtime_error ("Fixed material/shadow changed");
            for (int i = 0; i <= 1000; ++i)
                if (qqsc::warm_asset::Material::emission (0.0f, i / 1000.0f, 0.03f) != 0.0f)
                    throw std::runtime_error ("Zero emits light");
            if (qqsc::ui::grAccent() != qqsc::ui::cyanAccent())
                throw std::runtime_error ("Light GR palette mismatch");
            std::vector<double> timings;
            material.render (0.0f, 160);
            for (int i = 0; i < 40; ++i)
            {
                const auto begin = juce::Time::getMillisecondCounterHiRes();
                material.render (i / 39.0f, 160);
                timings.push_back (juce::Time::getMillisecondCounterHiRes() - begin);
            }
            std::sort (timings.begin(), timings.end());
            std::cout << "PASS: production zero-light/fixed material checks; 160px composition median "
                      << timings[20] << " ms, p95 " << timings[38] << " ms.\n";
        }
        for (auto skinTheme : {qqsc::ui::Theme::light, qqsc::ui::Theme::dark, qqsc::ui::Theme::classic})
        {
            editor.theme = skinTheme;
            editor.applyTheme();
            const bool dark = skinTheme == qqsc::ui::Theme::dark;
            const juce::String skin = dark ? "dark" : (skinTheme == qqsc::ui::Theme::classic ? "classic" : "warm");
            if (dark && (qqsc::ui::grAccent() != qqsc::ui::cyanAccent()
                            || editor.themeButton.getButtonText() != "DARK"))
                throw std::runtime_error ("Dark palette or theme label mismatch");
            snapshot (editor, dir.getChildFile (skin + "-ST.png"));
            snapshot (editor, dir.getChildFile (skin + "-ST-2x.png"), 2.0f);
            auto layout = output (dir.getChildFile (skin + "-ST-bounds.txt"));
            bounds (editor.contentRoot, *layout, "root");
            editor.toggleSidechainPanel();
            snapshot (editor, dir.getChildFile (skin + "-sidechain.png"));
            editor.toggleSidechainPanel();
        }
        editor.theme = qqsc::ui::Theme::light;
        editor.applyTheme();
        for (int mode : {qqsc::params::midSide, qqsc::params::leftRight})
        {
            parameter (processor, "processingMode", static_cast<float> (mode));
            editor.timerCallback();
            feedHistory();
            const juce::String modeName = mode == qqsc::params::midSide ? "MS" : "LR";
            snapshot (editor, dir.getChildFile ("warm-" + modeName + ".png"));
            auto layout = output (dir.getChildFile ("warm-" + modeName + "-bounds.txt"));
            bounds (editor.contentRoot, *layout, "root");
            for (auto skinTheme : {qqsc::ui::Theme::dark, qqsc::ui::Theme::classic})
            {
                editor.theme = skinTheme;
                editor.applyTheme();
                const juce::String name = skinTheme == qqsc::ui::Theme::dark ? "dark" : "classic";
                snapshot (editor, dir.getChildFile (name + "-" + modeName + ".png"));
                auto skinBounds = output (dir.getChildFile (name + "-" + modeName + "-bounds.txt"));
                bounds (editor.contentRoot, *skinBounds, "root");
            }
            editor.theme = qqsc::ui::Theme::light;
            editor.applyTheme();
        }
        parameter (processor, "processingMode", static_cast<float> (qqsc::params::stereoLinked));
        editor.timerCallback();
        feedHistory();
        editor.setSize (1008, 672);
        snapshot (editor, dir.getChildFile ("warm-minimum.png"));
        editor.setSize (1800, 1200);
        snapshot (editor, dir.getChildFile ("warm-maximum.png"));
        editor.setSize (1200, 800);
        for (int i = 0; i < editor.ratioSlider.getNumChildComponents(); ++i)
            if (auto* label = dynamic_cast<juce::Label*> (editor.ratioSlider.getChildComponent (i)))
            {
                label->showEditor();
                snapshot (editor, dir.getChildFile ("warm-numeric-edit.png"));
                label->hideEditor (true);
                break;
            }
        // Render the same production LookAndFeel at compact and large knob sizes.
        juce::Image details (juce::Image::RGB, 1020, 400, true);
        {
        juce::Graphics graphics (details);
        graphics.fillAll (qqsc::ui::canvas());
        juce::Slider dial;
        dial.setLookAndFeel (&editor.utf8LookAndFeel);
        dial.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
        for (int i = 0; i < 5; ++i)
        {
            const float positions[] { 0.0f, 0.1f, 0.5f, 0.75f, 1.0f };
            const float position = positions[i];
            editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 2, 4, 200, 200,
                position, juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, dial);
            graphics.setColour (qqsc::ui::text());
            graphics.setFont (16.0f);
            graphics.drawText (juce::String (juce::roundToInt (position * 100.0f)) + "%", i * 204, 204, 204, 24, juce::Justification::centred);
            editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 62, 252, 80, 90,
                position, juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, dial);
        }
        dial.setLookAndFeel (nullptr);
        } // Flush deferred native drawing before PNG encoding.
        auto detailStream = output (dir.getChildFile ("warm-control-detail.png"));
        if (! juce::PNGImageFormat().writeImageToStream (details, *detailStream)) return 3;
        meterGradientCheck (editor, processor, dir);
        darkCheck (editor, processor, dir);
        upDownChecks (editor, processor, dir);
        revisionThreeChecks (editor, processor, dir);
        revisionFourChecks (editor, processor, dir);
        up1000Checks (editor, processor, dir);
        inputOutputLinkChecks (editor, processor, dir);
        dbComparisonChecks (editor, processor, dir);
        algorithmChecks (editor, processor, dir);
        floorChecks (editor, processor, dir);
        processor.releaseResources();
        std::cout << "PASS: actual editor offscreen snapshots; user preferences not written.\n";
        return 0;
    }
};

#include "ratio_revision3_checks.h"
#include "branch_revision4_visual_checks.h"
#include "up1000_visual_checks.h"
#include "input_output_link_checks.h"
#include "db_visual_checks.h"
#include "algorithm_visual_checks.h"
#include "floor_visual_checks.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    if (argc != 2 && argc != 3) return 2;
    const bool displayUnityOnly = argc == 3 && juce::String (argv[2]) == "--display-unity";
    const bool zeroDetectorOnly = argc == 3 && juce::String (argv[2]) == "--detector-zero-display";
    if (argc == 3 && ! displayUnityOnly && ! zeroDetectorOnly) return 2;
    try { return QQSCVisualCheck::run (juce::File (juce::String::fromUTF8 (argv[1])), displayUnityOnly, zeroDetectorOnly); }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
