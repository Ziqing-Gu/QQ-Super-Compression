    static void tpRecoveryChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p; baseline(p);
        QQSuperCompressionAudioProcessorEditor e(p,settings(root)); sync(p,e);
        check(p.getTpRecoveryMode()==qqsc::params::tpAuto,"TP Recovery must default AUTO");
        e.limiterButton.onClick(); sync(p,e);
        check(p.isTruePeakSelected(),"TP must default ON");
        check(e.tpRecoveryButton.isVisible(),"TP Recovery control hidden while TP is ON");
        check(e.tpRecoveryButton.getButtonText()=="AUTO","TP Recovery default label");

        e.tpRecoveryButton.onClick(); sync(p,e);
        check(p.getTpRecoveryMode()==qqsc::params::tpTight && e.tpRecoveryButton.getButtonText()=="TIGHT","AUTO -> TIGHT cycle");
        e.tpRecoveryButton.onClick(); sync(p,e);
        check(p.getTpRecoveryMode()==qqsc::params::tpSmooth && e.tpRecoveryButton.getButtonText()=="SMOOTH","TIGHT -> SMOOTH cycle");
        e.tpRecoveryButton.onClick(); sync(p,e);
        check(p.getTpRecoveryMode()==qqsc::params::tpAuto && e.tpRecoveryButton.getButtonText()=="AUTO","SMOOTH -> AUTO cycle");

        // TP OFF keeps the selected Recovery but hides the inactive control.
        e.truePeakButton.onClick(); sync(p,e);
        check(!p.isTruePeakSelected() && !e.tpRecoveryButton.isVisible(),"Recovery must hide when TP is OFF");
        set(p,qqsc::params::tpRecoveryMode,float(qqsc::params::tpSmooth));
        e.truePeakButton.onClick(); sync(p,e);
        check(p.isTruePeakSelected() && p.getTpRecoveryMode()==qqsc::params::tpSmooth && e.tpRecoveryButton.isVisible(),"Recovery selection lost across TP OFF/ON");

        // Project existing history without adding a new point. Recovery must be
        // retrospective just like the other display parameters.
        QQSCVisualCheck::seedTpRecoveryHistory(e.display);
        set(p,"limiterMix",0.0f); set(p,"limiterInputGainDb",0.0f); set(p,"limiterOutputDb",0.0f); set(p,"ceilingDb",0.0f);
        set(p,"truePeakLimiting",1.0f);
        set(p,qqsc::params::tpRecoveryMode,float(qqsc::params::tpTight));
        const auto tight=QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        set(p,qqsc::params::tpRecoveryMode,float(qqsc::params::tpAuto));
        const auto automatic=QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        set(p,qqsc::params::tpRecoveryMode,float(qqsc::params::tpSmooth));
        const auto smooth=QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        check(tight.blue.size()==4 && automatic.blue.size()==4 && smooth.blue.size()==4,"TP Recovery display history size");
        check(tight.blue[1]>automatic.blue[1]+.05f && automatic.blue[1]>smooth.blue[1]+.05f,
              "Retrospective blue Recovery ordering must be TIGHT > AUTO > SMOOTH after a TP hit");
        // Recovery changes only the TP contribution. Base dynamics evidence is
        // untouched, and orange may differ because it also includes Makeup/Output.

        // A/B sound banks own Recovery independently.
        set(p,qqsc::params::tpRecoveryMode,float(qqsc::params::tpTight));
        p.selectABSlot(1);
        set(p,qqsc::params::tpRecoveryMode,float(qqsc::params::tpSmooth));
        p.selectABSlot(0);
        check(p.getTpRecoveryMode()==qqsc::params::tpTight,"A slot lost TP Recovery");
        p.selectABSlot(1);
        check(p.getTpRecoveryMode()==qqsc::params::tpSmooth,"B slot lost TP Recovery");

        juce::MemoryBlock saved; p.getStateInformation(saved);
        QQSuperCompressionAudioProcessor q; q.setStateInformation(saved.getData(),int(saved.getSize()));
        check(q.getTpRecoveryMode()==qqsc::params::tpSmooth,"Saved state lost TP Recovery");

        std::cout<<"PASS: TP Recovery TIGHT/AUTO/SMOOTH UI, state/A-B memory and retrospective Display ordering.\n";
    }
