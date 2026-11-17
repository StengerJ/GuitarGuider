#pragma once

#include "Theme.h"
#include "core/ChordLibrary.h"

namespace gg
{

/** Draws a vertical chord box: six strings, nut or fret marker, finger dots and barres. */
namespace ChordDiagram
{
    struct Style
    {
        juce::Colour ink = Theme::Colours::cardInk;
        juce::Colour dot = Theme::Colours::cardInk;
        juce::Colour dotText = Theme::Colours::card;
        juce::Colour muted = Theme::Colours::cardInk2;
        bool showFingers = true;
        bool showStringNames = false;
    };

    void draw (juce::Graphics&, juce::Rectangle<float> area, const Chord&, const Style& = {});
}

} // namespace gg
