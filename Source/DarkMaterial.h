#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace qqsc::dark
{
// Clean graphite: no noise textures or large blurred highlights. Only edges
// and physical controls receive directional light. Coordinates are design px.
inline void surface (juce::Graphics& g, juce::Rectangle<float> r, float corner,
                     bool inset = false)
{
    juce::ColourGradient body (juce::Colour (inset ? 0xff202123 : 0xff2e3032), r.getX(), r.getY(),
                              juce::Colour (inset ? 0xff282a2c : 0xff222426), r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawRoundedRectangle (r.reduced (0.4f), corner, 0.8f);
    juce::ColourGradient rim (juce::Colour (0xff555759).withAlpha (0.68f), r.getX(), r.getY(),
                             juce::Colour (0xff393b3d).withAlpha (0.30f), r.getX(), r.getBottom(), false);
    g.setGradientFill (rim);
    g.drawRoundedRectangle (r.reduced (1.25f), juce::jmax (1.0f, corner - 0.8f), 0.65f);
}

inline void panel (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setColour (juce::Colour (0xff1d1e20));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colour (0xff101214));
    g.drawRoundedRectangle (r.reduced (0.4f), corner, 0.8f);
    g.setColour (juce::Colour (0xff383a3c).withAlpha (0.80f));
    g.drawRoundedRectangle (r.reduced (1.6f), juce::jmax (1.0f, corner - 1.0f), 0.65f);
}

inline void button (juce::Graphics& g, juce::Button& control, juce::Colour accent,
                    bool lit, bool highlighted, bool down)
{
    const auto r = control.getLocalBounds().toFloat().reduced (1.0f);
    const auto corner = juce::jmin (7.0f, r.getHeight() * 0.25f);
    juce::Graphics::ScopedSaveState save (g);
    if (! control.isEnabled()) g.setOpacity (0.40f);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.7f).expanded (0.5f), corner);
    surface (g, r, corner, down);
    if (lit || highlighted || control.hasKeyboardFocus (true))
    {
        g.setColour (accent.withAlpha (lit ? 0.015f : 0.028f));
        g.fillRoundedRectangle (r.reduced (1.5f), corner);
        g.setColour (accent.withAlpha (lit ? 0.80f : 0.40f));
        g.drawRoundedRectangle (r.reduced (0.7f), corner, 0.75f);
    }
}

class Material
{
public:
    // The same 270-degree normalized sweep as the existing controls. Zero is
    // a truly unlit physical track, not a dimmed full-ring light.
    static constexpr float start = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float end = juce::MathConstants<float>::pi * 2.75f;
    static constexpr float radius = 34.0f;
    static float angleForValue (float value) noexcept { return start + value * (end - start); }

    juce::Image render (float value, int size)
    {
        value = juce::jlimit (0.0f, 1.0f, value);
        auto frame = baseFor (size).createCopy();
        {
            juce::Graphics g (frame);
            g.addTransform (juce::AffineTransform::scale (size / 100.0f));
            const auto angle = angleForValue (value);
            const juce::PathStrokeType::JointStyle joint = juce::PathStrokeType::curved;
            const juce::PathStrokeType::EndCapStyle cap = juce::PathStrokeType::rounded;
            if (value > 0.0f)
            {
                juce::Path arc;
                arc.addCentredArc (50.0f, 47.0f, radius, radius, 0.0f, start, angle, true);
                const auto strength = juce::jmin (1.0f, value * 25.0f);
                // Glass-guide core with a short falloff, never a broad neon halo.
                for (int layer = 5; layer >= 1; --layer)
                {
                    g.setColour (juce::Colour (0xff78b7d5).withAlpha (0.018f * strength));
                    g.strokePath (arc, juce::PathStrokeType (1.1f + layer * 0.85f, joint, cap));
                }
                g.setColour (juce::Colour (0xff528ca8).withAlpha (0.80f * strength));
                g.strokePath (arc, juce::PathStrokeType (1.8f, joint, cap));
                g.setColour (juce::Colour (0xff9bd8ed).withAlpha (0.94f * strength));
                g.strokePath (arc, juce::PathStrokeType (0.70f, joint, cap));
                // A weaker, displaced reflection lives on the metal sidewall.
                juce::Path reflection;
                reflection.addCentredArc (50.0f, 47.0f, radius - 2.2f, radius - 2.2f,
                                          0.0f, start, angle, true);
                g.setColour (juce::Colour (0xff80bed7).withAlpha (0.18f * strength));
                g.strokePath (reflection, juce::PathStrokeType (0.8f, joint, cap));
            }
            const auto axis = juce::Point<float> (std::sin (angle), -std::cos (angle));
            const auto centre = juce::Point<float> (50.0f, 47.0f);
            juce::Path pointer;
            pointer.startNewSubPath (centre + axis * 18.3f);
            pointer.lineTo (centre + axis * 23.0f);
            // Recess remains visible when there is no emitted light.
            g.setColour (juce::Colour (0xff202426));
            g.strokePath (pointer, juce::PathStrokeType (2.6f, joint, cap));
            if (value > 0.0f)
            {
                g.setColour (juce::Colour (0xff74b8d5).withAlpha (0.10f));
                g.strokePath (pointer, juce::PathStrokeType (4.0f, joint, cap));
                g.setColour (juce::Colour (0xff6dadc8));
                g.strokePath (pointer, juce::PathStrokeType (1.9f, joint, cap));
                g.setColour (juce::Colour (0xffb0dfeb));
                g.strokePath (pointer, juce::PathStrokeType (0.8f, joint, cap));
            }
            else
            {
                g.setColour (juce::Colour (0xff656a6d));
                g.strokePath (pointer, juce::PathStrokeType (0.9f, joint, cap));
            }
        }
        return frame;
    }

private:
    std::map<int, juce::Image> bases;
    const juce::Image& baseFor (int size)
    {
        if (auto found = bases.find (size); found != bases.end()) return found->second;
        juce::Image base (juce::Image::ARGB, size, size, true);
        {
            juce::Graphics g (base);
            g.addTransform (juce::AffineTransform::scale (size / 100.0f));
            // Stationary soft cast shadow under the complete metal body.
            for (int layer = 12; layer >= 1; --layer)
            {
                const float spread = layer * 0.56f;
                g.setColour (juce::Colours::black.withAlpha (0.023f));
                g.fillEllipse (juce::Rectangle<float> (15.5f, 19.0f, 69.0f, 67.0f).expanded (spread));
            }
            const auto ellipse = [&] (juce::Rectangle<float> r, juce::Colour top, juce::Colour bottom)
            {
                g.setGradientFill (juce::ColourGradient (top, r.getX() + r.getWidth() * 0.28f, r.getY(),
                                                        bottom, r.getRight(), r.getBottom(), false));
                g.fillEllipse (r);
            };
            ellipse ({ 15.0f, 12.0f, 70.0f, 70.0f }, juce::Colour (0xff46494b), juce::Colour (0xff141719));
            ellipse ({ 16.3f, 13.3f, 67.4f, 67.4f }, juce::Colour (0xff202426), juce::Colour (0xff1a1d1f));
            ellipse ({ 18.5f, 15.5f, 63.0f, 63.0f }, juce::Colour (0xff505456), juce::Colour (0xff242729));
            ellipse ({ 20.0f, 17.0f, 60.0f, 60.0f }, juce::Colour (0xff303436), juce::Colour (0xff171b1d));
            ellipse ({ 22.8f, 19.8f, 54.4f, 54.4f }, juce::Colour (0xff4b4e50), juce::Colour (0xff292d2f));
            ellipse ({ 23.6f, 20.6f, 52.8f, 52.8f }, juce::Colour (0xff393c3e), juce::Colour (0xff2c2f31));
        }
        while (bases.size() >= 4) bases.erase (bases.begin());
        return bases.emplace (size, std::move (base)).first->second;
    }
};

// Message-thread-only, one current frame per live control. Repainting the
// history cannot recompute an unchanged knob or allocate a growing filmstrip.
class Renderer
{
public:
    void draw (juce::Graphics& g, juce::Rectangle<float> bounds, float position, juce::Slider& slider)
    {
        position = juce::jlimit (0.0f, 1.0f, position);
        entries.erase (std::remove_if (entries.begin(), entries.end(),
            [] (const Entry& e) { return e.slider == nullptr; }), entries.end());
        auto found = std::find_if (entries.begin(), entries.end(),
            [&] (const Entry& e) { return e.slider.getComponent() == &slider; });
        if (found == entries.end())
        {
            entries.push_back (Entry { juce::Component::SafePointer<juce::Slider> (&slider) });
            found = entries.end() - 1;
        }
        const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto scale = juce::jmax (1.5f, g.getInternalContext().getPhysicalPixelScaleFactor() * 1.25f);
        const auto requested = juce::roundToInt (diameter * scale);
        int resolution = 512;
        for (int choice : { 128, 160, 192, 256, 384, 512 })
            if (requested <= choice) { resolution = choice; break; }
        if (! found->image.isValid() || found->resolution != resolution || found->position != position)
        {
            found->image = material.render (position, resolution);
            found->resolution = resolution;
            found->position = position;
            ++renderCount;
        }
        juce::Graphics::ScopedSaveState save (g);
        g.setOpacity (slider.isEnabled() ? 1.0f : 0.40f);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (found->image, juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre()));
    }
    size_t getRenderCount() const noexcept { return renderCount; }
private:
    struct Entry
    {
        juce::Component::SafePointer<juce::Slider> slider;
        juce::Image image;
        int resolution = 0;
        float position = -1.0f;
    };
    Material material;
    std::vector<Entry> entries;
    size_t renderCount = 0;
};
}
