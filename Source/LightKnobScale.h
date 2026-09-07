#pragma once
#include "WarmKnobAsset.h"

namespace qqsc::warm_asset
{
// Fixed, non-emissive reference marks outside the original 260-degree light.
// Normalized travel matches the existing pointer, including nonlinear controls.
inline void drawScale (juce::Graphics& g, juce::Rectangle<float> bounds, bool enabled)
{
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto square = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
    const auto centre = square.getPosition() + juce::Point<float> (diameter * 0.5f, diameter * 0.4975f);
    juce::Graphics::ScopedSaveState save (g);
    for (int i = 0; i <= 12; ++i)
    {
        const bool major = (i % 3) == 0;
        const float angle = juce::degreesToRadians (Material::angleForValue (static_cast<float> (i) / 12.0f));
        const juce::Point<float> axis (std::sin (angle), -std::cos (angle));
        const auto inner = centre + axis * (diameter * 0.435f);
        const auto outer = centre + axis * (diameter * (major ? 0.481f : 0.468f));
        g.setColour (juce::Colour (0xffa18470).withAlpha ((major ? 0.62f : 0.48f) * (enabled ? 1.0f : 0.43f)));
        g.drawLine ({inner, outer}, diameter * (major ? 0.0075f : 0.0060f));
    }
}
}
