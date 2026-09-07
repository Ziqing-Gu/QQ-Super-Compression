#pragma once

#include <JuceHeader.h>

namespace qqsc::warm
{
// A slightly deeper warm-grey backplate lets the accepted metal knobs sit in
// the panel instead of looking pasted onto white. Control surfaces stay as-is.
inline void backplate (juce::Graphics& g, juce::Rectangle<float> bounds, float radius)
{
    juce::ColourGradient body (juce::Colour (0xffeeeae5), bounds.getX(), bounds.getY(),
                              juce::Colour (0xffdfd9d1), bounds.getRight(), bounds.getBottom(), false);
    body.addColour (0.45, juce::Colour (0xffe9e4de));
    g.setGradientFill (body);
    g.fillRoundedRectangle (bounds, radius);
    juce::ColourGradient sheen (juce::Colour (0xfffffcf7).withAlpha (0.30f),
                               bounds.getX() + bounds.getWidth() * 0.32f, bounds.getY(),
                               juce::Colours::transparentWhite,
                               bounds.getX() + bounds.getWidth() * 0.76f, bounds.getBottom(), true);
    g.setGradientFill (sheen);
    g.fillRoundedRectangle (bounds, radius);
    g.setColour (juce::Colour (0xffaea397).withAlpha (0.56f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 0.85f);
    g.setColour (juce::Colours::white.withAlpha (0.78f));
    g.drawRoundedRectangle (bounds.reduced (1.55f), juce::jmax (1.0f, radius - 1.0f), 0.8f);
}

inline void surface (juce::Graphics& g, juce::Rectangle<float> bounds, float radius,
                     bool inset = false, float strength = 1.0f)
{
    juce::ColourGradient body (juce::Colour (0xfffaf8f5), bounds.getX(), bounds.getY(),
                              juce::Colour (0xffeae5de), bounds.getRight(), bounds.getBottom(), false);
    body.addColour (0.45, juce::Colour (0xfff4f1ec));
    if (inset)
        body = juce::ColourGradient (juce::Colour (0xffe8e3dc), bounds.getX(), bounds.getY(),
                                    juce::Colour (0xfffaf8f5), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (bounds, radius);
    g.setColour (juce::Colour (0xffaea397).withAlpha (0.56f * strength));
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 0.85f);
    g.setColour (juce::Colours::white.withAlpha (0.90f * strength));
    g.drawRoundedRectangle (bounds.reduced (1.55f), juce::jmax (1.0f, radius - 1.0f), 0.8f);
}

inline void button (juce::Graphics& g, juce::Button& control, juce::Colour accent,
                    bool lit, bool highlighted, bool down)
{
    auto r = control.getLocalBounds().toFloat().reduced (1.0f);
    const auto radius = juce::jmin (8.0f, r.getHeight() * 0.28f);
    juce::Graphics::ScopedSaveState save (g);
    if (! control.isEnabled()) g.setOpacity (0.43f);
    g.setColour (juce::Colour (0xff93816d).withAlpha (0.11f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.4f), radius);
    surface (g, r, radius, down);
    if (lit || highlighted)
    {
        g.setColour (accent.withAlpha (lit ? 0.06f : 0.025f));
        g.fillRoundedRectangle (r.reduced (1.8f), radius - 1.0f);
        g.setColour (accent.withAlpha (lit ? 0.72f : 0.36f));
        g.drawRoundedRectangle (r.reduced (0.55f), radius, 0.85f);
    }
    if (lit)
    {
        const float inset = juce::jmin (8.0f, r.getWidth() * 0.2f);
        juce::ColourGradient light (accent.withAlpha (0.0f), r.getX() + inset, r.getBottom(),
                                   accent.withAlpha (0.0f), r.getRight() - inset, r.getBottom(), false);
        light.addColour (0.5, accent.withAlpha (0.88f));
        g.setGradientFill (light);
        g.fillRect (r.getX() + inset, r.getBottom() - 1.5f, r.getWidth() - 2.0f * inset, 1.0f);
    }
    if (control.hasKeyboardFocus (true))
    {
        g.setColour (accent.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (2.5f), radius - 1.0f, 1.0f);
    }
}
}
