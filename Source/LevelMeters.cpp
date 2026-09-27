#include "LevelMeters.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"

namespace
{
juce::Colour gainIncreaseColour() noexcept
{
    if (qqsc::ui::isDarkTheme())
        return juce::Colour (0xff80c897);
    return qqsc::ui::isClassicTheme() ? juce::Colour (0xff80d59c)
                                      : juce::Colour (0xff2c8452);
}

juce::String signedGainText (float reductionDb)
{
    const auto gainDb = std::abs (reductionDb) < 0.05f ? 0.0f : -reductionDb;
    return (gainDb > 0.0f ? "+" : "") + juce::String (gainDb, 1);
}
}

juce::Rectangle<float> LevelMeters::truePeakBounds() const noexcept
{
    auto inner=getLocalBounds().toFloat().reduced(6.0f);
    const auto width=(inner.getWidth()-8.0f)/3.0f;
    inner.removeFromLeft(width+4.0f);
    return inner.removeFromLeft(width).removeFromTop(28.0f);
}

void LevelMeters::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (truePeakBounds().contains(event.position))
    {
        processor.resetTruePeakHold();
        repaint();
    }
}

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

    const auto tp=m.truePeakHoldDb.load(std::memory_order_relaxed);
    auto tpArea=outputArea.removeFromTop(28.0f);
    inputArea.removeFromTop(28.0f);grArea.removeFromTop(28.0f);
    g.setColour(tp>0 ? juce::Colours::orangered : qqsc::ui::outputAccent());
    g.setFont(juce::Font(juce::FontOptions(8.5f,juce::Font::bold)));
    g.drawFittedText("TP L/R",tpArea.removeFromTop(12).toNearestInt(),juce::Justification::centred,1);
    g.setFont(juce::Font(juce::FontOptions(8.5f)));
    g.drawFittedText((tp<=-119.9f ? juce::String("-inf") : juce::String(tp,2))+" dBTP",tpArea.toNearestInt(),juce::Justification::centred,1);

    drawDualMeter (g, inputArea, "INPUT", ch0, ch1,
                   m.inputDb0.load (std::memory_order_relaxed),
                   m.inputDb1.load (std::memory_order_relaxed),
                   -60.0f, 3.0f, false, 0.0f, 0.0f, qqsc::ui::dryTrace());

    drawDualMeter (g, outputArea, "OUTPUT", ch0, ch1,
                   m.outputDb0.load (std::memory_order_relaxed),
                   m.outputDb1.load (std::memory_order_relaxed),
                   -60.0f, 3.0f, false, 0.0f, 0.0f, qqsc::ui::outputAccent());

    drawDualMeter (g, grArea, "GAIN +/-", ch0, ch1,
                   m.gainReductionDb0.load (std::memory_order_relaxed),
                   m.gainReductionDb1.load (std::memory_order_relaxed),
                   -36.0f, 36.0f, true,
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

    if (qqsc::ui::isDarkTheme())
    {
        g.setColour (juce::Colour (0xff191c1e));
        g.fillRoundedRectangle (barArea, 2.0f);
        g.setColour (juce::Colour (0xff393e41).withAlpha (0.30f));
        for (float lineY = barArea.getY() + 2.5f; lineY < barArea.getBottom(); lineY += 3.2f)
            g.drawLine (barArea.getX() + 0.8f, lineY, barArea.getRight() - 0.8f, lineY, 0.65f);
    }
    else if (qqsc::ui::isClassicTheme())
    {
        g.setColour (qqsc::ui::panelAlt().withAlpha (0.88f));
        g.fillRoundedRectangle (barArea, 3.0f);
    }
    else
    {
        // Quiet ivory well and fine unlit ladder, inspired by the accepted
        // concept. Bar bounds, dB mapping and meter timing are unchanged.
        juce::ColourGradient well (juce::Colour (0xffe8e3dc), barArea.getX(), barArea.getY(),
                                  juce::Colour (0xfff8f5ef), barArea.getRight(), barArea.getY(), false);
        well.addColour (0.24, juce::Colour (0xfff2eee7));
        g.setGradientFill (well);
        g.fillRoundedRectangle (barArea, 2.0f);
        g.setColour (juce::Colour (0xffc6beb3).withAlpha (0.45f));
        for (float lineY = barArea.getY() + 2.5f; lineY < barArea.getBottom(); lineY += 3.2f)
            g.drawLine (barArea.getX() + 0.8f, lineY, barArea.getRight() - 0.8f, lineY, 0.5f);
    }

    auto fill = barArea;
    const auto zeroY = barArea.getCentreY();
    const bool growsDownward = reductionMeter && valueDb >= 0.0f;

    if (reductionMeter)
    {
        // Processor GR stays positive for reduction. The visual is bipolar:
        // boost rises above unity; reduction falls below unity.
        const auto valueY = juce::jmap (juce::jlimit (minDb, maxDb, valueDb),
                                        minDb, maxDb, barArea.getY(), barArea.getBottom());
        fill.setY (juce::jmin (zeroY, valueY));
        fill.setHeight (std::abs (valueY - zeroY));
        if (valueDb < 0.0f)
            barColour = gainIncreaseColour();
    }
    else
    {
        const auto proportion = juce::jlimit (0.0f, 1.0f, (valueDb - minDb) / (maxDb - minDb));
        const auto fillHeight = fill.getHeight() * proportion;
        fill.setY (barArea.getBottom() - fillHeight);
        fill.setHeight (fillHeight);
    }

    if (fill.getHeight() > 0.5f)
    {
        if (qqsc::ui::isDarkTheme())
        {
            // Keep active silver visibly above the unlit well, with fine
            // segments and tonal depth but no bloom or full-height ghost fill.
            const auto originY = growsDownward ? fill.getY() : fill.getBottom();
            const auto leadingY = growsDownward ? fill.getBottom() : fill.getY();
            const auto pale = barColour.interpolatedWith (juce::Colour (0xffc4d1d2), 0.30f);
            g.setGradientFill (juce::ColourGradient (barColour.darker (0.16f), fill.getCentreX(), originY,
                                                    pale, fill.getCentreX(), leadingY, false));
            g.fillRoundedRectangle (fill, 2.0f);
            g.setColour (juce::Colour (0xff191c1e).withAlpha (0.86f));
            for (float lineY = barArea.getY() + 2.5f; lineY < barArea.getBottom(); lineY += 3.2f)
                if (lineY > fill.getY() && lineY < fill.getBottom())
                    g.drawLine (fill.getX(), lineY, fill.getRight(), lineY, 1.1f);
            g.setColour (pale.withAlpha (0.75f));
            const float edgeY = growsDownward ? fill.getBottom() - 0.5f : fill.getY() + 0.5f;
            g.drawLine (fill.getX() + 0.8f, edgeY, fill.getRight() - 0.8f, edgeY, 0.7f);
        }
        else if (! qqsc::ui::isClassicTheme())
        {
            // Rich colour at the origin, fading toward the moving leading edge.
            const auto originY = growsDownward ? fill.getY() : fill.getBottom();
            const auto leadingY = growsDownward ? fill.getBottom() : fill.getY();
            const auto ivory = juce::Colour (0xfff8f5ef);
            juce::ColourGradient light (barColour.darker (0.12f), fill.getCentreX(), originY,
                                       barColour.interpolatedWith (ivory, 0.82f), fill.getCentreX(), leadingY, false);
            light.addColour (0.32, barColour);
            light.addColour (0.65, barColour.interpolatedWith (ivory, 0.34f));
            light.addColour (0.86, barColour.interpolatedWith (ivory, 0.62f));
            g.setGradientFill (light);
            g.fillRoundedRectangle (fill, 2.0f);
            // Constant spacing: display-only segments, with unchanged dB mapping.
            g.setColour (juce::Colour (0xfffaf7f1).withAlpha (0.79f));
            for (float lineY = barArea.getY() + 2.5f; lineY < barArea.getBottom(); lineY += 3.2f)
                if (lineY > fill.getY() && lineY < fill.getBottom())
                    g.drawLine (fill.getX(), lineY, fill.getRight(), lineY, 0.75f);
            g.setColour (juce::Colours::white.withAlpha (0.48f));
            g.drawLine (fill.getX() + 1.2f, fill.getY(), fill.getX() + 1.2f, fill.getBottom(), 0.65f);
        }
        else
        {
        g.setColour (barColour.withAlpha (0.11f));
        g.fillRoundedRectangle (fill.expanded (1.5f, 0.0f), 4.0f);
        g.setColour (barColour.withAlpha (0.90f));
        g.fillRoundedRectangle (fill, 3.0f);
        }
    }

    g.setColour (qqsc::ui::border().withAlpha (0.78f));
    g.drawRoundedRectangle (barArea, 3.0f, 1.0f);

    if (reductionMeter)
    {
        g.setColour (qqsc::ui::textMuted().withAlpha (0.65f));
        g.drawLine (barArea.getX(), zeroY, barArea.getRight(), zeroY, 1.0f);
        g.setFont (7.2f);
        g.drawText ("0", juce::Rectangle<float> (barArea.getX(), zeroY - 11.0f,
                                                barArea.getWidth(), 10.0f).toNearestInt(),
                    juce::Justification::centred);
    }

    if (reductionMeter && std::abs (holdDb) > 0.05f)
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
        g.drawFittedText (signedGainText (valueDb) + " dB",
                          currentArea.toNearestInt(), juce::Justification::centred, 1);

        g.setColour (qqsc::ui::textMuted().withAlpha (0.66f));
        g.setFont (8.5f);
        const auto holdText = signedGainText (holdDb);
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
