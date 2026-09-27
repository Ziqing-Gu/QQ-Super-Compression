    static void modeMemoryChecks(const juce::File& root)
    {
        auto close=[](float a,float b){return std::abs(a-b)<.0002f*std::max(1.f,std::abs(b));};
        auto activeSet=[](auto& p,const char* id,float value){set(p,p.soundParameterID(id),value);};
        auto capture=[](auto& p)
        {
            std::vector<float> values;
            for(const auto* id:qqsc::params::normalSoundIds) values.push_back(p.readSoundParameter(id));
            for(const auto* id:qqsc::params::normalModeIds) values.push_back(p.readSoundParameter(id));
            values.push_back(p.readSoundParameter("outputGainDb"));
            return values;
        };
        auto same=[&](auto& p,const auto& expected,const char* context)
        {
            const auto actual=capture(p);
            for(size_t i=0;i<actual.size();++i)
                if(!close(actual[i],expected[i]))
                    throw std::runtime_error(std::string(context)+" index "+std::to_string(i)+" got "+std::to_string(actual[i])+" expected "+std::to_string(expected[i]));
        };
        auto fill=[&](auto& p,int seed)
        {
            for(size_t i=0;i<qqsc::params::normalModeIds.size();++i)
            {
                const auto* id=p.soundParameterID(qqsc::params::normalModeIds[i]);
                auto* param=p.getAPVTS().getParameter(id);
                set(p,id,param->convertFrom0to1(float((int(i)*17+seed*13)%81+9)/100.f));
            }
            activeSet(p,"lookaheadMs",seed%2 ? 26.f : 40.f);
            activeSet(p,"outputGainDb",float(seed*3));
            for(size_t d=0;d<5;++d)
            {
                activeSet(p,qqsc::params::ratioIds[d],p.isLimiterMode()?float(250+seed*50+d):float(seed+2+d));
                activeSet(p,qqsc::params::upRatioIds[d],1.f/float(2+seed+d));
                activeSet(p,qqsc::params::downRatioIds[d],p.isLimiterMode()?float(300+seed*50+d):float(seed+3+d));
                p.setBoundaryForDomainDb(false,true,int(d),-float(2+seed));
                p.setBoundaryForDomainDb(false,false,int(d),-float(30+seed+d));
                p.setBoundaryForDomainDb(true,false,int(d),-float(50+seed+d));
                p.setBoundaryForDomainDb(true,true,int(d),-float(8+seed+d));
            }
            (void)p.getAPVTS().copyState();
        };

        QQSuperCompressionAudioProcessor p;baseline(p);fill(p,1);
        const auto normalA=capture(p);
        p.enterLimiterMode();fill(p,2);set(p,"ceilingDb",-2);set(p,"truePeakLimiting",0);set(p,"limiterLink",0);
        const auto limiterA=capture(p);
        for(int i=0;i<20;++i)
        {
            p.leaveLimiterMode();same(p,normalA,"Normal A return");
            p.enterLimiterMode();same(p,limiterA,"Limiter A return");
        }
        p.copyAToB();p.selectABSlot(1);same(p,limiterA,"A to B active Limiter copy");
        p.leaveLimiterMode();same(p,normalA,"A to B dormant Normal copy");
        fill(p,3);const auto normalB=capture(p);
        p.enterLimiterMode();fill(p,4);set(p,"ceilingDb",-6);set(p,"truePeakLimiting",1);set(p,"limiterLink",1);
        const auto limiterB=capture(p);
        p.leaveLimiterMode(); // B remembers Normal, A remembers Limiter.
        for(int i=0;i<12;++i)
        {
            p.selectABSlot(0);check(p.isLimiterMode(),"A must restore its Limiter switch");same(p,limiterA,"A Limiter snapshot");
            check(close(get(p,"ceilingDb"),-2)&&get(p,"truePeakLimiting")==0,"A ceiling/TP lost");
            p.leaveLimiterMode();same(p,normalA,"A dormant normal snapshot");p.enterLimiterMode();
            p.selectABSlot(1);check(!p.isLimiterMode(),"B must restore its normal switch");same(p,normalB,"B normal snapshot");
            p.enterLimiterMode();same(p,limiterB,"B dormant limiter snapshot");
            check(close(get(p,"ceilingDb"),-6)&&get(p,"truePeakLimiting")==1,"B ceiling/TP lost");
            p.leaveLimiterMode();
        }
        juce::MemoryBlock saved;p.getStateInformation(saved);
        QQSuperCompressionAudioProcessor q;q.setStateInformation(saved.getData(),int(saved.getSize()));
        check(q.getActiveABSlot()==1&&!q.isLimiterMode(),"Project active A/B/mode lost");same(q,normalB,"Reload B normal");
        q.enterLimiterMode();same(q,limiterB,"Reload B limiter");
        q.selectABSlot(0);same(q,limiterA,"Reload A limiter");q.leaveLimiterMode();same(q,normalA,"Reload A normal");
        q.copyBToA();check(q.isLimiterMode(),"B to A copies the whole current B state");same(q,limiterB,"B to A limiter");
        q.leaveLimiterMode();same(q,normalB,"B to A dormant normal");
        // Headless host toggles must route to existing banks without reseeding.
        for(int i=0;i<10;++i)
        {set(q,"limiterMode",1);same(q,limiterB,"Host Limiter recall");set(q,"limiterMode",0);same(q,normalB,"Host Normal recall");}
        std::cout<<"PASS: all 69 active values in each of four banks, repeated mode toggles, outer A/B switch, both copy directions, saved reload and headless host mode automation.\n";

        // The actual editor must bind new Input/Makeup/Mix/choices, not merely
        // display remembered numbers while sending edits to the other bank.
        QQSuperCompressionAudioProcessor u;baseline(u);set(u,"inputOutputLink",0);
        QQSuperCompressionAudioProcessorEditor e(u,settings(root));sync(u,e);
        const auto uNormal=capture(u);e.limiterButton.onClick();sync(u,e);
        const auto drag=[&](auto& slider,double value)
        {const auto ev=mouse(slider,slider.getLocalBounds().toFloat().getCentre());slider.mouseDown(ev);slider.setValue(value,juce::sendNotificationSync);slider.mouseUp(ev);};
        e.limiterLinkButton.onClick(); // independent edits for routing assertions
        drag(e.inputGainSlider,7);drag(e.makeupSTSlider,9);drag(e.mixSlider,37);
        check(close(get(u,"limiterInputGainDb"),7)&&close(get(u,"limiterMakeupGainDb"),9)&&close(get(u,"limiterMix"),37),"UI edits hit wrong bank");
        e.algorithmButton.onClick();e.compressionModeButton.onClick();e.modeButton.onClick();sync(u,e);
        e.lookaheadCombo.setSelectedItemIndex(3,juce::dontSendNotification);e.commitLookaheadChoice();
        check(close(u.readSoundParameter("lookaheadMs"),40)&&close(get(u,"lookaheadMs"),26),"Lookahead edit leaked");
        const auto uLimiter=capture(u);e.limiterButton.onClick();sync(u,e);same(u,uNormal,"Editor Normal return");
        check(close(float(e.inputGainSlider.getValue()),0)&&close(float(e.mixSlider.getValue()),100),"Normal controls stale");
        e.limiterButton.onClick();sync(u,e);same(u,uLimiter,"Editor Limiter return");
        check(close(float(e.inputGainSlider.getValue()),7)&&close(float(e.mixSlider.getValue()),37),"Limiter controls stale");
        // After switching back, undo/redo still addresses the original bank.
        u.getUndoManager().clearUndoHistory();(void)u.getAPVTS().copyState();
        drag(e.mixSlider,58);sync(u,e);u.getUndoManager().beginNewTransaction();
        check(u.getUndoManager().undo(),"Mode Mix undo unavailable");sync(u,e);check(close(get(u,"limiterMix"),37),"Mode Mix undo wrong bank");
        check(u.getUndoManager().redo(),"Mode Mix redo unavailable");sync(u,e);check(close(get(u,"limiterMix"),58),"Mode Mix redo wrong bank");
        check(close(get(u,"mix"),100),"Undo/redo touched Normal Mix");
        std::cout<<"PASS: real editor Input/Makeup/Mix, algorithm, Single/Dual, channel mode and Lookahead route to correct bank; repeated controls and undo/redo.\n";

        QQSuperCompressionAudioProcessor first;baseline(first);
        QQSuperCompressionAudioProcessorEditor firstEditor(first,settings(root));sync(first,firstEditor);
        first.getUndoManager().clearUndoHistory();
        firstEditor.limiterButton.onClick();sync(first,firstEditor);
        const auto firstLimiter=capture(first);first.getUndoManager().beginNewTransaction();
        check(first.getUndoManager().undo(),"First mode entry undo");sync(first,firstEditor);
        check(!first.isLimiterMode(),"First mode entry undo left Limiter enabled");
        check(first.getUndoManager().redo(),"First mode entry redo");sync(first,firstEditor);same(first,firstLimiter,"First entry redo state");
        check(first.getUndoManager().undo(),"First mode entry second undo");sync(first,firstEditor);
        set(first,"inputGainDb",5);sync(first,firstEditor);
        firstEditor.limiterButton.onClick();sync(first,firstEditor);
        check(close(get(first,"limiterInputGainDb"),5),"Undone first entry left an initialised stale bank");
        std::cout<<"PASS: initial mode setup undo/redo and reseeding after an undone first entry.\n";

        // Migration from the prior saved format: each old A/B has its own
        // shared values; seed new fields from that slot, never the active one.
        auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize()));
        auto tree=juce::ValueTree::fromXml(*xml);tree.setProperty("qqscStateSchemaVersion",20,nullptr);
        tree.removeProperty("qqscLimiterBankInitialised",nullptr);
        for(const auto* id:qqsc::params::limiterModeIds)
        {
            tree.removeChild(tree.getChildWithProperty("id",id),nullptr);
            tree.removeProperty(juce::String("qqscAB_A_")+id,nullptr);tree.removeProperty(juce::String("qqscAB_B_")+id,nullptr);
        }
        for(const auto* prefix:{"qqscAB_A_","qqscAB_B_"}) tree.removeProperty(juce::String(prefix)+"limiterInitialised",nullptr);
        juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),saved);
        QQSuperCompressionAudioProcessor old;old.setStateInformation(saved.getData(),int(saved.getSize()));
        check(close(get(old,"limiterInputGainDb"),get(old,"inputGainDb")),"Legacy current shared input migration");
        old.selectABSlot(0);
        for(const auto* id:qqsc::params::normalModeIds)
        {
            const auto ordinary=get(old,id);const auto active=old.readSoundParameter(id);
            check(close(ordinary,active),"Legacy A/B shared setting migration");
        }
        old.leaveLimiterMode();old.enterLimiterMode();
        check(close(old.readSoundParameter("ratio"),limiterA[0]),"Legacy Limiter bank reseeded on entry");
        std::cout<<"PASS: legacy shared-setting migration, separate inactive A/B defaults and saved Limiter bank retention.\n";
    }
