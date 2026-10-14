    static void unconditionalLimiterLinkChecks(const juce::File& root)
    {
        using Knob=QQSuperCompressionAudioProcessorEditor::FineKnob;
        auto drag=[&](Knob& slider,double value)
        {
            const auto ev=mouse(slider,slider.getLocalBounds().toFloat().getCentre());
            slider.mouseDown(ev);slider.setValue(value,juce::sendNotificationSync);slider.mouseUp(ev);
        };
        auto type=[&](Knob& slider,double value)
        {
            slider.showTextBox();juce::TextEditor* entry=nullptr;
            for(auto* child:slider.getChildren())
                if(auto* label=dynamic_cast<juce::Label*>(child))
                    if(label->getCurrentTextEditor())entry=label->getCurrentTextEditor();
            check(entry!=nullptr,"LINK numeric editor missing");
            entry->setText(juce::String(value,2));slider.hideTextBox(false);
        };
        auto near=[](double actual,double expected,const char* message)
        {
            if(std::abs(actual-expected)>.021)
                std::cerr<<message<<": actual="<<actual<<", expected="<<expected<<"\n";
            check(std::abs(actual-expected)<=.021,message);
        };
        int cases=0,edits=0;
        for(bool clipper:{false,true})for(bool dual:{false,true})
        for(int algorithm:{0,1})for(bool link:{true,false})for(bool enabled:{false,true})
        for(float ratio:{1.f,8.f,1000.f})
        {
            if(clipper && (dual || !enabled || ratio==8.f))continue;
            QQSuperCompressionAudioProcessor p;baseline(p);
            if(clipper)p.enterClipperMode();else p.enterLimiterMode();
            p.setCompressionModeFromEditor(dual?1:0);
            auto sound=[&](const char* id,float value){set(p,p.soundParameterID(id),value);};
            sound("algorithmMode",float(algorithm));sound("downAlgorithmMode",float(algorithm));
            sound("upAlgorithmMode",float(algorithm));sound("limiterLink",link?1.f:0.f);
            sound("ratioL",clipper && ratio==1.f?10.f:ratio);sound("downRatioL",ratio);
            sound("downEnabledL",enabled?1.f:0.f);sound("upRatioL",1.f);
            sound("outputGainDb",6.f);sound("makeupGainLDb",0.f);
            sound("mixL",100.f);sound("inputGainDb",0.f);
            p.setBoundaryForDomainDb(dual,false,1,dual?-60.f:-20.f);
            if(dual)p.setBoundaryForDomainDb(true,true,1,-20.f);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            auto& threshold=dual?*e.upperBoundarySliders[1]:e.thresholdLSlider;
            auto thresholdDb=[&]{return p.getBoundaryForDomainDb(dual,dual,1);};
            auto outputDb=[&]{return p.readSoundParameter("outputGainDb");};
            auto makeupDb=[&]{return p.readSoundParameter("makeupGainLDb");};
            const auto normalOutput=get(p,"outputGainDb"),normalMakeup=get(p,"makeupGainLDb");
            const auto upBefore=dual?p.getBoundaryForDomainDb(true,false,1):0.f;
            std::cout<<"CHECK "<<(clipper?"Clipper":"Limiter")<<" dual="<<dual<<" algorithm="<<algorithm
                     <<" LINK="<<link<<" downOn="<<enabled<<" ratio="<<ratio<<"\n";
            auto verifyEdits=[&](bool numeric)
            {
                auto edit=[&](Knob& knob,double value)
                {
                    if(numeric && &knob==&threshold)
                    {
                        auto& label=dual?e.upperBoundaryValues[1]:e.lowerBoundaryValues[1];
                        label.showEditor();check(label.getCurrentTextEditor()!=nullptr,"Threshold numeric editor missing");
                        label.getCurrentTextEditor()->setText(juce::String(value,2));label.hideEditor(false);
                    }
                    else if(numeric)type(knob,value);else drag(knob,value);
                    sync(p,e);
                };
                auto out=outputDb(),thr=thresholdDb(),makeup=makeupDb();
                edit(threshold,thr-1.25);
                near(thresholdDb(),thr-1.25,"Threshold source delta");
                near(outputDb(),out+(link?1.25:0.),"Threshold must link at unity and with DOWN off");
                near(makeupDb(),makeup,"Threshold must not alter Makeup");
                out=outputDb();thr=thresholdDb();makeup=makeupDb();
                edit(e.makeupLSlider,makeup+2.5);
                near(makeupDb(),makeup+2.5,"Makeup source delta");
                near(outputDb(),out-(link?2.5:0.),"Makeup must link regardless of compression settings");
                near(thresholdDb(),thr,"Makeup must not alter Threshold");
                out=outputDb();thr=thresholdDb();makeup=makeupDb();
                edit(e.outputGainSlider,out+1.75);
                near(outputDb(),out+1.75,"Output source delta");
                near(thresholdDb(),thr-(link?1.75:0.),"Output must link at unity and with DOWN off");
                near(makeupDb(),makeup,"Output must not alter Makeup");
                if(dual)near(p.getBoundaryForDomainDb(true,false,1),upBefore,"UP must stay independent");
                near(get(p,"outputGainDb"),normalOutput,"Normal Output bank changed");
                near(get(p,"makeupGainLDb"),normalMakeup,"Normal Makeup bank changed");
                edits+=3;
            };
            verifyEdits(false);verifyEdits(true);
            // These settings neither move the linked controls nor disable the
            // next explicit edit. Exercise zero Mix/Window/Lookahead and 16x OS.
            const auto out=outputDb(),thr=thresholdDb(),makeup=makeupDb();
            sound("mixL",0);sound("inputGainDb",7);sound("limiterStereoLink",100);
            sound("detectorMode",1);sound("lookaheadMs",0);sound("oversampling",3);
            set(p,"ceilingDb",-6);sync(p,e);
            near(outputDb(),out,"Other settings moved Output");
            near(thresholdDb(),thr,"Other settings moved Threshold");
            near(makeupDb(),makeup,"Other settings moved Makeup");
            verifyEdits(false);
            // Returning through unity must not leave a stale anchor or create
            // a deferred compensation on the next edit.
            const auto out2=outputDb(),thr2=thresholdDb();
            sound(dual?"downRatioL":"ratioL",clipper?10.f:1.f);sync(p,e);
            near(outputDb(),out2,"Ratio change moved Output");near(thresholdDb(),thr2,"Ratio change moved Threshold");
            verifyEdits(true);
            if(link)
            {
                const auto out3=outputDb(),thr3=thresholdDb();
                const auto ev=mouse(e.makeupLSlider,e.makeupLSlider.getLocalBounds().toFloat().getCentre(),true);
                const auto makeup3=makeupDb();e.makeupLSlider.mouseDown(ev);e.makeupLSlider.mouseUp(ev);sync(p,e);
                near(makeupDb(),0,"Makeup Alt reset");near(outputDb(),out3+makeup3,"Makeup Alt reset link");
                near(thresholdDb(),thr3,"Makeup Alt reset changed Threshold");
                ++edits;
            }
            ++cases;
        }
        // The linked pair must undo/redo together without replaying the link.
        for(bool dual:{false,true})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
            p.setCompressionModeFromEditor(dual?1:0);
            set(p,"limiterLink",1);set(p,"limiterRatioL",1);set(p,"limiterDownRatioL",1);
            set(p,"limiterDownEnabledL",0);set(p,"limiterOutputDb",6);set(p,"limiterMakeupGainLDb",0);
            set(p,"limiterAlgorithmMode",0);set(p,"limiterUpAlgorithmMode",0);set(p,"limiterDownAlgorithmMode",0);
            p.setBoundaryForDomainDb(dual,false,1,dual?-60.f:-20.f);
            if(dual)p.setBoundaryForDomainDb(true,true,1,-20);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
            auto& threshold=dual?*e.upperBoundarySliders[1]:e.thresholdLSlider;
            auto values=[&]{return std::array<float,4>{get(p,"limiterOutputDb"),get(p,"limiterMakeupGainLDb"),
                p.getBoundaryForDomainDb(dual,dual,1),dual?p.getBoundaryForDomainDb(true,false,1):0.f};};
            for(auto* knob:{&threshold,&e.makeupLSlider,&e.outputGainSlider})
            {
                (void)p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
                const auto before=values();drag(*knob,knob->getValue()+1.25);sync(p,e);const auto after=values();
                p.getUndoManager().beginNewTransaction();check(p.getUndoManager().undo(),"Linked edit missing Undo");sync(p,e);
                for(size_t i=0;i<before.size();++i)near(values()[i],before[i],"Linked Undo split or reapplied");
                check(p.getUndoManager().redo(),"Linked edit missing Redo");sync(p,e);
                for(size_t i=0;i<after.size();++i)near(values()[i],after[i],"Linked Redo split or reapplied");
            }
            // Respect real threshold/output bounds by clamping the joint delta,
            // not by silently disabling the relationship at unity or DOWN OFF.
            const float floor=dual?-60.f:p.boundaryMinimumDb(false);
            set(p,"limiterOutputDb",0);p.setBoundaryForDomainDb(dual,dual,1,-20);sync(p,e);
            drag(e.outputGainSlider,119);sync(p,e);
            near(p.getBoundaryForDomainDb(dual,dual,1),floor,"Output reached threshold lower bound incorrectly");
            near(get(p,"limiterOutputDb"),-20-floor,"Lower bound broke inverse delta");
            drag(e.outputGainSlider,-119);sync(p,e);
            near(p.getBoundaryForDomainDb(dual,dual,1),0,"Output reached threshold upper bound incorrectly");
            near(get(p,"limiterOutputDb"),-20,"Upper bound broke inverse delta");
            set(p,"limiterOutputDb",119);p.setBoundaryForDomainDb(dual,dual,1,-20);sync(p,e);
            drag(threshold,-25);sync(p,e);
            near(get(p,"limiterOutputDb"),120,"Threshold exceeded Output maximum");
            near(p.getBoundaryForDomainDb(dual,dual,1),-21,"Output bound broke threshold delta");
        }
        std::cout<<"PASS: linked Undo/Redo and shared range clamping at Ratio 1:1 / DOWN OFF in Single/Dual.\n";
        std::cout<<"PASS: "<<cases<<" unconditional LINK cases, "<<edits
                 <<" explicit edits; unity/non-unity, Single/Dual, Classic/Super, DOWN on/off, LINK on/off, "
                   "drag/numeric/Alt, zero Mix/Lookahead/Window, OS and Clipper shared path; UP and Comp banks preserved.\n";
    }
