#include "DynamicDisplay.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"

namespace
{
constexpr float minDb = -90.0f;
constexpr float maxDb = 0.0f;
constexpr float silenceDb = -120.0f;

float dbToDetectorLevel (float db) noexcept
{
    return db <= silenceDb + 0.1f ? 0.0f
                                  : juce::Decibels::decibelsToGain (juce::jmin (0.0f, db));
}

float readParameter (QQSuperCompressionAudioProcessor& processor, const char* parameterID,
                     float fallback = 0.0f) noexcept
{
    if (auto* value = processor.getAPVTS().getRawParameterValue (parameterID))
        return value->load();

    return fallback;
}

juce::String grText (float db)
{
    return juce::String (juce::jmax (0.0f, db), 1) + " dB";
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
        h.points.clear();
}

void DynamicDisplay::timerCallback()
{
    auto& m = processor.getMeterState();
    const auto mode = m.processingMode.load (std::memory_order_relaxed);
    const auto keySource = juce::jlimit (
        static_cast<int> (qqsc::params::keyInternal),
        static_cast<int> (qqsc::params::keyExternal),
        juce::roundToInt (readParameter (processor, qqsc::params::keySource)));
    const auto capturedInputGainDb = readParameter (processor, qqsc::params::inputGainDb);

    if (mode != lastMode || keySource != lastKeySource)
    {
        clearHistories();
        lastMode = mode;
        lastKeySource = keySource;
    }

    pushHistory (histories[0],
                 { m.displayInputDb0.load (std::memory_order_relaxed),
                   m.displayDetectorDb0.load (std::memory_order_relaxed),
                   capturedInputGainDb });

    if (mode != qqsc::params::stereoLinked)
    {
        pushHistory (histories[1],
                     { m.displayInputDb1.load (std::memory_order_relaxed),
                       m.displayDetectorDb1.load (std::memory_order_relaxed),
                       capturedInputGainDb });
    }

    repaint();
}

void DynamicDisplay::pushHistory (HistorySet& history, HistoryPoint point)
{
    point.inputDb = juce::jlimit (silenceDb, 24.0f, point.inputDb);
    point.detectorDb = juce::jlimit (silenceDb, maxDb, point.detectorDb);
    history.points.push_back (point);

    while (static_cast<int> (history.points.size()) > historyLength)
        history.points.pop_front();
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

juce::Path DynamicDisplay::makeBandPath (const std::deque<float>& upper,
                                         const std::deque<float>& lower,
                                         juce::Rectangle<float> plot) const
{
    juce::Path path;
    if (upper.empty() || upper.size() != lower.size())
        return path;

    const auto denom = static_cast<float> (juce::jmax (1, historyLength - 1));
    const auto startOffset = historyLength - static_cast<int> (upper.size());

    for (size_t i = 0; i < upper.size(); ++i)
    {
        const auto xIndex = static_cast<float> (startOffset + static_cast<int> (i));
        const auto x = plot.getX() + plot.getWidth() * xIndex / denom;
        const auto y = dbToY (upper[i], plot);

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    for (size_t i = lower.size(); i-- > 0;)
    {
        const auto xIndex = static_cast<float> (startOffset + static_cast<int> (i));
        const auto x = plot.getX() + plot.getWidth() * xIndex / denom;
        path.lineTo (x, dbToY (lower[i], plot));
    }

    path.closeSubPath();
    return path;
}

float DynamicDisplay::thresholdDbForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::thresholdDb;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::thresholdLDb : qqsc::params::thresholdRDb;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::thresholdMDb : qqsc::params::thresholdSDb;

    return readParameter (processor, parameterID, qqsc::params::thresholdOffDb);
}

float DynamicDisplay::ratioForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::ratio;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::ratioL : qqsc::params::ratioR;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::ratioM : qqsc::params::ratioS;

    return readParameter (processor, parameterID, 1.0f);
}

float DynamicDisplay::makeupDbForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::makeupGainDb;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::makeupGainLDb : qqsc::params::makeupGainRDb;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::makeupGainMDb : qqsc::params::makeupGainSDb;

    return readParameter (processor, parameterID);
}

float DynamicDisplay::mixForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::mix;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::mixL : qqsc::params::mixR;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::mixM : qqsc::params::mixS;

    return juce::jlimit (0.0f, 1.0f, readParameter (processor, parameterID, 100.0f) * 0.01f);
}

DynamicDisplay::ProjectedHistory DynamicDisplay::projectHistory (int domainIndex, int mode,
                                                                 bool externalKey,
                                                                 bool bypassed) const
{
    ProjectedHistory projected;
    const auto inputGainDb = readParameter (processor, qqsc::params::inputGainDb);
    const auto inputGain = juce::Decibels::decibelsToGain (inputGainDb);
    const auto ratio = ratioForDomain (domainIndex, mode);
    const auto thresholdLinear = qqsc::params::thresholdLinear (thresholdDbForDomain (domainIndex, mode));
    const auto wetMix = mixForDomain (domainIndex, mode);
    const auto makeupGain = juce::Decibels::decibelsToGain (makeupDbForDomain (domainIndex, mode));
    const auto outputGain = juce::Decibels::decibelsToGain (
        readParameter (processor, qqsc::params::outputGainDb));

    for (const auto& point : histories[static_cast<size_t> (domainIndex)].points)
    {
        auto detectorDb = point.detectorDb;

        // INT detector history was captured post-Input-Gain. Refer it back to
        // the capture-time setting, then apply the current setting so the whole
        // visible history follows Input edits. EXT is independent of Input.
        if (! externalKey)
            detectorDb += inputGainDb - point.capturedInputGainDb;

        const auto compressedGain = qqsc::StaticCompressionEngine::gainForLevel (
            dbToDetectorLevel (detectorDb), ratio, thresholdLinear);
        const auto effectiveGr = bypassed ? 0.0f
                                          : qqsc::StaticCompressionEngine::effectiveGainReductionDb (
                                                compressedGain, wetMix);

        auto projectedOutputDb = point.inputDb;
        if (! bypassed)
        {
            // Makeup affects only the Wet leg, exactly as in processBlock.
            // It changes Output, but it is intentionally not part of GR.
            const auto mixedGain = 1.0f + (compressedGain * makeupGain - 1.0f) * wetMix;
            const auto totalGain = inputGain * juce::jmax (0.0f, mixedGain) * outputGain;
            projectedOutputDb += juce::Decibels::gainToDecibels (
                juce::jmax (totalGain, 1.0e-9f), -180.0f);
        }

        projected.input.push_back (point.inputDb);
        projected.gainReductionBoundary.push_back (point.inputDb - effectiveGr);
        projected.output.push_back (projectedOutputDb);
        projected.externalKey.push_back (detectorDb);
        projected.effectiveGainReduction.push_back (effectiveGr);
    }

    return projected;
}

void DynamicDisplay::drawDomainPanel (juce::Graphics& g, juce::Rectangle<float> panel, int domainIndex,
                                      const juce::String& domainName, int mode)
{
    const auto keySource = juce::roundToInt (readParameter (processor, qqsc::params::keySource));
    const bool externalKey = keySource == qqsc::params::keyExternal;
    const bool externalAvailable = processor.isExternalSidechainBusAvailable();
    const bool bypassed = readParameter (processor, qqsc::params::bypass) >= 0.5f;
    const auto projected = projectHistory (domainIndex, mode, externalKey, bypassed);
    const auto currentGr = projected.effectiveGainReduction.empty()
        ? 0.0f : projected.effectiveGainReduction.back();

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

    auto readout = "GR (MIX)  " + grText (currentGr);
    if (externalKey && ! externalAvailable)
        readout += "   EXT N/A";

    g.setColour (qqsc::ui::grAccent());
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawFittedText (readout, readoutArea.toNearestInt(),
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
        const auto inputGainDb = readParameter (processor, qqsc::params::inputGainDb);
        const auto effectiveThresholdDb = externalKey
            ? thresholdDb : qqsc::params::effectiveDisplayThresholdDb (thresholdDb, inputGainDb);
        const auto thresholdY = dbToY (effectiveThresholdDb, plot);
        const float dashPattern[] { 5.0f, 4.0f };
        g.setColour (qqsc::ui::warmAccent().withAlpha (0.82f));
        g.drawDashedLine ({ plot.getX(), thresholdY, plot.getRight(), thresholdY }, dashPattern, 2, 1.2f);

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

    if (externalKey && externalAvailable)
    {
        const auto keyPath = makePath (projected.externalKey, plot);
        g.setColour (qqsc::ui::cyanAccent().withAlpha (0.10f));
        g.strokePath (keyPath, juce::PathStrokeType (4.0f));
        g.setColour (qqsc::ui::cyanAccent().withAlpha (0.34f));
        g.strokePath (keyPath, juce::PathStrokeType (1.0f));
    }

    g.setColour (qqsc::ui::grAccent().withAlpha (0.14f));
    g.fillPath (makeBandPath (projected.input, projected.gainReductionBoundary, plot));

    g.setColour (qqsc::ui::dryTrace().withAlpha (0.82f));
    g.strokePath (makePath (projected.input, plot), juce::PathStrokeType (1.15f));

    g.setColour (qqsc::ui::grAccent().withAlpha (0.90f));
    g.strokePath (makePath (projected.gainReductionBoundary, plot), juce::PathStrokeType (1.25f));

    g.setColour (qqsc::ui::outputAccent().withAlpha (0.96f));
    g.strokePath (makePath (projected.output, plot), juce::PathStrokeType (1.5f));
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
    auto titleArea = header.removeFromLeft (juce::jmin (290.0f, header.getWidth() * 0.62f));
    g.setColour (qqsc::ui::text().withAlpha (0.84f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText ("DYNAMIC LEVEL / GAIN REDUCTION HISTORY", titleArea.toNearestInt(),
                      juce::Justification::centredLeft, 1);
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

    const bool externalKey = juce::roundToInt (
        readParameter (processor, qqsc::params::keySource)) == qqsc::params::keyExternal;
    auto legendArea = juce::Rectangle<int> (54, getHeight() - 22, juce::jmax (1, getWidth() - 64), 17);
    const int itemCount = externalKey ? 4 : 3;
    const auto itemW = legendArea.getWidth() / itemCount;

    auto drawLegend = [&] (juce::Rectangle<int> item, juce::Colour colour, const juce::String& text)
    {
        const int lineY = item.getCentreY();
        g.setColour (colour);
        g.drawLine (static_cast<float> (item.getX() + 4), static_cast<float> (lineY),
                    static_cast<float> (item.getX() + 18), static_cast<float> (lineY), 1.8f);
        g.setFont (8.2f);
        g.drawFittedText (text, item.withTrimmedLeft (23), juce::Justification::centredLeft, 1);
    };

    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::dryTrace(), "Dry / Input");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::grAccent(), "GR incl. Mix");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::outputAccent(), "Output post-mix");

    if (externalKey)
        drawLegend (legendArea, qqsc::ui::cyanAccent().withAlpha (0.45f), "External key");
}
