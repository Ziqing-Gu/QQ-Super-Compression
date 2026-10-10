#define main HistoricalDynamicsMain
#include "updown_processor_test.cpp"
#undef main
#include "PluginEditor.h"

// Test-only friend accessor for DynamicDisplay. DynamicDisplay already grants
// QQSCVisualCheck friendship for the existing visual test target, so keep the
// product class private surface unchanged and expose only copies needed by the
// limiter regression executable.
struct QQSCVisualCheck
{
    struct ProjectedView
    {
        std::vector<float> blue;
        std::vector<float> output;
    };

    static void seedRetrospectiveTpHistory (DynamicDisplay& display)
    {
        display.clearHistories();
        DynamicDisplay::HistoryPoint a;
        a.inputDb = -8.0f;
        a.detectorDb = -8.0f;
        a.capturedInputGainDb = 0.0f;
        a.truePeakExcessDb = 3.0103f;

        auto b = a;
        b.inputDb = -5.0f;
        b.detectorDb = -5.0f;
        b.truePeakExcessDb = 1.5f;

        auto c = a;
        c.inputDb = -2.0f;
        c.detectorDb = -2.0f;
        c.truePeakExcessDb = 3.0103f;

        display.histories[0].points = { a, b, c };
    }

    static void seedTpRecoveryHistory (DynamicDisplay& display)
    {
        display.clearHistories();
        DynamicDisplay::HistoryPoint hit;
        hit.inputDb=-1.0f; hit.detectorDb=-1.0f; hit.capturedInputGainDb=0.0f; hit.truePeakExcessDb=3.0103f;
        DynamicDisplay::HistoryPoint tail=hit;
        tail.inputDb=-12.0f; tail.detectorDb=-12.0f; tail.truePeakExcessDb=0.0f;
        auto tail2=tail,tail3=tail;
        display.histories[0].points={hit,tail,tail2,tail3};
    }

    static void seedTransferContractHistory (DynamicDisplay& display)
    {
        display.clearHistories();
        DynamicDisplay::HistoryPoint a;
        a.inputDb=-3.0f; a.detectorDb=-10.0f; a.capturedInputGainDb=0.0f; a.truePeakExcessDb=0.0f;
        DynamicDisplay::HistoryPoint b=a;
        // Same detector evidence but a wildly different captured carrier peak.
        // INT transfer projection must not turn this unrelated block peak into
        // a fake processed-output spike.
        b.inputDb=-50.0f;
        DynamicDisplay::HistoryPoint c=a;
        c.inputDb=-14.0f; c.detectorDb=-6.0f; c.truePeakExcessDb=3.0103f;
        DynamicDisplay::HistoryPoint d=a;
        d.inputDb=-22.0f; d.detectorDb=-14.0f; d.truePeakExcessDb=1.2f;
        display.histories[0].points={a,b,c,d};
    }

    static size_t retrospectiveHistorySize (DynamicDisplay& display, int domain=0)
    {
        return display.histories[static_cast<size_t> (domain)].points.size();
    }

    static void tickDisplay (DynamicDisplay& display)
    {
        display.timerCallback();
    }

    static float historyXForTest (DynamicDisplay& display, uint64_t referenceCounter,
                                  uint64_t pointCounter, double sampleRate, float width)
    {
        display.renderReferenceCounter = referenceCounter;
        display.renderReferenceSampleRate = sampleRate;
        return display.historyXForCounter (pointCounter, { 0.0f, 0.0f, width, 100.0f });
    }

    static size_t trimHistoryForTest (DynamicDisplay& display, uint64_t referenceCounter,
                                      double sampleRate, std::initializer_list<uint64_t> counters)
    {
        display.clearHistories();
        display.renderReferenceCounter = referenceCounter;
        display.renderReferenceSampleRate = sampleRate;
        for (const auto counter : counters)
        {
            DynamicDisplay::HistoryPoint point;
            point.captureCounter = counter;
            display.histories[0].points.push_back (point);
        }
        display.trimHistoryToVisibleWindow (display.histories[0], referenceCounter);
        return display.histories[0].points.size();
    }

    static void verifyStereoLimiterPresentation()
    {
        QQSuperCompressionAudioProcessor p;p.enterLimiterMode();
        set(p,"limiterRatioL",4);set(p,"limiterThresholdLDb",-12);
        set(p,"limiterThresholdRDb",-60);set(p,"limiterKeySource",1);
        set(p,"truePeakLimiting",0);set(p,"ceilingDb",0);
        DynamicDisplay d(p);d.setSize(1000,530);d.clearHistories();
        d.renderReferenceCounter=300;d.renderReferenceSampleRate=48000;
        for(int ch=0;ch<2;++ch)for(uint64_t counter:{100u,200u,300u})
        {
            DynamicDisplay::HistoryPoint point;
            point.inputDb=ch==0?-2.f:-8.f;point.detectorDb=ch==0?0.f:-30.f;
            point.captureCounter=counter;point.captureGeneration=1;
            d.histories[size_t(ch)].points.push_back(point);
        }
        for(float link:{0.f,50.f,100.f})
        {
            set(p,"limiterStereoLink",link);d.refreshRenderCaches(qqsc::params::leftRight);
            const auto& c=d.stereoLimiterCache.projected;
            check(c.size==3,"Single ST view retains stereo timestamps");
            for(size_t i=0;i<c.size;++i)
            {
                const auto& l=d.renderCaches[0].projected;const auto& r=d.renderCaches[1].projected;
                check(c.captureCounter[i]==l.captureCounter[i] && c.captureCounter[i]==r.captureCounter[i],"ST aligned counters");
                check(std::abs(c.output[i]-std::max(l.output[i],r.output[i]))<.001f,"ST combines processed peaks, not detectors");
                check(std::abs(c.input[i]+2)<.001f,"ST combines input peaks");
            }
            if(link==0)check(std::abs(c.output[0]+8)<.002f,"Independent limiting shown in ST");
            if(link==100)check(std::abs(c.output[0]+11)<.002f,"Fully coupled limiting shown in ST");
        }
        for(float look:{0.f,26.f,100.f})
        {
            set(p,"limiterLookaheadMs",look);set(p,"limiterRatioL",1);
            d.refreshRenderCaches(qqsc::params::leftRight);
            const auto& c=d.stereoLimiterCache.projected;
            check(std::abs(c.output[0]-c.input[0])<.001f,"Unity ST curves coincide at every Lookahead");
        }
        check(d.domainPanelBounds(0,qqsc::params::leftRight)==d.domainPanelBounds(0,qqsc::params::stereoLinked),"Full ST panel");
        check(d.getBoundaryPlotForDomain(1)==d.getBoundaryPlotForDomain(0),"Shared boundary uses ST scale");
        p.leaveLimiterMode();
        check(d.domainPanelBounds(0,qqsc::params::leftRight)!=d.domainPanelBounds(1,qqsc::params::leftRight),"Normal LR stays split");
        std::cout<<"PASS: stereo Limiter single display, aligned independent/coupled peak projection and unity at 0/26/100 ms.\n";
    }

    static void verifyStereoLinkProjection()
    {
        QQSuperCompressionAudioProcessor p;p.enterLimiterMode();
        set(p,"limiterRatioL",4);set(p,"limiterRatioR",1);set(p,"limiterThresholdLDb",-12);
        set(p,"truePeakLimiting",0);set(p,"ceilingDb",0);
        DynamicDisplay display(p);display.setSize(1000,500);display.clearHistories();
        for(int ch=0;ch<2;++ch)
        {
            DynamicDisplay::HistoryPoint point;
            point.inputDb=point.detectorDb=ch==0 ? -2.f : -30.f;
            point.captureCounter=100;point.captureGeneration=1;
            display.histories[size_t(ch)].points={point};
        }
        set(p,"limiterStereoLink",0);display.refreshRenderCaches(qqsc::params::leftRight);
        const float right0=display.renderCaches[1].projected.output[0];
        set(p,"limiterStereoLink",100);display.refreshRenderCaches(qqsc::params::leftRight);
        const float right100=display.renderCaches[1].projected.output[0];
        // Classic 4:1 above -12 dB: at -2 dB the cut is exactly 7.5 dB.
        check(std::abs(right0+30)<.002f && std::abs(right100+37.5f)<.002f,"Linked display uses peer gain, including a unity local curve");
        check(std::abs(display.renderCaches[1].projected.effectiveGainReduction[0]-7.5f)<.002f,"Linked display GR");
        set(p,"limiterStereoLink",0);display.refreshRenderCaches(qqsc::params::leftRight);
        check(std::abs(display.renderCaches[1].projected.output[0]-right0)<.002f,"Display link return");
        std::cout<<"PASS: asymmetric LR display, unity local curve and link changes.\n";
    }

    static ProjectedView projectRetrospectiveTpHistory (DynamicDisplay& display, int mode)
    {
        display.refreshRenderCaches (mode);
        const auto& projected = display.renderCaches[0].projected;
        ProjectedView view;
        view.blue.reserve (projected.size);
        view.output.reserve (projected.size);
        for (size_t i = 0; i < projected.size; ++i)
        {
            view.blue.push_back (projected.gainReductionBoundary[i]);
            view.output.push_back (projected.output[i]);
        }
        return view;
    }
};

#include "shared_snapshot_match_checks.h"

struct QQSCLimiterCheck
{
#include "output_link_checks.inc"
#include "strict_output_link_checks.inc"
#include "dual_algorithm_checks.inc"
#include "ratio_mix_checks.inc"
#include "mode_memory_checks.inc"
#include "limiter_mode_continuity_checks.inc"
#include "independent_banks_full_link_checks.inc"
#include "strict_one_to_one_link_checks.inc"
#include "algorithm_boundary_toggle_checks.inc"
#include "detector_window_checks.inc"
#include "distort_checks.inc"
#include "window_link_checks.inc"
#include "shared_controls_checks.inc"
#include "limiter_45_checks.inc"
#include "lookahead_presets_checks.inc"
#include "overall_os_checks.inc"
#include "retrospective_tp_display_checks.inc"
#include "tp_recovery_checks.inc"
#include "revision1225_limiter_unity_link_checks.inc"
#include "revision1226_shift_drag_continuity_checks.inc"
#include "revision1227_display_drag_performance_checks.inc"
#include "revision1228_ceiling_mode_checks.inc"
#include "revision1234_performance_architecture_checks.inc"
    static juce::MouseEvent mouse(juce::Component& c,juce::Point<float> position,bool alt=false)
    {
        const auto now=juce::Time::getCurrentTime();
        return {juce::Desktop::getInstance().getMainMouseSource(),position,
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | (alt ? juce::ModifierKeys::altModifier : 0)),
                1.0f,0.0f,0.0f,0.0f,0.0f,&c,&c,now,position,now,1,false};
    }
    static std::unique_ptr<juce::PropertiesFile> settings(const juce::File& root)
    {
        juce::PropertiesFile::Options options; options.applicationName="LimiterCheck";
        options.storageFormat=juce::PropertiesFile::storeAsXML;
        return std::make_unique<juce::PropertiesFile>(root.getChildFile("ui-test.settings"),options);
    }
    static void sync(QQSuperCompressionAudioProcessor& p,QQSuperCompressionAudioProcessorEditor& e)
    { (void)p.getAPVTS().copyState(); e.updateCompressionUi(); e.updateModeUi(); }
    static void image(QQSuperCompressionAudioProcessorEditor& e,const juce::File& file)
    {
        e.setSize(1200,800);e.resized();
        for(auto* child:e.contentRoot.getChildren())
            if(child->isVisible()) check(e.contentRoot.getLocalBounds().contains(child->getBounds()),"Visible control outside editor");
        check(e.limiterButton.getY()==e.sidechainButton.getY() && e.limiterButton.getRight()<e.sidechainButton.getX(),"Limiter must occupy the former header algorithm position");
        if(e.ceilingPanel.isVisible())
            check(e.contentRoot.getIndexOfChildComponent(&e.ceilingLabel)>e.contentRoot.getIndexOfChildComponent(&e.ceilingPanel),"Ceiling panel occludes title");
        auto stream=file.createOutputStream();check(stream!=nullptr,"PNG stream");
        stream->setPosition(0);stream->truncate();
        check(juce::PNGImageFormat().writeImageToStream(e.createComponentSnapshot(e.getLocalBounds()),*stream),"PNG write");
    }
    static void run(const juce::File& root)
    {
        std::cout<<"CHECK: Limiter UI entry/restore.\n";
        for(int algo:{0,1}) for(float out:{2.0f,-3.0f,0.0f})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);set(p,"rangeDb",1);set(p,"outputGainDb",out);set(p,"algorithmMode",float(algo));
            set(p,"inputOutputLink",1);set(p,"compressionMode",1);set(p,"upThresholdDb",-36);set(p,"downThresholdDb",-12);
            (void)p.getAPVTS().copyState();
            std::array<float,35> before;
            for(size_t i=0;i<35;++i)before[i]=get(p,qqsc::params::normalSoundIds[i]);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));
            sync(p,e);e.limiterButton.onClick();sync(p,e);
            check(p.isLimiterMode(),"Limiter not enabled");
            check(std::abs(e.outputGainSlider.getValue()-std::max(0.0f,out))<0.005,"Entry output");
            check(std::abs(p.getBoundaryForDomainDb(true,true,0)+std::max(0.0f,out))<0.005,"Entry down threshold");
            check(std::abs(p.readSoundParameter("upRatio")-.125f)<1e-6,"UP ratio changed on entry");
            check(e.downRatioSliders[0]->getMinimum()==200 && e.downRatioSliders[0]->getMaximum()==1000,"Down ratio range");
            check(std::abs(e.ratioSlider.getMinimum()-.005)<1e-7 && e.ratioSlider.getMaximum()==1,"Limiter UP range");
            check(!e.inputOutputLinkButton.isVisible() && e.limiterLinkButton.isVisible(),"I/O link visibility");
            check(e.ceilingValue.isVisible() && !e.ceilingSlider.isVisible(),"Ceiling must be a numeric field only");
            check(e.unityMonitorButton.isVisible() && p.isTruePeakSelected(),"Limiter TP must default on and show monitor button");
            check(!p.isUnityMonitorEnabled(),"Monitor must default off");
            check(e.ceilingValue.getText().contains("dBTP"),"TP ceiling unit");
            const auto outputBefore=get(p,"limiterOutputDb");
            e.inputGainSlider.onGestureStart();e.inputGainSlider.setValue(3,juce::sendNotificationSync);e.inputGainSlider.onGestureEnd();
            check(std::abs(get(p,"limiterOutputDb")-outputBefore)<1e-6 && std::abs(get(p,"outputGainDb")-out)<1e-6,"Hidden I/O Link still active");
            const auto parse=e.inputGainSlider.getValueFromText("5");e.inputGainSlider.setValue(parse,juce::sendNotificationSync);
            check(std::abs(get(p,"limiterOutputDb")-outputBefore)<1e-6,"Input numeric entry still links");
            // DOWN threshold adjusts Output; UP threshold does not.
            e.beginBoundaryGesture(0,true);e.upperBoundarySliders[0]->setValue(-10,juce::sendNotificationSync);e.endLinkedGesture();
            const auto linked=get(p,"limiterOutputDb");check(linked>outputBefore+5,"Down threshold failed inverse link");
            e.beginBoundaryGesture(0,false);e.thresholdSlider.setValue(-42,juce::sendNotificationSync);e.endLinkedGesture();
            check(std::abs(get(p,"limiterOutputDb")-linked)<.001,"UP threshold incorrectly links to Output");
            // Output reverses only the DOWN threshold.
            e.outputGainSlider.onGestureStart();e.outputGainSlider.setValue(linked+1,juce::sendNotificationSync);e.outputGainSlider.onGestureEnd();
            check(p.getBoundaryForDomainDb(true,true,0)<-10,"Output did not lower DOWN threshold");
            check(std::abs(p.getBoundaryForDomainDb(true,false,0)+42)<.005,"Output touched UP threshold");
            e.limiterLinkButton.onClick();const auto unlinked=get(p,"limiterOutputDb");
            e.beginBoundaryGesture(0,true);e.upperBoundarySliders[0]->setValue(-7,juce::sendNotificationSync);e.endLinkedGesture();
            check(std::abs(get(p,"limiterOutputDb")-unlinked)<.001,"Disabled Limiter Link still runs");
            // Saved sessions retain both banks; OFF restores every named normal parameter.
            juce::MemoryBlock saved;p.getStateInformation(saved);
            QQSuperCompressionAudioProcessor q;q.setStateInformation(saved.getData(),int(saved.getSize()));
            check(q.isLimiterMode() && q.isTruePeakSelected(),"Saved Limiter/TP mode missing");set(q,"limiterMode",0);
            for(size_t i=0;i<35;++i)check(std::abs(get(q,qqsc::params::normalSoundIds[i])-before[i])<.0001,"Reload damaged pre-Limiter state");
            e.limiterButton.onClick();sync(p,e);
            check(!e.ceilingValue.isVisible() && e.inputOutputLinkButton.isVisible(),"OFF visibility");
            for(size_t i=0;i<35;++i)check(std::abs(get(p,qqsc::params::normalSoundIds[i])-before[i])<.0001,"OFF failed full parameter restoration");
            check(std::abs(e.outputGainSlider.getValue()-out)<.005,"OFF failed Output restore");
            check(get(p,"inputOutputLink")==1,"Original I/O Link preference lost");
        }
        std::cout<<"PASS: 6 entry cases, positive/negative/zero output, Classic/Super; full restore and save/reload; I/O Link disabled including typed edits; only DOWN links both directions; Link OFF.\n";

        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"algorithmMode",0);set(p,"rangeDb",1);set(p,"compressionMode",1);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root));sync(p,e);e.limiterButton.onClick();sync(p,e);
        check(get(p,"ceilingDb")==0,"Fresh Ceiling default must be 0 dBFS");
        // Numeric edit and the exact global keyboard handlers requested by the user.
        p.getUndoManager().clearUndoHistory();
        e.ceilingValue.setText("-3.25",juce::sendNotificationSync);sync(p,e);
        check(std::abs(get(p,"ceilingDb")+3.25)<.001,"Ceiling numeric commit");
        const auto ctrl=juce::ModifierKeys::ctrlModifier;
        check(e.keyPressed(juce::KeyPress('Z',ctrl,0),&e),"Ctrl+Z not consumed");sync(p,e);
        check(std::abs(get(p,"ceilingDb"))<.001,"Ceiling undo must return to the 0 dB default");
        e.keyPressed(juce::KeyPress('Z',ctrl|juce::ModifierKeys::shiftModifier,0),&e);sync(p,e);
        check(std::abs(get(p,"ceilingDb")+3.25)<.001,"Ceiling redo");
        e.ceilingValue.start();e.ceilingValue.move(10,false);e.ceilingValue.finish();sync(p,e);
        check(std::abs(get(p,"ceilingDb")+2.75)<.001,"Ceiling drag");
        e.ceilingValue.start();for(int i=0;i<10;++i)e.ceilingValue.move(.1f,true);e.ceilingValue.finish();sync(p,e);
        check(std::abs(get(p,"ceilingDb")+2.74)<.011,"Subpixel Shift accumulation");
        const auto final=get(p,"ceilingDb");e.keyPressed(juce::KeyPress('Z',ctrl,0),&e);sync(p,e);
        check(std::abs(get(p,"ceilingDb")+2.75)<.001,"Shift drag must be one undo step");
        e.keyPressed(juce::KeyPress('Z',ctrl|juce::ModifierKeys::shiftModifier,0),&e);sync(p,e);
        check(std::abs(get(p,"ceilingDb")-final)<.001,"Shift drag redo");
        // Actual double click opens JUCE's editor; commit participates in same path.
        e.ceilingValue.showEditor();check(e.ceilingValue.getCurrentTextEditor()!=nullptr,"Ceiling inline editor missing");
        e.ceilingValue.getCurrentTextEditor()->setText("-0.70");e.ceilingValue.hideEditor(false);sync(p,e);
        check(std::abs(get(p,"ceilingDb")+.7)<.001,"Inline text commit");
        const auto altClick=mouse(e.ceilingValue,e.ceilingValue.getLocalBounds().toFloat().getCentre(),true);
        std::cout<<"CHECK: Ceiling Alt-click reset.\n";
        e.ceilingValue.mouseDown(altClick);e.ceilingValue.mouseUp(altClick);sync(p,e);
        check(get(p,"ceilingDb")==0,"Alt-click did not reset Ceiling to zero");
        check(!e.ceilingValue.isBeingEdited(),"Alt-click must not open inline editing");
        e.keyPressed(juce::KeyPress('Z',ctrl,0),&e);sync(p,e);
        check(std::abs(get(p,"ceilingDb")+.7)<.001,"Alt-click reset must undo to previous Ceiling");
        e.keyPressed(juce::KeyPress('Z',ctrl|juce::ModifierKeys::shiftModifier,0),&e);sync(p,e);
        check(get(p,"ceilingDb")==0,"Alt-click reset redo");
        p.getMeterState().truePeakHoldDb.store(2.0f);
        std::cout<<"CHECK: TP double-click.\n";
        e.meters.mouseDoubleClick(mouse(e.meters,{3.0f,100.0f}));
        check(p.getMeterState().truePeakHoldDb.load()==2.0f,"Click outside TP must not clear it");
        e.meters.mouseDoubleClick(mouse(e.meters,{e.meters.getWidth()*.5f,20.0f}));
        check(p.getMeterState().truePeakHoldDb.load()<=-120.0f,"TP double-click must clear immediately without audio callbacks");
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();e.updateLimiterUi();
            image(e,root.getChildFile(juce::String("limiter-")+qqsc::ui::themeKey(theme)+".png"));
            e.unityMonitorButton.onClick();sync(p,e);
            image(e,root.getChildFile(juce::String("limiter-monitor-on-")+qqsc::ui::themeKey(theme)+".png"));
            e.unityMonitorButton.onClick();sync(p,e);
        }
        e.unityMonitorButton.onClick();sync(p,e);
        image(e,root.getChildFile("limiter-monitor-on.png"));
        e.limiterButton.onClick();sync(p,e);image(e,root.getChildFile("limiter-off.png"));
        check(!e.unityMonitorButton.isVisible() && !e.ceilingValue.isVisible() && !e.monitorCeilingLabel.isVisible(),"Monitor/Ceiling must hide together");
        set(p,"truePeakLimiting",0);
        std::cout<<"PASS: Ceiling default 0, typed edit, drag, Alt-click reset/undo/redo; TP double-click resets without audio; 3 themes and OFF layout.\n";
        set(p,"ratio",6);set(p,"outputGainDb",2);p.copyAToB();
        e.limiterButton.onClick();sync(p,e);set(p,"limiterDownRatio",400);set(p,"ceilingDb",-2.5f);set(p,"truePeakLimiting",1);
        p.selectABSlot(1);sync(p,e);
        check(!p.isLimiterMode() && std::abs(get(p,"ratio")-6)<.0001,"B did not restore ordinary bank");
        check(!p.isTruePeakSelected(),"B lost TP OFF state");
        set(p,"ratio",9);p.selectABSlot(0);sync(p,e);
        check(p.isLimiterMode() && std::abs(p.readSoundParameter("downRatio")-400)<.001 && std::abs(get(p,"ceilingDb")+2.5)<.001,"A lost Limiter bank/Ceiling");
        check(p.isTruePeakSelected(),"A lost TP ON state");
        e.limiterButton.onClick();sync(p,e);
        check(std::abs(get(p,"ratio")-6)<.0001 && std::abs(get(p,"outputGainDb")-2)<.0001,"A/B lost pre-Limiter restoration state");
        std::cout<<"PASS: A/B stores both complete parameter banks, Ceiling and pre-Limiter restoration.\n";
    }
};

void limiterAudioChecks()
{
    for(int algo:{0,1})for(int mode:{0,1,2})for(int dual:{0,1})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"algorithmMode",float(algo));set(p,"processingMode",float(mode));set(p,"compressionMode",float(dual));
        set(p,"rangeDb",1);set(p,"rangeLDb",1);set(p,"rangeRDb",1);set(p,"rangeMDb",1);set(p,"rangeSDb",1);
        const auto original=render(p,.12f);p.enterLimiterMode();set(p,"ceilingDb",-24);set(p,"truePeakLimiting",1);set(p,"limiterMode",0);
        const auto off=render(p,.12f);check(original.output==off.output && original.right==off.right,"Ceiling/limiter processing leaks while OFF");
        p.enterLimiterMode();set(p,"truePeakLimiting",0);set(p,"ceilingDb",0);const auto a=render(p,.12f);
        set(p,"ceilingDb",-6);const auto b=render(p,.12f);
        const auto tpMinus6=p.getMeterState().truePeakHoldDb.load();
        set(p,"ceilingDb",0);render(p,.12f);
        const auto tpZero=p.getMeterState().truePeakHoldDb.load();
        check(std::abs((tpZero-tpMinus6)-6)<.01,"TP must include final Ceiling gain in ST/LR/MS and Single/Dual");
        const double factor=std::pow(10.0,-6.0/20);
        double err=0;for(size_t i=0;i<a.output.size();++i)err=std::max(err,std::abs(double(b.output[i])-double(a.output[i])*factor));
        check(err<1e-6,"Ceiling scaling under steady input inconsistent");
        check(a.latency==b.latency && a.latency==original.latency && a.latency>1248 && a.latency<1920,"Guard latency must be constant and reported");
    }
    std::cout<<"PASS: 12 DSP cases, hidden Ceiling/TP OFF bit-identical; active steady-input ceiling scaling; constant reported latency below 40ms.\n";
    for(int algo:{0,1})for(float output:{0.0f,2.0f,18.0f})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"algorithmMode",float(algo));set(p,"rangeDb",1);set(p,"ratio",100);set(p,"outputGainDb",output);
        p.enterLimiterMode();const auto audio=render(p,1.0f);
        float peak=0;for(size_t i=16000;i<audio.output.size();++i)peak=std::max(peak,std::abs(audio.output[i]));
        check(20*std::log10(peak)<=.002 && 20*std::log10(peak)>-.2,"Default TP Ceiling reference calibration inaccurate");
        check(std::abs(get(p,"limiterOutputDb")-output)<.0001,"Calibration rewrote visible Output");
    }
    std::cout<<"PASS: 6 full-scale TP reference cases within reconstruction margin; 12 post-Ceiling TP scalar checks.\n";
    for(double rate:{44100.0,48000.0,96000.0})for(int block:{17,256,1024})
    {
        QQSuperCompressionAudioProcessor p;set(p,"lookaheadMs",0);set(p,"oversampling",0);set(p,"ratio",1);set(p,"rangeDb",1);
        p.setRateAndBufferSizeDetails(rate,block);p.prepareToPlay(rate,block);
        juce::MidiBuffer midi;int64_t offset=0;
        auto run=[&](double seconds,float amplitude)
        {
            const int count=int(seconds*rate);
            for(int n=0;n<count;n+=block)
            {
                const int size=std::min(block,count-n);juce::AudioBuffer<float>b(2,size);
                for(int i=0;i<size;++i)
                {const float x=amplitude*float(std::sin(juce::MathConstants<double>::halfPi*double(offset+i)+juce::MathConstants<double>::pi/4));b.setSample(0,i,x);b.setSample(1,i,x);}
                p.processBlock(b,midi);offset+=size;
            }
        };
        run(.1,1);const auto tp=p.getMeterState().truePeakHoldDb.load();
        check(std::abs(tp)<.15,"TP interpolation missed known 3.01dB intersample peak");
        run(19.0,.1f);check(p.getMeterState().truePeakHoldDb.load()>-1,"TP hold lost peak before 20s");
        run(1.2,.1f);check(p.getMeterState().truePeakHoldDb.load()<-19.8,"TP hold did not expire after 20s");
        run(.1,1);run(.1,.1f);check(p.getMeterState().truePeakHoldDb.load()>-1,"New peak was not held");
        p.resetTruePeakHold();check(p.getMeterState().truePeakHoldDb.load()<=-120,"Manual reset was not immediate");
        run(.02,.1f);check(p.getMeterState().truePeakHoldDb.load()<-19.8,"Manual reset retained the old accumulator");
    }
    std::cout<<"PASS: 9 TP cases (44.1/48/96k,17/256/1024 blocks), analytic quarter-rate sine +/-0.15dB, 20s hold/expiry and manual reset.\n";
}

#include "peak_guard_processor_checks.h"

int main(int argc,char** argv)
{
    std::cout<<std::unitbuf;
    if(argc!=2 && argc!=3)return 2;juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        const juce::File root(argv[1]);root.createDirectory();
        if(argc==3 && juce::String(argv[2])=="cumulative-match")
        { QQSCReviewCheck::cumulativeMatchChecks();return 0; }
        if(argc==3 && juce::String(argv[2])=="limiter-45")
        { QQSCLimiterCheck::limiter45Checks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="shared-controls")
        { QQSCLimiterCheck::sharedControlsChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="window-link")
        { QQSCLimiterCheck::windowLinkChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="distort")
        { QQSCLimiterCheck::distortChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="lookahead-presets")
        { QQSCLimiterCheck::lookaheadPresetsChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="overall-os")
        { QQSCLimiterCheck::overallOversamplingChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="detector-window")
        { QQSCLimiterCheck::detectorWindowChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="algorithm-boundary-toggle")
        { QQSCLimiterCheck::algorithmBoundaryToggleChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="dual")
        { QQSCLimiterCheck::dualAlgorithmChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="continuity")
        { QQSCLimiterCheck::limiterModeContinuityChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision129")
        { QQSCLimiterCheck::independentBanksFullLinkChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1211")
        { QQSCLimiterCheck::strictOneToOneLinkChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1217")
        { QQSCLimiterCheck::retrospectiveTpDisplayChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1218")
        { QQSCLimiterCheck::tpRecoveryChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1220")
        { QQSCLimiterCheck::retrospectiveTpDisplayChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1221")
        { QQSCLimiterCheck::retrospectiveTpDisplayChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1222")
        { QQSCLimiterCheck::ratioMixChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1223")
        { QQSCLimiterCheck::ratioMixChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1224")
        { QQSCLimiterCheck::ratioMixChecks(root);return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1225")
        { QQSCLimiterCheck::ratioMixChecks(root); QQSCLimiterCheck::limiterUnityAndBidirectionalLinkChecks(root); return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1226")
        { QQSCLimiterCheck::shiftDragContinuityChecks(root); return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1227")
        { QQSCLimiterCheck::displayDragPerformanceChecks(root); return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1228")
        { QQSCLimiterCheck::ceilingModeChecks(root); QQSCLimiterCheck::retrospectiveTpDisplayChecks(root); return 0; }
        if(argc==3 && juce::String(argv[2])=="revision1234")
        { QQSCLimiterCheck::performanceArchitectureChecks(root); return 0; }
        // 1.2.11 intentionally supersedes the 1.2.9 full-reference Ratio/Mix
        // link contract. Keep historical checks in source, but do not run them
        // on the default acceptance path.
        QQSCLimiterCheck::strictOneToOneLinkChecks(root);
        QQSCLimiterCheck::limiterModeContinuityChecks(root);
        QQSCLimiterCheck::dualAlgorithmChecks(root);
        std::cout<<"PASS: Limiter candidate checks complete.\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
