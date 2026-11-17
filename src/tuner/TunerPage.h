#pragma once

#include "AppContext.h"
#include "core/PitchDetector.h"
#include "ui/Widgets.h"

namespace gg
{

class TunerPage : public juce::Component, private juce::Timer
{
public:
    explicit TunerPage (AppContext&);
    ~TunerPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    AppContext& ctx;
    PitchDetector detector;
    std::vector<float> buffer;
    std::vector<float> recentHz;
    float shownCents = 0.0f, a4 = 440.0f;
    PitchDetector::Note note;
    float hz = 0.0f;
    int silentFrames = 100;
    bool wasMonitoring = true;

    IconButton a4Down { "Lower reference", Icons::arrowLeft() }, a4Up { "Raise reference", Icons::arrowRight() };
    juce::ToggleButton muteToggle { "Mute amp while tuning" };
    juce::Rectangle<int> gaugeArea;

    void timerCallback() override;
};

} // namespace gg
