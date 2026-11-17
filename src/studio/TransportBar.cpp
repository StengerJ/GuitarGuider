#include "StudioViews.h"

namespace gg
{

BpmBox::BpmBox (Session& s) : session (s)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTooltip ("Tempo: drag up or down, scroll, or double-click to type");
}

void BpmBox::set (double bpm)
{
    session.bpm = juce::jlimit (40.0, 240.0, std::round (bpm));
    repaint();
}

void BpmBox::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().toFloat();
    g.setColour (raised);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (ink);
    g.setFont (Theme::cond (24.0f, Theme::Weight::bold));
    g.drawText (juce::String ((int) session.bpm.load()), r.withTrimmedRight (r.getWidth() * 0.42f), juce::Justification::centredRight, false);
    g.setColour (ink2);
    g.setFont (Theme::cond (13.0f, Theme::Weight::semibold));
    g.drawText ("BPM", r.withTrimmedLeft (r.getWidth() * 0.62f), juce::Justification::centredLeft, false);
}

void BpmBox::mouseDown (const juce::MouseEvent&) { dragStart = session.bpm.load(); }
void BpmBox::mouseDrag (const juce::MouseEvent& e) { set (dragStart - e.getDistanceFromDragStartY() * 0.4); }
void BpmBox::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) { set (session.bpm.load() + (w.deltaY > 0 ? 1 : -1)); }

void BpmBox::mouseDoubleClick (const juce::MouseEvent&)
{
    auto* ed = new juce::TextEditor();
    ed->setText (juce::String ((int) session.bpm.load()));
    ed->setJustification (juce::Justification::centred);
    ed->setFont (Theme::cond (22.0f, Theme::Weight::bold));
    ed->setInputRestrictions (3, "0123456789");
    ed->setBounds (getLocalBounds());
    addAndMakeVisible (ed);
    ed->selectAll();
    ed->grabKeyboardFocus();
    auto finish = [this, ed]
    {
        set (ed->getText().getDoubleValue() > 0 ? ed->getText().getDoubleValue() : session.bpm.load());
        juce::MessageManager::callAsync ([ed] { delete ed; });
    };
    ed->onReturnKey = finish;
    ed->onFocusLost = finish;
    ed->onEscapeKey = [ed] { juce::MessageManager::callAsync ([ed] { delete ed; }); };
}

//==============================================================================
TransportBar::TransportBar (AppContext& c)
    : ctx (c), bpmBox (c.audio.session), outMeter ([&c] { return c.audio.takeOutputPeak(); }, false)
{
    presetBox.setTextWhenNothingSelected ("Choose a preset");
    presetBox.setTooltip ("Rig presets: amp, cabinet and pedals");
    presetBox.onChange = [this] { loadPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);
    variant (savePresetButton, "ghost");
    savePresetButton.setTooltip ("Save the current rig as a preset");
    savePresetButton.onClick = [this] { savePreset(); };
    addAndMakeVisible (savePresetButton);
    refreshPresets();

    for (auto* b : { &rewindButton, &playButton, &recordButton, &metronomeButton })
    {
        variant (*b, "solid");
        addAndMakeVisible (b);
    }
    playButton.getProperties().set ("filled", true);
    recordButton.getProperties().set ("filled", true);
    rewindButton.getProperties().set ("filled", true);
    recordButton.setActiveColour (Theme::Colours::glow);
    playButton.setActiveColour (Theme::Colours::good);
    rewindButton.setTooltip ("Back to start (Enter)");
    playButton.setTooltip ("Play / stop (Space)");
    recordButton.setTooltip ("Record onto the armed track (R)");
    metronomeButton.setTooltip ("Metronome (M)");

    rewindButton.onClick = [this] { ctx.audio.setPosition (0); };
    playButton.onClick = [this] { ctx.audio.togglePlay(); };
    recordButton.onClick = [this]
    {
        if (ctx.audio.isRecording()) ctx.audio.stop();
        else ctx.audio.record (getArmedTrack ? getArmedTrack() : 0);
    };
    metronomeButton.onClick = [this] { ctx.audio.metronomeOn = ! ctx.audio.metronomeOn.load(); };

    variant (countInChip, "chip");
    countInChip.setClickingTogglesState (true);
    countInChip.setToggleState (ctx.audio.countIn.load(), juce::dontSendNotification);
    countInChip.setTooltip ("Play one bar of clicks before recording starts");
    countInChip.onClick = [this] { ctx.audio.countIn = countInChip.getToggleState(); };
    addAndMakeVisible (countInChip);
    addAndMakeVisible (bpmBox);

    for (auto* chip : { &guitarChip, &demoChip })
    {
        variant (*chip, "chip");
        chip->setRadioGroupId (3001);
        chip->setClickingTogglesState (true);
        addAndMakeVisible (chip);
    }
    guitarChip.setTooltip ("Play through the amp with the guitar plugged into your audio interface");
    demoChip.setTooltip ("No guitar handy? Loop a built-in riff through the rig");
    guitarChip.onClick = [this] { ctx.audio.demoInput = false; };
    demoChip.onClick = [this] { ctx.audio.demoInput = true; };

    masterSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterSlider.setRange (0.0, 1.0);
    masterSlider.setValue (ctx.audio.masterVolume.load(), juce::dontSendNotification);
    masterSlider.setTooltip ("Output volume");
    masterSlider.onValueChange = [this] { ctx.audio.masterVolume = (float) masterSlider.getValue(); };
    addAndMakeVisible (masterSlider);
    addAndMakeVisible (outMeter);

    startTimerHz (24);
}

void TransportBar::refreshPresets()
{
    presetList = ctx.presets.all();
    presetBox.clear (juce::dontSendNotification);
    int id = 1;
    bool addedHeading = false;
    for (auto& p : presetList)
    {
        if (! p.factory && ! addedHeading)
        {
            presetBox.addSeparator();
            presetBox.addSectionHeading ("Your presets");
            addedHeading = true;
        }
        presetBox.addItem (p.name, id++);
    }
    const auto last = ctx.settings.getValue ("lastPreset");
    for (int i = 0; i < (int) presetList.size(); ++i)
        if (presetList[(size_t) i].name == last)
            presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
}

void TransportBar::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presetList.size())) return;
    ctx.audio.chain.fromVar (presetList[(size_t) index].state);
    ctx.settings.setValue ("lastPreset", presetList[(size_t) index].name);
}

void TransportBar::savePreset()
{
    saveDialog = std::make_unique<juce::AlertWindow> ("Save preset", "Name this rig so you can come back to it.",
                                                      juce::MessageBoxIconType::NoIcon, this);
    saveDialog->addTextEditor ("name", presetBox.getText().isNotEmpty() ? presetBox.getText() + " (mine)" : "My rig");
    saveDialog->addButton ("Save preset", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveDialog != nullptr)
        {
            const auto name = saveDialog->getTextEditorContents ("name").trim();
            if (name.isNotEmpty() && ctx.presets.saveUserPreset (name, ctx.audio.chain))
            {
                ctx.settings.setValue ("lastPreset", juce::File::createLegalFileName (name));
                refreshPresets();
            }
        }
        saveDialog = nullptr;
    }), false);
}

void TransportBar::timerCallback()
{
    auto& a = ctx.audio;
    playButton.setToggleState (a.isPlaying() && ! a.isRecording(), juce::dontSendNotification);
    playButton.setIcon (a.isPlaying() ? Icons::stop() : Icons::play());
    recordButton.setToggleState (a.isRecording(), juce::dontSendNotification);
    metronomeButton.setToggleState (a.metronomeOn.load(), juce::dontSendNotification);
    (a.demoInput.load() ? demoChip : guitarChip).setToggleState (true, juce::dontSendNotification);
    repaint (lcd);
}

void TransportBar::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    g.setColour (panel);
    g.fillRect (getLocalBounds());
    g.setColour (line.withAlpha (0.7f));
    g.fillRect (getLocalBounds().removeFromBottom (1));

    // LCD-style position display
    auto r = lcd.toFloat();
    g.setColour (juce::Colour (0xff111316));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (line.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

    auto& a = ctx.audio;
    const double sr = a.sampleRate();
    const auto pos = a.position();
    const double secs = (double) pos / sr;
    const double beatLen = 60.0 / juce::jmax (30.0, a.session.bpm.load());
    const int bpb = a.session.beatsPerBar.load();
    const auto beatIndex = (juce::int64) std::floor (secs / beatLen);
    const int bar = (int) std::floor ((double) beatIndex / bpb) + 1;
    const int beat = (int) (((beatIndex % bpb) + bpb) % bpb) + 1;

    const auto lit = a.isRecording() ? glow : brass;
    auto inner = r.reduced (14.0f, 6.0f);
    g.setColour (lit);
    g.setFont (Theme::cond (30.0f, Theme::Weight::bold));
    const bool countingIn = a.isRecording() && pos < a.recordingStart();
    g.drawText (countingIn ? juce::String ("COUNT " + juce::String (beat))
                           : juce::String (bar).paddedLeft ('0', 3) + "." + juce::String (beat),
                inner.removeFromLeft (inner.getWidth() * 0.55f), juce::Justification::centredLeft, false);
    const double absSecs = juce::jmax (0.0, secs);
    g.setColour (lit.withAlpha (0.7f));
    g.setFont (Theme::cond (19.0f, Theme::Weight::semibold));
    g.drawText (juce::String ((int) absSecs / 60) + ":" + juce::String (std::fmod (absSecs, 60.0), 1).paddedLeft ('0', 4),
                inner, juce::Justification::centredRight, false);

    if (a.isRecording())
    {
        g.setColour (glow);
        g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre ({ r.getRight() - 10, r.getY() + 10 }));
    }

    g.setColour (ink3);
    g.setFont (Theme::cond (12.0f, Theme::Weight::semibold));
    g.drawText ("OUT", outMeter.getBounds().translated (0, -16).withHeight (14), juce::Justification::centredLeft, false);
}

void TransportBar::resized()
{
    auto r = getLocalBounds().reduced (Theme::s5, 14);
    const bool narrow = r.getWidth() < 1120;

    auto left = r.removeFromLeft (292);
    presetBox.setBounds (left.removeFromLeft (210).withSizeKeepingCentre (210, 40));
    left.removeFromLeft (8);
    savePresetButton.setBounds (left.removeFromLeft (70).withSizeKeepingCentre (70, 40));

    auto right = r.removeFromRight (narrow ? 172 : 296);
    masterSlider.setVisible (! narrow);
    outMeter.setVisible (! narrow);
    if (! narrow)
    {
        auto meterCol = right.removeFromRight (112);
        masterSlider.setBounds (meterCol.removeFromTop (24));
        outMeter.setBounds (meterCol.withSizeKeepingCentre (meterCol.getWidth() - 8, 8).translated (0, 6));
        right.removeFromRight (12);
    }
    demoChip.setBounds (right.removeFromRight (78).withSizeKeepingCentre (78, 34));
    right.removeFromRight (6);
    guitarChip.setBounds (right.removeFromRight (84).withSizeKeepingCentre (84, 34));

    // Centre cluster: transport, position display, tempo
    countInChip.setVisible (! narrow);
    const int fixed = 44 + 6 + 52 + 6 + 52 + 12 + 12 + 84 + 6 + 40 + (narrow ? 0 : 6 + 80);
    const int lcdW = juce::jlimit (110, 200, r.getWidth() - fixed - 8);
    auto centre = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), fixed + lcdW), r.getHeight());
    rewindButton.setBounds (centre.removeFromLeft (44).withSizeKeepingCentre (44, 44));
    centre.removeFromLeft (6);
    playButton.setBounds (centre.removeFromLeft (52).withSizeKeepingCentre (52, 52));
    centre.removeFromLeft (6);
    recordButton.setBounds (centre.removeFromLeft (52).withSizeKeepingCentre (52, 52));
    centre.removeFromLeft (12);
    lcd = centre.removeFromLeft (lcdW).withSizeKeepingCentre (lcdW, 46);
    centre.removeFromLeft (12);
    bpmBox.setBounds (centre.removeFromLeft (84).withSizeKeepingCentre (84, 40));
    centre.removeFromLeft (6);
    metronomeButton.setBounds (centre.removeFromLeft (40).withSizeKeepingCentre (40, 40));
    centre.removeFromLeft (6);
    countInChip.setBounds (centre.removeFromLeft (80).withSizeKeepingCentre (80, 32));
}

} // namespace gg
