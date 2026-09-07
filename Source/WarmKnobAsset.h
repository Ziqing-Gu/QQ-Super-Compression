#pragma once
#include <JuceHeader.h>
#include <QQSCWarmKnobData.h>
#include <array>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <map>
#include <vector>

namespace qqsc::warm_asset
{
inline float smooth(float lo, float hi, float x)
{
    const auto t = juce::jlimit(0.0f, 1.0f, (x - lo) / (hi - lo));
    return t * t * (3.0f - 2.0f * t);
}

// The approved image supplies the actual metal, bevel, sidewall and light material.
// Geometry is not redrawn with concentric circles. Only the emission mask and
// movable indicator vary. The source texture, its camera and shadows never rotate.
class Material
{
public:
    static constexpr float travel = 260.0f;
    static constexpr float startDegrees = -130.0f;
    static constexpr float sourceWidth = 800.0f;
    static constexpr float centreX = 400.0f;
    static constexpr float centreY = 398.0f;

    Material()
        : lit(juce::ImageFileFormat::loadFrom(QQSCWarmKnobData::approvedlit_png, QQSCWarmKnobData::approvedlit_pngSize)),
          off(juce::ImageFileFormat::loadFrom(QQSCWarmKnobData::unlitbase_png, QQSCWarmKnobData::unlitbase_pngSize))
    {
        if (!lit.isValid() || !off.isValid() || lit.getBounds() != off.getBounds())
            throw std::runtime_error("The registered material images could not be loaded.");
    }

    static float angleForValue(float value) { return startDegrees + travel * juce::jlimit(0.0f, 1.0f, value); }

    static float emission(float value, float position, float feather)
    {
        if (value <= 0.0f) return 0.0f;
        if (value >= 1.0f) return 1.0f;
        const auto floor = smooth(position - feather, position + feather, 0.0f);
        const auto ceiling = smooth(position - feather, position + feather, 1.0f);
        return (smooth(position - feather, position + feather, value) - floor) / (ceiling - floor);
    }

    juce::Image render(float value, int size)
    {
        value = juce::jlimit(0.0f, 1.0f, value);
        auto& basis = getBasis(size);
        juce::Image image(juce::Image::ARGB, size, size, true);
        const float leftEndSpill = emission(value, 0.0f, 0.35f);
        const float rightEndSpill = emission(value, 1.0f, 0.35f);
        {
            juce::Image::BitmapData dest(image, juce::Image::BitmapData::writeOnly);
            for (int y = 0, i = 0; y < size; ++y)
                for (int x = 0; x < size; ++x, ++i)
                {
                    const auto& p = basis.pixels[static_cast<size_t>(i)];
                    const auto t = juce::jlimit(0.0f, 1.0f, (value - p.curveLow) * p.invSpan);
                    const auto arcStrength = (t * t * (3.0f - 2.0f * t) - p.curveFloor) * p.curveScale;
                    const auto spill = leftEndSpill + (rightEndSpill - leftEndSpill) * p.spillBalance;
                    const auto strength = arcStrength + (spill - arcStrength) * p.gapWeight;
                    dest.setPixelColour(x, y, juce::Colour(
                        channel(p.base[0] + strength * p.delta[0]),
                        channel(p.base[1] + strength * p.delta[1]),
                        channel(p.base[2] + strength * p.delta[2]), p.alpha));
                }
        }
        juce::Graphics g(image);
        g.addTransform(juce::AffineTransform::scale(static_cast<float>(size) / sourceWidth));
        paintPointer(g, value);
        return image;
    }

    static juce::Colour panelColour() { return juce::Colour(0xffeee9e3); }

private:
    struct Pixel
    {
        std::array<float, 3> base {}, delta {};
        float curveLow = 0.0f, invSpan = 0.0f, curveFloor = 0.0f, curveScale = 1.0f;
        float spillBalance = 0.5f, gapWeight = 0.0f;
        juce::uint8 alpha = 255;
    };
    struct Basis { std::vector<Pixel> pixels; };
    juce::Image lit, off;
    std::map<int, Basis> bases;

    static juce::uint8 channel(float f) { return static_cast<juce::uint8>(juce::jlimit(0, 255, juce::roundToInt(f))); }
    static std::array<float, 3> rgb(juce::Colour c) { return { float(c.getRed()), float(c.getGreen()), float(c.getBlue()) }; }

    Basis& getBasis(int size)
    {
        auto existing = bases.find(size);
        if (existing != bases.end()) return existing->second;
        Basis b;
        b.pixels.resize(static_cast<size_t>(size * size));
        juce::Image a(juce::Image::RGB, size, size, true), z(juce::Image::RGB, size, size, true);
        for (auto pair : { std::make_pair(&a, &lit), std::make_pair(&z, &off) })
        {
            juce::Graphics g(*pair.first);
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImage(*pair.second, 0, 0, size, size, 222, 222, 800, 800);
        }
        juce::Image::BitmapData aData(a, juce::Image::BitmapData::readOnly), zData(z, juce::Image::BitmapData::readOnly);
        for (int y = 0, i = 0; y < size; ++y)
            for (int x = 0; x < size; ++x, ++i)
            {
                const float sx = (x + 0.5f) * sourceWidth / size;
                const float sy = (y + 0.5f) * sourceWidth / size;
                const float dx = sx - centreX, dy = sy - centreY;
                const float radius = std::hypot(dx, dy);
                const float degrees = std::atan2(dx, -dy) * 180.0f / juce::MathConstants<float>::pi;
                auto base = rgb(zData.getPixelColour(x, y));
                auto light = rgb(aData.getPixelColour(x, y));

                // Fixed approved face texture; use the neutral companion only
                // where the source's baked 100% pointer/glow has to be removed.
                const float pointerDistance = std::hypot(sx - 504.0f, sy - 505.0f);
                const float pointerHole = 1.0f - smooth(45.0f, 78.0f, pointerDistance);
                const float face = 1.0f - smooth(199.0f, 214.0f, radius);
                for (int c = 0; c < 3; ++c)
                {
                    base[c] += (light[c] - base[c]) * face * (1.0f - pointerHole);
                    light[c] += (base[c] - light[c]) * face;
                }
                auto& p = b.pixels[static_cast<size_t>(i)];
                p.base = base;
                // The neutral cast shadow is fixed. Do not sweep two slightly
                // different baked shadows with a radial pie mask.
                const float lightRegion = 1.0f - smooth(200.0f, 262.0f, dy);
                for (int c = 0; c < 3; ++c) p.delta[c] = (light[c] - base[c]) * lightRegion;
                const float position = juce::jlimit(0.0f, 1.0f, (degrees - startDegrees) / travel);
                // Crisp emissive core, softer neighbouring light spill. No
                // always-on halo: the entire light layer is exactly zero at 0%.
                const float feather = 0.002f + 0.115f * smooth(5.0f, 40.0f, std::abs(radius - 288.0f));
                p.curveLow = position - feather;
                p.invSpan = 1.0f / (2.0f * feather);
                p.curveFloor = smooth(position - feather, position + feather, 0.0f);
                p.curveScale = 1.0f / (smooth(position - feather, position + feather, 1.0f) - p.curveFloor);
                // Avoid atan2's bottom branch cut slicing the metal reflection.
                // There is no lamp in the bottom gap; only smoothly overlapping
                // spill from the two physical arc ends reaches this sidewall.
                p.gapWeight = smooth(130.0f, 151.0f, std::abs(degrees));
                p.spillBalance = smooth(-180.0f, 180.0f, dx);
                p.alpha = channel(255.0f * (1.0f - smooth(345.0f, 402.0f, radius)));
            }
        // Bounded per-editor resolution cache, never a frame-history allocation.
        while (bases.size() >= 4) bases.erase(bases.begin());
        return bases.emplace(size, std::move(b)).first->second;
    }

    static void paintPointer(juce::Graphics& g, float value)
    {
        const float angle = juce::degreesToRadians(angleForValue(value));
        const juce::Point<float> axis(std::sin(angle), -std::cos(angle));
        const juce::Point<float> centre(centreX, centreY);
        // Longer inward-reaching index, retaining the approved face and arc.
        const auto inner = centre + axis * 60.0f;
        const auto outer = centre + axis * 184.0f;
        juce::Path pointer;
        pointer.startNewSubPath(inner);
        pointer.lineTo(outer);
        if (value > 0.0f)
        {
            for (int layer = 12; layer >= 1; --layer)
            {
                const float width = 12.0f + layer * 2.8f;
                g.setColour(juce::Colour(0xffff984a).withAlpha(0.021f * juce::jmin(1.0f, value * 30.0f)));
                g.strokePath(pointer, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            // Readable at native/minimum UI size: a distinct luminous bar,
            // not a thin hairline. Retain the same bounded surrounding halo.
            g.setColour(juce::Colour(0xffffb454));
            g.strokePath(pointer, juce::PathStrokeType(30.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(juce::Colour(0xfffffbed));
            g.strokePath(pointer, juce::PathStrokeType(22.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            // Readable position at zero, but no luminous core or orange glow.
            g.setColour(juce::Colour(0xff8c8072));
            g.strokePath(pointer, juce::PathStrokeType(14.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
};


// Owned by one LookAndFeel/editor and only touched on the message thread.
// Cache by control: repainting the display never recomposes unchanged knobs.
// SafePointer makes removal/recreation of controls safe; assets are embedded.
class Renderer
{
public:
    void draw(juce::Graphics& g, juce::Rectangle<float> bounds,
              float position, juce::Slider& slider)
    {
        position = juce::jlimit(0.0f, 1.0f, position);
        entries.erase(std::remove_if(entries.begin(), entries.end(),
            [](const Entry& e) { return e.slider == nullptr; }), entries.end());
        auto found = std::find_if(entries.begin(), entries.end(),
            [&](const Entry& e) { return e.slider.getComponent() == &slider; });
        if (found == entries.end())
        {
            entries.push_back(Entry{juce::Component::SafePointer<juce::Slider>(&slider)});
            found = entries.end() - 1;
        }
        const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const auto square = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
        const float scale = juce::jmax(1.5f, g.getInternalContext().getPhysicalPixelScaleFactor() * 1.25f);
        const int requested = juce::roundToInt(diameter * scale);
        int resolution = 512;
        for (const int choice : {128, 160, 192, 256, 384, 512})
            if (requested <= choice) { resolution = choice; break; }
        if (!found->image.isValid() || found->resolution != resolution || found->position != position)
        {
            found->image = material.render(position, resolution);
            found->resolution = resolution;
            found->position = position;
            ++renderCount;
        }
        juce::Graphics::ScopedSaveState save(g);
        g.setOpacity(slider.isEnabled() ? 1.0f : 0.43f);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(found->image, square);
    }

    size_t getRenderCount() const noexcept { return renderCount; }
    size_t getControlCacheCount() const noexcept { return entries.size(); }

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
