#pragma once
#include <JuceHeader.h>

namespace qqsc
{
// Right click opens diagnostics without changing this instance's FULL/ECO mode.
class PerformanceModeButton final : public juce::TextButton
{
public:
    PerformanceModeButton() : juce::TextButton("FULL") {}
    std::function<void()> onDiagnostics;
    void mouseDown(const juce::MouseEvent& event) override
    {
        contextGesture=event.mods.isPopupMenu();
        if(contextGesture)
        {
            if(onDiagnostics) onDiagnostics();
            return;
        }
        juce::TextButton::mouseDown(event);
    }
    void mouseUp(const juce::MouseEvent& event) override
    {
        if(contextGesture){contextGesture=false;return;}
        juce::TextButton::mouseUp(event);
    }
private:
    bool contextGesture=false;
};

// Editor-owned child, not a native/modal window. A frozen, selectable snapshot
// remains readable and can be copied even while audio processing continues.
class PerformanceDiagnosticsPanel final : public juce::Component
{
public:
    std::function<juce::String()> readSnapshot;
    PerformanceDiagnosticsPanel()
    {
        setLookAndFeel(&panelLookAndFeel);
        setOpaque(true);
        title.setText("Performance / Idle diagnostics",juce::dontSendNotification);
        title.setColour(juce::Label::textColourId,juce::Colour(0xffedf1f3));
        title.setFont(juce::Font(juce::FontOptions(19.f)));
        addAndMakeVisible(title);
        text.setMultiLine(true,true);text.setReadOnly(true);text.setScrollbarsShown(true);
        text.setPopupMenuEnabled(true);text.setCaretVisible(false);
        text.setFont(juce::Font(juce::FontOptions(15.f)));
        text.setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff161a1c));
        text.setColour(juce::TextEditor::textColourId,juce::Colour(0xffedf1f3));
        text.setColour(juce::TextEditor::outlineColourId,juce::Colour(0xff586166));
        addAndMakeVisible(text);
        for(auto* button:{&copy,&refresh,&close})
        {
            button->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff30383c));
            button->setColour(juce::TextButton::textColourOffId,juce::Colour(0xffedf1f3));
            addAndMakeVisible(button);
        }
        copy.onClick=[this]{juce::SystemClipboard::copyTextToClipboard(text.getText());copy.setButtonText("Copied");};
        refresh.onClick=[this]{refreshSnapshot();};
        close.onClick=[this]{setVisible(false);};
    }
    ~PerformanceDiagnosticsPanel() override {setLookAndFeel(nullptr);}
    void refreshSnapshot()
    {
        if(readSnapshot)text.setText(readSnapshot(),false);
        text.moveCaretToTop(false);copy.setButtonText("Copy report");
    }
    juce::String snapshot() const {return text.getText();}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff242a2d));
        g.setColour(juce::Colour(0xff7b919b));g.drawRect(getLocalBounds(),1);
    }
    void resized() override
    {
        auto area=getLocalBounds().reduced(18);
        auto header=area.removeFromTop(34);close.setBounds(header.removeFromRight(86));title.setBounds(header);
        area.removeFromTop(12);auto buttons=area.removeFromBottom(32);
        copy.setBounds(buttons.removeFromLeft(140));buttons.removeFromLeft(12);refresh.setBounds(buttons.removeFromLeft(100));
        area.removeFromBottom(12);text.setBounds(area);
    }
private:
    juce::LookAndFeel_V4 panelLookAndFeel;
    juce::Label title;
    juce::TextEditor text;
    juce::TextButton copy{"Copy report"},refresh{"Refresh"},close{"Close"};
};
}
