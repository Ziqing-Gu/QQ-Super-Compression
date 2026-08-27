#include "LevelMeters.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"

void LevelMeters::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (qqsc::ui::panel().withAlpha (0.97f));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.72f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    auto inner = bounds.reduced (6.0f);
    const float gap = 4.0f;
    const float groupWidth = (inner.getWidth() - gap * 2.0f) / 3.0f;

    auto& m = processor.getMeterState();
    const auto mode = m.processingMode.load (std::memory_order_relaxed);
    const bool ms = mode == qqsc::params::midSide;
    const juce::String ch0 = ms ? "M" : "L";
    const juce::String ch1 = ms ? "S" : "R";

    auto inputArea = inner.removeFromLeft (groupWidth);
    inner.removeFromLeft (gap);
    auto outputArea = inner.removeFromLeft (groupWidth);
    inner.removeFromLeft (gap);
    auto grArea = inner;

    drawDualMeter (g, inputArea, "INPUT", ch0, ch1,
                   m.inputDb0.load (std::memory_order_relaxed),
                   m.inputDb1.load (std::memory_order_relaxed),
                   -60.0f, 3.0f, false, 0.0f, 0.0f, qqsc::ui::dryTrace());

    drawDualMeter (g, outputArea, "OUTPUT", ch0, ch1,
                   m.outputDb0.load (std::memory_order_relaxed),
                   m.outputDb1.load (std::memory_order_relaxed),
                   -60.0f, 3.0f, false, 0.0f, 0.0f, qqsc::ui::outputAccent());

    drawDualMeter (g, grArea, "GAIN RED.", ch0, ch1,
                   m.gainReductionDb0.load (std::memory_order_relaxed),
                   m.gainReductionDb1.load (std::memory_order_relaxed),
                   0.0f, 36.0f, true,
                   m.gainReductionHoldDb0.load (std::memory_order_relaxed),
                   m.gainReductionHoldDb1.load (std::memory_order_relaxed),
                   qqsc::ui::grAccent());
}

void LevelMeters::drawDualMeter (juce::Graphics& g,
                                 juce::Rectangle<float> area,
                                 const juce::String& title,
                                 const juce::String& channel0,
                                 const juce::String& channel1,
                                 float value0Db,
                                 float value1Db,
                                 float minDb,
                                 float maxDb,
                                 bool reductionMeter,
                                 float hold0Db,
                                 float hold1Db,
                                 juce::Colour barColour)
{
    auto titleArea = area.removeFromTop (24.0f);
    g.setColour (qqsc::ui::textMuted().withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawFittedText (title, titleArea.toNearestInt(), juce::Justification::centred, 1);

    const float pairGap = 2.5f;
    const float barW = (area.getWidth() - pairGap) * 0.5f;
    auto left = area.removeFromLeft (barW);
    area.removeFromLeft (pairGap);
    auto right = area;

    drawSingleBar (g, left, channel0, value0Db, minDb, maxDb, reductionMeter, hold0Db, barColour);
    drawSingleBar (g, right, channel1, value1Db, minDb, maxDb, reductionMeter, hold1Db, barColour);
}

void LevelMeters::drawSingleBar (juce::Graphics& g,
                                 juce::Rectangle<float> area,
                                 const juce::String& channelName,
                                 float valueDb,
                                 float minDb,
                                 float maxDb,
                                 bool reductionMeter,
                                 float holdDb,
                                 juce::Colour barColour)
{
    auto channelArea = area.removeFromTop (18.0f);
    auto valueArea = area.removeFromBottom (32.0f);
    auto barArea = area.reduced (2.0f, 3.0f);

    g.setColour (qqsc::ui::text().withAlpha (0.82f));
    g.setFont (juce::Font (juce::FontOptions (9.4f, juce::Font::bold)));
    g.drawText (channelName, channelArea.toNearestInt(), juce::Justification::centred);

    g.setColour (qqsc::ui::panelAlt().withAlpha (0.88f));
    g.fillRoundedRectangle (barArea, 3.0f);

    const auto proportion = juce::jlimit (0.0f, 1.0f, (valueDb - minDb) / (maxDb - minDb));
    auto fill = barArea;
    const auto fillHeight = fill.getHeight() * proportion;

    if (reductionMeter)
    {
        // 0 dB GR lives at the TOP. Increasing Gain Reduction grows DOWNWARD.
        fill.setY (barArea.getY());
        fill.setHeight (fillHeight);
    }
    else
    {
        fill.setY (barArea.getBottom() - fillHeight);
        fill.setHeight (fillHeight);
    }

    if (fill.getHeight() > 0.5f)
    {
        g.setColour (barColour.withAlpha (0.11f));
        g.fillRoundedRectangle (fill.expanded (1.5f, 0.0f), 4.0f);
        g.setColour (barColour.withAlpha (0.90f));
        g.fillRoundedRectangle (fill, 3.0f);
    }

    g.setColour (qqsc::ui::border().withAlpha (0.78f));
    g.drawRoundedRectangle (barArea, 3.0f, 1.0f);

    if (reductionMeter && holdDb > 0.05f)
    {
        const auto holdProportion = juce::jlimit (0.0f, 1.0f, (holdDb - minDb) / (maxDb - minDb));
        const auto holdY = barArea.getY() + barArea.getHeight() * holdProportion;
        g.setColour (qqsc::ui::text().withAlpha (0.88f));
        g.drawLine (barArea.getX() + 1.0f, holdY, barArea.getRight() - 1.0f, holdY, 1.5f);
    }

    if (reductionMeter)
    {
        auto currentArea = valueArea.removeFromTop (17.0f);
        g.setColour (qqsc::ui::text().withAlpha (0.82f));
        g.setFont (8.7f);
        g.drawFittedText (juce::String (juce::jmax (0.0f, valueDb), 1) + " dB",
                          currentArea.toNearestInt(), juce::Justification::centred, 1);

        g.setColour (qqsc::ui::textMuted().withAlpha (0.66f));
        g.setFont (8.5f);
        const auto holdText = juce::String (juce::jmax (0.0f, holdDb), 1);
        g.drawFittedText (holdText, valueArea.toNearestInt(), juce::Justification::centredTop, 1);
    }
    else
    {
        g.setColour (qqsc::ui::text().withAlpha (0.82f));
        g.setFont (8.7f);
        const auto text = valueDb <= -119.9f ? juce::String ("-inf") : juce::String (valueDb, 1) + " dB";
        g.drawFittedText (text, valueArea.toNearestInt(), juce::Justification::centred, 1);
    }
}
