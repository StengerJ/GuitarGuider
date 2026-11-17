#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace gg
{

/** Allocation-free RBJ biquad, safe to retune on the audio thread. */
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void reset() noexcept { z1 = z2 = 0; }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void processBlock (float* data, int n) noexcept
    {
        for (int i = 0; i < n; ++i) data[i] = process (data[i]);
    }

    enum class Type { lowPass, highPass, bandPass, peak, lowShelf, highShelf, notch };

    void set (Type type, double sr, double freq, double q, double gainDb = 0.0) noexcept
    {
        freq = juce::jlimit (10.0, sr * 0.49, freq);
        q = juce::jmax (0.05, q);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = juce::MathConstants<double>::twoPi * freq / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * q);
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;

        switch (type)
        {
            case Type::lowPass:
                B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = (1 - cw) / 2;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::highPass:
                B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = (1 + cw) / 2;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::bandPass:
                B0 = alpha; B1 = 0; B2 = -alpha;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::notch:
                B0 = 1; B1 = -2 * cw; B2 = 1;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::peak:
                B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A;
                A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
            case Type::lowShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) - (A - 1) * cw + s);
                B1 = 2 * A * ((A - 1) - (A + 1) * cw);
                B2 = A * ((A + 1) - (A - 1) * cw - s);
                A0 = (A + 1) + (A - 1) * cw + s;
                A1 = -2 * ((A - 1) + (A + 1) * cw);
                A2 = (A + 1) + (A - 1) * cw - s; break;
            }
            case Type::highShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) + (A - 1) * cw + s);
                B1 = -2 * A * ((A - 1) + (A + 1) * cw);
                B2 = A * ((A + 1) + (A - 1) * cw - s);
                A0 = (A + 1) - (A - 1) * cw + s;
                A1 = 2 * ((A - 1) - (A + 1) * cw);
                A2 = (A + 1) - (A - 1) * cw - s; break;
            }
        }

        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
        a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
};

/** One-pole lowpass / DC blocker helpers. */
struct OnePole
{
    float a = 0.0f, z = 0.0f;
    void setLowPass (double sr, double hz) noexcept { a = (float) std::exp (-juce::MathConstants<double>::twoPi * hz / sr); }
    inline float lp (float x) noexcept { z = x + a * (z - x); return z; }
    void reset() noexcept { z = 0; }
};

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

} // namespace gg
