    static void displayScaleChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;p.setEcoMode(false);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));
        e.stopTimer();auto& d=e.display;d.stopTimer();e.setSize(1200,800);
        const auto event=[&](juce::Point<float> position,bool ctrl=false,bool right=false,bool left=false,bool shift=false)
        {
            auto now=juce::Time::getCurrentTime();return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),position,
                juce::ModifierKeys((right?juce::ModifierKeys::rightButtonModifier:left?juce::ModifierKeys::leftButtonModifier:0)|(ctrl?juce::ModifierKeys::ctrlModifier:0)|(shift?juce::ModifierKeys::shiftModifier:0)),
                1.f,0.f,0.f,0.f,0.f,&d,&d,now,position,now,0,false);
        };
        const auto wheel=[&](juce::Point<float> pos,float delta,bool ctrl=false,bool xAxis=false,bool shift=false)
        {
            juce::MouseWheelDetails details {};if(xAxis)details.deltaX=delta;else details.deltaY=delta;
            // Four finer events now cover the former one-event range. Keep
            // all existing geometry, anchor and audio expectations unchanged.
            for(int step=0;step<4;++step) d.mouseWheelMove(event(pos,ctrl,false,false,shift),details);
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
            const auto anchor=plot.getRelativePoint(.25f,.2f);
            // The whole Display accepts starts; only the actual plot is zoomed.
            for(int panel=0;panel<(family==0 && mode!=0?2:1);++panel)
            {
                const auto panelPlot=d.getBoundaryPlotForDomain(domain+panel);
                const auto testRectangle=[&](juce::Point<float> from,juce::Point<float> to,
                                             float floor,float top,float seconds)
                {
                    d.resetZoom();const auto press=event(from,false,false,true);
                    d.mouseDown(press);
                    check(d.zoomDragActive,"Display margin must accept rectangle start");
                    check(d.zoomPlot==panelPlot,"Margin picked wrong subplot");
                    d.mouseDrag(press.withNewPosition(to));
                    check(d.zoomSelection.getX()>=panelPlot.getX() && d.zoomSelection.getRight()<=panelPlot.getRight()
                          && d.zoomSelection.getY()>=panelPlot.getY() && d.zoomSelection.getBottom()<=panelPlot.getBottom(),
                          "Expanded rectangle escaped plot boundary");
                    if(family==1 && mode==0 && dual==0 && panel==0 && from==juce::Point<float>(1.f,panelPlot.getY()-8.f)) {
                        auto image=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;
                        juce::FileOutputStream out(root.getChildFile("margin-rectangle.png"));png.writeImageToStream(image,out);
                    }
                    d.mouseUp(press.withNewPosition(to));const auto v=p.getDisplayZoom();
                    check(std::abs(v[0]-floor)<.002f && std::abs(v[1]-top)<.002f
                          && std::abs(v[2]-seconds)<.002f && v[3]==0.f,"Clamped rectangle zoom differs from edge selection");
                    check(p.getBoundaryForDomainDb(activeDual,false,domain)==threshold && p.getDynamicsGainForDomain(.4f,domain)==gain,
                          "Expanded rectangle changed audio parameters");
                };
                testRectangle({1.f,panelPlot.getY()-8.f},panelPlot.getRelativePoint(.6f,.4f),-36,0,4.8f);
                testRectangle({1.f,panelPlot.getRelativePoint(0.f,.2f).y},panelPlot.getRelativePoint(.75f,.7f),-63,-18,6);
                testRectangle({float(d.getWidth()-1),panelPlot.getRelativePoint(1.f,.8f).y},panelPlot.getRelativePoint(.25f,.3f),-72,-27,6);
                const bool lastPanel=panel==((family==0 && mode!=0)?1:0);
                testRectangle({panelPlot.getRelativePoint(.3f,1.f).x,lastPanel?float(d.getHeight()-1):panelPlot.getBottom()+8.f},
                              panelPlot.getRelativePoint(.8f,.25f),-90,-22.5f,4);
                testRectangle(panelPlot.getRelativePoint(.25f,.2f),{-50,-50},-18,0,2);
                testRectangle(panelPlot.getRelativePoint(.25f,.2f),{float(d.getWidth()+50),float(d.getHeight()+50)},-90,-18,6);
                testRectangle({1.f,panel==0?1.f:panelPlot.getY()-8.f},{float(d.getWidth()+50),float(d.getHeight()+50)},-90,0,8);
                // Moving solely inside the header maps to a zero-height edge and does not zoom.
                d.resetZoom();auto from=juce::Point<float>(panelPlot.getX(),panelPlot.getY()-12.f);
                auto to=juce::Point<float>(panelPlot.getRight(),panelPlot.getY()-3.f);
                const auto press=event(from,false,false,true);d.mouseDown(press);d.mouseDrag(press.withNewPosition(to));d.mouseUp(press.withNewPosition(to));
                check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"All-margin rectangle must not invent a vertical selection");
            }
            d.resetZoom();const bool freezeBefore=p.isDisplayFreezeOnStop();
            d.mouseDown(event(d.freezeButton.getBounds().toFloat().getCentre(),false,false,true));
            check(!d.zoomDragActive && p.isDisplayFreezeOnStop()==freezeBefore,"Rectangle intercepted Freeze button");
            // Shift pans the vertical viewport without changing either scale.
            for(int panArea=0;panArea<(family==0 && mode!=0?3:2);++panArea)
            {
                const auto panPlot=d.getBoundaryPlotForDomain(domain+(panArea==2?1:0));
                const auto at=panArea==0 ? juce::Point<float>(anchor.x,12.f) : panPlot.getCentre();
                p.setDisplayZoom(-60,-30,4,0);d.refreshScale();
                const auto oldY=d.getBoundaryYForDomainDb(domain,-45);
                wheel(at,1.f/3.f,false,false,true);const auto shifted=p.getDisplayZoom();
                check(std::abs(d.getVisibleDbSpan()-30.f)<.001f && shifted[2]==4.f && shifted[3]==0.f,
                      "Shift wheel must pan without zooming or changing time");
                check(std::abs(shifted[0]+54.f)<.001f && std::abs(shifted[1]+24.f)<.001f,
                      "Shift up must view higher levels in fine steps");
                check(std::abs(d.getBoundaryYForDomainDb(domain,-45)-(oldY+plot.getHeight()*.2f))<.02f,
                      "Panned threshold line lost vertical alignment");
                check(std::abs(d.getBoundaryDbForY(domain,d.getBoundaryYForDomainDb(domain,-45))+45.f)<.001f,
                      "Panned threshold inverse mapping");
                check(p.getBoundaryForDomainDb(activeDual,false,domain)==threshold && p.getDynamicsGainForDomain(.4f,domain)==gain,
                      "Shift pan changed threshold or audio gain");
                wheel(at,-1.f/3.f,false,false,true);
                check(p.getDisplayZoom()==std::array<float,4>{-60,-30,4,0},"Reverse pan failed");
                wheel(at,100,false,false,true);
                check(p.getDisplayZoom()==std::array<float,4>{-30,0,4,0},"Upper pan limit changed span");
                wheel(at,-100,false,false,true);
                check(p.getDisplayZoom()==std::array<float,4>{-90,-60,4,0},"Lower pan limit changed span");
                wheel(at,1.f/3.f,true,false,true);
                check(std::abs(p.getDisplayZoom()[2]-2.f)<.001f && p.getDisplayZoom()[0]==-90.f && p.getDisplayZoom()[1]==-60.f,
                      "Ctrl must retain horizontal priority with Shift held");
                d.mouseDown(event(at,false,true));wheel(at,1.f/3.f,false,false,true);
                check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Full view must not shrink during pan");
                wheel(at,-1.f/3.f,false,false,true);
                check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Full view must not move outside global limits");
            }
            // Header uses a fixed 0 dB top; plot gestures keep their pointer anchor.
            for(const auto header: {juce::Point<float>(11.f,12.f),
                                    juce::Point<float>(anchor.x,plot.getY()-1.f),
                                    juce::Point<float>(float(d.getWidth()-12),32.f)})
            {
                d.resetZoom();wheel(header,1.f/3.f);
                auto h=p.getDisplayZoom();
                check(std::abs(h[0]+45.f)<.001f && h[1]==0.f && h[2]==8.f,
                      "Header wheel must zoom vertically from 0 dB");
                wheel(header,1.f/3.f,true);h=p.getDisplayZoom();
                check(std::abs(h[2]-4.f)<.001f && h[1]==0.f && h[3]==0.f,"Header Ctrl zoom lost live edge");
                wheel(header,-1.f/3.f);wheel(header,-1.f/3.f,true,true);
                check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Reverse header wheel did not restore global range");
                p.setDisplayZoom(-55,-10,4,0);d.refreshScale();wheel(header,1.f/3.f);
                h=p.getDisplayZoom();
                check(std::abs(h[0]+22.5f)<.001f && h[1]==0.f && h[2]==4.f,"Header must restore 0 dB anchor after panned plot zoom");
                wheel(header,100);check(p.getDisplayZoom()[0]==-1 && p.getDisplayZoom()[1]==0,"Header minimum vertical span");
                wheel(header,-100);check(p.getDisplayZoom()[0]==-90 && p.getDisplayZoom()[1]==0,"Header maximum vertical span");
                check(p.getBoundaryForDomainDb(activeDual,false,domain)==threshold && p.getDynamicsGainForDomain(.4f,domain)==gain,"Header zoom changed audio parameters");
                d.mouseDown(event(header,false,true));
                check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Header right-click failed");
            }
            // A wheel over the child Freeze button propagates without toggling it.
            const bool freeze=p.isDisplayFreezeOnStop();
            auto buttonAt=d.freezeButton.getBounds().toFloat().getCentre();
            juce::MouseWheelDetails details {};details.deltaY=1.f/3.f;
            d.freezeButton.mouseWheelMove(event(buttonAt).getEventRelativeTo(&d.freezeButton),details);
            check(d.getVisibleDbSpan()<90.f && p.isDisplayFreezeOnStop()==freeze,"Freeze child wheel not forwarded or toggled Freeze");
            d.resetZoom();
            wheel(anchor,1.f/3.f);
            auto vertical=p.getDisplayZoom();
            check(std::abs(vertical[0]+54)<.001f && std::abs(vertical[1]+9)<.001f && vertical[2]==8.f,"Vertical wheel anchor or independent axis changed");
            check(std::abs(d.getBoundaryYForDomainDb(domain,-18)-anchor.y)<.01f,"Wheel moved the level beneath the pointer");
            check(std::abs(d.getBoundaryDbForY(domain,anchor.y)+18)<.01f,"Wheel threshold inverse alignment");
            wheel(anchor,1.f/3.f,true);const auto view=p.getDisplayZoom();
            check(view[0]==vertical[0] && view[1]==vertical[1] && std::abs(view[2]-4)<.001f && view[3]==0.f,"Ctrl wheel must affect only time scale and keep live following");
            check(p.getBoundaryForDomainDb(activeDual,false,domain)==threshold && p.getDynamicsGainForDomain(.4f,domain)==gain,"Wheel changed audio parameters");
            d.renderReferenceSampleRate=48000;d.renderReferenceCounter=480000;
            const auto x=d.historyXForCounter(336000,plot);d.renderReferenceCounter+=4800;
            check(std::abs(d.historyXForCounter(336000,plot)-(x-plot.getWidth()*.025f))<.02f,"Zoom must continue scrolling at fixed scale");
            p.copyAToB();p.selectABSlot(1);check(p.getDisplayZoom()==view,"A/B changed view zoom");
            for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
            {e.theme=theme;e.applyTheme();e.resized();check(p.getDisplayZoom()==view,"Theme changed view zoom");}
            wheel(anchor,-1.f/3.f);wheel(anchor,-1.f/3.f,true,true);
            check(std::abs(p.getDisplayZoom()[0]+90)<.001f && std::abs(p.getDisplayZoom()[1])<.001f && p.getDisplayZoom()[2]==8.f,"Reverse wheel did not restore scales");
            wheel(anchor,1.f/3.f);d.mouseDown(event(anchor,false,true));
            check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Right click must restore global view");
            const auto boxStart=plot.getRelativePoint(.25f,.1f),boxEnd=plot.getRelativePoint(.75f,.3f);
            auto drag=event(boxStart,false,false,true).withNewPosition(boxEnd);
            d.mouseDown(event(boxStart,false,false,true));d.mouseDrag(drag);
            check(d.zoomDragActive && d.zoomSelection.getWidth()>6.f,"Rectangle outline missing");
            wheel(anchor,1.f/3.f);wheel(anchor,1.f/3.f,true);
            wheel(anchor,1.f/3.f,false,false,true);
            check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Wheel changed scale during rectangle gesture");
            if(cases==0)
            {auto image=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;
             juce::FileOutputStream out(root.getChildFile("rectangle-selection.png"));png.writeImageToStream(image,out);
             root.getChildFile("display-tooltip.txt").replaceWithText(d.getTooltip());}
            d.mouseUp(drag);const auto boxView=p.getDisplayZoom();
            check(std::abs(boxView[0]+27.f)<.001f && std::abs(boxView[1]+9.f)<.001f && std::abs(boxView[2]-4.f)<.001f && boxView[3]==0.f,"Rectangle scale or live offset incorrect");
            check(p.getBoundaryForDomainDb(activeDual,false,domain)==threshold && p.getDynamicsGainForDomain(.4f,domain)==gain,"Rectangle changed audio parameters");
            check(std::abs(d.historyXForCounter(d.renderReferenceCounter,plot)-plot.getRight())<.01f,"Rectangle hid newest audio");
            check(std::abs(d.getBoundaryYForDomainDb(domain,-18)-plot.getCentreY())<.01f,"Rectangle threshold alignment");
            wheel(anchor,1.f/3.f);wheel(anchor,1.f/3.f,true);
            check(std::abs(d.getVisibleDbSpan()-9.f)<.001f && std::abs(p.getDisplayZoom()[2]-2.f)<.001f && p.getDisplayZoom()[3]==0.f,"Fine wheels after rectangle did not preserve live scaling");
            d.resetZoom();d.mouseDown(event(boxEnd,false,false,true));
            const auto reverseDrag=event(boxEnd,false,false,true).withNewPosition(boxStart);d.mouseDrag(reverseDrag);d.mouseUp(reverseDrag);
            for(size_t i=0;i<4;++i)check(std::abs(p.getDisplayZoom()[i]-boxView[i])<.001f,"Reverse rectangle differs");
            d.resetZoom();d.mouseDown(event(boxStart,false,false,true));
            const auto tinyDrag=event(boxStart,false,false,true).withNewPosition(boxStart.translated(2,2));d.mouseDrag(tinyDrag);d.mouseUp(tinyDrag);
            check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Tiny rectangle changed zoom");
            d.mouseDown(event(boxStart,false,false,true));d.mouseDown(event(anchor,false,true));d.mouseUp(event(boxEnd));
            check(!d.zoomDragActive && p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Right-click failed to cancel rectangle");
            d.mouseDown(event(boxStart,false,false,true));d.resized();d.mouseUp(event(boxEnd));
            check(!d.zoomDragActive && p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Resize failed to cancel rectangle");
            d.mouseDown(event({-1,-1},false,false,true));d.mouseUp(event(boxEnd));
            check(!d.zoomDragActive && p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Rectangle started outside Display");
            wheel({1,1},1);wheel(anchor,0);wheel(anchor,std::numeric_limits<float>::quiet_NaN());wheel(anchor,1,false,true);
            check(p.getDisplayZoom()==std::array<float,4>{-90,0,8,0},"Outside/zero/invalid/unmodified horizontal wheel changed zoom");
            // Check both panels in LR/MS, multiple pointer heights and limits.
            for(int panel=0;panel<(family==0 && mode!=0?2:1);++panel)
            {
                const auto panelPlot=d.getBoundaryPlotForDomain(domain+panel);
                for(float fraction:{.1f,.5f,.9f})
                {
                    d.resetZoom();const auto pos=panelPlot.getRelativePoint(.5f,fraction);
                    const auto db=d.getBoundaryDbForY(domain+panel,pos.y);
                    wheel(pos,1.f/3.f);
                    check(std::abs(d.getBoundaryDbForY(domain+panel,pos.y)-db)<.001f,"LR/MS panel pointer anchor changed");
                    wheel(pos,100);check(std::abs(d.getVisibleDbSpan()-1)<.001f,"Vertical minimum span");
                    wheel(pos,-100);check(p.getDisplayZoom()[0]==-90 && p.getDisplayZoom()[1]==0,"Vertical global limits");
                    wheel(pos,100,true);check(p.getDisplayZoom()[2]==.1f && p.getDisplayZoom()[3]==0,"Horizontal minimum/live edge");
                    wheel(pos,-100,true);check(p.getDisplayZoom()[2]==8.f,"Horizontal maximum");
                    const auto start=panelPlot.getRelativePoint(.25f,.1f),end=panelPlot.getRelativePoint(.75f,.3f);
                    auto boxDrag=event(start,false,false,true).withNewPosition(end);
                    d.mouseDown(event(start,false,false,true));d.mouseDrag(boxDrag);d.mouseUp(boxDrag);
                    const auto local=p.getDisplayZoom();
                    check(std::abs(local[0]+27.f)<.001f && std::abs(local[1]+9.f)<.001f && std::abs(local[2]-4.f)<.001f && local[3]==0.f,"LR/MS subpanel rectangle scale incorrect");
                }
            }
            d.resetZoom();wheel(anchor,1.f/3.f);wheel(anchor,1.f/3.f,true);
            wheel(anchor,-1.f/3.f,false,false,true);
            const auto panned=p.getDisplayZoom();p.copyAToB();p.selectABSlot(1);
            check(p.getDisplayZoom()==panned,"A/B changed vertical pan");
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
        {p.setDisplayZoom(zoom?-27.f:-90.f,zoom?-9.f:0.f,zoom?4.f:8.f,0.f);d.refreshScale();d.refreshRenderCaches(0);
         auto image=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;juce::FileOutputStream out(root.getChildFile(zoom?"zoom-view.png":"global-view.png"));png.writeImageToStream(image,out);}
        p.setDisplayZoom(-51.3962f,-17.44f,8.f,0.f);d.refreshScale();d.refreshRenderCaches(0);
        {auto image=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;
         juce::FileOutputStream out(root.getChildFile("decimal-grid-view.png"));png.writeImageToStream(image,out);}
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
        std::cout<<"PASS: "<<cases<<" combined wheel/rectangle mode cases, expanded starts/edge clamping/nearest subplot, Shift pan/bounds/header/both panels, pointer anchoring, independent axes/limits, reverse/tiny rectangles, mixed gestures/cancellation, right-click reset, live scrolling, threshold alignment, A/B/project, Shift; 12 exact audio-invariance cases\n";
    }

