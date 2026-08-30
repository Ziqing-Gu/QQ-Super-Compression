#include "DynamicDisplay.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"

namespace
{
constexpr float minDb = -90.0f;
constexpr float maxDb = 0.0f;

juce::String dbText (float db)
{
    return db <= -119.9f ? juce::String ("-inf") : juce::String (db, 1) + " dB";
}
}

DynamicDisplay::DynamicDisplay (QQSuperCompressionAudioProcessor& p)
    : processor (p)
{
    startTimerHz (30);
}

void DynamicDisplay::clearHistories()
{
    for (auto& h : histories)
    {
        h.input.clear();
        h.wet.clear();
        h.output.clear();
    }
}

void DynamicDisplay::timerCallback()
{
    auto& m = processor.getMeterState();
    const auto mode = m.processingMode.load (std::memory_order_relaxed);

    if (mode != lastMode)
    {
        clearHistories();
        lastMode = mode;
    }

    pushHistory (histories[0].input,  m.displayInputDb0.load (std::memory_order_relaxed));
    pushHistory (histories[0].wet,    m.displayWetDb0.load (std::memory_order_relaxed));
    pushHistory (histories[0].output, m.displayOutputDb0.load (std::memory_order_relaxed));

    if (mode != qqsc::params::stereoLinked)
    {
        pushHistory (histories[1].input,  m.displayInputDb1.load (std::memory_order_relaxed));
        pushHistory (histories[1].wet,    m.displayWetDb1.load (std::memory_order_relaxed));
        pushHistory (histories[1].output, m.displayOutputDb1.load (std::memory_order_relaxed));
    }

    repaint();
}

void DynamicDisplay::pushHistory (std::deque<float>& history, float value)
{
    history.push_back (juce::jlimit (minDb, maxDb, value));
    while (static_cast<int> (history.size()) > historyLength)
        history.pop_front();
}

float DynamicDisplay::dbToY (float db, juce::Rectangle<float> plot) const noexcept
{
    return juce::jmap (juce::jlimit (minDb, maxDb, db), maxDb, minDb, plot.getY(), plot.getBottom());
}

juce::Path DynamicDisplay::makePath (const std::deque<float>& values, juce::Rectangle<float> plot) const
{
    juce::Path path;
    if (values.empty())
        return path;

    const auto denom = static_cast<float> (juce::jmax (1, historyLength - 1));
    const auto startOffset = historyLength - static_cast<int> (values.size());

    bool first = true;
    int i = 0;
    for (const auto db : values)
    {
        const auto xIndex = static_cast<float> (startOffset + i);
        const auto x = plot.getX() + plot.getWidth() * xIndex / denom;
        const auto y = dbToY (db, plot);
        if (first)
        {
            path.startNewSubPath (x, y);
            first = false;
        }
        else
        {
            path.lineTo (x, y);
        }
        ++i;
    }

    return path;
}

float DynamicDisplay::thresholdDbForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::thresholdDb;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::thresholdLDb : qqsc::params::thresholdRDb;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::thresholdMDb : qqsc::params::thresholdSDb;

    if (auto* value = processor.getAPVTS().getRawParameterValue (parameterID))
        return value->load();

    return qqsc::params::thresholdOffDb;
}

void DynamicDisplay::drawDomainPanel (juce::Graphics& g, juce::Rectangle<float> panel, int domainIndex,
                                      const juce::String& domainName, int mode)
{
    g.setColour (qqsc::ui::panelAlt().withAlpha (0.34f));
    g.fillRoundedRectangle (panel, 7.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.45f));
    g.drawRoundedRectangle (panel.reduced (0.5f), 7.0f, 0.8f);

    auto header = panel.reduced (8.0f, 4.0f).removeFromTop (19.0f);
    auto domainArea = header.removeFromLeft (42.0f);
    auto readoutArea = header;

    g.setColour (qqsc::ui::text().withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText (domainName, domainArea.toNearestInt(), juce::Justification::centredLeft, 1);

    auto& m = processor.getMeterState();
    const auto wetDb = domainIndex == 0 ? m.displayWetDb0.load (std::memory_order_relaxed)
                                        : m.displayWetDb1.load (std::memory_order_relaxed);
    g.setColour (qqsc::ui::cyanAccent());
    g.setFont (9.5f);
    g.drawFittedText ("WET PRE-MAKEUP  " + dbText (wetDb), readoutArea.toNearestInt(),
                      juce::Justification::centredRight, 1);

    auto plot = panel.reduced (8.0f);
    plot.removeFromTop (22.0f);
    plot.removeFromBottom (3.0f);
    plot.removeFromLeft (44.0f);
    plot.removeFromRight (4.0f);

    for (float db : { 0.0f, -15.0f, -30.0f, -45.0f, -60.0f, -75.0f, -90.0f })
    {
        const auto y = dbToY (db, plot);
        g.setColour (qqsc::ui::text().withAlpha (0.065f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (qqsc::ui::textMuted().withAlpha (0.64f));
        g.setFont (8.0f);
        g.drawText (juce::String (static_cast<int> (db)),
                    juce::roundToInt (panel.getX() + 4.0f), juce::roundToInt (y - 6.0f), 37, 12,
                    juce::Justification::right);
    }

    const auto thresholdDb = thresholdDbForDomain (domainIndex, mode);
    if (qqsc::params::isThresholdEnabled (thresholdDb))
    {
        const auto inputGainDb = processor.getAPVTS().getRawParameterValue (qqsc::params::inputGainDb)->load();
        const auto effectiveThresholdDb = qqsc::params::effectiveDisplayThresholdDb (thresholdDb, inputGainDb);
        const auto thresholdY = dbToY (effectiveThresholdDb, plot);
        const float dashPattern[] { 5.0f, 4.0f };
        g.setColour (qqsc::ui::warmAccent().withAlpha (0.82f));
        g.drawDashedLine ({ plot.getX(), thresholdY, plot.getRight(), thresholdY }, dashPattern, 2, 1.2f);

        // v1.0.1: Threshold is a working visual reference, not just decoration.
        // Show the actual parameter value directly on its line so users can find
        // a useful boundary while watching the dynamic history without looking
        // away at the lower control row.
        const auto thresholdText = juce::String (thresholdDb, 2) + " dB";
        const auto tagWidth = 70.0f;
        auto tag = juce::Rectangle<float> (plot.getRight() - tagWidth - 2.0f,
                                            thresholdY - 8.0f, tagWidth, 16.0f);
        tag.setY (juce::jlimit (plot.getY(), plot.getBottom() - tag.getHeight(), tag.getY()));
        g.setColour (qqsc::ui::panel().withAlpha (0.90f));
        g.fillRoundedRectangle (tag, 4.0f);
        g.setColour (qqsc::ui::warmAccent().darker (0.12f));
        g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.drawFittedText (thresholdText, tag.toNearestInt(), juce::Justification::centred, 1);
    }

    g.saveState();
    g.reduceClipRegion (plot.toNearestInt());
    g.setColour (qqsc::ui::dryTrace().withAlpha (0.82f));
    g.strokePath (makePath (histories[static_cast<size_t> (domainIndex)].input, plot), juce::PathStrokeType (1.15f));
    g.setColour (qqsc::ui::cyanAccent());
    g.strokePath (makePath (histories[static_cast<size_t> (domainIndex)].wet, plot), juce::PathStrokeType (1.8f));
    g.setColour (qqsc::ui::outputAccent().withAlpha (0.94f));
    g.strokePath (makePath (histories[static_cast<size_t> (domainIndex)].output, plot), juce::PathStrokeType (1.35f));
    g.restoreState();
}

void DynamicDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (qqsc::ui::panel().withAlpha (0.97f));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.72f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    auto& m = processor.getMeterState();
    const auto mode = m.processingMode.load (std::memory_order_relaxed);

    auto header = bounds.reduced (14.0f, 5.0f).removeFromTop (22.0f);
    auto titleArea = header.removeFromLeft (juce::jmin (210.0f, header.getWidth() * 0.50f));
    g.setColour (qqsc::ui::text().withAlpha (0.84f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText ("DYNAMIC LEVEL HISTORY", titleArea.toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour (qqsc::ui::textMuted().withAlpha (0.82f));
    g.setFont (9.5f);
    g.drawFittedText ("MODE  " + qqsc::params::modeName (mode), header.toNearestInt(),
                      juce::Justification::centredRight, 1);

    auto content = bounds.reduced (10.0f, 6.0f);
    content.removeFromTop (24.0f);
    content.removeFromBottom (23.0f);

    if (mode == qqsc::params::stereoLinked)
    {
        drawDomainPanel (g, content, 0, "ST", mode);
    }
    else
    {
        const auto gap = 6.0f;
        auto top = content.removeFromTop ((content.getHeight() - gap) * 0.5f);
        content.removeFromTop (gap);
        auto bottom = content;

        if (mode == qqsc::params::leftRight)
        {
            drawDomainPanel (g, top, 0, "L", mode);
            drawDomainPanel (g, bottom, 1, "R", mode);
        }
        else
        {
            drawDomainPanel (g, top, 0, "M", mode);
            drawDomainPanel (g, bottom, 1, "S", mode);
        }
    }

    auto legendArea = juce::Rectangle<int> (54, getHeight() - 22, juce::jmax (1, getWidth() - 64), 17);
    const auto itemW = legendArea.getWidth() / 3;
    auto drawLegend = [&] (juce::Rectangle<int> item, juce::Colour colour, const juce::String& text)
    {
        const int lineY = item.getCentreY();
        g.setColour (colour);
        g.drawLine (static_cast<float> (item.getX() + 4), static_cast<float> (lineY),
                    static_cast<float> (item.getX() + 18), static_cast<float> (lineY), 1.8f);
        g.setFont (8.5f);
        g.drawFittedText (text, item.withTrimmedLeft (23), juce::Justification::centredLeft, 1);
    };

    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::dryTrace(), "Dry / Input");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::cyanAccent(), "Wet pre-makeup");
    drawLegend (legendArea, qqsc::ui::outputAccent(), "Output post-mix");
}
