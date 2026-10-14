    static void displayScaleChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;p.setEcoMode(false);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));
        e.stopTimer();auto& d=e.display;d.stopTimer();e.setSize(1200,800);
        const auto event=[&](juce::Point<float> position,juce::Point<float> start,bool right=false)
        {
            auto now=juce::Time::getCurrentTime();return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),position,
                juce::ModifierKeys(right?juce::ModifierKeys::rightButtonModifier:juce::ModifierKeys::leftButtonModifier),
                1.f,0.f,0.f,0.f,0.f,&d,&d,now,start,now,1,false);
        };
        int cases=0;
        for(int family:{0,1,2})for(int mode:{0,1,2})for(int dual:{0,1})
        {
            p.selectABSlot(0);
            p.setProcessorModeFromEditor(family);set(p,p.soundParameterID("processingMode"),float(mode));
            p.setCompressionModeFromEditor(dual);e.updateModeUi();e.resized();d.resetZoom();
            const int domain=family?1:mode==0?0:mode==1?3:1;
            const bool activeDual=p.readSoundParameter("compressionMode")>.5f;
            p.setBoundaryForDomainDb(activeDual,false,domain,-18);
            const auto threshold=p.getBoundaryForDomainDb(activeDual,false,domain);
            const auto gain=p.getDynamicsGainForDomain(.4f,domain);
            const auto plot=d.getBoundaryPlotForDomain(domain);
            const auto start=plot.getRelativePoint(.25f,.1f),end=plot.getRelativePoint(.75f,.3f);
            d.mouseDown(event(start,start));d.mouseDrag(event(end,start));
            check(d.zoomDragActive && d.zoomSelection.getWidth()>6,"Box selection did not appear");
            d.mouseUp(event(end,start));const auto view=p.getDisplayZoom();
            check(std::abs(view[0]+27)<.001f && std::abs(view[1]+9)<.001f && std::abs(view[2]-4)<.001f && std::abs(view[3]-2)<.001f,"Box did not map selected dB/time region");
            check(p.getBoundaryForDomainDb(activeDual,false,domain)==threshold && p.getDynamicsGainForDomain(.4f,domain)==gain,"Zoom changed audio parameters");
            check(std::abs(d.getBoundaryYForDomainDb(domain,-18)-plot.getCentreY())<.01f,"Zoom threshold forward alignment");
            check(std::abs(d.getBoundaryDbForY(domain,plot.getCentreY())+18)<.01f,"Zoom threshold inverse alignment");
            d.renderReferenceSampleRate=48000;d.renderReferenceCounter=480000;
            const auto x=d.historyXForCounter(336000,plot);d.renderReferenceCounter+=4800;
            check(std::abs(d.historyXForCounter(336000,plot)-(x-plot.getWidth()*.025f))<.02f,"Zoom must continue scrolling at fixed scale");
            p.copyAToB();p.selectABSlot(1);check(p.getDisplayZoom()==view,"A/B changed view zoom");
            for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
            {e.theme=theme;e.applyTheme();e.resized();check(p.getDisplayZoom()==view,"Theme changed view zoom");}
            d.mouseDown(event(start,start,true));check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Right click must restore global view");
            d.mouseDown(event(start,start));d.mouseUp(event(start,start));check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Click without drag changed zoom");
            d.mouseDown(event(end,end));d.mouseDrag(event(start,end));d.mouseUp(event(start,end));
            const auto reverse=p.getDisplayZoom();
            for(size_t c=0;c<4;++c){if(std::abs(reverse[c]-view[c])>=.001f)std::cout<<"reverse family="<<family<<" mode="<<mode<<" dual="<<dual<<" c="<<c<<" expected="<<view[c]<<" actual="<<reverse[c]<<std::endl;check(std::abs(reverse[c]-view[c])<.001f,"Reverse selection differs");}
            ++cases;
        }
        juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));check(restored.getDisplayZoom()==p.getDisplayZoom(),"Project zoom restore");
        p.leaveLimiterMode();set(p,"processingMode",0);set(p,"compressionMode",0);set(p,"inputOutputLink",0);
        set(p,"inputGainDb",0);set(p,"rangeDb",1);e.updateModeUi();
        for(bool fine:{false,true})
        {
            p.setDisplayZoom(-30,-10,2,0);d.refreshScale();set(p,"thresholdDb",-20);sync(p,e);
            auto& slider=e.thresholdSlider;const auto plot=slider.boundaryPlotBounds();
            const float pixels=plot.getHeight()/20.f*(fine?8.f:1.f);
            const auto start=juce::Point<float>(slider.getWidth()*.5f,slider.getHeight()*.5f);
            auto dragEvent=[&](juce::Point<float> pos){auto now=juce::Time::getCurrentTime();return juce::MouseEvent(
                juce::Desktop::getInstance().getMainMouseSource(),pos,
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier|(fine?juce::ModifierKeys::shiftModifier:0)),
                1.f,0.f,0.f,0.f,0.f,&slider,&slider,now,start,now,1,false);};
            slider.mouseDown(dragEvent(start));slider.mouseDrag(dragEvent(start.translated(0,-pixels)));slider.mouseUp(dragEvent(start.translated(0,-pixels)));
            check(std::abs(slider.getValue()+19)<.025,"Zoom fader/Shift sensitivity");
        }
        d.histories[0].points.clear();d.histories[1].points.clear();
        d.renderReferenceCounter=480000;d.renderReferenceSampleRate=48000;
        set(p,"ratio",8);set(p,"thresholdDb",-18);sync(p,e);
        for(int i=0;i<480;++i)
        {DynamicDisplay::HistoryPoint point;point.inputDb=-19+8*std::sin(float(i)*.13f);point.detectorDb=point.inputDb+2;
         point.captureCounter=960000/10+uint64_t(i+1)*800;point.captureGeneration=1;d.pushHistory(d.histories[0],point);}
        for(bool zoom:{false,true})
        {p.setDisplayZoom(zoom?-27.f:-90.f,zoom?-9.f:0.f,zoom?4.f:8.f,zoom?2.f:0.f);d.refreshScale();d.refreshRenderCaches(0);
         auto image=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;juce::FileOutputStream out(root.getChildFile(zoom?"zoom-view.png":"global-view.png"));png.writeImageToStream(image,out);}
        for(int family:{0,1,2})for(int os:{0,1,2,3})
        {
            QQSuperCompressionAudioProcessor a,b;
            for(auto* x:{&a,&b}){x->setEcoMode(false);x->setProcessorModeFromEditor(family);
                auto value=[&](const char* id,float v){set(*x,x->soundParameterID(id),v);};
                value("oversampling",float(os));value("lookaheadMs",26);value("algorithmMode",0);
                value(family?"thresholdLDb":"thresholdDb",-30);value(family?"ratioL":"ratio",20);
                x->setRateAndBufferSizeDetails(48000,256);x->prepareToPlay(48000,256);}
            juce::MidiBuffer midi;float delta=0;
            for(int n=0;n<8192;n+=256){a.setDisplayZoom(-35,-5,.1f+(n/256)%8,0);
                juce::AudioBuffer<float> x(2,256),y(2,256);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)x.setSample(ch,i,.3f*std::sin(float(n+i)*(.031f+ch*.019f)));
                y.makeCopyOf(x);a.processBlock(x,midi);b.processBlock(y,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)delta=std::max(delta,std::abs(x.getSample(ch,i)-y.getSample(ch,i)));}
            check(delta==0,"Zoom changed audio samples");
        }
        std::cout<<"PASS: "<<cases<<" rectangle-zoom mode cases, right-click reset, reverse/tiny drag, live scrolling, threshold alignment, A/B/project, Shift; 12 exact audio-invariance cases\n";
    }
