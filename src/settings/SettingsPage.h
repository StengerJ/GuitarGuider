#pragma once

#include "AppContext.h"
#include "ui/Widgets.h"

namespace gg
{

class SettingsPage : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit SettingsPage (AppContext&);
    ~SettingsPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AppContext& ctx;
    juce::ComboBox driverBox, inputBox, outputBox, channelBox, rateBox, bufferBox;
    juce::Slider inputGain, outputVolume;
    juce::ToggleButton monitorToggle { "Hear the amp while you play" };
    juce::ToggleButton demoToggle { "Use the demo guitar instead of the input" };
    juce::TextButton testButton { "Play test sound" }, resetButton { "Reset study progress" };
    LevelMeter inputMeter;
    juce::Rectangle<int> audioCard, playCard, studyCard;
    bool updating = false;

    void refresh();
    void applySetup (std::function<void (juce::AudioDeviceManager::AudioDeviceSetup&)> change);
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override { repaint (audioCard); }
};

} // namespace gg
