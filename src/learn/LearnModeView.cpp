#include "LearnViews.h"

namespace gg
{

struct LearnModeView::Option : public juce::Button
{
    enum class Look { idle, correct, wrong, faded };

    Option (const juce::String& id, bool diagram, int number)
        : juce::Button (id), chordId (id), showDiagram (diagram), keyNumber (number)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus (true);
        if (auto* c = ChordLibrary::get().find (id)) setTitle (c->name);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        using namespace Theme::Colours;
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        auto* chord = ChordLibrary::get().find (chordId);
        if (chord == nullptr) return;

        juce::Colour edge = line;
        if (look == Look::correct) edge = good;
        else if (look == Look::wrong) edge = bad;
        else if (over && look == Look::idle) edge = brass;

        const float alpha = look == Look::faded ? 0.45f : 1.0f;
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (alpha);

        if (showDiagram)
        {
            drawPaperCard (g, r.translated (0, down ? 1.0f : 0.0f), 12.0f, look != Look::faded);
            ChordDiagram::Style st;
            st.showFingers = false;
            ChordDiagram::draw (g, r.reduced (r.getWidth() * 0.12f, r.getHeight() * 0.08f), *chord, st);
        }
        else
        {
            g.setColour (over && look == Look::idle ? raised.brighter (0.06f) : raised);
            g.fillRoundedRectangle (r, 12.0f);
            auto t = r.reduced (20.0f, 0);
            g.setColour (ink);
            g.setFont (Theme::cond (34.0f, Theme::Weight::bold));
            g.drawText (chord->symbol, t.removeFromLeft (110.0f), juce::Justification::centredLeft, false);
            g.setColour (ink2);
            g.setFont (Theme::sans (15.0f, Theme::Weight::medium));
            g.drawText (chord->name, t, juce::Justification::centredLeft, true);
        }

        g.setColour (edge);
        g.drawRoundedRectangle (r, 12.0f, look == Look::idle && ! over ? 1.0f : 2.5f);

        // Number key hint
        auto key = juce::Rectangle<float> (22, 22).withPosition (r.getRight() - 30, r.getY() + 8);
        g.setColour ((showDiagram ? cardInk : ink).withAlpha (0.35f));
        g.drawRoundedRectangle (key, 5.0f, 1.0f);
        g.setFont (Theme::sans (13.0f, Theme::Weight::semibold));
        g.drawText (juce::String (keyNumber), key, juce::Justification::centred, false);

        if (hasKeyboardFocus (false))
        {
            g.setColour (brass);
            g.drawRoundedRectangle (r.expanded (2.0f), 14.0f, 1.5f);
        }
    }

    juce::String chordId;
    bool showDiagram;
    int keyNumber;
    Look look = Look::idle;
};

//==============================================================================
LearnModeView::LearnModeView (AppContext& c, juce::StringArray ids)
    : ctx (c), deck (ids), session (ids)
{
    variant (continueButton, "primary");
    variant (againButton, "primary");
    continueButton.onClick = [this] { advance(); };
    againButton.onClick = [this] { session = LearnSession (deck); againButton.setVisible (false); buildOptions(); };
    addChildComponent (continueButton);
    addChildComponent (againButton);
    variant (hearButton, "solid");
    hearButton.onClick = [this] { hearChord (ctx, session.question().target); };
    hearButton.setTooltip ("Hear the chord (H)");
    addAndMakeVisible (hearButton);
    setWantsKeyboardFocus (true);
    buildOptions();
}

LearnModeView::~LearnModeView() = default;

void LearnModeView::buildOptions()
{
    options.clear();
    chosen = -1;
    answered = false;
    continueButton.setVisible (false);

    if (session.finished())
    {
        againButton.setVisible (true);
        hearButton.setVisible (false);
        resized();
        repaint();
        return;
    }

    hearButton.setVisible (true);
    const auto& q = session.question();
    const bool diagrams = q.type == LearnSession::QuestionType::NameToDiagram;
    for (int i = 0; i < q.options.size(); ++i)
    {
        auto* o = options.add (new Option (q.options[i], diagrams, i + 1));
        o->onClick = [this, i] { choose (i); };
        addAndMakeVisible (o);
    }
    resized();
    repaint();
}

void LearnModeView::choose (int index)
{
    if (answered || session.finished()) return;
    answered = true;
    chosen = index;
    const auto target = session.question().target;
    wasCorrect = session.answer (index);
    ctx.progress.recordAnswer (target, wasCorrect);

    for (int i = 0; i < options.size(); ++i)
    {
        auto* o = options[i];
        if (i == session.question().correctIndex) o->look = Option::Look::correct;
        else if (i == index) o->look = Option::Look::wrong;
        else o->look = Option::Look::faded;
        o->repaint();
    }

    if (wasCorrect) startTimer (850);
    else
    {
        continueButton.setVisible (true);
        continueButton.grabKeyboardFocus();
    }
    repaint();
}

void LearnModeView::answerForScreenshot (bool correct)
{
    const int idx = correct ? session.question().correctIndex : (session.question().correctIndex + 1) % options.size();
    choose (idx);
    stopTimer();
}

void LearnModeView::advance()
{
    stopTimer();
    session.next();
    buildOptions();
    grabKeyboardFocus();
}

void LearnModeView::timerCallback() { advance(); }

void LearnModeView::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (28);

    // Progress: one pip per chord (empty -> half -> full)
    auto top = r.removeFromTop (40);
    g.setColour (ink);
    g.setFont (Theme::sans (16.0f, Theme::Weight::semibold));
    g.drawText ("Mastered " + juce::String (session.masteredCount()) + " of " + juce::String (session.total()),
                top.removeFromLeft (220), juce::Justification::centredLeft, false);
    const float pip = 14.0f, gap = 6.0f;
    float x = (float) top.getRight() - deck.size() * (pip + gap);
    for (auto& id : deck)
    {
        auto p = juce::Rectangle<float> (pip, pip).withPosition (x, top.getCentreY() - pip * 0.5f);
        const int s = session.streakFor (id);
        g.setColour (line);
        g.fillEllipse (p);
        if (s >= LearnSession::requiredStreak) { g.setColour (good); g.fillEllipse (p); }
        else if (s == 1)
        {
            juce::Path half;
            half.addPieSegment (p, 0.0f, juce::MathConstants<float>::pi, 0.0f);
            g.setColour (brass);
            g.fillPath (half);
        }
        x += pip + gap;
    }

    if (session.finished())
    {
        auto centre = getLocalBounds().toFloat().getCentre();
        const int pct = session.questionsAsked() > 0 ? juce::roundToInt (100.0 * session.correctAnswers() / session.questionsAsked()) : 0;
        g.setColour (good);
        g.setFont (Theme::cond (80.0f, Theme::Weight::bold));
        g.drawText (juce::String (pct) + "%", juce::Rectangle<float> (400, 90).withCentre (centre.translated (0, -90)), juce::Justification::centred, false);
        g.setColour (ink);
        g.setFont (Theme::sans (26.0f, Theme::Weight::bold));
        g.drawText ("Round complete", juce::Rectangle<float> (600, 36).withCentre (centre.translated (0, -20)), juce::Justification::centred, false);
        g.setColour (ink2);
        g.setFont (Theme::sans (16.0f));
        g.drawText (juce::String (session.correctAnswers()) + " right out of " + juce::String (session.questionsAsked())
                        + " answers. Every chord was answered correctly twice in a row.",
                    juce::Rectangle<float> (700, 24).withCentre (centre.translated (0, 16)), juce::Justification::centred, false);
        return;
    }

    const auto& q = session.question();
    auto* chord = ChordLibrary::get().find (q.target);
    if (chord == nullptr) return;

    // Prompt
    g.setColour (ink2);
    g.setFont (Theme::sans (17.0f, Theme::Weight::medium));
    const bool diagramPrompt = q.type == LearnSession::QuestionType::DiagramToName;
    g.drawText (diagramPrompt ? "Which chord is this?" : "Find the shape for this chord",
                promptArea.withY (promptArea.getY() - 36).withHeight (28).toNearestInt(), juce::Justification::centredLeft, false);

    drawPaperCard (g, promptArea, Theme::radiusLg, true);
    if (diagramPrompt)
    {
        ChordDiagram::Style st;
        st.showStringNames = true;
        ChordDiagram::draw (g, promptArea.reduced (promptArea.getWidth() * 0.12f, promptArea.getHeight() * 0.1f), *chord, st);
    }
    else
    {
        g.setColour (cardInk);
        g.setFont (Theme::cond (promptArea.getHeight() * 0.36f, Theme::Weight::bold));
        g.drawText (chord->symbol, promptArea.withTrimmedBottom (promptArea.getHeight() * 0.15f), juce::Justification::centred, false);
        g.setColour (cardInk2);
        g.setFont (Theme::sans (20.0f, Theme::Weight::medium));
        g.drawText (chord->name, promptArea.withTrimmedTop (promptArea.getHeight() * 0.62f), juce::Justification::centredTop, false);
    }

    // Feedback line
    if (answered && ! options.isEmpty())
    {
        auto fb = juce::Rectangle<float> (promptArea.getX(), promptArea.getBottom() + 22.0f, (float) r.getWidth(), 30.0f);
        if (wasCorrect)
        {
            g.setColour (good);
            g.setFont (Theme::sans (20.0f, Theme::Weight::bold));
            g.drawText ("Correct", fb, juce::Justification::centredLeft, false);
        }
        else
        {
            auto* picked = ChordLibrary::get().find (session.question().options[chosen]);
            g.setColour (bad);
            g.setFont (Theme::sans (20.0f, Theme::Weight::bold));
            g.drawText ("Not quite", fb.removeFromLeft (110.0f), juce::Justification::centredLeft, false);
            g.setColour (ink2);
            g.setFont (Theme::sans (16.0f));
            g.drawText ("You picked " + (picked ? picked->symbol : juce::String()) + ". The answer is " + chord->symbol
                            + ". It'll come back around.",
                        fb, juce::Justification::centredLeft, false);
        }
    }
}

void LearnModeView::resized()
{
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (28 + 40 + 60);
    auto bottom = r.removeFromBottom (110);

    auto left = r.removeFromLeft (juce::jmin (420, r.getWidth() * 2 / 5));
    const float ph = juce::jmin ((float) left.getHeight(), left.getWidth() * 1.05f);
    promptArea = left.toFloat().withHeight (ph).reduced (0, 0);
    r.removeFromLeft (40);

    hearButton.setBounds (juce::Rectangle<int> (130, 40).withPosition ((int) promptArea.getRight() - 130, (int) promptArea.getY() - 44));

    if (! options.isEmpty())
    {
        const bool diagrams = options[0]->showDiagram;
        auto grid = r.withHeight (diagrams ? (int) ph : juce::jmin ((int) ph, 4 * 84));
        if (diagrams)
        {
            const int cw = (grid.getWidth() - 16) / 2, ch = (grid.getHeight() - 16) / 2;
            for (int i = 0; i < options.size(); ++i)
                options[i]->setBounds (grid.getX() + (i % 2) * (cw + 16), grid.getY() + (i / 2) * (ch + 16), cw, ch);
        }
        else
        {
            const int oh = juce::jmin (84, (grid.getHeight() - 36) / 4);
            for (int i = 0; i < options.size(); ++i)
                options[i]->setBounds (grid.getX(), grid.getY() + i * (oh + 12), grid.getWidth(), oh);
        }
    }

    continueButton.setBounds (bottom.withSizeKeepingCentre (bottom.getWidth(), 46).removeFromRight (180).withY ((int) promptArea.getBottom() + 14));
    againButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 48).translated (0, 80));
}

bool LearnModeView::keyPressed (const juce::KeyPress& k)
{
    const auto c = k.getTextCharacter();
    if (c >= '1' && c <= '4' && ! answered) { choose (c - '1'); return true; }
    if ((k == juce::KeyPress::returnKey || k == juce::KeyPress::spaceKey) && answered) { advance(); return true; }
    if (k == juce::KeyPress::returnKey && session.finished()) { againButton.triggerClick(); return true; }
    if (c == 'h' || c == 'H') { hearButton.triggerClick(); return true; }
    return false;
}

} // namespace gg
