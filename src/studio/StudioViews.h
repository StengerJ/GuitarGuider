#pragma once

#include "AppContext.h"
#include "ui/Widgets.h"

namespace gg
{

//==============================================================================
/** Draggable tempo readout. */
class BpmBox : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BpmBox (Session& s);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    Session& session;
    double dragStart = 100.0;
    void set (double bpm);
};

//==============================================================================
class TransportBar : public juce::Component, private juce::Timer
{
public:
    explicit TransportBar (AppContext&);
    void paint (juce::Graphics&) override;
    void resized() override;

    void refreshPresets();
    std::function<int()> getArmedTrack;

private:
    AppContext& ctx;
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { "Save" };
    IconButton rewindButton { "Back to start", Icons::rewind() };
    IconButton playButton { "Play", Icons::play() };
    IconButton recordButton { "Record", Icons::record() };
    IconButton metronomeButton { "Metronome", Icons::metronome() };
    juce::TextButton countInChip { "Count-in" };
    BpmBox bpmBox;
    juce::TextButton guitarChip { "Guitar" }, demoChip { "Demo" };
    juce::Slider masterSlider;
    LevelMeter outMeter;
    juce::Rectangle<int> lcd;
    std::vector<Preset> presetList;
    std::unique_ptr<juce::AlertWindow> saveDialog;

    void timerCallback() override;
    void loadPreset (int index);
    void savePreset();
};

//==============================================================================
class TrackArea : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit TrackArea (AppContext&);
    ~TrackArea() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int x, int y) override;

    int armedTrack() const noexcept { return armed; }
    void rebuildLanes();

private:
    struct Lane;
    AppContext& ctx;
    juce::OwnedArray<Lane> lanes;
    int armed = 0;
    double pixelsPerSecond = 40.0, viewStart = 0.0;   // seconds at the left edge
    int lastVersion = -1;
    juce::AudioFormatManager formats;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::TextButton addTrackButton { "Add track" }, importButton { "Import audio" }, exportButton { "Export mix" };
    juce::TextButton loopChip { "Loop" };
    juce::ScrollBar hScroll { false };

    static constexpr int headerW = 232, rulerH = 26, toolbarH = 44;
    juce::Rectangle<int> timelineArea() const;
    float timeToX (double seconds) const;
    double xToTime (float x) const;
    void timerCallback() override;
    void updateScroll();
    void importInto (int trackIndex);
    void exportMix();
};

//==============================================================================
class PedalboardView : public juce::Component, private juce::Timer
{
public:
    explicit PedalboardView (AppContext&);
    ~PedalboardView() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    struct Pedal;
    void rebuild();
    void beginDrag (Pedal*, const juce::MouseEvent&);
    void dragPedal (Pedal*, const juce::MouseEvent&);
    void endDrag (Pedal*);
    void showPicker();

private:
    struct Board;
    AppContext& ctx;
    juce::Viewport viewport;
    std::unique_ptr<Board> board;
    int lastVersion = -1;
    int dragOffset = 0;
    Pedal* dragging = nullptr;
    void timerCallback() override;
};

//==============================================================================
class AmpPanel : public juce::Component, private juce::Timer
{
public:
    explicit AmpPanel (AppContext&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    AppContext& ctx;
    juce::OwnedArray<ParamKnob> mainKnobs;
    std::unique_ptr<ParamKnob> micKnob, roomKnob, gateKnob, inputKnob;
    juce::ComboBox cabBox;
    IconButton prevModel { "Previous amp", Icons::arrowLeft() }, nextModel { "Next amp", Icons::arrowRight() };
    IconButton powerButton { "Amp on/off", Icons::power() };
    juce::ToggleButton gateToggle;
    juce::Rectangle<int> face, leftSection, cabSection;
    int lastVersion = -1, lastModel = -1;

    int model() const;
    void setModel (int m);
    juce::Colour faceInk() const;
    void timerCallback() override;
};

} // namespace gg
