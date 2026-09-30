    static void shiftDragContinuityChecks(const juce::File& root)
    {
        using Knob = QQSuperCompressionAudioProcessorEditor::FineKnob;
        auto near=[](double a,double b,double eps=.015){return std::abs(a-b)<=eps;};
        auto event=[&](juce::Component& c, juce::Point<float> pos, juce::Point<float> down,
                       bool shift, bool dragged)
        {
            const auto now=juce::Time::getCurrentTime();
            const auto flags=juce::ModifierKeys::leftButtonModifier
                | (shift ? juce::ModifierKeys::shiftModifier : 0);
            return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), pos,
                                    juce::ModifierKeys(flags), 1.0f,0.0f,0.0f,0.0f,0.0f,
                                    &c,&c,now,down,now,1,dragged);
        };
        auto prepare=[&](QQSuperCompressionAudioProcessorEditor& e)
        { e.setSize(1200,800); e.resized(); };

        // Rotary linear-value control: modifier transitions at an unchanged mouse
        // coordinate must be value-continuous in both directions. Movement after
        // the transition must use the new sensitivity only.
        {
            QQSuperCompressionAudioProcessor p; baseline(p); set(p,"mix",50.0f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root)); sync(p,e); prepare(e);
            Knob& s=e.mixSlider;
            const auto down=s.getLocalBounds().toFloat().getCentre();
            s.mouseDown(event(s,down,down,false,false));
            auto p1=down.translated(0.0f,-24.0f);
            s.mouseDrag(event(s,p1,down,false,true));
            const auto v1=s.getValue();
            s.mouseDrag(event(s,p1,down,true,true));
            check(near(s.getValue(),v1,.001),"Shift press jumped rotary value mid-drag");
            auto p2=p1.translated(0.0f,-12.0f);
            s.mouseDrag(event(s,p2,down,true,true));
            const auto v2=s.getValue();
            const auto fineDelta=v2-v1;
            check(fineDelta>0.05,"Shift fine drag did not move rotary value");
            s.mouseDrag(event(s,p2,down,false,true));
            check(near(s.getValue(),v2,.001),"Shift release jumped rotary value mid-drag");
            auto p3=p2.translated(0.0f,-12.0f);
            s.mouseDrag(event(s,p3,down,false,true));
            const auto normalDelta=s.getValue()-v2;
            check(normalDelta>fineDelta*4.0,"Normal rotary drag was not substantially faster than Shift fine drag");
            s.mouseUp(event(s,p3,down,false,true));
        }

        // Starting fine and releasing Shift during the same physical drag must be
        // equally continuous.
        {
            QQSuperCompressionAudioProcessor p; baseline(p); set(p,"mix",50.0f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root)); sync(p,e); prepare(e);
            Knob& s=e.mixSlider;
            const auto down=s.getLocalBounds().toFloat().getCentre();
            s.mouseDown(event(s,down,down,true,false));
            auto p1=down.translated(0.0f,-12.0f);
            s.mouseDrag(event(s,p1,down,true,true));
            const auto v1=s.getValue();
            s.mouseDrag(event(s,p1,down,false,true));
            check(near(s.getValue(),v1,.001),"Fine-to-normal Shift release jumped rotary value");
            auto p2=p1.translated(12.0f,0.0f);
            s.mouseDrag(event(s,p2,down,false,true));
            check(s.getValue()>v1,"Horizontal rotary drag stopped working after Shift transition");
            s.mouseUp(event(s,p2,down,false,true));
        }

        // Ratio uses a non-linear NormalisableRange. It must retain the exact
        // current ratio when Shift changes, rather than being re-evaluated from
        // the original mouse-down anchor.
        {
            QQSuperCompressionAudioProcessor p; baseline(p); set(p,"ratio",4.0f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root)); sync(p,e); prepare(e);
            Knob& s=e.ratioSlider;
            const auto down=s.getLocalBounds().toFloat().getCentre();
            s.mouseDown(event(s,down,down,false,false));
            auto p1=down.translated(0.0f,-18.0f);
            s.mouseDrag(event(s,p1,down,false,true));
            const auto beforeShift=s.getValue();
            s.mouseDrag(event(s,p1,down,true,true));
            check(near(s.getValue(),beforeShift,1.0e-6),"Shift press jumped non-linear Ratio value");
            auto p2=p1.translated(0.0f,-8.0f);
            s.mouseDrag(event(s,p2,down,true,true));
            const auto beforeRelease=s.getValue();
            s.mouseDrag(event(s,p2,down,false,true));
            check(near(s.getValue(),beforeRelease,1.0e-6),"Shift release jumped non-linear Ratio value");
            s.mouseUp(event(s,p2,down,false,true));
        }

        // Limiter parameter rebinding uses the same FineKnob class. Verify the
        // continuous 1:8..1:1..1000:1 Single Ratio path in Limiter mode too.
        {
            QQSuperCompressionAudioProcessor p; baseline(p); p.enterLimiterMode();
            set(p,"limiterCompressionMode",0); set(p,"limiterRatio",4.0f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root)); sync(p,e); prepare(e);
            Knob& s=e.ratioSlider;
            const auto down=s.getLocalBounds().toFloat().getCentre();
            s.mouseDown(event(s,down,down,false,false));
            auto p1=down.translated(0.0f,-10.0f);
            s.mouseDrag(event(s,p1,down,false,true));
            const auto v=s.getValue();
            s.mouseDrag(event(s,p1,down,true,true));
            check(near(s.getValue(),v,1.0e-6),"Limiter Ratio jumped when Shift was pressed mid-drag");
            s.mouseUp(event(s,p1,down,true,true));
        }

        // Threshold/Range faders already use an incremental drag implementation;
        // keep that continuity contract protected while the rotary path changes.
        {
            QQSuperCompressionAudioProcessor p; baseline(p); set(p,"thresholdDb",-24.0f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root)); sync(p,e); prepare(e);
            Knob& s=e.thresholdSlider;
            const auto down=s.getLocalBounds().toFloat().getCentre();
            s.mouseDown(event(s,down,down,false,false));
            auto p1=down.translated(0.0f,-10.0f);
            s.mouseDrag(event(s,p1,down,false,true));
            const auto v=s.getValue();
            s.mouseDrag(event(s,p1,down,true,true));
            check(near(s.getValue(),v,.001),"Shift press jumped Threshold fader value");
            s.mouseUp(event(s,p1,down,true,true));
        }

        std::cout<<"PASS: 1.2.26 Shift modifier transitions are continuous for rotary, non-linear Ratio, Limiter-rebound Ratio and Threshold drag; fine sensitivity applies only to subsequent movement.\n";
    }
