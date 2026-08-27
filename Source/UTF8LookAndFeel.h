#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace qqsc
{
namespace ui
{
    inline juce::Colour canvas()          { return juce::Colour::fromRGB (247, 243, 237); }
    inline juce::Colour panel()           { return juce::Colour::fromRGB (253, 249, 244); }
    inline juce::Colour panelAlt()        { return juce::Colour::fromRGB (243, 236, 227); }
    inline juce::Colour text()            { return juce::Colour::fromRGB (70, 61, 54); }
    inline juce::Colour textMuted()       { return juce::Colour::fromRGB (112, 100, 91); }
    inline juce::Colour border()          { return juce::Colour::fromRGB (214, 201, 188); }
    inline juce::Colour warmAccent()      { return juce::Colour::fromRGB (241, 139, 82); }
    inline juce::Colour warmAccentSoft()  { return juce::Colour::fromRGB (250, 184, 143); }
    inline juce::Colour cyanAccent()      { return juce::Colour::fromRGB (79, 184, 194); }
    inline juce::Colour dryTrace()        { return juce::Colour::fromRGB (174, 162, 151); }
    inline juce::Colour outputAccent()    { return juce::Colour::fromRGB (255, 119, 72); }
    inline juce::Colour grAccent()        { return juce::Colour::fromRGB (240, 116, 92); }
}

class UTF8LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    UTF8LookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, ui::text());
        setColour (juce::Slider::textBoxBackgroundColourId, ui::panel().withAlpha (0.76f));
        setColour (juce::Slider::textBoxOutlineColourId, ui::border().withAlpha (0.66f));
        setColour (juce::Slider::rotarySliderOutlineColourId, ui::border());
        setColour (juce::Slider::rotarySliderFillColourId, ui::warmAccent());

        setColour (juce::PopupMenu::backgroundColourId, ui::panel());
        setColour (juce::PopupMenu::textColourId, ui::text());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::warmAccentSoft().withAlpha (0.34f));
        setColour (juce::PopupMenu::highlightedTextColourId, ui::text());
    }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        auto f = font;

       #if JUCE_MAC
        // PingFang SC ships with modern macOS and contains Simplified Chinese glyphs.
        f.setTypefaceName ("PingFang SC");
       #elif JUCE_WINDOWS
        // Microsoft YaHei is present on supported modern Windows installations.
        f.setTypefaceName ("Microsoft YaHei");
       #endif

        if (auto face = juce::Typeface::createSystemTypefaceFor (f))
            return face;

        return juce::LookAndFeel_V4::getTypefaceForFont (font);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font (juce::FontOptions (juce::jlimit (10.0f, 13.0f, buttonHeight * 0.38f), juce::Font::plain));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (11.0f));
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto area = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                            static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
        const auto diameter = juce::jmin (area.getWidth(), area.getHeight());
        const auto radius = diameter * 0.43f;
        const auto centre = area.getCentre();
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const auto track = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
        const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        juce::Path fullArc;
        fullArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                               rotaryStartAngle, rotaryEndAngle, true);

        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                rotaryStartAngle, angle, true);

        // 0.9.1 lighting refinement:
        // The 0.9.0 implementation technically drew transparent orange strokes,
        // but on a bright ivory surface they read as coloured outlines rather than
        // as illumination.  The new stack deliberately separates a large soft halo,
        // a medium-energy bloom, and the crisp illuminated arc.
        g.setColour (accent.withAlpha (0.035f));
        g.strokePath (valueArc, juce::PathStrokeType (18.0f));
        g.setColour (accent.withAlpha (0.070f));
        g.strokePath (valueArc, juce::PathStrokeType (12.0f));
        g.setColour (accent.withAlpha (0.125f));
        g.strokePath (valueArc, juce::PathStrokeType (7.5f));
        g.setColour (accent.withAlpha (0.230f));
        g.strokePath (valueArc, juce::PathStrokeType (4.6f));

        g.setColour (track.withAlpha (0.60f));
        g.strokePath (fullArc, juce::PathStrokeType (2.0f));
        g.setColour (accent.withAlpha (0.98f));
        g.strokePath (valueArc, juce::PathStrokeType (2.5f));

        auto knob = juce::Rectangle<float> (diameter * 0.64f, diameter * 0.64f).withCentre (centre);

        // A broad, low-energy pool below the knob makes the light appear to live
        // underneath the control and spill onto the matte panel.  The layers are
        // intentionally shallow and warm: visible in a DAW screenshot, but never
        // dark/neon enough to imply distortion or aggressive saturation.
        const auto poolCentre = juce::Point<float> (centre.x, knob.getBottom() + knob.getHeight() * 0.055f);
        const auto basePool = juce::Rectangle<float> (knob.getWidth() * 1.20f, knob.getHeight() * 0.36f).withCentre (poolCentre);
        g.setColour (accent.withAlpha (0.026f));
        g.fillEllipse (basePool.expanded (13.0f, 5.0f));
        g.setColour (accent.withAlpha (0.045f));
        g.fillEllipse (basePool.expanded (8.0f, 3.0f));
        g.setColour (accent.withAlpha (0.075f));
        g.fillEllipse (basePool.expanded (3.0f, 1.0f));
        g.setColour (accent.withAlpha (0.105f));
        g.fillEllipse (basePool.reduced (2.0f, 1.0f));

        // Neutral body shadow remains soft so the control still feels light and clean.
        auto shadow = knob.translated (0.0f, 3.0f).expanded (2.4f);
        g.setColour (juce::Colours::black.withAlpha (0.080f));
        g.fillEllipse (shadow);

        // Warm physical knob body: brighter top surface, slightly denser lower edge.
        juce::ColourGradient bodyGradient (juce::Colour::fromRGB (255, 254, 251), centre.x, knob.getY(),
                                           juce::Colour::fromRGB (222, 213, 202), centre.x, knob.getBottom(), false);
        bodyGradient.addColour (0.52, juce::Colour::fromRGB (245, 239, 231));
        g.setGradientFill (bodyGradient);
        g.fillEllipse (knob);

        // Let a small amount of the lamp colour reflect into the lower half of
        // the knob itself.  This is clipped to the knob, so it reads as reflected
        // light rather than a second orange ring.
        g.saveState();
        g.reduceClipRegion (knob.toNearestInt());
        const auto reflectedLight = juce::Rectangle<float> (knob.getWidth() * 1.08f, knob.getHeight() * 0.42f)
                                        .withCentre (juce::Point<float> (centre.x, knob.getBottom() - knob.getHeight() * 0.02f));
        g.setColour (accent.withAlpha (0.075f));
        g.fillEllipse (reflectedLight);
        g.restoreState();

        // Fine rim + glass-like top highlight add material depth without changing
        // the established geometry.
        g.setColour (juce::Colour::fromRGB (177, 164, 151).withAlpha (0.66f));
        g.drawEllipse (knob, 1.0f);
        juce::Path topRim;
        topRim.addCentredArc (centre.x, centre.y,
                              knob.getWidth() * 0.5f - 2.0f, knob.getHeight() * 0.5f - 2.0f,
                              0.0f,
                              juce::MathConstants<float>::pi * 1.08f,
                              juce::MathConstants<float>::pi * 1.92f,
                              true);
        g.setColour (juce::Colours::white.withAlpha (0.72f));
        g.strokePath (topRim, juce::PathStrokeType (1.15f));

        juce::Path lowerShade;
        lowerShade.addCentredArc (centre.x, centre.y,
                                  knob.getWidth() * 0.5f - 2.0f, knob.getHeight() * 0.5f - 2.0f,
                                  0.0f,
                                  juce::MathConstants<float>::pi * 0.08f,
                                  juce::MathConstants<float>::pi * 0.92f,
                                  true);
        g.setColour (juce::Colours::black.withAlpha (0.055f));
        g.strokePath (lowerShade, juce::PathStrokeType (1.0f));

        const auto pointerRadius = knob.getWidth() * 0.31f;
        const auto pointerEnd = juce::Point<float> (centre.x + std::sin (angle) * pointerRadius,
                                                    centre.y - std::cos (angle) * pointerRadius);
        g.setColour (accent.withAlpha (0.98f));
        g.drawLine (centre.x, centre.y, pointerEnd.x, pointerEnd.y, 2.0f);

        // Illuminated endpoint: a wide faint bloom + hot core.  This is the visual
        // "lamp" that was missing perceptually in 0.9.0.
        const auto lamp = juce::Point<float> (centre.x + std::sin (angle) * radius,
                                              centre.y - std::cos (angle) * radius);
        g.setColour (accent.withAlpha (0.055f));
        g.fillEllipse (lamp.x - 10.0f, lamp.y - 10.0f, 20.0f, 20.0f);
        g.setColour (accent.withAlpha (0.115f));
        g.fillEllipse (lamp.x - 6.5f, lamp.y - 6.5f, 13.0f, 13.0f);
        g.setColour (accent.withAlpha (0.28f));
        g.fillEllipse (lamp.x - 3.8f, lamp.y - 3.8f, 7.6f, 7.6f);
        g.setColour (accent.brighter (0.35f));
        g.fillEllipse (lamp.x - 1.75f, lamp.y - 1.75f, 3.5f, 3.5f);
        g.setColour (juce::Colours::white.withAlpha (0.88f));
        g.fillEllipse (lamp.x - 0.65f, lamp.y - 0.65f, 1.3f, 1.3f);
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isHighlighted,
                               bool isDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const auto corner = juce::jmin (9.0f, bounds.getHeight() * 0.28f);
        const bool alwaysLit = static_cast<bool> (button.getProperties().getWithDefault (juce::Identifier ("qqscAlwaysLit"), false));
        const bool lit = button.getToggleState() || alwaysLit;
        const bool enabled = button.isEnabled();
        const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

        if (lit && enabled)
        {
            // Larger soft halo plus a low spill underneath.  This mirrors the
            // knob lighting language so active buttons feel back-lit, not simply
            // outlined in orange/cyan.
            g.setColour (accent.withAlpha (0.032f));
            g.fillRoundedRectangle (bounds.expanded (6.0f), corner + 5.0f);
            g.setColour (accent.withAlpha (0.060f));
            g.fillRoundedRectangle (bounds.expanded (4.0f), corner + 3.5f);
            g.setColour (accent.withAlpha (0.115f));
            g.fillRoundedRectangle (bounds.expanded (2.1f), corner + 2.0f);

            const auto spill = juce::Rectangle<float> (bounds.getWidth() * 0.78f, 9.0f)
                                   .withCentre (juce::Point<float> (bounds.getCentreX(), bounds.getBottom() + 2.8f));
            g.setColour (accent.withAlpha (0.040f));
            g.fillEllipse (spill.expanded (5.0f, 2.0f));
            g.setColour (accent.withAlpha (0.085f));
            g.fillEllipse (spill);
        }

        auto fill = backgroundColour;
        if (lit)
            fill = ui::panel().overlaidWith (accent.withAlpha (isDown ? 0.19f : 0.105f));
        else if (isDown)
            fill = ui::panelAlt();
        else if (isHighlighted)
            fill = ui::panelAlt().interpolatedWith (ui::panel(), 0.48f);

        g.setColour (juce::Colours::black.withAlpha (0.045f));
        g.fillRoundedRectangle (bounds.translated (0.0f, 1.5f), corner);
        g.setColour (fill);
        g.fillRoundedRectangle (bounds, corner);

        // A fine highlight along the top gives the button a translucent/ceramic
        // surface instead of a flat JUCE rectangle.
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.68f : 0.48f));
        g.drawLine (bounds.getX() + corner * 0.55f, bounds.getY() + 1.35f,
                    bounds.getRight() - corner * 0.55f, bounds.getY() + 1.35f, 0.85f);

        g.setColour ((lit ? accent : ui::border()).withAlpha (enabled ? (lit ? 0.80f : 0.72f) : 0.34f));
        g.drawRoundedRectangle (bounds, corner, lit ? 1.25f : 0.95f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool,
                         bool) override
    {
        const bool alwaysLit = static_cast<bool> (button.getProperties().getWithDefault (juce::Identifier ("qqscAlwaysLit"), false));
        const bool lit = button.getToggleState() || alwaysLit;
        const bool enabled = button.isEnabled();
        auto textColour = lit ? button.findColour (juce::TextButton::buttonOnColourId).darker (0.38f)
                              : button.findColour (juce::TextButton::textColourOffId);
        if (! enabled)
            textColour = textColour.withAlpha (0.36f);
        g.setColour (textColour);
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (5, 1), juce::Justification::centred, 1);
    }

    void drawComboBox (juce::Graphics& g,
                       int width,
                       int height,
                       bool isButtonDown,
                       int buttonX,
                       int buttonY,
                       int buttonW,
                       int buttonH,
                       juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width - 1), static_cast<float> (height - 1));
        g.setColour (juce::Colours::black.withAlpha (0.032f));
        g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 8.0f);
        g.setColour (isButtonDown ? ui::panelAlt() : ui::panel());
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (ui::border().withAlpha (0.72f));
        g.drawRoundedRectangle (bounds, 8.0f, 0.95f);

        auto arrowArea = juce::Rectangle<float> (static_cast<float> (buttonX), static_cast<float> (buttonY),
                                                 static_cast<float> (buttonW), static_cast<float> (buttonH)).reduced (8.0f, 9.0f);
        juce::Path arrow;
        arrow.startNewSubPath (arrowArea.getX(), arrowArea.getY());
        arrow.lineTo (arrowArea.getCentreX(), arrowArea.getBottom());
        arrow.lineTo (arrowArea.getRight(), arrowArea.getY());
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (arrow, juce::PathStrokeType (1.4f));
    }
};
}
