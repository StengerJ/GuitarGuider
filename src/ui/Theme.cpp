#include "Theme.h"
#include "BinaryData.h"

namespace gg
{

namespace
{
    struct Typefaces
    {
        juce::Typeface::Ptr regular, medium, semibold, bold, condMedium, condSemibold, condBold;

        Typefaces()
        {
            auto load = [] (const char* data, int size)
            { return juce::Typeface::createSystemTypefaceFor (data, (size_t) size); };

            regular      = load (GGBinary::Barlow_Regular_ttf, GGBinary::Barlow_Regular_ttfSize);
            medium       = load (GGBinary::Barlow_Medium_ttf, GGBinary::Barlow_Medium_ttfSize);
            semibold     = load (GGBinary::Barlow_SemiBold_ttf, GGBinary::Barlow_SemiBold_ttfSize);
            bold         = load (GGBinary::Barlow_Bold_ttf, GGBinary::Barlow_Bold_ttfSize);
            condMedium   = load (GGBinary::BarlowCondensed_Medium_ttf, GGBinary::BarlowCondensed_Medium_ttfSize);
            condSemibold = load (GGBinary::BarlowCondensed_SemiBold_ttf, GGBinary::BarlowCondensed_SemiBold_ttfSize);
            condBold     = load (GGBinary::BarlowCondensed_Bold_ttf, GGBinary::BarlowCondensed_Bold_ttfSize);
        }

        static Typefaces& get() { static Typefaces t; return t; }
    };

    juce::Font makeFont (juce::Typeface::Ptr tf, float h)
    {
        return juce::Font (juce::FontOptions (tf).withHeight (h));
    }
}

juce::Font Theme::sans (float h, Weight w)
{
    auto& t = Typefaces::get();
    switch (w)
    {
        case Weight::regular:  return makeFont (t.regular, h);
        case Weight::medium:   return makeFont (t.medium, h);
        case Weight::semibold: return makeFont (t.semibold, h);
        case Weight::bold:     return makeFont (t.bold, h);
    }
    return makeFont (t.regular, h);
}

juce::Font Theme::cond (float h, Weight w)
{
    auto& t = Typefaces::get();
    switch (w)
    {
        case Weight::regular:
        case Weight::medium:   return makeFont (t.condMedium, h);
        case Weight::semibold: return makeFont (t.condSemibold, h);
        case Weight::bold:     return makeFont (t.condBold, h);
    }
    return makeFont (t.condSemibold, h);
}

juce::Colour Theme::inkOn (juce::Colour fill)
{
    return fill.getPerceivedBrightness() > 0.62f ? Colours::cardInk : Colours::ink;
}

void Theme::dropShadow (juce::Graphics& g, juce::Rectangle<float> r, float radius, float strength)
{
    for (int i = 4; i >= 1; --i)
    {
        const float grow = (float) i * 2.5f;
        g.setColour (juce::Colours::black.withAlpha (strength * 0.09f * (float) (5 - i)));
        g.fillRoundedRectangle (r.expanded (grow * 0.6f).translated (0, grow * 0.8f), radius + grow);
    }
}

//==============================================================================
GuitarGuiderLookAndFeel::GuitarGuiderLookAndFeel()
{
    using namespace Theme::Colours;
    auto& t = Typefaces::get();
    regular = t.regular; medium = t.medium; semibold = t.semibold; bold = t.bold;

    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::DocumentWindow::textColourId, ink);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::TextButton::buttonColourId, raised);
    setColour (juce::TextButton::buttonOnColourId, brass);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, bg);
    setColour (juce::ToggleButton::textColourId, ink);
    setColour (juce::ToggleButton::tickColourId, brass);
    setColour (juce::ComboBox::backgroundColourId, raised);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::arrowColourId, ink2);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, raised);
    setColour (juce::PopupMenu::highlightedTextColourId, ink);
    setColour (juce::TextEditor::backgroundColourId, bg);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::outlineColourId, line);
    setColour (juce::TextEditor::focusedOutlineColourId, brass);
    setColour (juce::TextEditor::highlightColourId, brass.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, brass);
    setColour (juce::ScrollBar::thumbColourId, line);
    setColour (juce::Slider::thumbColourId, brass);
    setColour (juce::Slider::trackColourId, brass);
    setColour (juce::Slider::backgroundColourId, line);
    setColour (juce::Slider::rotarySliderFillColourId, brass);
    setColour (juce::Slider::rotarySliderOutlineColourId, line);
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::BubbleComponent::backgroundColourId, bg.darker (0.3f));
    setColour (juce::BubbleComponent::outlineColourId, line);
    setColour (juce::TooltipWindow::backgroundColourId, bg.darker (0.3f));
    setColour (juce::TooltipWindow::textColourId, ink);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, ink);
    setColour (juce::AlertWindow::outlineColourId, line);
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

    auto scheme = getDarkColourScheme();
    scheme.setUIColour (ColourScheme::windowBackground, bg);
    scheme.setUIColour (ColourScheme::widgetBackground, raised);
    scheme.setUIColour (ColourScheme::menuBackground, panel);
    scheme.setUIColour (ColourScheme::outline, line);
    scheme.setUIColour (ColourScheme::defaultText, ink);
    scheme.setUIColour (ColourScheme::defaultFill, brass);
    scheme.setUIColour (ColourScheme::highlightedText, ink);
    scheme.setUIColour (ColourScheme::highlightedFill, raised);
    scheme.setUIColour (ColourScheme::menuText, ink);
    setColourScheme (scheme);
}

juce::Typeface::Ptr GuitarGuiderLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()
        || f.getTypefaceName() == "<Sans-Serif>")
        return f.isBold() ? bold : regular;
    return LookAndFeel_V4::getTypefaceForFont (f);
}

void GuitarGuiderLookAndFeel::drawFocusOutline (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour (Theme::Colours::brass.withAlpha (0.9f));
    g.drawRoundedRectangle (r.reduced (1.0f), radius, 2.0f);
}

//==============================================================================
void GuitarGuiderLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    using namespace Theme::Colours;
    auto r = b.getLocalBounds().toFloat();
    const auto variant = b.getProperties()["variant"].toString();
    const bool on = b.getToggleState();
    const float alpha = b.isEnabled() ? 1.0f : 0.4f;

    if (variant == "tab")
    {
        if (on || over)
        {
            g.setColour ((on ? brass : ink3).withAlpha (alpha));
            g.fillRoundedRectangle (r.removeFromBottom (3.0f).reduced (r.getWidth() * 0.08f, 0), 1.5f);
        }
        return;
    }

    if (variant == "chip")
    {
        const float rad = r.getHeight() * 0.5f;
        if (on)
        {
            g.setColour (brass.withAlpha (alpha));
            g.fillRoundedRectangle (r.reduced (0.5f), rad);
        }
        else
        {
            g.setColour ((over ? raised.brighter (0.08f) : raised).withAlpha (alpha));
            g.fillRoundedRectangle (r.reduced (0.5f), rad);
        }
        if (b.hasKeyboardFocus (false)) drawFocusOutline (g, r, rad);
        return;
    }

    const float rad = 8.0f;
    juce::Colour fill;
    juce::Colour border = juce::Colours::transparentBlack;

    if (variant == "primary")
        fill = down ? brass.darker (0.15f) : (over ? brass.brighter (0.1f) : brass);
    else if (variant == "ghost" || variant == "danger")
    {
        fill = over ? raised : juce::Colours::transparentBlack;
        border = variant == "danger" ? bad.withAlpha (0.6f) : line;
        if (down) fill = raised.brighter (0.05f);
    }
    else if (variant == "record")
        fill = on ? glow : (over ? raised.brighter (0.08f) : raised);
    else // subtle / default
        fill = on ? brass : (down ? raised.darker (0.1f) : (over ? raised.brighter (0.07f) : raised));

    g.setColour (fill.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (r.reduced (0.5f), rad);
    if (! border.isTransparent())
    {
        g.setColour (border.withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    if (b.hasKeyboardFocus (false)) drawFocusOutline (g, r, rad);
}

juce::Font GuitarGuiderLookAndFeel::getTextButtonFont (juce::TextButton& b, int h)
{
    const auto variant = b.getProperties()["variant"].toString();
    if (variant == "tab") return Theme::sans (juce::jmin (17.0f, h * 0.5f), Theme::Weight::semibold);
    return Theme::sans (juce::jmin (16.0f, h * 0.44f), Theme::Weight::semibold);
}

void GuitarGuiderLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool)
{
    using namespace Theme::Colours;
    const auto variant = b.getProperties()["variant"].toString();
    const bool on = b.getToggleState();
    juce::Colour c = ink;

    if (variant == "primary") c = bg;
    else if (variant == "danger") c = bad;
    else if (variant == "tab") c = on ? ink : (over ? ink : ink2);
    else if ((variant == "chip" || variant.isEmpty() || variant == "subtle") && on) c = bg;
    else if (variant == "record" && on) c = bg;

    if (! b.isEnabled()) c = c.withAlpha (0.4f);
    g.setColour (c);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (10, 0), juce::Justification::centred, 1, 0.9f);
}

void GuitarGuiderLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    using namespace Theme::Colours;
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();

    // Switch on the right, label on the left
    auto sw = r.removeFromRight (40.0f).withSizeKeepingCentre (38.0f, 22.0f);
    g.setColour (on ? brass : (over ? raised.brighter (0.1f) : raised));
    g.fillRoundedRectangle (sw, 11.0f);
    g.setColour (line);
    if (! on) g.drawRoundedRectangle (sw, 11.0f, 1.0f);
    auto knob = juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ on ? sw.getRight() - 11.0f : sw.getX() + 11.0f, sw.getCentreY() });
    g.setColour (on ? bg : ink2);
    g.fillEllipse (knob);
    if (b.hasKeyboardFocus (false)) drawFocusOutline (g, sw.expanded (2.0f), 13.0f);

    g.setColour (b.isEnabled() ? ink : ink3);
    g.setFont (Theme::sans (15.0f, Theme::Weight::medium));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedRight (8), juce::Justification::centredLeft, 2);
}

//==============================================================================
void GuitarGuiderLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                                float start, float end, juce::Slider& s)
{
    using namespace Theme::Colours;
    const auto style = s.getProperties()["knob"].toString();
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);

    if (style == "amp")
    {
        // Scale ticks printed on the faceplate, numbered 0-10
        const juce::Colour faceInk = s.getProperties().contains ("ink")
            ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["ink"]) : cardInk;
        const float rOuter = size * 0.5f;
        const float rKnob = rOuter * 0.66f;

        g.setFont (Theme::cond (juce::jmax (9.0f, size * 0.12f), Theme::Weight::semibold));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = start + (float) i / 10.0f * (end - start);
            const auto dir = juce::Point<float> (std::sin (a), -std::cos (a));
            g.setColour (faceInk.withAlpha (i % 5 == 0 ? 0.85f : 0.45f));
            g.drawLine (juce::Line<float> (centre + dir * (rKnob + 3.0f), centre + dir * (rKnob + (i % 5 == 0 ? 8.0f : 6.0f))),
                        i % 5 == 0 ? 1.6f : 1.1f);
            if (i % 5 == 0 && size > 60)
            {
                auto p = centre + dir * (rOuter - 2.0f);
                g.drawText (juce::String (i), juce::Rectangle<float> (16, 12).withCentre (p), juce::Justification::centred);
            }
        }

        // Knob skirt with a soft shadow
        auto knob = juce::Rectangle<float> (rKnob * 2, rKnob * 2).withCentre (centre);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (knob.translated (0, 2.5f).expanded (1.5f));
        juce::ColourGradient grad (juce::Colour (0xff3b3a38), knob.getX(), knob.getY(),
                                   juce::Colour (0xff121212), knob.getRight(), knob.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (knob);
        // knurled edge
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        for (int i = 0; i < 28; ++i)
        {
            const float a = (float) i / 28.0f * juce::MathConstants<float>::twoPi;
            auto d = juce::Point<float> (std::sin (a), -std::cos (a));
            g.drawLine (juce::Line<float> (centre + d * (rKnob - 3.0f), centre + d * rKnob), 1.0f);
        }

        // Chicken-head pointer
        juce::Path p;
        const float len = rKnob * 1.18f, wBase = rKnob * 0.62f;
        p.startNewSubPath (0, -len);
        p.quadraticTo (wBase * 0.35f, -len * 0.3f, wBase * 0.5f, rKnob * 0.25f);
        p.quadraticTo (0, rKnob * 0.62f, -wBase * 0.5f, rKnob * 0.25f);
        p.quadraticTo (-wBase * 0.35f, -len * 0.3f, 0, -len);
        p.closeSubPath();
        auto xf = juce::AffineTransform::rotation (angle).translated (centre);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillPath (p, xf.translated (0, 2.0f));
        juce::ColourGradient pg (juce::Colour (0xfff3eee2), centre.x - rKnob, centre.y - rKnob,
                                 juce::Colour (0xffbdb6a6), centre.x + rKnob, centre.y + rKnob, false);
        g.setGradientFill (pg);
        g.fillPath (p, xf);
        g.setColour (cardInk.withAlpha (0.8f));
        g.drawLine (juce::Line<float> (centre, centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (len - 4.0f)), 2.0f);
    }
    else if (style == "pedal")
    {
        const juce::Colour faceInk = s.getProperties().contains ("ink")
            ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["ink"]) : ink;
        const float r = size * 0.5f - 3.0f;

        juce::Path track, value;
        track.addCentredArc (centre.x, centre.y, r, r, 0, start, end, true);
        value.addCentredArc (centre.x, centre.y, r, r, 0, start, angle, true);
        g.setColour (faceInk.withAlpha (0.18f));
        g.strokePath (track, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (faceInk.withAlpha (0.75f));
        g.strokePath (value, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto knob = juce::Rectangle<float> (r * 1.45f, r * 1.45f).withCentre (centre);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (knob.translated (0, 2.0f));
        juce::ColourGradient grad (juce::Colour (0xff3a3d42), knob.getX(), knob.getY(),
                                   juce::Colour (0xff0f1012), knob.getRight(), knob.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (knob);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (knob.reduced (knob.getWidth() * 0.18f), 1.0f);

        auto dir = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (juce::Colour (0xfff2efe8));
        g.drawLine (juce::Line<float> (centre + dir * (knob.getWidth() * 0.12f), centre + dir * (knob.getWidth() * 0.46f)), 2.2f);
    }
    else
    {
        const float r = size * 0.5f - 4.0f;
        juce::Path track, value;
        track.addCentredArc (centre.x, centre.y, r, r, 0, start, end, true);
        value.addCentredArc (centre.x, centre.y, r, r, 0, start, angle, true);
        g.setColour (line);
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (brass);
        g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        auto knob = juce::Rectangle<float> (r * 1.3f, r * 1.3f).withCentre (centre);
        g.setColour (raised);
        g.fillEllipse (knob);
        auto dir = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (ink);
        g.drawLine (juce::Line<float> (centre + dir * (r * 0.15f), centre + dir * (r * 0.6f)), 2.0f);
    }

    if (s.hasKeyboardFocus (false))
    {
        g.setColour (brass.withAlpha (0.8f));
        g.drawEllipse (juce::Rectangle<float> (size, size).withCentre (centre).reduced (1.0f), 1.5f);
    }
}

void GuitarGuiderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                                juce::Slider::SliderStyle style, juce::Slider& s)
{
    using namespace Theme::Colours;
    auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
    const bool horizontal = style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar;
    const auto accent = s.getProperties().contains ("accent")
        ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["accent"]) : brass;

    if (horizontal)
    {
        auto track = r.withSizeKeepingCentre (r.getWidth(), 4.0f);
        g.setColour (line);
        g.fillRoundedRectangle (track, 2.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (track.withRight (pos), 2.0f);
        g.setColour (ink);
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ pos, r.getCentreY() }));
    }
    else
    {
        auto track = r.withSizeKeepingCentre (4.0f, r.getHeight());
        g.setColour (line);
        g.fillRoundedRectangle (track, 2.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (track.withTop (pos), 2.0f);
        auto thumb = juce::Rectangle<float> (22.0f, 10.0f).withCentre ({ r.getCentreX(), pos });
        g.setColour (raised.brighter (0.2f));
        g.fillRoundedRectangle (thumb, 3.0f);
        g.setColour (ink);
        g.drawLine (thumb.getX() + 4, pos, thumb.getRight() - 4, pos, 1.5f);
    }
    if (s.hasKeyboardFocus (false))
        drawFocusOutline (g, r.expanded (2.0f), 4.0f);
}

juce::Slider::SliderLayout GuitarGuiderLookAndFeel::getSliderLayout (juce::Slider& s)
{
    auto layout = LookAndFeel_V4::getSliderLayout (s);
    if (s.isRotary())
        layout.sliderBounds = s.getLocalBounds();
    return layout;
}

juce::Label* GuitarGuiderLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (Theme::cond (14.0f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

//==============================================================================
void GuitarGuiderLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    using namespace Theme::Colours;
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (box.isMouseOver (true) ? raised.brighter (0.06f) : raised);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (box.hasKeyboardFocus (true) ? brass : line);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) w - 16.0f, cy = (float) h * 0.5f;
    arrow.startNewSubPath (cx - 4.5f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.5f);
    arrow.lineTo (cx + 4.5f, cy - 2.0f);
    g.setColour (ink2);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font GuitarGuiderLookAndFeel::getComboBoxFont (juce::ComboBox&) { return Theme::sans (15.0f, Theme::Weight::medium); }

void GuitarGuiderLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void GuitarGuiderLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    using namespace Theme::Colours;
    g.fillAll (panel);
    g.setColour (line);
    g.drawRect (0, 0, w, h, 1);
}

juce::Font GuitarGuiderLookAndFeel::getPopupMenuFont() { return Theme::sans (15.0f, Theme::Weight::medium); }

void GuitarGuiderLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator,
                                                 bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                                                 const juce::String& text, const juce::String& shortcut,
                                                 const juce::Drawable*, const juce::Colour* textColour)
{
    using namespace Theme::Colours;
    if (isSeparator)
    {
        g.setColour (line);
        g.fillRect (area.withSizeKeepingCentre (area.getWidth() - 16, 1));
        return;
    }

    auto r = area.reduced (4, 1);
    if (isHighlighted && isActive)
    {
        g.setColour (raised);
        g.fillRoundedRectangle (r.toFloat(), 6.0f);
    }

    auto c = textColour != nullptr ? *textColour : ink;
    g.setColour (isActive ? c : ink3);
    g.setFont (getPopupMenuFont());
    auto textArea = r.reduced (10, 0);
    if (isTicked)
    {
        auto tick = textArea.removeFromLeft (14).toFloat();
        g.setColour (brass);
        g.fillEllipse (tick.withSizeKeepingCentre (7, 7));
        g.setColour (isActive ? c : ink3);
    }
    else
        textArea.removeFromLeft (14);

    g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
    if (shortcut.isNotEmpty())
    {
        g.setColour (ink3);
        g.drawText (shortcut, textArea, juce::Justification::centredRight);
    }
    if (hasSubMenu)
    {
        g.setColour (ink2);
        g.drawText (">", textArea, juce::Justification::centredRight);
    }
}

void GuitarGuiderLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                             int thumbStart, int thumbSize, bool over, bool down)
{
    juce::Rectangle<int> thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize)
                                          : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour (Theme::Colours::line.withAlpha (over || down ? 1.0f : 0.7f));
    g.fillRoundedRectangle (thumb.toFloat().reduced (2.5f), 3.0f);
}

void GuitarGuiderLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.findColour (juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle (juce::Rectangle<int> (0, 0, w, h).toFloat(), 8.0f);
}

void GuitarGuiderLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.hasKeyboardFocus (true) ? Theme::Colours::brass : Theme::Colours::line);
    g.drawRoundedRectangle (juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f), 8.0f, e.hasKeyboardFocus (true) ? 1.5f : 1.0f);
}

void GuitarGuiderLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat();
    g.setColour (Theme::Colours::bg.darker (0.35f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Theme::Colours::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (Theme::Colours::ink);
    g.setFont (Theme::sans (14.0f, Theme::Weight::medium));
    g.drawFittedText (text, r.toNearestInt().reduced (10, 4), juce::Justification::centredLeft, 4);
}

juce::Rectangle<int> GuitarGuiderLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                                                juce::Rectangle<int> parentArea)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (Theme::sans (14.0f, Theme::Weight::medium), tipText, 0, 0);
    const int w = juce::jmin (320, (int) ga.getBoundingBox (0, -1, true).getWidth() + 22);
    const int lines = 1 + (int) ga.getBoundingBox (0, -1, true).getWidth() / 300;
    const int h = 10 + lines * 18;
    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                                 w, h).constrainedWithin (parentArea);
}

void GuitarGuiderLookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow& alert,
                                            const juce::Rectangle<int>& textArea, juce::TextLayout& layout)
{
    auto r = alert.getLocalBounds().toFloat();
    g.setColour (Theme::Colours::panel);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (Theme::Colours::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    layout.draw (g, textArea.toFloat());
}

juce::Font GuitarGuiderLookAndFeel::getAlertWindowTitleFont()   { return Theme::sans (20.0f, Theme::Weight::bold); }
juce::Font GuitarGuiderLookAndFeel::getAlertWindowMessageFont() { return Theme::sans (15.0f); }
juce::Font GuitarGuiderLookAndFeel::getAlertWindowFont()        { return Theme::sans (15.0f); }

} // namespace gg
