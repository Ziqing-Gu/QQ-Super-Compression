#pragma once
#include <cmath>

namespace qqsc
{
    class Biquad
    {
    public:
        void set (double newB0, double newB1, double newB2,
                  double newA1, double newA2) noexcept
        {
            b0 = newB0; b1 = newB1; b2 = newB2;
            a1 = newA1; a2 = newA2;
            reset();
        }

        void reset() noexcept
        {
            x1 = x2 = y1 = y2 = 0.0;
        }

        float process (float input) noexcept
        {
            const auto x0 = static_cast<double> (input);
            const auto y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x0;
            y2 = y1; y1 = y0;
            return static_cast<float> (y0);
        }

    private:
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    };

    class KWeightingFilter
    {
    public:
        void prepare (double sampleRate)
        {
            // De Man / BS.1770-compatible coefficient derivation. At 48 kHz
            // these reproduce the tabulated BS.1770 K-weighting coefficients.
            constexpr double shelfGainDb = 3.99984385397;
            constexpr double shelfQ = 0.7071752369554193;
            constexpr double shelfHz = 1681.9744509555319;
            constexpr double shelfExponent = 0.499666774155;

            const auto kShelf = std::tan (pi * shelfHz / sampleRate);
            const auto vh = std::pow (10.0, shelfGainDb / 20.0);
            const auto vb = std::pow (vh, shelfExponent);
            const auto a0Shelf = 1.0 + kShelf / shelfQ + kShelf * kShelf;

            shelf.set ((vh + vb * kShelf / shelfQ + kShelf * kShelf) / a0Shelf,
                       2.0 * (kShelf * kShelf - vh) / a0Shelf,
                       (vh - vb * kShelf / shelfQ + kShelf * kShelf) / a0Shelf,
                       2.0 * (kShelf * kShelf - 1.0) / a0Shelf,
                       (1.0 - kShelf / shelfQ + kShelf * kShelf) / a0Shelf);

            constexpr double hpQ = 0.5003270373253953;
            constexpr double hpHz = 38.13547087613982;
            const auto kHp = std::tan (pi * hpHz / sampleRate);
            const auto a0Hp = 1.0 + kHp / hpQ + kHp * kHp;

            // This form intentionally keeps the BS.1770 RLB numerator at
            // [1, -2, 1] while the denominator is normalised by a0.
            highPass.set (1.0,
                          -2.0,
                          1.0,
                          2.0 * (kHp * kHp - 1.0) / a0Hp,
                          (1.0 - kHp / hpQ + kHp * kHp) / a0Hp);
        }

        void reset() noexcept
        {
            shelf.reset();
            highPass.reset();
        }

        float process (float input) noexcept
        {
            return highPass.process (shelf.process (input));
        }

    private:
        static constexpr double pi = 3.141592653589793238462643383279502884;
        Biquad shelf;
        Biquad highPass;
    };

}
