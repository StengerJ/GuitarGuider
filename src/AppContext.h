#pragma once

#include "audio/AudioEngine.h"
#include "core/StudyProgress.h"
#include "dsp/Presets.h"

namespace gg
{

/** Long-lived services shared by every page. */
struct AppContext
{
    AudioEngine& audio;
    StudyProgress& progress;
    PresetManager& presets;
    juce::PropertiesFile& settings;
    std::function<void (int pageIndex)> showPage;   // 0 Learn, 1 Studio, 2 Tuner, 3 Settings
};

} // namespace gg
