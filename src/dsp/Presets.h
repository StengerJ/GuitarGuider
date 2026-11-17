#pragma once

#include "SignalChain.h"

namespace gg
{

struct Preset
{
    juce::String name;
    juce::var state;      // SignalChain::toVar() format
    bool factory = true;
    juce::File file;      // for user presets
};

class PresetManager
{
public:
    explicit PresetManager (juce::File userDirectory = defaultDirectory());

    static juce::File defaultDirectory();
    static std::vector<Preset> factoryPresets();

    std::vector<Preset> all() const;    // factory first, then user (alphabetical)
    bool saveUserPreset (const juce::String& name, const SignalChain& chain);
    bool deleteUserPreset (const juce::String& name);

private:
    juce::File dir;
};

} // namespace gg
