#pragma once
#include "DarkMaterial.h"
#include <cstdint>

namespace qqsc::dark
{
// Fine powder-coated graphite, exclusively for the lower control panel.
// This is a fixed design-space material, not animated noise or a tiled photo.
// One 2x image is created lazily on the message thread and reused at every size.
class BottomPanelMaterial
{
public:
    void draw (juce::Graphics& g, juce::Rectangle<float> r, float corner)
    {
        panel (g, r, corner);
        if (! finish.isValid()) generate();
        juce::Graphics::ScopedSaveState state (g);
        juce::Path mask;
        mask.addRoundedRectangle (r.reduced (2.2f), juce::jmax (1.0f, corner - 2.2f));
        g.reduceClipRegion (mask);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (finish, r);
    }
    size_t getGenerationCount() const noexcept { return generations; }

private:
    juce::Image finish;
    size_t generations = 0;

    static float grain (int x, int y) noexcept
    {
        // Integer-coordinate hash has no seams, visible repeat or slow cloud layer.
        auto bits = static_cast<std::uint32_t> (x) * 0x8da6b343u
                  ^ static_cast<std::uint32_t> (y) * 0xd8163841u ^ 0x719eca53u;
        bits ^= bits >> 16; bits *= 0x7feb352du;
        bits ^= bits >> 15; bits *= 0x846ca68bu;
        bits ^= bits >> 16;
        return static_cast<float> (bits & 65535u) / 65535.0f;
    }
    static float sample (float x, float y) noexcept
    {
        const int ix = static_cast<int> (std::floor (x));
        const int iy = static_cast<int> (std::floor (y));
        const float fx = x - static_cast<float> (ix), fy = y - static_cast<float> (iy);
        const float upper = juce::jmap (fx, grain (ix, iy), grain (ix + 1, iy));
        const float lower = juce::jmap (fx, grain (ix, iy + 1), grain (ix + 1, iy + 1));
        return juce::jmap (fy, upper, lower);
    }
    void generate()
    {
        constexpr int width = 1976, height = 324;
        finish = juce::Image (juce::Image::RGB, width, height, false);
        juce::Image::BitmapData data (finish, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                // Sub-pixel facets and tiny paired highlights/shadows suggest
                // a frosted finish. Never use isolated white specks or scratches.
                const float gx = (static_cast<float> (x) + 0.5f) / 1.4f;
                const float gy = (static_cast<float> (y) + 0.5f) / 1.4f;
                const float facets = (sample (gx, gy) - 0.5f) * 8.0f
                    + (sample (gx - 0.55f, gy - 0.55f) - sample (gx + 0.55f, gy + 0.55f)) * 3.0f;
                const float light = 1.4f - 2.0f * static_cast<float> (y) / static_cast<float> (height - 1);
                const auto tone = static_cast<juce::uint8> (juce::roundToInt (29.0f + light + facets));
                data.setPixelColour (x, y, juce::Colour (tone, static_cast<juce::uint8> (tone + 1),
                                                       static_cast<juce::uint8> (tone + 3)));
            }
        ++generations;
    }
};
}
