#include "ChordDiagram.h"

namespace gg::ChordDiagram
{

void draw (juce::Graphics& g, juce::Rectangle<float> area, const Chord& chord, const Style& st)
{
    constexpr int numFrets = 4;
    const int base = chord.baseFret();

    // Fit a grid with a 5:6 (w:h) ratio plus room for markers and the fret label
    auto avail = area;
    const float markerH = avail.getHeight() * 0.11f;
    const float namesH = st.showStringNames ? avail.getHeight() * 0.09f : 0.0f;
    const float labelW = avail.getWidth() * 0.14f;
    auto gridArea = avail.withTrimmedTop (markerH).withTrimmedBottom (namesH).withTrimmedLeft (labelW).withTrimmedRight (labelW * 0.5f);

    float gw = gridArea.getWidth(), gh = gw * 1.15f;
    if (gh > gridArea.getHeight()) { gh = gridArea.getHeight(); gw = gh / 1.15f; }
    auto grid = juce::Rectangle<float> (gw, gh).withCentre (gridArea.getCentre()).withY (gridArea.getY());

    const float sx = grid.getWidth() / 5.0f;
    const float fy = grid.getHeight() / numFrets;
    const float lineW = juce::jmax (1.0f, gw / 110.0f);
    auto stringX = [&] (int s) { return grid.getX() + sx * (float) s; };

    // Frets
    g.setColour (st.ink.withAlpha (0.8f));
    for (int f = 0; f <= numFrets; ++f)
        g.fillRect (juce::Rectangle<float> (grid.getX(), grid.getY() + fy * f - lineW * 0.5f, grid.getWidth(), lineW));
    // Nut
    if (base == 1)
        g.fillRoundedRectangle (juce::Rectangle<float> (grid.getX() - lineW, grid.getY() - lineW * 4.0f, grid.getWidth() + lineW * 2, lineW * 4.5f), lineW);
    else
    {
        g.setFont (Theme::cond (juce::jmax (11.0f, fy * 0.42f), Theme::Weight::semibold));
        g.setColour (st.muted);
        g.drawText (juce::String (base) + "fr", juce::Rectangle<float> (grid.getX() - labelW - 4.0f, grid.getY(), labelW, fy),
                    juce::Justification::centredRight, false);
    }

    // Strings (low E thicker)
    for (int s = 0; s < 6; ++s)
    {
        const float w = lineW * (1.6f - s * 0.12f);
        g.setColour (st.ink.withAlpha (0.85f));
        g.fillRect (juce::Rectangle<float> (stringX (s) - w * 0.5f, grid.getY(), w, grid.getHeight()));
    }

    // Open / muted markers
    const float mk = juce::jmin (sx * 0.5f, markerH * 0.75f);
    for (int s = 0; s < 6; ++s)
    {
        auto c = juce::Point<float> (stringX (s), grid.getY() - markerH * 0.55f - (base == 1 ? lineW * 2 : 0.0f));
        auto r = juce::Rectangle<float> (mk, mk).withCentre (c);
        if (chord.frets[(size_t) s] == 0)
        {
            g.setColour (st.ink);
            g.drawEllipse (r.reduced (lineW * 0.6f), lineW * 1.3f);
        }
        else if (chord.frets[(size_t) s] < 0)
        {
            g.setColour (st.muted);
            auto x = r.reduced (mk * 0.12f);
            g.drawLine ({ x.getTopLeft(), x.getBottomRight() }, lineW * 1.3f);
            g.drawLine ({ x.getTopRight(), x.getBottomLeft() }, lineW * 1.3f);
        }
    }

    const float dotD = juce::jmin (sx * 0.78f, fy * 0.7f);
    auto fretCentreY = [&] (int fret) { return grid.getY() + fy * ((float) (fret - base) + 0.5f); };

    // Barres
    auto barres = chord.barres();
    for (auto& b : barres)
    {
        auto r = juce::Rectangle<float> (stringX (b.fromString) - dotD * 0.5f, fretCentreY (b.fret) - dotD * 0.5f,
                                         stringX (b.toString) - stringX (b.fromString) + dotD, dotD);
        g.setColour (st.dot);
        g.fillRoundedRectangle (r, dotD * 0.5f);
    }

    // Finger dots
    g.setFont (Theme::sans (dotD * 0.62f, Theme::Weight::bold));
    for (int s = 0; s < 6; ++s)
    {
        const int f = chord.frets[(size_t) s];
        if (f <= 0) continue;

        bool inBarre = false;
        for (auto& b : barres)
            if (b.fret == f && s >= b.fromString && s <= b.toString && chord.fingers[(size_t) s] == chord.fingers[(size_t) b.fromString])
                inBarre = true;

        auto r = juce::Rectangle<float> (dotD, dotD).withCentre ({ stringX (s), fretCentreY (f) });
        if (! inBarre)
        {
            g.setColour (st.dot);
            g.fillEllipse (r);
        }
        if (st.showFingers && chord.fingers[(size_t) s] > 0)
        {
            // On a barre, only label its two ends to avoid clutter
            bool draw = ! inBarre;
            for (auto& b : barres)
                if (inBarre && b.fret == f && (s == b.fromString || s == b.toString)) draw = true;
            if (draw)
            {
                g.setColour (st.dotText);
                g.drawText (juce::String (chord.fingers[(size_t) s]), r, juce::Justification::centred, false);
            }
        }
    }

    if (st.showStringNames)
    {
        static const char* names[] { "E", "A", "D", "G", "B", "e" };
        g.setColour (st.muted);
        g.setFont (Theme::cond (juce::jmax (10.0f, namesH * 0.7f), Theme::Weight::medium));
        for (int s = 0; s < 6; ++s)
            g.drawText (names[s], juce::Rectangle<float> (sx, namesH).withCentre ({ stringX (s), grid.getBottom() + namesH * 0.6f }),
                        juce::Justification::centred, false);
    }
}

} // namespace gg::ChordDiagram
