#include "StudioViews.h"

namespace gg
{

namespace
{
    void drawScrew (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (juce::Colour (0xff9a9ea4));
        g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (juce::Colour (0xff4a4e54));
        g.drawLine (c.x - r * 0.7f, c.y - r * 0.2f, c.x + r * 0.7f, c.y + r * 0.2f, 1.0f);
    }

    void drawEnclosure (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float radius)
    {
        Theme::dropShadow (g, r, radius, 0.6f);
        juce::ColourGradient grad (colour.brighter (0.12f), r.getX(), r.getY(), colour.darker (0.18f), r.getRight(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, radius);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        const float s = juce::jmax (2.0f, r.getWidth() * 0.022f);
        const float in = s * 3.2f;
        drawScrew (g, { r.getX() + in, r.getY() + in }, s);
        drawScrew (g, { r.getRight() - in, r.getY() + in }, s);
        drawScrew (g, { r.getX() + in, r.getBottom() - in }, s);
        drawScrew (g, { r.getRight() - in, r.getBottom() - in }, s);
    }
}

//==============================================================================
struct PedalboardView::Pedal : public juce::Component
{
    Pedal (PedalboardView& o, std::shared_ptr<Effect> e, int i) : owner (o), fx (std::move (e)), index (i)
    {
        info = PedalRegistry::find (fx->typeId());
        colour = juce::Colour (info != nullptr ? info->colour : 0xff777777);
        ink = Theme::inkOn (colour);

        for (auto& p : fx->params())
        {
            auto* k = knobs.add (new ParamKnob (*p, "pedal", ink));
            k->setCaptionFont (Theme::cond (12.0f, Theme::Weight::semibold));
            addAndMakeVisible (k);
        }

        removeButton.setTooltip ("Remove pedal");
        removeButton.onClick = [this]
        {
            const int idx = index;
            auto* board = &owner;
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<PedalboardView> (board), idx]
            { if (safe != nullptr) safe->ctx.audio.chain.removePedal (idx); });
        };
        addChildComponent (removeButton);
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTitle (info != nullptr ? info->kind : fx->typeId());
        setWantsKeyboardFocus (true);
    }

    juce::Rectangle<float> footswitch() const
    {
        auto r = getLocalBounds().toFloat();
        return juce::Rectangle<float> (34.0f, 34.0f).withCentre ({ r.getCentreX(), r.getBottom() - 30.0f });
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        drawEnclosure (g, r, colour, 11.0f);
        const bool on = fx->enabled.load();

        // Name plate sits just above the footswitch
        auto plate = juce::Rectangle<float> (r.getX() + 8, footswitch().getY() - 46.0f, r.getWidth() - 16, 40.0f);
        g.setColour (ink);
        g.setFont (Theme::cond (juce::jmin (21.0f, r.getWidth() * 0.15f), Theme::Weight::bold));
        g.drawFittedText (info != nullptr ? info->name : fx->typeId(), plate.removeFromTop (24.0f).toNearestInt(),
                          juce::Justification::centred, 1, 0.8f);
        g.setColour (ink.withAlpha (0.65f));
        g.setFont (Theme::sans (11.5f, Theme::Weight::semibold));
        g.drawText (info != nullptr ? info->kind : juce::String(), plate, juce::Justification::centredTop, false);

        // LED, top centre between the screws
        auto led = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ r.getCentreX(), r.getY() + 12.0f });
        if (on)
        {
            g.setColour (Theme::Colours::glow.withAlpha (0.35f));
            g.fillEllipse (led.expanded (5.0f));
            g.setColour (juce::Colour (0xffff5a2a));
        }
        else
            g.setColour (juce::Colour (0xff3a1d16));
        g.fillEllipse (led);

        // Footswitch
        auto fs = footswitch();
        juce::ColourGradient chrome (juce::Colour (0xffe9ebee), fs.getX(), fs.getY(), juce::Colour (0xff6b7077), fs.getRight(), fs.getBottom(), false);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillEllipse (fs.translated (0, 2.0f));
        g.setGradientFill (chrome);
        g.fillEllipse (fs);
        g.setColour (juce::Colour (0xff8d9299));
        g.drawEllipse (fs.reduced (5.0f), 1.5f);
        if (pressing)
        {
            g.setColour (juce::Colours::black.withAlpha (0.15f));
            g.fillEllipse (fs);
        }

        if (! on)
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillRoundedRectangle (r, 11.0f);
        }
        if (hasKeyboardFocus (false))
        {
            g.setColour (Theme::Colours::brass);
            g.drawRoundedRectangle (r.expanded (1.0f), 12.0f, 2.0f);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 0);
        r.removeFromTop (22);
        r.setBottom ((int) footswitch().getY() - 50);
        removeButton.setBounds (getWidth() - 30, 4, 26, 26);

        const int n = knobs.size();
        const int perRow = n == 4 ? 2 : juce::jmax (1, n);
        const int rows = (n + perRow - 1) / perRow;
        const int kw = r.getWidth() / perRow;
        const int kh = juce::jmin (kw + 14, r.getHeight() / juce::jmax (1, rows));
        const int yOff = (r.getHeight() - kh * rows) / 2;
        for (int i = 0; i < n; ++i)
            knobs[i]->setBounds (r.getX() + (i % perRow) * kw, r.getY() + yOff + (i / perRow) * kh, kw, kh);
    }

    void mouseEnter (const juce::MouseEvent&) override { removeButton.setVisible (true); }
    void mouseExit (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getEventRelativeTo (this).getPosition())) removeButton.setVisible (false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (footswitch().expanded (6.0f).contains (e.position)) { pressing = true; repaint(); return; }
        owner.beginDrag (this, e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (! pressing) owner.dragPedal (this, e); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (pressing)
        {
            pressing = false;
            if (footswitch().expanded (6.0f).contains (e.position)) toggle();
            repaint();
            return;
        }
        owner.endDrag (this);
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::spaceKey || k == juce::KeyPress::returnKey) { toggle(); return true; }
        if (k == juce::KeyPress::deleteKey) { removeButton.triggerClick(); return true; }
        return false;
    }

    void toggle()
    {
        fx->enabled = ! fx->enabled.load();
        repaint();
    }

    PedalboardView& owner;
    std::shared_ptr<Effect> fx;
    const PedalInfo* info = nullptr;
    juce::Colour colour, ink;
    juce::OwnedArray<ParamKnob> knobs;
    IconButton removeButton { "Remove", Icons::close() };
    int index;
    bool pressing = false;
};

//==============================================================================
struct PedalboardView::Board : public juce::Component
{
    explicit Board (PedalboardView& o) : owner (o)
    {
        addSlot.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (addSlot);
        addSlot.onClick = [this] { owner.showPicker(); };
        addSlot.setTooltip ("Add a pedal to the board");
    }

    struct AddSlot : public juce::Button
    {
        AddSlot() : juce::Button ("Add pedal") { setWantsKeyboardFocus (true); }
        void paintButton (juce::Graphics& g, bool over, bool) override
        {
            using namespace Theme::Colours;
            auto r = getLocalBounds().toFloat().reduced (2.0f);
            juce::Path outline, dashed;
            outline.addRoundedRectangle (r, 11.0f);
            const float d[] { 6.0f, 5.0f };
            juce::PathStrokeType (1.4f).createDashedStroke (dashed, outline, d, 2);
            g.setColour (over || hasKeyboardFocus (false) ? brass : ink3);
            g.fillPath (dashed);
            Icons::draw (g, Icons::plus(), juce::Rectangle<float> (30, 30).withCentre (r.getCentre().translated (0, -14)), over ? brass : ink2, 2.0f);
            g.setColour (over ? ink : ink2);
            g.setFont (Theme::sans (15.0f, Theme::Weight::semibold));
            g.drawText ("Add pedal", r.withTrimmedTop (r.getHeight() * 0.5f + 4).withHeight (22), juce::Justification::centred, false);
        }
    };

    int slotX (int i) const { return startX + i * (pedalW + gap); }

    void layout (int height)
    {
        pedalH = juce::jmax (150, height - 30);
        pedalW = (int) (pedalH * 0.7f);
        for (int i = 0; i < pedals.size(); ++i)
            if (pedals[i] != owner.dragging)
                pedals[i]->setBounds (slotX (i), 16, pedalW, pedalH);
        addSlot.setBounds (slotX (pedals.size()), 16, pedalW, pedalH);
        setSize (slotX (pedals.size() + 1) + endW, height);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace Theme::Colours;
        auto r = getLocalBounds().toFloat();

        // Board surface with rails
        g.setColour (juce::Colour (0xff23272d));
        g.fillRoundedRectangle (r, Theme::radiusMd);
        for (float y = r.getY() + 10.0f; y < r.getBottom() - 6.0f; y += 22.0f)
        {
            g.setColour (juce::Colour (0xff2c3138));
            g.fillRect (r.getX() + 8.0f, y, r.getWidth() - 16.0f, 12.0f);
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.fillRect (r.getX() + 8.0f, y + 12.0f, r.getWidth() - 16.0f, 1.0f);
        }

        // Signal cable: input jack -> every pedal -> amp
        const float cy = 16.0f + pedalH * 0.36f;
        juce::Path cable;
        float x = 22.0f;
        cable.startNewSubPath (x, cy);
        for (int i = 0; i <= pedals.size(); ++i)
        {
            const float nx = i == pedals.size() ? (float) getWidth() - endW + 36.0f : (float) slotX (i);
            const float sag = juce::jmin (26.0f, (nx - x) * 0.6f);
            cable.cubicTo (x + (nx - x) * 0.3f, cy + sag, x + (nx - x) * 0.7f, cy + sag, nx, cy);
            x = nx + pedalW;
        }
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.strokePath (cable, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (0, 2.0f));
        g.setColour (juce::Colour (0xff15171a));
        g.strokePath (cable, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Jack labels
        g.setColour (ink2);
        g.setFont (Theme::cond (13.0f, Theme::Weight::bold));
        auto jack = juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ 16.0f, cy });
        g.setColour (juce::Colour (0xff9a9ea4));
        g.fillEllipse (jack);
        g.setColour (juce::Colour (0xff15171a));
        g.fillEllipse (jack.reduced (4.0f));
        g.setColour (ink2);
        g.drawText ("GUITAR", juce::Rectangle<float> (60.0f, 16.0f).withPosition (6.0f, cy + 12.0f), juce::Justification::centredLeft, false);

        g.drawText ("TO AMP", juce::Rectangle<float> (70.0f, 16.0f).withPosition ((float) getWidth() - endW - 4.0f, cy + 12.0f),
                    juce::Justification::centredLeft, false);
        juce::Path arrow;
        const float ax = (float) getWidth() - endW + 40.0f;
        arrow.addTriangle (ax, cy - 6.0f, ax, cy + 6.0f, ax + 9.0f, cy);
        g.fillPath (arrow);
    }

    void resized() override {}

    PedalboardView& owner;
    juce::OwnedArray<Pedal> pedals;
    AddSlot addSlot;
    int pedalW = 140, pedalH = 200, gap = 18, startX = 54, endW = 84;
};

//==============================================================================
PedalboardView::PedalboardView (AppContext& c) : ctx (c), board (std::make_unique<Board> (*this))
{
    viewport.setViewedComponent (board.get(), false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);
    rebuild();
    startTimerHz (10);
}

PedalboardView::~PedalboardView() = default;

void PedalboardView::rebuild()
{
    board->pedals.clear();
    auto list = ctx.audio.chain.pedals();
    for (int i = 0; i < (int) list.size(); ++i)
        board->addAndMakeVisible (board->pedals.add (new Pedal (*this, list[(size_t) i], i)));
    lastVersion = ctx.audio.chain.structureVersion.load();
    resized();
    repaint();
}

void PedalboardView::timerCallback()
{
    if (ctx.audio.chain.structureVersion.load() != lastVersion && dragging == nullptr)
        rebuild();
}

void PedalboardView::paint (juce::Graphics&) {}

void PedalboardView::resized()
{
    viewport.setBounds (getLocalBounds());
    board->layout (getHeight() - (board->getWidth() > getWidth() ? 10 : 0));
    if (board->getWidth() < getWidth()) board->setSize (getWidth(), board->getHeight());
}

void PedalboardView::beginDrag (Pedal* p, const juce::MouseEvent& e)
{
    dragging = p;
    dragOffset = e.getEventRelativeTo (p).x;
    p->toFront (false);
}

void PedalboardView::dragPedal (Pedal* p, const juce::MouseEvent& e)
{
    if (dragging != p || e.getDistanceFromDragStart() < 4) return;
    const int x = e.getEventRelativeTo (board.get()).x - dragOffset;
    p->setTopLeftPosition (juce::jlimit (0, board->getWidth() - p->getWidth(), x), 10);

    // Live preview: slide the other pedals to show the drop position
    const int target = juce::jlimit (0, board->pedals.size() - 1,
                                     (p->getBounds().getCentreX() - board->startX) / (board->pedalW + board->gap));
    int slot = 0;
    for (auto* other : board->pedals)
    {
        if (other == p) continue;
        if (slot == target) ++slot;
        other->setTopLeftPosition (board->slotX (slot), 16);
        ++slot;
    }
}

void PedalboardView::endDrag (Pedal* p)
{
    if (dragging != p) return;
    const int from = board->pedals.indexOf (p);
    const int to = juce::jlimit (0, board->pedals.size() - 1,
                                 (p->getBounds().getCentreX() - board->startX) / (board->pedalW + board->gap));
    dragging = nullptr;
    if (from != to) ctx.audio.chain.movePedal (from, to);
    else resized();
}

void PedalboardView::showPicker()
{
    struct Picker : public juce::Component
    {
        explicit Picker (PedalboardView& o) : owner (o)
        {
            for (auto& info : PedalRegistry::all())
            {
                auto* b = items.add (new Item (info));
                b->onClick = [this, type = info.typeId]
                {
                    owner.ctx.audio.chain.addPedal (type);
                    if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
                };
                addAndMakeVisible (b);
            }
            setSize (3 * 250 + 24, ((items.size() + 2) / 3) * 70 + 24);
        }

        struct Item : public juce::Button
        {
            explicit Item (const PedalInfo& i) : juce::Button (i.name), info (i)
            {
                setMouseCursor (juce::MouseCursor::PointingHandCursor);
                setTooltip (i.blurb);
            }
            void paintButton (juce::Graphics& g, bool over, bool) override
            {
                using namespace Theme::Colours;
                auto r = getLocalBounds().toFloat().reduced (3.0f);
                if (over) { g.setColour (raised); g.fillRoundedRectangle (r, 10.0f); }
                auto sw = r.removeFromLeft (44.0f).reduced (4.0f, 6.0f);
                const juce::Colour c (info.colour);
                g.setColour (c);
                g.fillRoundedRectangle (sw, 6.0f);
                g.setColour (juce::Colour (0xffd8dbe0));
                g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre ({ sw.getCentreX(), sw.getBottom() - 11.0f }));
                r.removeFromLeft (8.0f);
                g.setColour (ink);
                g.setFont (Theme::sans (16.0f, Theme::Weight::semibold));
                g.drawText (info.kind, r.removeFromTop (r.getHeight() * 0.5f), juce::Justification::bottomLeft, false);
                g.setColour (ink2);
                g.setFont (Theme::sans (13.0f));
                g.drawText (info.blurb, r, juce::Justification::topLeft, true);
            }
            PedalInfo info;
        };

        void resized() override
        {
            for (int i = 0; i < items.size(); ++i)
                items[i]->setBounds (12 + (i % 3) * 250, 12 + (i / 3) * 70, 250, 70);
        }

        PedalboardView& owner;
        juce::OwnedArray<Item> items;
    };

    auto picker = std::make_unique<Picker> (*this);
    auto area = board->addSlot.getScreenBounds();
    juce::CallOutBox::launchAsynchronously (std::move (picker), area, nullptr);
}

} // namespace gg
