#include "StudioViews.h"

namespace gg
{

struct TrackArea::Lane : public juce::Component
{
    Lane (TrackArea& o, int i) : owner (o), index (i)
    {
        auto* t = owner.ctx.audio.session.track (index);
        name.setText (t->name, juce::dontSendNotification);
        name.setFont (Theme::sans (15.0f, Theme::Weight::semibold));
        name.setEditable (false, true, false);
        name.setTooltip ("Double-click to rename");
        name.setColour (juce::Label::textWhenEditingColourId, Theme::Colours::ink);
        name.setColour (juce::Label::backgroundWhenEditingColourId, Theme::Colours::bg);
        name.onTextChange = [this] { if (auto* tr = track()) tr->name = name.getText(); };
        addAndMakeVisible (name);

        armButton.getProperties().set ("filled", true);
        armButton.setActiveColour (Theme::Colours::glow);
        armButton.setTooltip ("Record onto this track");
        armButton.onClick = [this] { owner.armed = index; for (auto* l : owner.lanes) l->refresh(); };
        addAndMakeVisible (armButton);

        for (auto* b : { &muteButton, &soloButton })
        {
            variant (*b, "chip");
            b->setClickingTogglesState (true);
            addAndMakeVisible (b);
        }
        muteButton.setTooltip ("Mute");
        soloButton.setTooltip ("Solo");
        muteButton.onClick = [this] { if (auto* tr = track()) tr->mute = muteButton.getToggleState(); };
        soloButton.onClick = [this] { if (auto* tr = track()) tr->solo = soloButton.getToggleState(); };

        volume.setSliderStyle (juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange (0.0, 1.25);
        volume.setValue (t->volume.load(), juce::dontSendNotification);
        volume.setDoubleClickReturnValue (true, 0.8);
        volume.getProperties().set ("accent", (juce::int64) t->colour);
        volume.setTooltip ("Track volume");
        volume.onValueChange = [this] { if (auto* tr = track()) tr->volume = (float) volume.getValue(); };
        addAndMakeVisible (volume);

        deleteButton.setTooltip ("Delete track");
        deleteButton.onClick = [this]
        {
            auto& s = owner.ctx.audio.session;
            if (owner.ctx.audio.isRecording()) return;
            s.removeTrack (index);
            if (s.numTracks() == 0) s.addTrack ("Guitar 1");
            owner.armed = juce::jlimit (0, s.numTracks() - 1, owner.armed);
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TrackArea> (&owner)]
                                             { if (safe != nullptr) safe->rebuildLanes(); });
        };
        addAndMakeVisible (deleteButton);
        refresh();
    }

    Track* track() const { return owner.ctx.audio.session.track (index); }

    void refresh()
    {
        armButton.setToggleState (owner.armed == index, juce::dontSendNotification);
        if (auto* t = track())
        {
            muteButton.setToggleState (t->mute.load(), juce::dontSendNotification);
            soloButton.setToggleState (t->solo.load(), juce::dontSendNotification);
        }
        repaint();
    }

    void resized() override
    {
        auto h = getLocalBounds().removeFromLeft (headerW).reduced (12, 8);
        auto top = h.removeFromTop (28);
        armButton.setBounds (top.removeFromLeft (28));
        top.removeFromLeft (6);
        deleteButton.setBounds (top.removeFromRight (28));
        name.setBounds (top);
        h.removeFromTop (6);
        auto bottom = h.removeFromTop (26);
        muteButton.setBounds (bottom.removeFromLeft (30));
        bottom.removeFromLeft (4);
        soloButton.setBounds (bottom.removeFromLeft (30));
        bottom.removeFromLeft (10);
        volume.setBounds (bottom);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace Theme::Colours;
        auto* t = track();
        if (t == nullptr) return;
        const juce::Colour colour (t->colour);
        auto r = getLocalBounds();

        auto header = r.removeFromLeft (headerW);
        g.setColour (owner.armed == index ? raised : panel);
        g.fillRect (header);
        g.setColour (colour);
        g.fillRect (header.removeFromLeft (4));

        g.setColour (bg.brighter (0.02f));
        g.fillRect (r);
        g.setColour (line.withAlpha (0.5f));
        g.fillRect (getLocalBounds().removeFromBottom (1));

        // Bar lines
        const double barSecs = 60.0 / juce::jmax (30.0, owner.ctx.audio.session.bpm.load()) * owner.ctx.audio.session.beatsPerBar.load();
        for (int bar = (int) (owner.viewStart / barSecs); ; ++bar)
        {
            const float x = owner.timeToX (bar * barSecs) - (float) headerW;
            if (x > r.getWidth()) break;
            if (x < 0) continue;
            g.setColour (line.withAlpha (0.35f));
            g.fillRect ((float) r.getX() + x, (float) r.getY(), 1.0f, (float) r.getHeight());
        }

        // Waveform (min/max per pixel)
        const double sr = owner.ctx.audio.session.sampleRate();
        auto drawWave = [&] (const std::vector<float>& audio, juce::int64 offset, juce::Colour c)
        {
            if (audio.empty()) return;
            const float mid = (float) r.getCentreY(), amp = r.getHeight() * 0.44f;
            juce::RectangleList<float> rects;
            for (int px = 0; px < r.getWidth(); ++px)
            {
                const double t0 = owner.xToTime ((float) (r.getX() + px));
                const double t1 = owner.xToTime ((float) (r.getX() + px + 1));
                auto s0 = (juce::int64) (t0 * sr) - offset, s1 = (juce::int64) (t1 * sr) - offset;
                if (s1 <= 0 || s0 >= (juce::int64) audio.size()) continue;
                s0 = juce::jmax ((juce::int64) 0, s0);
                s1 = juce::jmin ((juce::int64) audio.size(), juce::jmax (s0 + 1, s1));
                float lo = 0, hi = 0;
                const auto step = juce::jmax ((juce::int64) 1, (s1 - s0) / 64);
                for (auto s = s0; s < s1; s += step) { lo = juce::jmin (lo, audio[(size_t) s]); hi = juce::jmax (hi, audio[(size_t) s]); }
                const float top = mid - juce::jmin (1.0f, hi * 1.6f) * amp, bot = mid - juce::jmax (-1.0f, lo * 1.6f) * amp;
                rects.addWithoutMerging ({ (float) (r.getX() + px), top, 1.0f, juce::jmax (1.0f, bot - top) });
            }
            g.setColour (c);
            g.fillRectList (rects);
        };

        if (! t->audio.empty())
        {
            // clip body
            const float x0 = juce::jmax ((float) r.getX(), owner.timeToX (0.0));
            const float x1 = juce::jmin ((float) r.getRight(), owner.timeToX ((double) t->audio.size() / sr));
            if (x1 > x0)
            {
                auto clip = juce::Rectangle<float> (x0, (float) r.getY() + 4, x1 - x0, (float) r.getHeight() - 8);
                g.setColour (colour.withAlpha (0.13f));
                g.fillRoundedRectangle (clip, 6.0f);
            }
            drawWave (t->audio, 0, colour.withAlpha (t->mute.load() ? 0.35f : 0.95f));
        }

        if (owner.ctx.audio.isRecording() && owner.ctx.audio.recordingTrack() == index)
            drawWave (owner.ctx.audio.liveTake(), owner.ctx.audio.recordingStart(), glow);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.x > headerW) owner.mouseDown (e.getEventRelativeTo (&owner));
        else { owner.armed = index; for (auto* l : owner.lanes) l->refresh(); }
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (e.getMouseDownX() > headerW) owner.mouseDrag (e.getEventRelativeTo (&owner)); }

    TrackArea& owner;
    int index;
    juce::Label name;
    IconButton armButton { "Arm", Icons::record() };
    juce::TextButton muteButton { "M" }, soloButton { "S" };
    juce::Slider volume;
    IconButton deleteButton { "Delete track", Icons::trash() };
};

//==============================================================================
TrackArea::TrackArea (AppContext& c) : ctx (c)
{
    formats.registerBasicFormats();

    for (auto* b : { &addTrackButton, &importButton, &exportButton })
    {
        variant (*b, "ghost");
        addAndMakeVisible (b);
    }
    addTrackButton.onClick = [this]
    {
        auto i = ctx.audio.session.addTrack();
        if (i >= 0) { armed = i; rebuildLanes(); }
    };
    importButton.setTooltip ("Load a backing track or loop (WAV, AIFF, FLAC, MP3) onto the armed track. You can also drag files onto a track.");
    importButton.onClick = [this] { importInto (armed); };
    exportButton.setTooltip ("Save the mix of all tracks as a WAV file");
    exportButton.onClick = [this] { exportMix(); };

    variant (loopChip, "chip");
    loopChip.setClickingTogglesState (true);
    loopChip.setTooltip ("Loop playback back to the start");
    loopChip.onClick = [this] { ctx.audio.loopPlayback = loopChip.getToggleState(); };
    addAndMakeVisible (loopChip);

    hScroll.setAutoHide (false);
    addAndMakeVisible (hScroll);

    rebuildLanes();
    startTimerHz (30);
}

TrackArea::~TrackArea() = default;

void TrackArea::rebuildLanes()
{
    lanes.clear();
    for (int i = 0; i < ctx.audio.session.numTracks(); ++i)
        addAndMakeVisible (lanes.add (new Lane (*this, i)));
    armed = juce::jlimit (0, juce::jmax (0, lanes.size() - 1), armed);
    addTrackButton.setEnabled (lanes.size() < Session::maxTracks);
    lastVersion = ctx.audio.session.version.load();
    resized();
    repaint();
}

juce::Rectangle<int> TrackArea::timelineArea() const
{
    return getLocalBounds().withTrimmedTop (toolbarH + rulerH).withTrimmedLeft (headerW).withTrimmedBottom (14);
}

float TrackArea::timeToX (double seconds) const
{
    return (float) headerW + (float) ((seconds - viewStart) * pixelsPerSecond);
}

double TrackArea::xToTime (float x) const
{
    return viewStart + (x - (float) headerW) / pixelsPerSecond;
}

void TrackArea::updateScroll()
{
    const double sr = ctx.audio.session.sampleRate();
    const double len = juce::jmax (30.0, (double) ctx.audio.session.lengthSamples() / sr + 15.0,
                                   (double) ctx.audio.position() / sr + 10.0);
    const double visible = timelineArea().getWidth() / pixelsPerSecond;
    hScroll.setRangeLimits (0.0, juce::jmax (len, visible));
    hScroll.setCurrentRange (viewStart, visible, juce::dontSendNotification);
}

void TrackArea::timerCallback()
{
    if (ctx.audio.session.version.load() != lastVersion || lanes.size() != ctx.audio.session.numTracks())
        rebuildLanes();

    if (hScroll.getCurrentRangeStart() != viewStart)
    {
        viewStart = hScroll.getCurrentRangeStart();
        repaint();
        for (auto* l : lanes) l->repaint();
    }

    // Follow the playhead while playing
    if (ctx.audio.isPlaying())
    {
        const double t = (double) ctx.audio.position() / ctx.audio.sampleRate();
        const double visible = timelineArea().getWidth() / pixelsPerSecond;
        if (t > viewStart + visible * 0.9 || t < viewStart)
            viewStart = juce::jmax (0.0, t - visible * 0.1);
        repaint();
        for (auto* l : lanes) l->repaint();
    }
    else
        repaint (getLocalBounds().withTrimmedTop (toolbarH));

    updateScroll();
}

void TrackArea::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds();
    g.setColour (panel);
    g.fillRoundedRectangle (r.toFloat(), Theme::radiusMd);

    auto toolbar = r.removeFromTop (toolbarH).reduced (14, 0);
    g.setColour (ink);
    g.setFont (Theme::sans (18.0f, Theme::Weight::semibold));
    g.drawText ("Tracks", toolbar, juce::Justification::centredLeft, false);

    // Ruler
    auto ruler = r.removeFromTop (rulerH).withTrimmedLeft (headerW);
    g.setColour (bg.darker (0.1f));
    g.fillRect (ruler);
    const double barSecs = 60.0 / juce::jmax (30.0, ctx.audio.session.bpm.load()) * ctx.audio.session.beatsPerBar.load();
    const int labelEvery = juce::jmax (1, (int) std::ceil (48.0 / (barSecs * pixelsPerSecond)));
    g.setFont (Theme::cond (13.0f, Theme::Weight::semibold));
    for (int bar = (int) (viewStart / barSecs); ; ++bar)
    {
        const float x = timeToX (bar * barSecs);
        if (x > ruler.getRight()) break;
        if (x < ruler.getX()) continue;
        g.setColour (ink3);
        g.fillRect (x, (float) ruler.getBottom() - 8.0f, 1.0f, 8.0f);
        if (bar % labelEvery == 0)
        {
            g.setColour (ink2);
            g.drawText (juce::String (bar + 1), juce::Rectangle<float> (x + 4.0f, (float) ruler.getY(), 40.0f, (float) ruler.getHeight()),
                        juce::Justification::centredLeft, false);
        }
    }

    // Empty state hint
    if (ctx.audio.session.lengthSamples() == 0 && ! ctx.audio.isRecording())
    {
        auto area = timelineArea().toFloat();
        g.setColour (ink3);
        g.setFont (Theme::sans (15.0f, Theme::Weight::medium));
        g.drawText ("Press record (R) to capture your playing through the rig, or drop a backing track here.",
                    area.withHeight (juce::jmin (area.getHeight(), 84.0f)).translated (16, 0), juce::Justification::centredLeft, true);
    }
}

void TrackArea::paintOverChildren (juce::Graphics& g)
{
    using namespace Theme::Colours;
    const double t = (double) ctx.audio.position() / ctx.audio.sampleRate();
    const float x = timeToX (juce::jmax (0.0, t));
    auto tl = timelineArea();
    if (x < (float) tl.getX() || x > (float) tl.getRight()) return;

    const auto c = ctx.audio.isRecording() ? glow : brass;
    const float top = (float) (toolbarH);
    g.setColour (c);
    g.fillRect (x - 0.75f, top + 6.0f, 1.5f, (float) tl.getBottom() - top - 6.0f);
    juce::Path head;
    head.addTriangle (x - 6.0f, top + 4.0f, x + 6.0f, top + 4.0f, x, top + 12.0f);
    g.fillPath (head);
}

void TrackArea::resized()
{
    auto r = getLocalBounds();
    auto toolbar = r.removeFromTop (toolbarH).reduced (12, 6);
    toolbar.removeFromLeft (80);
    toolbar.removeFromRight (112); // room for the page's rig toggle
    addTrackButton.setBounds (toolbar.removeFromLeft (104));
    toolbar.removeFromLeft (6);
    importButton.setBounds (toolbar.removeFromLeft (124));
    toolbar.removeFromLeft (6);
    exportButton.setBounds (toolbar.removeFromLeft (110));
    toolbar.removeFromLeft (10);
    loopChip.setBounds (toolbar.removeFromLeft (64).reduced (0, 2));

    r.removeFromTop (rulerH);
    hScroll.setBounds (r.removeFromBottom (12).withTrimmedLeft (headerW));
    const int laneH = juce::jlimit (64, 92, lanes.isEmpty() ? 80 : r.getHeight() / juce::jmax (1, lanes.size()));
    for (auto* l : lanes)
        l->setBounds (r.removeFromTop (laneH));
    updateScroll();
}

void TrackArea::mouseDown (const juce::MouseEvent& e)
{
    if (e.y > toolbarH && e.x > headerW)
        ctx.audio.setPosition ((juce::int64) (juce::jmax (0.0, xToTime ((float) e.x)) * ctx.audio.sampleRate()));
}

void TrackArea::mouseDrag (const juce::MouseEvent& e) { mouseDown (e); }

void TrackArea::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.mods.isCommandDown())
    {
        const double anchor = xToTime ((float) e.x);
        pixelsPerSecond = juce::jlimit (8.0, 400.0, pixelsPerSecond * (w.deltaY > 0 ? 1.2 : 1.0 / 1.2));
        viewStart = juce::jmax (0.0, anchor - (e.x - headerW) / pixelsPerSecond);
    }
    else
        viewStart = juce::jmax (0.0, viewStart - (w.deltaX != 0 ? w.deltaX : w.deltaY) * 200.0 / pixelsPerSecond);
    updateScroll();
    repaint();
    for (auto* l : lanes) l->repaint();
}

bool TrackArea::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (formats.findFormatForFileExtension (juce::File (f).getFileExtension()) != nullptr) return true;
    return false;
}

void TrackArea::filesDropped (const juce::StringArray& files, int, int y)
{
    int target = armed;
    for (int i = 0; i < lanes.size(); ++i)
        if (lanes[i]->getBounds().contains (lanes[i]->getX() + 1, y)) target = i;
    if (ctx.audio.session.importAudioFile (target, juce::File (files[0]), formats))
        rebuildLanes();
}

void TrackArea::importInto (int trackIndex)
{
    chooser = std::make_unique<juce::FileChooser> ("Import audio onto the track", juce::File(), formats.getWildcardForAllFormats());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, trackIndex] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile() && ctx.audio.session.importAudioFile (trackIndex, f, formats))
            rebuildLanes();
    });
}

void TrackArea::exportMix()
{
    if (ctx.audio.session.lengthSamples() == 0)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "Nothing to export yet",
                                                "Record something or import a file first, then export the mix.");
        return;
    }
    auto defaultFile = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                           .getChildFile ("GuitarGuider mix " + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H%M") + ".wav");
    chooser = std::make_unique<juce::FileChooser> ("Export mix as WAV", defaultFile, "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        if (! f.hasFileExtension ("wav")) f = f.withFileExtension ("wav");
        const bool ok = ctx.audio.session.exportWav (f);
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon,
                                                ok ? "Mix exported" : "Couldn't export the mix",
                                                ok ? "Saved to " + f.getFullPathName()
                                                   : "Check that the folder exists and you can write to it, then try again.");
    });
}

} // namespace gg
