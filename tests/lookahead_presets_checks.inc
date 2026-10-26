    static void lookaheadPresetsChecks(const juce::File& root)
    {
        // Explicit old -> new values. Include both inactive mode banks and A/B.
        const std::array<std::pair<float,float>,11> fixtures {{{0,0},{5,25.3f},{10,25.3f},
            {26,25.3f},{40,50.6f},{80,76},{100,100},{25.3f,25.3f},{50.6f,50.6f},{76,76},{105,100}}};
        for(const auto& fixture:fixtures)for(int family:{0,1,2})
        {
            const auto legacy=fixture.first,expected=fixture.second;
            const auto expectedText=juce::String(expected,std::abs(expected-std::round(expected))<.001f?0:1)+" ms";
            QQSuperCompressionAudioProcessor p(settings(root));
            for(const auto* id:{"lookaheadMs","limiterLookaheadMs","clipperLookaheadMs"})set(p,id,26);
            set(p,"oversampling",0);set(p,"limiterOversampling",0);set(p,"clipperOversampling",0);
            p.setProcessorModeFromEditor(family);set(p,"distort",25);p.copyAToB();
            juce::MemoryBlock data;p.getStateInformation(data);
            auto tree=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize())));
            for(const auto* id:{"lookaheadMs","limiterLookaheadMs","clipperLookaheadMs"})
            {
                tree.getChildWithProperty("id",id).setProperty("value",legacy,nullptr);
                for(const auto* prefix:{"qqscAB_A_","qqscAB_B_"})
                    tree.setProperty(juce::String(prefix)+id,legacy,nullptr);
            }
            tree.setProperty("qqscLimiterBankInitialised",true,nullptr);
            juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);
            QQSuperCompressionAudioProcessor q(settings(root));q.setStateInformation(data.getData(),int(data.getSize()));
            for(int slot:{0,1})
            {
                q.selectABSlot(slot);
                check(std::abs(q.readSoundParameter("distort")-25)<.001f,"Lookahead migration lost saved Distort");
                for(const auto* id:{"lookaheadMs","limiterLookaheadMs","clipperLookaheadMs"})
                {
                    check(std::abs(get(q,id)-expected)<.001f,"Lookahead or inactive mode/A-B bank migration");
                    auto* param=q.getAPVTS().getParameter(id);
                    check(param->getText(param->convertTo0to1(legacy),32)==expectedText,"Host Lookahead readout");
                    // The old float parameters were 0..100. Existing automation
                    // at the old preset positions must select the same migration.
                    if(legacy<=100)
                        check(param->getText(legacy/100.f,32)==expectedText,"Legacy normalized Lookahead automation");
                }
            }
            q.setProcessorModeFromEditor(0);q.setEcoMode(false);
            q.setRateAndBufferSizeDetails(48000,256);q.prepareToPlay(48000,256);
            check(q.getLatencySamples()==int(std::round(double(expected)*48.0)),"Lookahead PDC migration");
            q.releaseResources();
        }
        QQSuperCompressionAudioProcessor p(settings(root));
        for(const auto* id:{"lookaheadMs","limiterLookaheadMs","clipperLookaheadMs"})set(p,id,26);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
        const juce::StringArray choices{"0 ms","25.3 ms","50.6 ms","76 ms","100 ms"};
        check(e.lookaheadCombo.getNumItems()==choices.size(),"Five Lookahead choices");
        // Full visible readout, including ms: item text alone cannot detect ellipsis.
        int readoutCases=0;
        for(auto theme:{qqsc::ui::Theme::dark,qqsc::ui::Theme::light,qqsc::ui::Theme::classic})
        for(int family:{0,1,2})for(int mode:{0,1,2})for(int dual:{0,1})for(int width:{1008,1200})
        {
            p.setProcessorModeFromEditor(family);set(p,p.soundParameterID("processingMode"),float(mode));
            p.setCompressionModeFromEditor(dual);e.theme=theme;e.applyTheme();e.setSize(width,width*2/3);e.updateModeUi();e.resized();
            for(int item=0;item<choices.size();++item)
            {
                e.lookaheadCombo.setSelectedItemIndex(item,juce::sendNotificationSync);
                juce::Label* label=nullptr;
                for(auto* child:e.lookaheadCombo.getChildren())if(auto* candidate=dynamic_cast<juce::Label*>(child))label=candidate;
                check(label!=nullptr,"Lookahead text label missing");
                const auto font=label->getLookAndFeel().getLabelFont(*label);
                const auto area=label->getBorderSize().subtractedFrom(label->getLocalBounds());
                const auto required=juce::GlyphArrangement::getStringWidth(font,choices[item]);
                if(required>float(area.getWidth()))std::cout<<"LOOKAHEAD READOUT: "<<choices[item]<<" needs "<<required<<" px, available "<<area.getWidth()<<" px"<<std::endl;
                check(label->getText()==choices[item],"Selected Lookahead label differs");
                check(font.getHeight()>=11.f && area.getHeight()>=font.getHeight(),"Lookahead font reduced or clipped vertically");
                check(required<=float(area.getWidth()),"Lookahead value/unit clipped by ellipsis");
                check(e.lookaheadCombo.getRight()<e.detectorWindowValue.getX(),"Lookahead overlaps Window");
                check(e.lookaheadCombo.getWidth()==e.detectorWindowValue.getWidth(),"Lookahead/Window widths differ");
                ++readoutCases;
            }
            if(family==0 && mode==2 && dual==1 && width==1008)
            {
                e.lookaheadCombo.setSelectedItemIndex(1,juce::sendNotificationSync);
                auto crop=e.getLocalArea(&e.contentRoot,juce::Rectangle<int>(1010,616,174,175));
                auto shot=e.createComponentSnapshot(crop);juce::PNGImageFormat png;
                juce::FileOutputStream out(root.getChildFile("lookahead-unit-"+juce::String(int(theme))+".png"));png.writeImageToStream(shot,out);
            }
        }
        std::cout<<"PASS: "<<readoutCases<<" full Lookahead value+unit readouts at unchanged 11px font, all themes/families/ST-LR-MS/Single-Dual and two editor sizes\n";

        for(int family:{0,1,2})
        {
            p.setProcessorModeFromEditor(family);sync(p,e);e.timerCallback();
            for(int i=0;i<choices.size();++i)
            {
                check(e.lookaheadCombo.getItemText(i)==choices[i],"Lookahead choice order");
                set(p,"distort",75);e.lookaheadCombo.setSelectedItemIndex(i,juce::sendNotificationSync);
                const float expected=choices[i].getFloatValue();
                check(std::abs(p.readSoundParameter("lookaheadMs")-expected)<.001f,"Lookahead menu commit");
                check(p.readSoundParameter("distort")==0,"Lookahead change must restore full Window");
                check(std::abs(p.readSoundParameter("detectorWindowMs")-expected)<.001f,"Window maximum must be 100 ms");
                e.setDetectorWindowValue(expected*.5f);e.detectorWindowValue.reset();
                check(std::abs(p.readSoundParameter("detectorWindowMs")-expected)<.001f,"Alt reset must restore full Window");
            }
            // The former experimental 105 ms range must now clamp at 100 ms.
            for(float window:{99.9f,100.f,100.1f,101.f,105.f})
            {
                e.setDetectorWindowValue(window);
                check(std::abs(p.readSoundParameter("detectorWindowMs")-std::min(window,100.f))<.006f,"Window maximum is not 100 ms");
            }
            // Selecting the same Lookahead is not a change and must retain Window.
            set(p,"distort",50);e.commitLookaheadChoice();
            check(std::abs(p.readSoundParameter("distort")-50)<.001f,"Same Lookahead reset Window");
            e.lookaheadCombo.setSelectedItemIndex(1,juce::sendNotificationSync);set(p,"detectorMode",1);sync(p,e);
            e.detectorWindowValue.setText("10 ms",juce::sendNotificationSync);
            check(std::abs(p.readSoundParameter("detectorWindowMs")-10)<.002f,"Safe Window 10 ms remains available");
            e.detectorWindowValue.reset();check(p.readSoundParameter("distort")==0,"Safe Alt reset remains full window");
        }
        // Exercise the expanded buffers at every OS and different host rates.
        // Subtract each mode's 0 ms latency to leave only the Lookahead portion.
        for(int family:{0,1,2})for(int os:{0,1,2,3})for(double rate:{44100.,48000.,96000.})
        {
            QQSuperCompressionAudioProcessor q(settings(root));q.setProcessorModeFromEditor(family);q.setEcoMode(false);
            set(q,q.soundParameterID("oversampling"),float(os));q.setLookaheadFromEditor(0);
            q.setRateAndBufferSizeDetails(rate,256);q.prepareToPlay(rate,256);
            const int baseLatency=q.getLatencySamples();const auto baseCoreDelay=q.getDisplayCoreDelaySeconds();
            juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,256);
            for(float look:{25.3f,50.6f,76.f,100.f,0.f})
            {
                q.setLookaheadFromEditor(look);
                // 2N plus one block covers the largest detector history.
                for(int n=0;n<int(rate*.21)+256;n+=256)
                {
                    audio.clear();q.processBlock(audio,midi);
                    for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)
                        check(std::isfinite(audio.getSample(ch,i)),"Non-finite output at new Lookahead");
                }
                const int samples=int(std::round(rate*double(look)*.001));
                check(q.getLatencySamples()-baseLatency==samples,"New Lookahead host latency");
                check(std::abs((q.getDisplayCoreDelaySeconds()-baseCoreDelay)*rate-samples)<.05,"Core delay was capped below new Lookahead");
                const int latency=q.getLatencySamples();set(q,"distort",100);audio.clear();q.processBlock(audio,midi);
                check(q.getLatencySamples()==latency,"Window changed host latency");
            }
            q.releaseResources();
        }
        // At native rate and unity gain, the pulse must arrive at the reported
        // sample, including the maximum 100 ms preset.
        for(float look:{0.f,25.3f,50.6f,76.f,100.f})for(double rate:{44100.,48000.,96000.})
        {
            QQSuperCompressionAudioProcessor q(settings(root));q.setProcessorModeFromEditor(0);q.setEcoMode(false);
            set(q,"inputOutputLink",0);set(q,"processingMode",0);set(q,"compressionMode",0);
            set(q,"oversampling",0);set(q,"inputGainDb",0);set(q,"outputGainDb",0);set(q,"makeupGainDb",0);
            set(q,"mix",100);set(q,"ratio",1);set(q,"rangeDb",qqsc::params::rangeOffDb);set(q,"detectorMode",0);q.setLookaheadFromEditor(look);
            q.setRateAndBufferSizeDetails(rate,256);q.prepareToPlay(rate,256);
            const int delay=int(std::round(rate*double(look)*.001)),pulseAt=256;
            juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,256);
            for(int offset=0;offset<pulseAt+delay+512;offset+=256)
            {
                audio.clear();if(offset==pulseAt)for(int ch=0;ch<2;++ch)audio.setSample(ch,0,.1f);
                q.processBlock(audio,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)
                    check(std::abs(audio.getSample(ch,i)-(offset+i==pulseAt+delay?.1f:0.f))<.00001f,"Actual audio delay differs from Lookahead/PDC");
            }
            q.releaseResources();
        }
        std::cout<<"PASS: 33 state/mode/A-B migrations; all-mode UI/Window reset; 36 rate/OS/mode latency fixtures; 15 actual pulse-delay fixtures\n";
    }
