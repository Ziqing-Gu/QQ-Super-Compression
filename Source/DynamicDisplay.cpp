#include "DynamicDisplay.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"

namespace
{
constexpr float minDb = -120.0f;
constexpr float maxDb = 6.0f;

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

void DynamicDisplay::timerCallback()
{
    auto& m = processor.getMeterState();
    pushHistory (inputHistory,  m.inputDb.load (std::memory_order_relaxed));
    pushHistory (wetHistory,    m.wetDb.load (std::memory_order_relaxed));
    pushHistory (outputHistory, m.outputDb.load (std::memory_order_relaxed));
    repaint();
}

void DynamicDisplay::pushHistory (std::deque<float>& history, float value)
{
    history.push_back (juce::jlimit (minDb, maxDb, value));
    while (static_cast<int> (history.size()) > historyLength)
        history.pop_front();
}

juce::Rectangle<float> DynamicDisplay::getPlotArea() const noexcept
{
    auto area = getLocalBounds().toFloat().reduced (12.0f);
    area.removeFromTop (32.0f);    // header row
    area.removeFromBottom (28.0f); // legend row
    area.removeFromLeft (54.0f);   // dB scale gutter
    area.removeFromRight (8.0f);
    return area;
}

float DynamicDisplay::dbToY (float db) const noexcept
{
    const auto area = getPlotArea();
    return juce::jmap (juce::jlimit (minDb, maxDb, db), maxDb, minDb, area.getY(), area.getBottom());
}

juce::Path DynamicDisplay::makePath (const std::deque<float>& values) const
{
    juce::Path path;
    if (values.empty())
        return path;

    const auto area = getPlotArea();
    const auto denom = static_cast<float> (juce::jmax (1, historyLength - 1));
    const auto startOffset = historyLength - static_cast<int> (values.size());

    bool first = true;
    int i = 0;
    for (const auto db : values)
    {
        const auto xIndex = static_cast<float> (startOffset + i);
        const auto x = area.getX() + area.getWidth() * xIndex / denom;
        const auto y = dbToY (db);

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

void DynamicDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (qqsc::ui::panel().withAlpha (0.97f));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.72f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    const auto plot = getPlotArea();
    auto& m = processor.getMeterState();

    // Header is split into reserved columns so changing numeric text cannot
    // collide with the title or with another readout.
    auto header = bounds.reduced (16.0f, 6.0f).removeFromTop (24.0f);
    auto titleArea = header.removeFromLeft (juce::jmin (190.0f, header.getWidth() * 0.34f));
    auto wetArea = header.removeFromLeft (header.getWidth() * 0.56f);
    auto outArea = header;

    g.setColour (qqsc::ui::text().withAlpha (0.84f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawFittedText ("DYNAMIC LEVEL HISTORY", titleArea.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour (qqsc::ui::cyanAccent());
    g.setFont (10.5f);
    g.drawFittedText ("WET PRE-MAKEUP  " + dbText (m.wetDb.load (std::memory_order_relaxed)),
                      wetArea.toNearestInt(), juce::Justification::centred, 1);

    const auto mode = m.processingMode.load (std::memory_order_relaxed);
    g.setColour (qqsc::ui::textMuted().withAlpha (0.82f));
    g.drawFittedText ("MODE  " + qqsc::params::modeName (mode),
                      outArea.toNearestInt(), juce::Justification::centredRight, 1);

    for (float db : { 0.0f, -20.0f, -40.0f, -60.0f, -80.0f, -100.0f, -120.0f })
    {
        const auto y = dbToY (db);
        g.setColour (qqsc::ui::text().withAlpha (0.075f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());

        g.setColour (qqsc::ui::textMuted().withAlpha (0.72f));
        g.setFont (10.0f);
        g.drawText (juce::String (static_cast<int> (db)) + " dB",
                    10, static_cast<int> (y - 7.0f), 48, 14, juce::Justification::right);
    }

    g.saveState();
    g.reduceClipRegion (plot.toNearestInt());

    g.setColour (qqsc::ui::dryTrace().withAlpha (0.82f));
    g.strokePath (makePath (inputHistory), juce::PathStrokeType (1.3f));

    g.setColour (qqsc::ui::cyanAccent());
    g.strokePath (makePath (wetHistory), juce::PathStrokeType (2.0f));

    // Output is the final signal AFTER Makeup and AFTER Dry/Wet Mix.
    g.setColour (qqsc::ui::outputAccent().withAlpha (0.94f));
    g.strokePath (makePath (outputHistory), juce::PathStrokeType (1.5f));
    g.restoreState();

    const auto legendY = getHeight() - 24;
    auto legendArea = juce::Rectangle<int> (62, legendY, juce::jmax (1, getWidth() - 74), 19);
    const auto itemW = legendArea.getWidth() / 3;

    auto drawLegend = [&] (juce::Rectangle<int> item, juce::Colour colour, const juce::String& text)
    {
        const int lineY = item.getCentreY();
        g.setColour (colour);
        g.drawLine (static_cast<float> (item.getX() + 4), static_cast<float> (lineY),
                    static_cast<float> (item.getX() + 22), static_cast<float> (lineY), 2.0f);
        g.setFont (10.0f);
        g.drawFittedText (text, item.withTrimmedLeft (28), juce::Justification::centredLeft, 1);
    };

    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::dryTrace(), "Dry / Input");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::cyanAccent(), "Wet (pre-makeup)");
    drawLegend (legendArea, qqsc::ui::outputAccent(), "Output (post-mix)");
}
