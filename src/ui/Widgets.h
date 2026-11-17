#pragma once

#include "Theme.h"
#include "dsp/Effects.h"

namespace gg
{

/** Vector icons drawn at any size (24x24 design grid). */
namespace Icons
{
    juce::Path cards();      // Learn
    juce::Path amp();        // Studio
    juce::Path tuningFork(); // Tuner
    juce::Path sliders();    // Settings
    juce::Path play();
    juce::Path stop();
    juce::Path record();
    juce::Path rewind();
    juce::Path metronome();
    juce::Path plus();
    juce::Path close();
    juce::Path trash();
    juce::Path speaker();
    juce::Path shuffle();
    juce::Path arrowLeft();
    juce::Path arrowRight();
    juce::Path check();
    juce::Path flip();
    juce::Path download();
    juce::Path folder();
    juce::Path guitar();
    juce::Path power();

    void draw (juce::Graphics&, const juce::Path& icon, juce::Rectangle<float> area, juce::Colour, float stroke = 1.8f);
}

/** Icon-only (or icon + label) button. Property "variant" works as for TextButton. */
class IconButton : public juce::Button
{
public:
    IconButton (const juce::String& name, juce::Path icon, const juce::String& label = {});
    void setIcon (juce::Path p) { icon = std::move (p); repaint(); }
    void setLabel (const juce::String& l) { label = l; repaint(); }
    void setActiveColour (juce::Colour c) { activeColour = c; repaint(); }
    void paintButton (juce::Graphics&, bool over, bool down) override;

private:
    juce::Path icon;
    juce::String label;
    juce::Colour activeColour = Theme::Colours::brass;
};

/** Rotary knob bound to a gg::Parameter with a caption underneath. */
class ParamKnob : public juce::Component, private juce::Slider::Listener
{
public:
    ParamKnob (Parameter& p, const juce::String& style, juce::Colour ink);
    void refresh();                 // pull value from the parameter
    void resized() override;
    void paint (juce::Graphics&) override;
    void setCaptionFont (juce::Font f) { captionFont = f; }
    void setInk (juce::Colour c)
    {
        ink = c;
        slider.getProperties().set ("ink", (juce::int64) c.getARGB());
        slider.repaint();
        repaint();
    }

    juce::Slider slider;

private:
    Parameter& param;
    juce::Colour ink;
    juce::Font captionFont = Theme::cond (13.0f);
    juce::String style;
    void sliderValueChanged (juce::Slider*) override;
};

/** Segmented LED-style level meter. */
class LevelMeter : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit LevelMeter (std::function<float()> source, bool vertical = false);
    void paint (juce::Graphics&) override;

private:
    std::function<float()> source;
    bool vertical;
    float level = 0.0f, peakHold = 0.0f;
    int holdFrames = 0;
    void timerCallback() override;
};

/** Small "pill" text badge. */
void drawBadge (juce::Graphics&, juce::Rectangle<float> r, const juce::String& text, juce::Colour fill, juce::Colour ink);

/** Section heading: large text with an optional secondary line. */
void drawHeading (juce::Graphics&, juce::Rectangle<int> r, const juce::String& title, const juce::String& subtitle = {});

/** Helper to set a button's variant property. */
template <typename B> B& variant (B& b, const char* v) { b.getProperties().set ("variant", v); return b; }

} // namespace gg
