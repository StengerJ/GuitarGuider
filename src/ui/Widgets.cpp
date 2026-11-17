#include "Widgets.h"

namespace gg
{

//==============================================================================
namespace Icons
{
    using P = juce::Path;

    P cards()
    {
        P p;
        p.addRoundedRectangle (4.0f, 6.0f, 13.0f, 15.0f, 2.0f);
        p.startNewSubPath (8.0f, 3.0f);
        p.lineTo (18.0f, 3.0f);
        p.quadraticTo (20.0f, 3.0f, 20.0f, 5.0f);
        p.lineTo (20.0f, 17.0f);
        p.startNewSubPath (7.5f, 11.0f); p.lineTo (13.5f, 11.0f);
        p.startNewSubPath (7.5f, 15.0f); p.lineTo (11.5f, 15.0f);
        return p;
    }

    P amp()
    {
        P p;
        p.addRoundedRectangle (3.0f, 5.0f, 18.0f, 15.0f, 2.0f);
        p.startNewSubPath (3.0f, 10.0f); p.lineTo (21.0f, 10.0f);
        p.addEllipse (6.0f, 6.6f, 2.0f, 2.0f);
        p.addEllipse (10.0f, 6.6f, 2.0f, 2.0f);
        p.addEllipse (14.0f, 6.6f, 2.0f, 2.0f);
        p.startNewSubPath (7.0f, 13.5f); p.lineTo (17.0f, 13.5f);
        p.startNewSubPath (7.0f, 16.5f); p.lineTo (17.0f, 16.5f);
        return p;
    }

    P tuningFork()
    {
        P p;
        p.startNewSubPath (8.0f, 3.0f);
        p.lineTo (8.0f, 11.0f);
        p.quadraticTo (8.0f, 15.0f, 12.0f, 15.0f);
        p.quadraticTo (16.0f, 15.0f, 16.0f, 11.0f);
        p.lineTo (16.0f, 3.0f);
        p.startNewSubPath (12.0f, 15.0f); p.lineTo (12.0f, 21.0f);
        return p;
    }

    P sliders()
    {
        P p;
        p.startNewSubPath (6.0f, 4.0f); p.lineTo (6.0f, 20.0f);
        p.startNewSubPath (12.0f, 4.0f); p.lineTo (12.0f, 20.0f);
        p.startNewSubPath (18.0f, 4.0f); p.lineTo (18.0f, 20.0f);
        p.addRoundedRectangle (3.5f, 13.0f, 5.0f, 3.0f, 1.0f);
        p.addRoundedRectangle (9.5f, 7.0f, 5.0f, 3.0f, 1.0f);
        p.addRoundedRectangle (15.5f, 15.0f, 5.0f, 3.0f, 1.0f);
        return p;
    }

    P play()     { P p; p.addTriangle (7.0f, 4.5f, 7.0f, 19.5f, 19.5f, 12.0f); return p; }
    P stop()     { P p; p.addRoundedRectangle (6.0f, 6.0f, 12.0f, 12.0f, 1.5f); return p; }
    P record()   { P p; p.addEllipse (5.5f, 5.5f, 13.0f, 13.0f); return p; }
    P rewind()
    {
        P p;
        p.addRectangle (5.0f, 6.0f, 2.2f, 12.0f);
        p.addTriangle (19.0f, 6.0f, 19.0f, 18.0f, 8.5f, 12.0f);
        return p;
    }

    P metronome()
    {
        P p;
        p.startNewSubPath (9.0f, 3.5f);
        p.lineTo (15.0f, 3.5f);
        p.lineTo (19.0f, 20.5f);
        p.lineTo (5.0f, 20.5f);
        p.closeSubPath();
        p.startNewSubPath (12.0f, 16.0f); p.lineTo (17.5f, 6.5f);
        p.startNewSubPath (7.0f, 16.0f); p.lineTo (17.0f, 16.0f);
        return p;
    }

    P plus()  { P p; p.startNewSubPath (12, 5); p.lineTo (12, 19); p.startNewSubPath (5, 12); p.lineTo (19, 12); return p; }
    P close() { P p; p.startNewSubPath (6, 6); p.lineTo (18, 18); p.startNewSubPath (18, 6); p.lineTo (6, 18); return p; }

    P trash()
    {
        P p;
        p.startNewSubPath (4.5f, 7.0f); p.lineTo (19.5f, 7.0f);
        p.startNewSubPath (9.5f, 7.0f); p.lineTo (10.0f, 4.0f); p.lineTo (14.0f, 4.0f); p.lineTo (14.5f, 7.0f);
        p.startNewSubPath (6.5f, 7.0f); p.lineTo (7.5f, 20.0f); p.lineTo (16.5f, 20.0f); p.lineTo (17.5f, 7.0f);
        p.startNewSubPath (10.5f, 11.0f); p.lineTo (10.5f, 16.5f);
        p.startNewSubPath (13.5f, 11.0f); p.lineTo (13.5f, 16.5f);
        return p;
    }

    P speaker()
    {
        P p;
        p.startNewSubPath (4.0f, 9.5f); p.lineTo (8.0f, 9.5f); p.lineTo (12.5f, 5.0f);
        p.lineTo (12.5f, 19.0f); p.lineTo (8.0f, 14.5f); p.lineTo (4.0f, 14.5f); p.closeSubPath();
        p.startNewSubPath (15.5f, 9.0f); p.quadraticTo (17.5f, 12.0f, 15.5f, 15.0f);
        p.startNewSubPath (18.0f, 6.5f); p.quadraticTo (22.0f, 12.0f, 18.0f, 17.5f);
        return p;
    }

    P shuffle()
    {
        P p;
        p.startNewSubPath (3.5f, 7.0f); p.lineTo (7.5f, 7.0f); p.cubicTo (12.0f, 7.0f, 12.0f, 17.0f, 16.5f, 17.0f); p.lineTo (20.0f, 17.0f);
        p.startNewSubPath (3.5f, 17.0f); p.lineTo (7.5f, 17.0f); p.cubicTo (12.0f, 17.0f, 12.0f, 7.0f, 16.5f, 7.0f); p.lineTo (20.0f, 7.0f);
        p.startNewSubPath (17.5f, 4.5f); p.lineTo (20.0f, 7.0f); p.lineTo (17.5f, 9.5f);
        p.startNewSubPath (17.5f, 14.5f); p.lineTo (20.0f, 17.0f); p.lineTo (17.5f, 19.5f);
        return p;
    }

    P arrowLeft()  { P p; p.startNewSubPath (19, 12); p.lineTo (5, 12); p.startNewSubPath (11, 6); p.lineTo (5, 12); p.lineTo (11, 18); return p; }
    P arrowRight() { P p; p.startNewSubPath (5, 12); p.lineTo (19, 12); p.startNewSubPath (13, 6); p.lineTo (19, 12); p.lineTo (13, 18); return p; }
    P check()      { P p; p.startNewSubPath (5, 12.5f); p.lineTo (10, 17.5f); p.lineTo (19, 7); return p; }

    P flip()
    {
        P p;
        p.startNewSubPath (5.0f, 10.0f); p.cubicTo (6.0f, 5.0f, 15.0f, 3.5f, 18.5f, 8.5f);
        p.startNewSubPath (18.5f, 4.5f); p.lineTo (18.5f, 8.5f); p.lineTo (14.5f, 8.5f);
        p.startNewSubPath (19.0f, 14.0f); p.cubicTo (18.0f, 19.0f, 9.0f, 20.5f, 5.5f, 15.5f);
        p.startNewSubPath (5.5f, 19.5f); p.lineTo (5.5f, 15.5f); p.lineTo (9.5f, 15.5f);
        return p;
    }

    P download()
    {
        P p;
        p.startNewSubPath (12, 4); p.lineTo (12, 15);
        p.startNewSubPath (7, 10.5f); p.lineTo (12, 15.5f); p.lineTo (17, 10.5f);
        p.startNewSubPath (5, 19.5f); p.lineTo (19, 19.5f);
        return p;
    }

    P folder()
    {
        P p;
        p.startNewSubPath (3.5f, 7.0f); p.lineTo (3.5f, 18.5f); p.lineTo (20.5f, 18.5f); p.lineTo (20.5f, 8.5f);
        p.lineTo (11.5f, 8.5f); p.lineTo (9.5f, 5.5f); p.lineTo (3.5f, 5.5f); p.closeSubPath();
        return p;
    }

    P guitar()
    {
        P p;
        // body
        p.addEllipse (3.0f, 11.0f, 10.0f, 10.0f);
        p.addEllipse (6.5f, 8.5f, 7.0f, 7.0f);
        // neck
        p.startNewSubPath (11.0f, 13.0f); p.lineTo (19.0f, 5.0f);
        p.startNewSubPath (18.0f, 3.0f); p.lineTo (21.0f, 6.0f);
        return p;
    }

    P power()
    {
        P p;
        p.addCentredArc (12, 13, 7, 7, 0, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
        p.startNewSubPath (12, 3.5f); p.lineTo (12, 11);
        return p;
    }

    void draw (juce::Graphics& g, const juce::Path& icon, juce::Rectangle<float> area, juce::Colour c, float stroke)
    {
        const float s = juce::jmin (area.getWidth(), area.getHeight()) / 24.0f;
        auto xf = juce::AffineTransform::scale (s).translated (area.getCentreX() - 12.0f * s, area.getCentreY() - 12.0f * s);
        g.setColour (c);
        if (stroke <= 0.0f)
            g.fillPath (icon, xf);
        else
            g.strokePath (icon, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), xf);
    }
}

//==============================================================================
IconButton::IconButton (const juce::String& name, juce::Path p, const juce::String& l)
    : juce::Button (name), icon (std::move (p)), label (l)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);
}

void IconButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().toFloat();
    const auto v = getProperties()["variant"].toString();
    const bool on = getToggleState();
    const bool filled = getProperties()["filled"];

    juce::Colour fill = juce::Colours::transparentBlack, fg = over ? ink : ink2;
    if (getProperties().contains ("fg"))
        fg = juce::Colour ((juce::uint32) (juce::int64) getProperties()["fg"]).withAlpha (over ? 1.0f : 0.75f);
    if (v == "solid")      { fill = on ? activeColour : (over ? raised.brighter (0.08f) : raised); fg = on ? Theme::inkOn (activeColour) : ink; }
    else if (v == "primary") { fill = over ? brass.brighter (0.1f) : brass; fg = bg; }
    else if (on)           { fg = activeColour; }
    if (down) fill = fill.isTransparent() ? raised : fill.darker (0.1f);
    if (! isEnabled()) fg = fg.withAlpha (0.35f);

    const float rad = v == "round" || label.isEmpty() ? juce::jmin (r.getWidth(), r.getHeight()) * 0.5f : 8.0f;
    if (! fill.isTransparent() || (over && v != "plain"))
    {
        g.setColour (fill.isTransparent() ? raised.withAlpha (0.7f) : fill);
        g.fillRoundedRectangle (r.reduced (0.5f), rad);
    }

    if (label.isEmpty())
    {
        const float iconSize = juce::jmin (r.getWidth(), r.getHeight()) * 0.58f;
        Icons::draw (g, icon, r.withSizeKeepingCentre (iconSize, iconSize), fg, filled ? 0.0f : 1.8f);
    }
    else
    {
        auto content = r.reduced (12.0f, 0);
        g.setFont (Theme::sans (15.0f, Theme::Weight::semibold));
        const float textW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), label);
        const float iconSize = juce::jmin (20.0f, r.getHeight() * 0.55f);
        const float total = iconSize + 8.0f + textW;
        auto row = content.withSizeKeepingCentre (juce::jmin (total, content.getWidth()), r.getHeight());
        Icons::draw (g, icon, row.removeFromLeft (iconSize).withSizeKeepingCentre (iconSize, iconSize), fg, filled ? 0.0f : 1.8f);
        row.removeFromLeft (8.0f);
        g.setColour (fg);
        g.drawText (label, row, juce::Justification::centredLeft, true);
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (brass);
        g.drawRoundedRectangle (r.reduced (1.0f), rad, 1.5f);
    }
}

//==============================================================================
ParamKnob::ParamKnob (Parameter& p, const juce::String& knobStyle, juce::Colour inkColour)
    : param (p), ink (inkColour), style (knobStyle)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.getProperties().set ("knob", knobStyle);
    slider.getProperties().set ("ink", (juce::int64) ink.getARGB());
    slider.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);

    const auto& spec = param.spec;
    if (! spec.choices.isEmpty())
        slider.setRange (spec.min, spec.max, 1.0);
    else
        slider.setRange (spec.min, spec.max, 0.0);
    slider.setDoubleClickReturnValue (true, spec.def);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (220);
    slider.setPopupDisplayEnabled (true, true, nullptr, 900);
    slider.textFromValueFunction = [this] (double) { return param.toText(); };
    slider.setTooltip (spec.name + " (double-click to reset)");
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    slider.addListener (this);
    addAndMakeVisible (slider);
    refresh();
}

void ParamKnob::refresh()
{
    slider.setValue (param.get(), juce::dontSendNotification);
}

void ParamKnob::sliderValueChanged (juce::Slider*)
{
    param.set ((float) slider.getValue());
}

void ParamKnob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom ((int) captionFont.getHeight() + 4);
    const int size = juce::jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (size, size));
}

void ParamKnob::paint (juce::Graphics& g)
{
    g.setColour (ink.withAlpha (0.9f));
    g.setFont (captionFont);
    g.drawText (param.spec.name, getLocalBounds().removeFromBottom ((int) captionFont.getHeight() + 4),
                juce::Justification::centred, false);
}

//==============================================================================
LevelMeter::LevelMeter (std::function<float()> src, bool isVertical) : source (std::move (src)), vertical (isVertical)
{
    startTimerHz (30);
    setInterceptsMouseClicks (false, false);
}

void LevelMeter::timerCallback()
{
    const float v = source ? source() : 0.0f;
    level = juce::jmax (v, level * 0.82f);
    if (v >= peakHold) { peakHold = v; holdFrames = 30; }
    else if (--holdFrames < 0) peakHold *= 0.92f;
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().toFloat();
    const int segments = 16;
    const float gap = 2.0f;
    const auto db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float norm = juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f);
    const float peakNorm = juce::jmap (juce::Decibels::gainToDecibels (peakHold, -60.0f), -60.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < segments; ++i)
    {
        const float f0 = (float) i / segments;
        juce::Rectangle<float> seg;
        if (vertical)
        {
            const float h = (r.getHeight() - gap * (segments - 1)) / segments;
            seg = { r.getX(), r.getBottom() - (i + 1) * h - i * gap, r.getWidth(), h };
        }
        else
        {
            const float w = (r.getWidth() - gap * (segments - 1)) / segments;
            seg = { r.getX() + i * (w + gap), r.getY(), w, r.getHeight() };
        }
        const juce::Colour hue = f0 > 0.86f ? bad : (f0 > 0.68f ? brass : good);
        const bool lit = f0 < norm;
        const bool peak = ! lit && peakHold > 0.002f && std::abs (f0 - peakNorm) < (1.0f / segments);
        g.setColour (lit || peak ? hue : line.withAlpha (0.6f));
        g.fillRoundedRectangle (seg, 1.5f);
    }
}

//==============================================================================
void drawBadge (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, juce::Colour fill, juce::Colour inkColour)
{
    g.setColour (fill);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (inkColour);
    g.setFont (Theme::sans (r.getHeight() * 0.62f, Theme::Weight::semibold));
    g.drawText (text, r, juce::Justification::centred, false);
}

void drawHeading (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, const juce::String& subtitle)
{
    g.setColour (Theme::Colours::ink);
    g.setFont (Theme::sans (30.0f, Theme::Weight::bold));
    g.drawText (title, r.removeFromTop (38), juce::Justification::centredLeft, true);
    if (subtitle.isNotEmpty())
    {
        g.setColour (Theme::Colours::ink2);
        g.setFont (Theme::sans (16.0f));
        g.drawText (subtitle, r.removeFromTop (24), juce::Justification::centredLeft, true);
    }
}

} // namespace gg
