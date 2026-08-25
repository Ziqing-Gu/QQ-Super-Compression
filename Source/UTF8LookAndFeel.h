#pragma once

#include <JuceHeader.h>

namespace qqsc
{
class UTF8LookAndFeel final : public juce::LookAndFeel_V4
{
public:
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
};
}
