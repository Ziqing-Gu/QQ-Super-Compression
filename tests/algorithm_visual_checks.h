void QQSCVisualCheck::algorithmChecks(QQSuperCompressionAudioProcessorEditor& e,
                                      QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    parameter(p,"algorithmMode",0);parameter(p,"processingMode",0);parameter(p,"compressionMode",0);
    parameter(p,"ratio",5);parameter(p,"makeupGainDb",0);parameter(p,"mix",100);
    p.setBoundaryForDomainDb(false,false,0,-10);p.setBoundaryForDomainDb(false,true,0,1);
    e.timerCallback();p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
    {
        GestureProbe touch(*p.getAPVTS().getParameter("algorithmMode"));e.algorithmButton.onClick();
        if(touch.events!=std::vector<bool>{true,false})throw std::runtime_error("Algorithm host gesture mismatch");
    }
    p.getAPVTS().copyState();
    if(e.algorithmButton.getButtonText()!="ALGO: SUPER")throw std::runtime_error("Algorithm click did not update label");
    p.getUndoManager().undo();e.timerCallback();
    if(e.algorithmButton.getButtonText()!="ALGO: CLASSIC")throw std::runtime_error("Algorithm undo failed");
    p.getUndoManager().redo();e.timerCallback();
    if(e.algorithmButton.getButtonText()!="ALGO: SUPER")throw std::runtime_error("Algorithm redo failed");
    auto& points=e.display.histories[0].points;points.clear();
    DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=0;points.push_back(point);
    for(int algorithm:{0,1})
    {
        parameter(p,"algorithmMode",float(algorithm));e.timerCallback();e.display.refreshRenderCaches(0);
        const auto expected=algorithm==0?-8.0:20*std::log10((1+4*std::pow(10.,-.5))/5);
        if(std::abs(e.display.renderCaches[0].projected.output[0]-expected)>.002)
            throw std::runtime_error("Algorithm Display did not follow automation");
    }
    for(auto skin:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        for(int size:{1008,1200,1800})for(int algorithm:{0,1})
        {
            parameter(p,"algorithmMode",float(algorithm));e.theme=skin;e.applyTheme();e.setSize(size,size*2/3);e.timerCallback();
            if(e.algorithmButton.getRight()+6!=e.sidechainButton.getX()
                ||e.algorithmButton.getBounds().intersects(e.title.getBounds())
                ||e.algorithmButton.getY()!=e.sidechainButton.getY())
                throw std::runtime_error("Algorithm toolbar overlap or alignment");
            const juce::String theme=skin==qqsc::ui::Theme::light?"Light":skin==qqsc::ui::Theme::dark?"Dark":"ClassicSkin";
            snapshot(e,dir.getChildFile("Algorithm-"+theme+"-"+juce::String(size)+(algorithm?"-Super.png":"-Classic.png")));
        }
    std::cout<<"PASS: algorithm toolbar click / automation / undo / redo / host gesture, Display both laws, 3 themes at3 sizes with no overlap.\n";
}
