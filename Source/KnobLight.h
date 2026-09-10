#pragma once
#include <JuceHeader.h>

namespace qqsc::knob_light
{
// Normalised neutral position. Ordinary controls retain their left endpoint;
// Single Ratio uses the centre, Dual UP the right endpoint, Dual DOWN the left.
inline float origin (const juce::Slider& slider) noexcept
{
    return juce::jlimit (0.0f, 1.0f,
        static_cast<float> (static_cast<double> (slider.getProperties().getWithDefault ("qqscLightOrigin", 0.0))));
}
}
