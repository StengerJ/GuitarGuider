#pragma once

#include "AppContext.h"
#include "ui/Widgets.h"

namespace gg
{

/** Left navigation rail + the active page. */
class MainComponent : public juce::Component, private juce::Timer
{
public:
    MainComponent (AudioEngine&, StudyProgress&, PresetManager&, juce::PropertiesFile&, const juce::String& commandLine);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void showPage (int index);

private:
    struct NavItem : public juce::Button
    {
        NavItem (const juce::String& name, juce::Path icon);
        void paintButton (juce::Graphics&, bool over, bool down) override;
        juce::Path icon;
    };

    AppContext ctx;
    juce::OwnedArray<NavItem> nav;
    std::unique_ptr<juce::Component> pages[4];
    int current = -1;
    LevelMeter inputMeter;
    juce::TooltipWindow tooltips { this, 600 };
    juce::String screenshotDir;

    juce::Component* page (int index);
    void timerCallback() override;   // used for --screenshot mode
};

} // namespace gg
