#pragma once
#include <JuceHeader.h>
#include "KnobLight.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <vector>

namespace qqsc::dark_refined
{
struct Vec3
{
    float x, y, z;
    Vec3 operator+ (Vec3 v) const noexcept { return { x + v.x, y + v.y, z + v.z }; }
    Vec3 operator* (float k) const noexcept { return { x * k, y * k, z * k }; }
    float dot (Vec3 v) const noexcept { return x * v.x + y * v.y + z * v.z; }
    Vec3 unit() const noexcept
    {
        const auto k = 1.0f / std::sqrt (juce::jmax (0.000001f, dot (*this)));
        return *this * k;
    }
};

inline float smooth (float a, float b, float x) noexcept
{
    const auto t = juce::jlimit (0.0f, 1.0f, (x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}
inline float gaussian (float x, float width) noexcept { return std::exp (-0.5f * x * x / (width * width)); }
inline juce::uint8 channel (float x) noexcept { return static_cast<juce::uint8> (juce::jlimit (0, 255, juce::roundToInt (x))); }

class Material
{
public:
    static constexpr float cx = 50.0f, cy = 46.0f, arcRadius = 33.7f;
    static constexpr float start = -juce::MathConstants<float>::pi * 0.75f;
    static constexpr float travel = juce::MathConstants<float>::pi * 1.5f;
    static float angleForValue (float value) noexcept { return start + value * travel; }

    juce::Image render (float value, int size, float origin = 0.0f)
    {
        value = juce::jlimit (0.0f, 1.0f, value);
        origin = juce::jlimit (0.0f, 1.0f, origin);
        auto& base = baseFor (size);
        juce::Image image (juce::Image::ARGB, size, size, true);
        {
            juce::Image::BitmapData data (image, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < size; ++y)
            {
                auto* row = reinterpret_cast<juce::PixelARGB*> (data.getLinePointer (y));
                for (int x = 0; x < size; ++x)
                {
                    const auto& p = base[static_cast<size_t> (y * size + x)];
                    const auto cumulative = [&p] (float end)
                    {
                        if (end <= 0.0f) return 0.0f;
                        const float finish = angleForValue (end);
                        // The core has a precise endpoint; reflected light has
                        // a softer local falloff, confined to the same surface.
                        const auto lit = smooth (start - 0.006f, start + 0.006f, p.angle)
                            * (1.0f - smooth (finish - p.feather, finish + p.feather, p.angle));
                        return lit * juce::jmin (1.0f, end * 100.0f);
                    };
                    const auto lit = std::abs (cumulative (value) - cumulative (origin));
                    const float spillAlpha = p.spill * lit;
                    const float alpha = p.alpha + spillAlpha * (1.0f - p.alpha);
                    if (alpha < 0.001f) continue;
                    // Surface reflection and the luminous guide are separate
                    // layers; no broad recolour or rotating painted highlight.
                    const auto blue = Vec3 { 79.0f, 155.0f, 195.0f };
                    const auto premul = p.base * p.alpha + blue * (spillAlpha * (1.0f - p.alpha));
                    const auto colour = premul * (1.0f / alpha)
                                      + Vec3 { 70.0f, 154.0f, 194.0f } * (p.reflection * lit)
                                      + Vec3 { 119.0f, 193.0f, 220.0f } * (p.emission * lit);
                    row[x].setARGB (channel (alpha * 255.0f), channel (colour.x), channel (colour.y), channel (colour.z));
                    row[x].premultiply();
                }
            }
        }
        {
            juce::Graphics g (image);
            g.addTransform (juce::AffineTransform::scale (static_cast<float> (size) / 100.0f));
            paintPointer (g, value, std::abs (value - origin));
        }
        return image;
    }
    size_t getBaseCount() const noexcept { return bases.size(); }

private:
    struct Pixel
    {
        Vec3 base {};
        float alpha = 0.0f, angle = 0.0f, emission = 0.0f, reflection = 0.0f, spill = 0.0f, feather = 0.012f;
    };
    std::map<int, std::vector<Pixel>> bases;

    static Vec3 metal (Vec3 normal, float faceRadius, bool bevel)
    {
        const auto view = Vec3 { 0.0f, -0.10f, 1.0f }.unit();
        const auto key = Vec3 { -0.52f, -0.64f, 0.76f }.unit();
        const auto fill = Vec3 { 0.70f, 0.15f, 0.44f }.unit();
        const auto reflected = normal * (2.0f * normal.dot (view)) + view * -1.0f;
        const auto keyHalf = (key + view).unit();
        const float diffuse = juce::jmax (0.0f, normal.dot (key));
        const float rimFill = juce::jmax (0.0f, normal.dot (fill));
        // Large soft studio source + a narrower edge highlight. No surface
        // noise: perceived metal comes from normals, roughness and reflected light.
        const float broad = std::pow (juce::jmax (0.0f, normal.dot (keyHalf)), bevel ? 18.0f : 5.0f);
        const float tight = std::pow (juce::jmax (0.0f, normal.dot (keyHalf)), 95.0f);
        const float environment = 0.5f + 0.5f * juce::jlimit (-1.0f, 1.0f, -reflected.y);
        const float fade = bevel ? 1.0f : 1.0f - 0.13f * faceRadius * faceRadius;
        const float tone = (20.0f + 22.0f * diffuse + 6.0f * rimFill + 8.0f * environment
                           + broad * (bevel ? 34.0f : 16.0f) + tight * (bevel ? 35.0f : 0.0f)) * fade;
        return { tone * 0.96f, tone, tone * 1.026f };
    }

    std::vector<Pixel>& baseFor (int size)
    {
        if (auto found = bases.find (size); found != bases.end()) return found->second;
        std::vector<Pixel> pixels (static_cast<size_t> (size * size));
        const float aa = 75.0f / static_cast<float> (size);
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const float px = (x + 0.5f) * 100.0f / size;
                const float py = (y + 0.5f) * 100.0f / size;
                const float dx = px - cx, dy = py - cy;
                const float r = std::hypot (dx, dy);
                auto& p = pixels[static_cast<size_t> (y * size + x)];
                p.angle = std::atan2 (dx, -dy);

                // Soft cast shadow offset down/right and a close, darker
                // contact shadow. Both are stationary and smoothly tapered.
                const float shadowR = std::hypot ((dx - 1.9f) / 1.06f, (dy - 6.4f) / 0.98f);
                const float softShadow = 0.33f * (1.0f - smooth (25.0f, 43.0f, shadowR));
                const float contactR = std::hypot (dx - 0.5f, dy - 4.2f);
                const float contact = 0.48f * (1.0f - smooth (28.0f, 34.8f, contactR));
                p.alpha = 1.0f - (1.0f - softShadow) * (1.0f - contact);
                p.base = { 0, 0, 0 };
                const auto over = [&] (Vec3 c, float alpha)
                {
                    const float combined = alpha + p.alpha * (1.0f - alpha);
                    if (combined > 0.0f) p.base = (c * alpha + p.base * (p.alpha * (1.0f - alpha))) * (1.0f / combined);
                    p.alpha = combined;
                };

                // A low, dark mounting socket. Unlike the original multiple
                // circles, only one restrained channel surrounds the raised cap.
                const float socket = 1.0f - smooth (34.7f - aa, 34.7f + aa, r);
                const float lightSide = juce::jlimit (0.0f, 1.0f, (-dx * 0.5f - dy * 0.8f) / 35.0f + 0.20f);
                const float socketValue = 15.0f + 12.0f * lightSide;
                over ({ socketValue, socketValue + 1, socketValue + 2 }, socket);

                // The lower cylindrical wall is projected below the raised
                // top face, creating actual visible thickness rather than a line.
                const float sideR = std::hypot (dx, dy - 3.7f);
                const float sideMask = 1.0f - smooth (30.7f - aa, 30.7f + aa, sideR);
                const float wallLight = 21.0f + 15.0f * juce::jlimit (0.0f, 1.0f, (-dx + 10.0f) / 55.0f);
                over ({ wallLight * 0.94f, wallLight, wallLight * 1.04f }, sideMask);

                const float faceMask = 1.0f - smooth (30.5f - aa, 30.5f + aa, r);
                if (faceMask > 0.0f)
                {
                    const float rr = juce::jmax (0.001f, r);
                    const float bevelFraction = smooth (25.8f, 30.7f, r);
                    const float slope = 0.02f + bevelFraction * 1.50f;
                    Vec3 normal { (dx / rr) * std::sin (slope), (dy / rr) * std::sin (slope), std::cos (slope) };
                    // Subtle convex crown changes reflection across the satin
                    // face; it is not a radial painted light spot.
                    if (r < 25.8f) normal = Vec3 { dx * 0.0040f, dy * 0.0040f, 1.0f }.unit();
                    auto colour = metal (normal, r / 30.5f, r > 25.8f);
                    // A tiny rounded break between the satin face and polished
                    // bevel, not a constant-brightness concentric outline.
                    const float breakShade = 1.0f - gaussian (r - 25.9f, 0.28f) * 0.10f;
                    colour = colour * breakShade;
                    over (colour, faceMask);
                }

                const float inArc = smooth (start - 0.006f, start + 0.006f, p.angle)
                                  * (1.0f - smooth (start + travel - 0.006f, start + travel + 0.006f, p.angle));
                // The unlit recessed lens is almost black, with just a physical
                // glass edge; no blue base and no always-on halo.
                const float groove = gaussian (r - arcRadius, 0.57f) * inArc;
                over ({ 9, 12, 14 }, groove * 0.78f);
                const float glassEdge = gaussian (r - (arcRadius - 0.69f), 0.17f) * inArc;
                over ({ 47, 51, 54 }, glassEdge * 0.30f);

                p.emission = inArc * (gaussian (r - arcRadius, 0.30f) * 0.88f
                                   + gaussian (r - arcRadius, 0.85f) * 0.17f);
                // The narrow reflected band follows the visible bevel, with
                // a dim, rougher tail onto the neighbouring face and sidewall.
                p.reflection = inArc * (gaussian (r - 30.0f, 0.95f) * 0.24f
                                    + gaussian (r - 28.8f, 1.5f) * 0.045f);
                p.spill = inArc * gaussian (r - arcRadius, 2.1f) * 0.090f;
                p.feather = 0.010f + 0.045f * smooth (0.7f, 3.0f, std::abs (r - arcRadius));
            }
        while (bases.size() >= 4) bases.erase (bases.begin());
        return bases.emplace (size, std::move (pixels)).first->second;
    }

    static void paintPointer (juce::Graphics& g, float value, float strength)
    {
        const auto angle = angleForValue (value);
        const auto axis = juce::Point<float> (std::sin (angle), -std::cos (angle));
        const auto centre = juce::Point<float> (cx, cy);
        juce::Path pointer;
        pointer.startNewSubPath (centre + axis * 18.1f);
        pointer.lineTo (centre + axis * 22.6f);
        const auto stroke = [&] (juce::Colour c, float width)
        {
            g.setColour (c);
            g.strokePath (pointer, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        // Recessed index with a narrow lip. It stays readable at zero, unlit.
        stroke (juce::Colour (0xff636a6e).withAlpha (0.45f), 2.5f);
        stroke (juce::Colour (0xff14191d), 1.95f);
        if (strength <= 0.0f) { stroke (juce::Colour (0xff535d63), 0.62f); return; }
        stroke (juce::Colour (0xff69b1d4).withAlpha (0.075f), 4.3f);
        stroke (juce::Colour (0xff467e9b).withAlpha (0.70f), 1.8f);
        stroke (juce::Colour (0xff9dd8eb), 0.82f);
    }
};

class Renderer
{
public:
    void draw (juce::Graphics& g, juce::Rectangle<float> bounds, float value, juce::Slider& control)
    {
        value = juce::jlimit (0.0f, 1.0f, value);
        const auto origin = knob_light::origin (control);
        entries.erase (std::remove_if (entries.begin(), entries.end(), [] (const Entry& e) { return e.control == nullptr; }), entries.end());
        auto entry = std::find_if (entries.begin(), entries.end(), [&] (const Entry& e) { return e.control.getComponent() == &control; });
        if (entry == entries.end()) { entries.push_back ({ juce::Component::SafePointer<juce::Slider> (&control) }); entry = entries.end() - 1; }
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto requested = juce::roundToInt (diameter * juce::jmax (1.5f, g.getInternalContext().getPhysicalPixelScaleFactor() * 1.25f));
        int resolution = 512;
        for (int choice : { 128, 160, 192, 256, 384, 512 }) if (requested <= choice) { resolution = choice; break; }
        if (! entry->image.isValid() || entry->resolution != resolution || entry->value != value || entry->origin != origin)
        {
            entry->image = material.render (value, resolution, origin);
            entry->resolution = resolution;
            entry->value = value;
            entry->origin = origin;
            ++renders;
        }
        juce::Graphics::ScopedSaveState save (g);
        g.setOpacity (control.isEnabled() ? 1.0f : 0.4f);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (entry->image, juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre()));
    }
    size_t getRenderCount() const noexcept { return renders; }
private:
    struct Entry { juce::Component::SafePointer<juce::Slider> control; juce::Image image; int resolution = 0; float value = -1.0f; float origin = -1.0f; };
    Material material;
    std::vector<Entry> entries;
    size_t renders = 0;
};
}
