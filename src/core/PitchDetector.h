#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace gg
{

/** YIN monophonic pitch detector (used by the tuner). */
class PitchDetector
{
public:
    struct Result
    {
        float frequency = 0.0f;  // Hz, 0 if no pitch
        float confidence = 0.0f; // 0..1
    };

    Result detect (const float* samples, int numSamples, double sampleRate,
                   float minHz = 60.0f, float maxHz = 1200.0f);

    struct Note
    {
        int midi = 0;
        float cents = 0.0f;
        juce::String name;       // e.g. "E"
        int octave = 0;
    };
    static Note toNote (float frequency, float a4 = 440.0f);

private:
    std::vector<float> diff;
};

} // namespace gg
