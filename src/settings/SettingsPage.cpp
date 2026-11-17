#include "SettingsPage.h"
#include "core/StrumSynth.h"

namespace gg
{

namespace
{
    constexpr int labelW = 170, rowH = 40, rowGap = 10;
}

SettingsPage::SettingsPage (AppContext& c)
    : ctx (c), inputMeter ([&c] { return c.audio.takeInputPeak(); }, false)
{
    auto& dm = ctx.audio.deviceManager;

    for (auto* box : { &driverBox, &inputBox, &outputBox, &channelBox, &rateBox, &bufferBox })
        addAndMakeVisible (box);

    driverBox.onChange = [this]
    {
        if (updating) return;
        ctx.audio.deviceManager.setCurrentAudioDeviceType (driverBox.getText(), true);
        refresh();
    };
    inputBox.onChange = [this]
    {
        if (updating) return;
        const auto name = inputBox.getSelectedId() == 1 ? juce::String() : inputBox.getText();
        applySetup ([name] (auto& s) { s.inputDeviceName = name; s.useDefaultInputChannels = true; });
    };
    outputBox.onChange = [this]
    {
        if (updating) return;
        const auto name = outputBox.getText();
        applySetup ([name] (auto& s) { s.outputDeviceName = name; s.useDefaultOutputChannels = true; });
    };
    channelBox.onChange = [this]
    {
        if (updating) return;
        const int ch = channelBox.getSelectedItemIndex();
        applySetup ([ch] (auto& s)
        {
            s.useDefaultInputChannels = false;
            s.inputChannels.clear();
            s.inputChannels.setBit (juce::jmax (0, ch));
        });
        ctx.audio.inputChannel = 0;
    };
    rateBox.onChange = [this]
    {
        if (updating) return;
        const double sr = rateBox.getText().getDoubleValue();
        applySetup ([sr] (auto& s) { s.sampleRate = sr; });
    };
    bufferBox.onChange = [this]
    {
        if (updating) return;
        const int bs = bufferBox.getText().getIntValue();
        applySetup ([bs] (auto& s) { s.bufferSize = bs; });
    };

    inputGain.setSliderStyle (juce::Slider::LinearHorizontal);
    inputGain.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 24);
    inputGain.setRange (-12.0, 24.0, 0.5);
    inputGain.setTextValueSuffix (" dB");
    inputGain.setDoubleClickReturnValue (true, 0.0);
    inputGain.setValue (ctx.audio.chain.inputGainDb.load(), juce::dontSendNotification);
    inputGain.onValueChange = [this] { ctx.audio.chain.inputGainDb = (float) inputGain.getValue(); };
    inputGain.setTooltip ("Boost a quiet guitar signal before it hits the pedals and amp");
    addAndMakeVisible (inputGain);

    outputVolume.setSliderStyle (juce::Slider::LinearHorizontal);
    outputVolume.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 24);
    outputVolume.setRange (0.0, 1.0, 0.01);
    outputVolume.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    outputVolume.setValue (ctx.audio.masterVolume.load(), juce::dontSendNotification);
    outputVolume.onValueChange = [this] { ctx.audio.masterVolume = (float) outputVolume.getValue(); };
    addAndMakeVisible (outputVolume);

    monitorToggle.setToggleState (ctx.audio.monitorOn.load(), juce::dontSendNotification);
    monitorToggle.onClick = [this] { ctx.audio.monitorOn = monitorToggle.getToggleState(); };
    demoToggle.setToggleState (ctx.audio.demoInput.load(), juce::dontSendNotification);
    demoToggle.onClick = [this] { ctx.audio.demoInput = demoToggle.getToggleState(); };
    addAndMakeVisible (monitorToggle);
    addAndMakeVisible (demoToggle);

    variant (testButton, "ghost");
    testButton.onClick = [this]
    {
        ctx.audio.playPreview (StrumSynth::renderChord (*ChordLibrary::get().find ("G"), ctx.audio.sampleRate()));
    };
    addAndMakeVisible (testButton);

    variant (resetButton, "danger");
    resetButton.onClick = [this]
    {
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::NoIcon, "Reset study progress?",
            "This clears what you've mastered and your match times. Your custom decks stay.",
            "Reset progress", "Keep progress", this,
            juce::ModalCallbackFunction::create ([this] (int r) { if (r == 1) { ctx.progress.resetAll(); repaint(); } }));
    };
    addAndMakeVisible (resetButton);
    addAndMakeVisible (inputMeter);

    dm.addChangeListener (this);
    refresh();
    startTimerHz (4);
}

SettingsPage::~SettingsPage()
{
    ctx.audio.deviceManager.removeChangeListener (this);
}

void SettingsPage::changeListenerCallback (juce::ChangeBroadcaster*) { refresh(); }

void SettingsPage::applySetup (std::function<void (juce::AudioDeviceManager::AudioDeviceSetup&)> change)
{
    auto& dm = ctx.audio.deviceManager;
    auto setup = dm.getAudioDeviceSetup();
    change (setup);
    auto err = dm.setAudioDeviceSetup (setup, true);
    if (err.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "That audio setting didn't work",
                                                err + "\n\nTry a different device, sample rate or buffer size.");
    refresh();
}

void SettingsPage::refresh()
{
    const juce::ScopedValueSetter<bool> svs (updating, true);
    auto& dm = ctx.audio.deviceManager;

    driverBox.clear (juce::dontSendNotification);
    int id = 1;
    for (auto* t : dm.getAvailableDeviceTypes())
    {
        driverBox.addItem (t->getTypeName(), id);
        if (t->getTypeName() == dm.getCurrentAudioDeviceType()) driverBox.setSelectedId (id, juce::dontSendNotification);
        ++id;
    }

    auto setup = dm.getAudioDeviceSetup();
    inputBox.clear (juce::dontSendNotification);
    outputBox.clear (juce::dontSendNotification);
    inputBox.addItem ("No input", 1);
    if (auto* type = dm.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        id = 2;
        for (auto& n : type->getDeviceNames (true))
        {
            inputBox.addItem (n, id);
            if (n == setup.inputDeviceName) inputBox.setSelectedId (id, juce::dontSendNotification);
            ++id;
        }
        id = 1;
        for (auto& n : type->getDeviceNames (false))
        {
            outputBox.addItem (n, id);
            if (n == setup.outputDeviceName) outputBox.setSelectedId (id, juce::dontSendNotification);
            ++id;
        }
    }
    if (setup.inputDeviceName.isEmpty()) inputBox.setSelectedId (1, juce::dontSendNotification);

    channelBox.clear (juce::dontSendNotification);
    rateBox.clear (juce::dontSendNotification);
    bufferBox.clear (juce::dontSendNotification);
    if (auto* dev = dm.getCurrentAudioDevice())
    {
        auto names = dev->getInputChannelNames();
        const auto active = dev->getActiveInputChannels();
        for (int i = 0; i < names.size(); ++i)
        {
            channelBox.addItem ("Input " + juce::String (i + 1) + (names[i].isNotEmpty() ? "  (" + names[i] + ")" : juce::String()), i + 1);
            if (active[i] && channelBox.getSelectedId() == 0) channelBox.setSelectedId (i + 1, juce::dontSendNotification);
        }
        id = 1;
        for (auto r : dev->getAvailableSampleRates())
        {
            rateBox.addItem (juce::String ((int) r) + " Hz", id);
            if (std::abs (r - dev->getCurrentSampleRate()) < 1) rateBox.setSelectedId (id, juce::dontSendNotification);
            ++id;
        }
        id = 1;
        for (auto b : dev->getAvailableBufferSizes())
        {
            bufferBox.addItem (juce::String (b) + " samples", id);
            if (b == dev->getCurrentBufferSizeSamples()) bufferBox.setSelectedId (id, juce::dontSendNotification);
            ++id;
        }
    }
    channelBox.setEnabled (channelBox.getNumItems() > 0);
    repaint();
}

void SettingsPage::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    g.fillAll (bg);
    drawHeading (g, getLocalBounds().reduced (Theme::s6, 0).withTrimmedTop (34).withHeight (70), "Settings",
                 "Connect your guitar through an audio interface and use headphones to avoid feedback.");

    auto drawCard = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r.toFloat(), Theme::radiusMd);
        g.setColour (ink);
        g.setFont (Theme::sans (19.0f, Theme::Weight::semibold));
        g.drawText (title, r.reduced (22, 16).withHeight (26), juce::Justification::centredLeft, false);
    };
    drawCard (audioCard, "Audio interface");
    drawCard (playCard, "Playing");
    drawCard (studyCard, "Study");

    // Row labels
    g.setColour (ink2);
    g.setFont (Theme::sans (15.0f, Theme::Weight::medium));
    auto label = [&] (juce::Component& c, const juce::String& text)
    {
        g.drawText (text, c.getX() - labelW, c.getY(), labelW - 12, c.getHeight(), juce::Justification::centredLeft, false);
    };
    label (driverBox, "Driver");
    label (inputBox, "Input device");
    label (channelBox, "Guitar plugged into");
    label (outputBox, "Output device");
    label (rateBox, "Sample rate");
    label (bufferBox, "Buffer size");
    label (inputGain, "Input gain");
    label (outputVolume, "Output volume");
    label (inputMeter, "Input level");

    // Status line
    auto status = audioCard.reduced (22, 16);
    status = status.removeFromBottom (24);
    const double lat = ctx.audio.latencyMs();
    g.setFont (Theme::sans (14.0f));
    if (auto* d = ctx.audio.deviceManager.getCurrentAudioDevice())
    {
        g.setColour (lat > 0 && lat < 15.0 ? good : (lat < 30.0 ? brass : bad));
        g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre ({ (float) status.getX() + 4, (float) status.getCentreY() }));
        g.setColour (ink2);
        g.drawText (d->getName() + ",  " + juce::String (juce::roundToInt (d->getCurrentSampleRate() / 100.0) / 10.0, 1) + " kHz,  about "
                        + juce::String (lat, 1) + " ms round trip" + (lat >= 15.0 ? ". Lower the buffer size for a snappier feel." : ""),
                    status.withTrimmedLeft (16), juce::Justification::centredLeft, true);
    }
    else
    {
        g.setColour (bad);
        g.drawText ("No audio device is open. Pick an output device above.", status, juce::Justification::centredLeft, true);
    }

    // Study stats
    auto s = studyCard.reduced (22, 16);
    s.removeFromTop (36);
    int seen = 0, mastered = 0;
    for (auto& chord : ChordLibrary::get().all())
    {
        const auto& st = ctx.progress.stats (chord.id);
        if (st.correct + st.wrong > 0) ++seen;
        if (st.level() == 3) ++mastered;
    }
    g.setColour (ink2);
    g.setFont (Theme::sans (15.0f));
    g.drawText ("You've practised " + juce::String (seen) + " of " + juce::String ((int) ChordLibrary::get().all().size())
                    + " chords and mastered " + juce::String (mastered) + ".",
                s.removeFromTop (24), juce::Justification::centredLeft, false);
    g.setColour (ink3);
    g.setFont (Theme::sans (13.5f));
    g.drawText ("Progress is saved to " + StudyProgress::defaultStorageFile().getParentDirectory().getFullPathName(),
                s.removeFromTop (22), juce::Justification::centredLeft, true);
}

void SettingsPage::resized()
{
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (124);

    const int colW = juce::jmin (620, (r.getWidth() - 20) / 2);
    auto left = r.removeFromLeft (colW);
    r.removeFromLeft (20);
    auto right = r.removeFromLeft (colW);

    audioCard = left.removeFromTop (60 + 6 * (rowH + rowGap) + 40);
    auto a = audioCard.reduced (22, 16);
    a.removeFromTop (40);
    for (auto* c : { &driverBox, &inputBox, &channelBox, &outputBox, &rateBox, &bufferBox })
    {
        auto row = a.removeFromTop (rowH);
        c->setBounds (row.withTrimmedLeft (labelW));
        a.removeFromTop (rowGap);
    }

    playCard = right.removeFromTop (60 + 3 * (rowH + rowGap) + 2 * 40 + 60);
    auto p = playCard.reduced (22, 16);
    p.removeFromTop (40);
    inputGain.setBounds (p.removeFromTop (rowH).withTrimmedLeft (labelW));
    p.removeFromTop (rowGap);
    inputMeter.setBounds (p.removeFromTop (rowH).withTrimmedLeft (labelW).withSizeKeepingCentre (p.getWidth() - labelW, 10));
    p.removeFromTop (rowGap);
    outputVolume.setBounds (p.removeFromTop (rowH).withTrimmedLeft (labelW));
    p.removeFromTop (rowGap);
    monitorToggle.setBounds (p.removeFromTop (36));
    demoToggle.setBounds (p.removeFromTop (36));
    p.removeFromTop (12);
    testButton.setBounds (p.removeFromTop (40).withWidth (170));

    right.removeFromTop (20);
    studyCard = right.removeFromTop (170);
    resetButton.setBounds (studyCard.reduced (22, 16).removeFromBottom (40).withWidth (220));
}

} // namespace gg
