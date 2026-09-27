#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <atomic>
#include "WarmMaterial.h"
#include "WarmKnobAsset.h"
#include "DarkMaterial.h"
#include "RefinedDarkKnob.h"
#include "LightKnobScale.h"

struct QQSCVisualCheck;

namespace qqsc
{
namespace ui
{
    enum class Theme { light, dark, classic };
    inline std::atomic<Theme>& themeFlag()
    {
        static std::atomic<Theme> current { Theme::light };
        return current;
    }
    inline Theme currentTheme() noexcept { return themeFlag().load (std::memory_order_relaxed); }
    inline bool isClassicTheme() noexcept { return currentTheme() == Theme::classic; }
    inline bool isDarkTheme() noexcept { return currentTheme() == Theme::dark; }
    inline void setTheme (Theme theme) noexcept { themeFlag().store (theme, std::memory_order_relaxed); }
    inline const char* themeKey (Theme theme) noexcept
    {
        return theme == Theme::dark ? "dark" : (theme == Theme::classic ? "classic" : "light");
    }
    inline const char* themeLabel (Theme theme) noexcept
    {
        return theme == Theme::dark ? "DARK" : (theme == Theme::classic ? "CLASSIC" : "LIGHT");
    }
    inline Theme themeFromPreferences (const juce::PropertiesFile& properties)
    {
        // A legacy user who chose Classic still gets Classic, not the new Dark.
        const auto saved = properties.getValue ("uiTheme");
        if (saved == "dark") return Theme::dark;
        if (saved == "classic") return Theme::classic;
        if (saved == "light") return Theme::light;
        return properties.getBoolValue ("classicTheme", false) ? Theme::classic : Theme::light;
    }

    inline juce::Colour canvas()
    {
        if (isDarkTheme()) return juce::Colour (0xff1b1c1e);
        return isClassicTheme() ? juce::Colour::fromRGB (10, 13, 17)
                                : juce::Colour::fromRGB (236, 232, 226);
    }
    inline juce::Colour panel()
    {
        if (isDarkTheme()) return juce::Colour (0xff1d1e20);
        return isClassicTheme() ? juce::Colour::fromRGB (16, 20, 26)
                                : juce::Colour::fromRGB (242, 239, 234);
    }
    inline juce::Colour panelAlt()
    {
        if (isDarkTheme()) return juce::Colour (0xff17191b);
        return isClassicTheme() ? juce::Colour::fromRGB (23, 29, 37)
                                : juce::Colour::fromRGB (230, 225, 219);
    }
    inline juce::Colour text()
    {
        if (isDarkTheme()) return juce::Colour (0xffc1bfb8);
        return isClassicTheme() ? juce::Colour::fromRGB (232, 237, 242)
                                : juce::Colour::fromRGB (59, 57, 54);
    }
    inline juce::Colour textMuted()
    {
        if (isDarkTheme()) return juce::Colour (0xff9c9c97);
        return isClassicTheme() ? juce::Colour::fromRGB (151, 163, 176)
                                : juce::Colour::fromRGB (112, 106, 99);
    }
    inline juce::Colour border()
    {
        if (isDarkTheme()) return juce::Colour (0xff3a3c3e);
        return isClassicTheme() ? juce::Colour::fromRGB (55, 66, 78)
                                : juce::Colour::fromRGB (193, 185, 174);
    }
    inline juce::Colour warmAccent()
    {
        if (isDarkTheme()) return juce::Colour (0xffd79a57);
        return isClassicTheme() ? juce::Colour::fromRGB (82, 201, 238)
                                : juce::Colour::fromRGB (242, 137, 73);
    }
    inline juce::Colour warmAccentSoft()
    {
        if (isDarkTheme()) return juce::Colour (0xffc2a584);
        return isClassicTheme() ? juce::Colour::fromRGB (110, 216, 244)
                                : juce::Colour::fromRGB (255, 185, 140);
    }
    inline juce::Colour cyanAccent()      { if (isDarkTheme()) return juce::Colour (0xff7cb3b8); return isClassicTheme() ? warmAccent() : juce::Colour::fromRGB (58, 169, 190); }
    inline juce::Colour dryTrace()        { if (isDarkTheme()) return juce::Colour (0xffa1a9ad); return isClassicTheme() ? juce::Colour::fromRGB (139, 151, 164) : juce::Colour::fromRGB (153, 151, 148); }
    inline juce::Colour outputAccent()    { if (isDarkTheme()) return juce::Colour (0xffe19a4d); return isClassicTheme() ? juce::Colour::fromRGB (246, 196, 83) : juce::Colour::fromRGB (248, 118, 47); }
    // Shared by Display paths/legend/readouts and the GR meters. Classic pink
    // is untouched; LIGHT pairs gray Input, orange Output and cyan-blue GR.
    inline juce::Colour grAccent()        { if (isDarkTheme()) return juce::Colour (0xff7cb3b8); return isClassicTheme() ? juce::Colour::fromRGB (255, 99, 125) : juce::Colour::fromRGB (58, 169, 190); }
}

class UTF8LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    UTF8LookAndFeel()
    {
        refreshThemeColours();
    }

    void setTheme (ui::Theme theme)
    {
        ui::setTheme (theme);
        refreshThemeColours();
    }

private:
    // Lazy: Classic-only editor instances do not load material assets.
    std::unique_ptr<warm_asset::Renderer> warmKnobs;
    std::unique_ptr<dark_refined::Renderer> darkKnobs;
    friend struct ::QQSCVisualCheck;

    void refreshThemeColours()
    {
        setColour (juce::Slider::textBoxTextColourId, ui::text());
        setColour (juce::Slider::textBoxBackgroundColourId, ui::panel().withAlpha (0.76f));
        setColour (juce::Slider::textBoxOutlineColourId, ui::border().withAlpha (0.66f));

        // v0.9.4: a Slider text box is a Label while idle, but JUCE creates an
        // internal TextEditor when the value is double-clicked for direct entry.
        // The light UI previously styled only the idle Slider text-box colours,
        // allowing the editor's default white text to become unreadable on the
        // ivory background.  Explicitly style both Label edit mode and the
        // TextEditor/caret so direct numeric entry stays visible.
        setColour (juce::Label::backgroundWhenEditingColourId, ui::panel());
        setColour (juce::Label::textWhenEditingColourId, ui::text());
        setColour (juce::Label::outlineWhenEditingColourId, ui::warmAccent().withAlpha (0.72f));
        setColour (juce::TextEditor::backgroundColourId, ui::panel());
        setColour (juce::TextEditor::textColourId, ui::text());
        setColour (juce::TextEditor::highlightColourId, ui::warmAccentSoft().withAlpha (0.58f));
        setColour (juce::TextEditor::highlightedTextColourId, ui::text());
        setColour (juce::TextEditor::outlineColourId, ui::border().withAlpha (0.78f));
        setColour (juce::TextEditor::focusedOutlineColourId, ui::warmAccent().withAlpha (0.72f));
        setColour (juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
        setColour (juce::CaretComponent::caretColourId, ui::warmAccent());
        setColour (juce::Slider::rotarySliderOutlineColourId, ui::border());
        setColour (juce::Slider::rotarySliderFillColourId, ui::warmAccent());

        setColour (juce::PopupMenu::backgroundColourId, ui::panel());
        setColour (juce::PopupMenu::textColourId, ui::text());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::warmAccentSoft().withAlpha (0.34f));
        setColour (juce::PopupMenu::highlightedTextColourId, ui::text());
    }

public:

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        auto f = font;

       #if JUCE_MAC
        // PingFang SC ships with modern macOS and contains Simplified Chinese glyphs.
        f.setTypefaceName (ui::isClassicTheme() ? "PingFang SC" : "Avenir Next");
        if (! ui::isClassicTheme()) f.setStyleFlags (juce::Font::plain);
       #elif JUCE_WINDOWS
        // Microsoft YaHei is present on supported modern Windows installations.
        f.setTypefaceName (ui::isClassicTheme() ? "Microsoft YaHei"
                                             : (font.getHeight() >= 24.0f ? "Century Gothic" : "Segoe UI"));
        if (! ui::isClassicTheme()) f.setStyleFlags (juce::Font::plain);
       #endif

        if (auto face = juce::Typeface::createSystemTypefaceFor (f))
            return face;

        return juce::LookAndFeel_V4::getTypefaceForFont (font);
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override
    {
        if (static_cast<bool> (button.getProperties().getWithDefault ("qqscSmallLink", false)))
            return juce::Font (juce::FontOptions (buttonHeight <= 14 ? 8.0f : 9.0f, juce::Font::plain));
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
        if (ui::isDarkTheme())
        {
            if (darkKnobs == nullptr) darkKnobs = std::make_unique<dark_refined::Renderer>();
            darkKnobs->draw (g, { static_cast<float> (x), static_cast<float> (y),
                                 static_cast<float> (width), static_cast<float> (height) },
                             sliderPosProportional, slider);
            return;
        }
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
        const auto lightOrigin = knob_light::origin (slider);
        const auto arcBegin = rotaryStartAngle + juce::jmin (lightOrigin, sliderPosProportional) * (rotaryEndAngle - rotaryStartAngle);
        const auto arcEnd = rotaryStartAngle + juce::jmax (lightOrigin, sliderPosProportional) * (rotaryEndAngle - rotaryStartAngle);
        valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                arcBegin, arcEnd, true);

        if (ui::isClassicTheme())
        {
            // The legacy skin is intentionally calm: a technical dark ring and
            // one clear cyan position lamp, without the light theme's large glow.
            const auto knob = juce::Rectangle<float> (diameter * 0.62f, diameter * 0.62f).withCentre (centre);
            g.setColour (juce::Colours::black.withAlpha (0.30f));
            g.fillEllipse (knob.translated (0.0f, 2.0f).expanded (1.0f));
            g.setColour (ui::panelAlt().withAlpha (0.90f));
            g.fillEllipse (knob);
            g.setColour (track.withAlpha (0.92f));
            g.strokePath (fullArc, juce::PathStrokeType (6.0f));
            g.setColour (accent.withAlpha (0.45f));
            g.strokePath (valueArc, juce::PathStrokeType (2.5f));
            g.setColour (ui::border().withAlpha (0.70f));
            g.drawEllipse (knob, 0.85f);

            const auto pointerEnd = juce::Point<float> (centre.x + std::sin (angle) * knob.getWidth() * 0.30f,
                                                        centre.y - std::cos (angle) * knob.getHeight() * 0.30f);
            g.setColour (accent.withAlpha (0.96f));
            g.drawLine (centre.x, centre.y, pointerEnd.x, pointerEnd.y, 1.7f);

            const auto lamp = juce::Point<float> (centre.x + std::sin (angle) * radius,
                                                  centre.y - std::cos (angle) * radius);
            g.setColour (accent.withAlpha (0.20f));
            g.fillEllipse (lamp.x - 7.0f, lamp.y - 7.0f, 14.0f, 14.0f);
            g.setColour (accent.withAlpha (0.98f));
            g.fillEllipse (lamp.x - 4.0f, lamp.y - 4.0f, 8.0f, 8.0f);
            return;
        }

        if (warmKnobs == nullptr) warmKnobs = std::make_unique<warm_asset::Renderer>();
        warmKnobs->draw (g, { static_cast<float> (x), static_cast<float> (y),
                             static_cast<float> (width), static_cast<float> (height) },
                         sliderPosProportional, slider);
        warm_asset::drawScale (g, {static_cast<float> (x), static_cast<float> (y),
                                  static_cast<float> (width), static_cast<float> (height)}, slider.isEnabled());

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

        if (ui::isDarkTheme())
        {
            dark::button (g, button, accent, lit, isHighlighted, isDown);
            return;
        }
        if (ui::isClassicTheme())
        {
            auto fill = lit ? accent : backgroundColour;
            if (isDown)
                fill = fill.darker (0.15f);
            else if (isHighlighted && ! lit)
                fill = ui::panelAlt();

            g.setColour (juce::Colours::black.withAlpha (0.24f));
            g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), corner);
            g.setColour (fill);
            g.fillRoundedRectangle (bounds, corner);
            g.setColour ((lit ? accent.brighter (0.18f) : ui::border()).withAlpha (enabled ? 0.96f : 0.34f));
            g.drawRoundedRectangle (bounds, corner, lit ? 1.1f : 0.9f);
            return;
        }

        warm::button (g, button, accent, lit, isHighlighted, isDown);

    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool,
                         bool) override
    {
        const bool alwaysLit = static_cast<bool> (button.getProperties().getWithDefault (juce::Identifier ("qqscAlwaysLit"), false));
        const bool lit = button.getToggleState() || alwaysLit;
        const bool enabled = button.isEnabled();
        auto textColour = ui::isClassicTheme() && lit
                            ? ui::text()
                            : (lit ? button.findColour (juce::TextButton::buttonOnColourId).darker (0.38f)
                                   : button.findColour (juce::TextButton::textColourOffId));
        if (ui::isDarkTheme())
            textColour = lit ? button.findColour (juce::TextButton::buttonOnColourId)
                             : button.findColour (juce::TextButton::textColourOffId);
        if (! enabled)
            textColour = textColour.withAlpha (0.36f);
        g.setColour (textColour);
        if(bool(button.getProperties().getWithDefault("qqscHeadphones",false)))
        {
            const auto b=button.getLocalBounds().toFloat();
            const float cx=b.getCentreX(),cy=b.getCentreY(),radius=5.0f;
            juce::Path band;
            band.addCentredArc(cx,cy+1.0f,radius,radius,0,-juce::MathConstants<float>::halfPi,
                              juce::MathConstants<float>::halfPi,true);
            g.strokePath(band,juce::PathStrokeType(1.4f));
            g.fillRoundedRectangle(cx-radius-1,cy,3,5,1);
            g.fillRoundedRectangle(cx+radius-2,cy,3,5,1);
            return;
        }
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
        if (ui::isDarkTheme())
            dark::surface (g, bounds, 6.0f, isButtonDown);
        else if (! ui::isClassicTheme())
            warm::surface (g, bounds, 6.0f, isButtonDown);
        else
        {
        g.setColour (juce::Colours::black.withAlpha (ui::isClassicTheme() ? 0.20f : 0.032f));
        g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 8.0f);
        g.setColour (isButtonDown ? ui::panelAlt() : ui::panel());
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (ui::border().withAlpha (0.72f));
        g.drawRoundedRectangle (bounds, 8.0f, 0.95f);

        }

        auto arrowArea = juce::Rectangle<float> (static_cast<float> (buttonX), static_cast<float> (buttonY),
                                                 static_cast<float> (buttonW), static_cast<float> (buttonH)).reduced (8.0f, 9.0f);
        juce::Path arrow;
        arrow.startNewSubPath (arrowArea.getX(), arrowArea.getY());
        arrow.lineTo (arrowArea.getCentreX(), arrowArea.getBottom());
        arrow.lineTo (arrowArea.getRight(), arrowArea.getY());
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (arrow, juce::PathStrokeType (1.4f));
    }
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        auto* slider = dynamic_cast<juce::Slider*> (label.getParentComponent());
        if (ui::isClassicTheme() || slider == nullptr)
        {
            juce::LookAndFeel_V4::drawLabel (g, label);
            return;
        }
        const auto r = label.getLocalBounds().toFloat().reduced (0.75f, 1.0f);
        if (ui::isDarkTheme()) dark::surface (g, r, 5.0f, true);
        else warm::surface (g, r, 5.0f, true, 0.65f);
        if (! label.isBeingEdited())
        {
            const bool rotary = slider->getSliderStyle() != juce::Slider::LinearVertical;
            const auto colour = rotary && ! ui::isDarkTheme() ? slider->findColour (juce::Slider::rotarySliderFillColourId).darker (0.28f)
                                       : ui::text();
            g.setColour (colour.withAlpha (label.isEnabled() ? 1.0f : 0.40f));
            g.setFont (juce::Font (juce::FontOptions (rotary ? 14.0f : 11.5f)));
            g.drawFittedText (label.getText(), label.getLocalBounds().reduced (3, 1),
                             juce::Justification::centred, 1);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (ui::isClassicTheme() || style != juce::Slider::LinearVertical)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                sliderPos, minSliderPos, maxSliderPos, style, slider);
            return;
        }
        const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        const auto track = juce::Rectangle<float> (cx - 2.2f, static_cast<float> (y), 4.4f, static_cast<float> (height));
        g.setColour (ui::isDarkTheme() ? juce::Colour (0xff414445) : juce::Colour (0xffd8d0c5));
        g.fillRoundedRectangle (track, 2.2f);
        g.setColour (juce::Colours::white.withAlpha (ui::isDarkTheme() ? 0.025f : 0.90f));
        g.drawLine (track.getRight() + 0.8f, track.getY(), track.getRight() + 0.8f, track.getBottom(), 0.8f);
        g.setColour (ui::warmAccent().withAlpha (ui::isDarkTheme() ? 0.42f : 0.70f));
        g.fillRoundedRectangle (track.withTop (sliderPos), 2.0f);
        const auto thumb = juce::Rectangle<float> (19.0f, 11.0f).withCentre ({ cx, sliderPos });
        g.setColour (ui::isDarkTheme() ? juce::Colours::black.withAlpha (0.28f) : juce::Colour (0xff796954).withAlpha (0.20f));
        g.fillRoundedRectangle (thumb.translated (0.0f, 1.0f).expanded (0.6f), 4.0f);
        if (ui::isDarkTheme()) dark::surface (g, thumb, 4.0f);
        else warm::surface (g, thumb, 4.0f);
        g.setColour (ui::warmAccent());
        g.drawLine (cx - 5.5f, sliderPos, cx + 5.5f, sliderPos, 1.3f);
    }

};
}
