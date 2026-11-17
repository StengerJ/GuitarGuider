#include "MainComponent.h"

namespace gg
{

class GuitarGuiderApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override           { return false; }

    void initialise (const juce::String& commandLine) override
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName = "GuitarGuider";
        opts.filenameSuffix = ".settings";
        opts.folderName = "GuitarGuider";
        opts.osxLibrarySubFolder = "Application Support";
        properties.setStorageParameters (opts);

        lookAndFeel = std::make_unique<GuitarGuiderLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (lookAndFeel.get());

        progress = std::make_unique<StudyProgress> (StudyProgress::defaultStorageFile());
        presets = std::make_unique<PresetManager>();
        audio = std::make_unique<AudioEngine>();
        audio->initialise (*properties.getUserSettings());

        mainWindow = std::make_unique<MainWindow> (getApplicationName(), *audio, *progress, *presets,
                                                   *properties.getUserSettings(), commandLine);
    }

    void shutdown() override
    {
        if (audio != nullptr)
            audio->saveState (*properties.getUserSettings());
        if (mainWindow != nullptr)
            properties.getUserSettings()->setValue ("windowState", mainWindow->getWindowStateAsString());
        mainWindow = nullptr;
        audio = nullptr;
        presets = nullptr;
        progress = nullptr;
        properties.closeFiles();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        lookAndFeel = nullptr;
    }

    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr) mainWindow->toFront (true);
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, AudioEngine& audioEngine, StudyProgress& prog, PresetManager& pm,
                    juce::PropertiesFile& settings, const juce::String& commandLine)
            : DocumentWindow (name, Theme::Colours::bg, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent (audioEngine, prog, pm, settings, commandLine), true);
            setResizable (true, true);
            setResizeLimits (1080, 700, 10000, 10000);

            auto state = settings.getValue ("windowState");
            if (state.isEmpty() || ! restoreWindowStateFromString (state))
                centreWithSize (1320, 860);
            setVisible (true);
        }

        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    juce::ApplicationProperties properties;
    std::unique_ptr<GuitarGuiderLookAndFeel> lookAndFeel;
    std::unique_ptr<StudyProgress> progress;
    std::unique_ptr<PresetManager> presets;
    std::unique_ptr<AudioEngine> audio;
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace gg

START_JUCE_APPLICATION (gg::GuitarGuiderApplication)
