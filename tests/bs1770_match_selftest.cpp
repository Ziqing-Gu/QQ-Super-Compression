// Standalone (no JUCE required) sanity test for QQ Super Compression 0.1.6.
// Example:
//   c++ -std=c++17 -O2 tests/bs1770_match_selftest.cpp -I. -o bs1770_test
//   ./bs1770_test

#define private public
#include "Source/BS1770LoudnessMatch.h"
#undef private

#include <cmath>
#include <iostream>

namespace
{
constexpr double pi = 3.141592653589793238462643383279502884;

bool near (double a, double b, double tolerance)
{
    return std::abs (a - b) <= tolerance;
}
}

int main()
{
    constexpr double sr = 48000.0;

    // BS.1770 reference point: 0 dBFS 1 kHz sine in one front channel is
    // approximately -3.01 LKFS/LUFS.
    qqsc::BS1770LoudnessMatch reference;
    reference.prepare (sr);
    for (int n = 0; n < static_cast<int> (sr * 5.0); ++n)
    {
        const auto sample = static_cast<float> (std::sin (2.0 * pi * 1000.0 * n / sr));
        reference.processSample (sample, 0.0f,
                                 sample, 0.0f,
                                 sample, 0.0f,
                                 sample, 0.0f,
                                 sample, 0.0f);
    }

    const auto referenceLufs = reference.integratedFor (&qqsc::BS1770LoudnessMatch::BlockEnergies::dryL);
    std::cout << "1 kHz 0 dBFS mono: " << referenceLufs << " LUFS\n";
    if (! near (referenceLufs, -3.01, 0.05))
        return 1;

    // Fixed 6 dB difference should produce a 6 dB Makeup target.
    qqsc::BS1770LoudnessMatch match;
    match.prepare (sr);
    const auto dryAmp = std::pow (10.0f, -20.0f / 20.0f);
    const auto wetAmp = std::pow (10.0f, -26.0f / 20.0f);

    for (int n = 0; n < static_cast<int> (sr * 10.0); ++n)
    {
        const auto carrier = static_cast<float> (std::sin (2.0 * pi * 1000.0 * n / sr));
        const auto dry = dryAmp * carrier;
        const auto wet = wetAmp * carrier;
        match.processSample (dry, dry,
                             wet, wet,
                             wet, wet,
                             dry, 0.0f,
                             wet, 0.0f);
    }

    const auto result = match.getLatestMatch();
    std::cout << "6 dB fixed difference, ST Match: " << result.st << " dB\n";
    if (! result.validST || ! near (result.st, 6.0, 0.03))
        return 2;

    std::cout << "PASS\n";
    return 0;
}
