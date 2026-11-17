#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gg
{

/** Design tokens. The app chrome is "amp tolex": blue-grey, warm ink, brass trim,
    with an orange tube-glow reserved for live/recording states. */
namespace Theme
{
    namespace Colours
    {
        inline const juce::Colour bg        { 0xff1d2127 };   // window / tolex
        inline const juce::Colour panel     { 0xff252a31 };   // raised surfaces
        inline const juce::Colour raised    { 0xff2e343c };   // controls, hover
        inline const juce::Colour line      { 0xff3a414a };   // hairlines
        inline const juce::Colour ink       { 0xffece6da };   // primary text
        inline const juce::Colour ink2      { 0xffaaa59b };   // secondary text
        inline const juce::Colour ink3      { 0xff7b7b78 };   // tertiary / disabled
        inline const juce::Colour brass     { 0xffd9a441 };   // accent, selection
        inline const juce::Colour brassDeep { 0xff8e6c2c };
        inline const juce::Colour glow      { 0xffff7a2f };   // record, live LEDs
        inline const juce::Colour good      { 0xff7cc48a };
        inline const juce::Colour bad       { 0xffe36a5e };
        inline const juce::Colour card      { 0xffefebe2 };   // printed chord-chart card
        inline const juce::Colour cardInk   { 0xff20242a };
        inline const juce::Colour cardInk2  { 0xff6d6a63 };
    }

    // Type: Barlow for UI text, Barlow Condensed for gear-style labels and numerals.
    enum class Weight { regular, medium, semibold, bold };
    juce::Font sans (float height, Weight w = Weight::regular);
    juce::Font cond (float height, Weight w = Weight::semibold);

    // Spacing scale
    inline constexpr int s1 = 4, s2 = 8, s3 = 12, s4 = 16, s5 = 24, s6 = 32, s7 = 48;
    inline constexpr float radiusSm = 6.0f, radiusMd = 10.0f, radiusLg = 16.0f;

    /** Black or white text, whichever reads better on the given fill. */
    juce::Colour inkOn (juce::Colour fill);

    /** A soft drop shadow for floating things (cards, popovers). */
    void dropShadow (juce::Graphics& g, juce::Rectangle<float> r, float radius, float strength = 0.45f);
}

class GuitarGuiderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GuitarGuiderLookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

    // Buttons: set component property "variant" to "primary", "ghost", "subtle", "danger", "tab" or "chip"
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;

    // Knobs: property "knob" = "amp" (chicken-head on a cream panel), "pedal" (small black), default = flat dial
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override { return 6; }

    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return 10; }

    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;

    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;

    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;

    void drawFocusOutline (juce::Graphics& g, juce::Rectangle<float> r, float radius);

private:
    juce::Typeface::Ptr regular, medium, semibold, bold;
};

} // namespace gg
