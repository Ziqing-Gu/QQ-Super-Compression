    static void lookaheadPresetsChecks(const juce::File& root)
    {
        // Real saved-state fixtures, including both inactive A/B banks.
        for(float legacy:{0.f,5.f,10.f,26.f,40.f,80.f,100.f})for(int limiter:{0,1})
        {
            const float expected=(legacy==5 || legacy==10)?26.f:legacy;
            QQSuperCompressionAudioProcessor p;
            set(p,"lookaheadMs",26);set(p,"limiterLookaheadMs",26);
            set(p,"oversampling",0);set(p,"limiterOversampling",0);
            set(p,"limiterMode",float(limiter));set(p,"distort",25);p.copyAToB();
            juce::MemoryBlock data;p.getStateInformation(data);
            auto tree=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize())));
            for(const auto* id:{"lookaheadMs","limiterLookaheadMs"})
            {
                tree.getChildWithProperty("id",id).setProperty("value",legacy,nullptr);
                for(const auto* prefix:{"qqscAB_A_","qqscAB_B_"})
                    tree.setProperty(juce::String(prefix)+id,legacy,nullptr);
            }
            tree.setProperty("qqscLimiterBankInitialised",true,nullptr);
            juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);
            QQSuperCompressionAudioProcessor q;q.setStateInformation(data.getData(),int(data.getSize()));
            for(int slot:{0,1})
            {
                q.selectABSlot(slot);
                check(get(q,"lookaheadMs")==expected && get(q,"limiterLookaheadMs")==expected,"Legacy Lookahead or A/B bank migration");
                check(std::abs(q.readSoundParameter("distort")-25)<.001f,"Lookahead migration lost saved Distort");
                for(const auto* id:{"lookaheadMs","limiterLookaheadMs"})
                {
                    auto* param=q.getAPVTS().getParameter(id);
                    check(param->getText(param->convertTo0to1(legacy),32)==juce::String(expected,0)+" ms","Host Lookahead readout");
                }
            }
            // Native compressor PDC must follow the migrated milliseconds.
            set(q,"limiterMode",0);q.setEcoMode(false);
            q.setRateAndBufferSizeDetails(48000,256);q.prepareToPlay(48000,256);
            check(q.getLatencySamples()==int(expected*48),"Lookahead PDC migration");
            q.releaseResources();
        }
        QQSuperCompressionAudioProcessor p;set(p,"lookaheadMs",26);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);
        const juce::StringArray choices{"0 ms","26 ms","40 ms","80 ms","100 ms"};
        check(e.lookaheadCombo.getNumItems()==choices.size(),"Five Lookahead choices");
        for(int i=0;i<choices.size();++i)
        {
            check(e.lookaheadCombo.getItemText(i)==choices[i],"Lookahead choice order");
            set(p,"distort",75);e.lookaheadCombo.setSelectedItemIndex(i,juce::sendNotificationSync);
            check(p.readSoundParameter("lookaheadMs")==choices[i].getFloatValue(),"Lookahead menu commit");
            check(p.readSoundParameter("distort")==0,"Lookahead reset still required");
        }
        e.lookaheadCombo.setSelectedItemIndex(1,juce::sendNotificationSync);set(p,"detectorMode",1);sync(p,e);
        e.detectorWindowValue.setText("10 ms",juce::sendNotificationSync);
        check(std::abs(p.readSoundParameter("detectorWindowMs")-10)<.002f,"Safe Window 10 ms remains available");
        e.detectorWindowValue.reset();check(p.readSoundParameter("distort")==0,"Safe Alt reset remains full window");
        std::cout<<"PASS: five UI presets; 14 legacy state/dual-bank/A-B/PDC fixtures; 10 ms Window retained; full-window reset retained.\n";
    }
