#include "MainComponent.h"
#include "learn/LearnPage.h"
#include "studio/StudioPage.h"
#include "tuner/TunerPage.h"
#include "settings/SettingsPage.h"
#include "ui/Shootable.h"

namespace gg
{

namespace { constexpr int railWidth = 92; }

MainComponent::NavItem::NavItem (const juce::String& name, juce::Path p) : juce::Button (name), icon (std::move (p))
{
    setClickingTogglesState (false);
    setRadioGroupId (1001);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);
}

void MainComponent::NavItem::paintButton (juce::Graphics& g, bool over, bool)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().toFloat().reduced (10.0f, 4.0f);
    const bool on = getToggleState();

    if (on || over)
    {
        g.setColour (on ? raised : raised.withAlpha (0.5f));
        g.fillRoundedRectangle (r, 12.0f);
    }
    if (on)
    {
        // brass "pilot light" marker on the rail edge
        g.setColour (brass);
        g.fillRoundedRectangle (juce::Rectangle<float> (3.0f, r.getHeight() * 0.42f).withCentre ({ 3.0f, r.getCentreY() }), 1.5f);
    }

    auto iconArea = r.removeFromTop (r.getHeight() * 0.62f).withTrimmedTop (6.0f);
    Icons::draw (g, icon, iconArea.withSizeKeepingCentre (24.0f, 24.0f), on ? brass : (over ? ink : ink2), 1.8f);
    g.setColour (on ? ink : ink2);
    g.setFont (Theme::sans (13.0f, on ? Theme::Weight::semibold : Theme::Weight::medium));
    g.drawText (getName(), r.withTrimmedBottom (4.0f), juce::Justification::centredTop, false);

    if (hasKeyboardFocus (false))
    {
        g.setColour (brass);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (10.0f, 4.0f), 12.0f, 1.5f);
    }
}

//==============================================================================
MainComponent::MainComponent (AudioEngine& a, StudyProgress& p, PresetManager& pm, juce::PropertiesFile& s,
                              const juce::String& commandLine)
    : ctx { a, p, pm, s, nullptr },
      inputMeter ([&a] { return a.takeInputPeak(); }, true)
{
    ctx.showPage = [this] (int i) { showPage (i); };

    const std::pair<const char*, juce::Path> items[] {
        { "Learn", Icons::cards() }, { "Studio", Icons::amp() },
        { "Tuner", Icons::tuningFork() }, { "Settings", Icons::sliders() } };

    for (int i = 0; i < 4; ++i)
    {
        auto* b = nav.add (new NavItem (items[i].first, items[i].second));
        b->setTooltip (juce::String (items[i].first) + "  (Ctrl+" + juce::String (i + 1) + ")");
        b->onClick = [this, i] { showPage (i); };
        addAndMakeVisible (b);
    }
    inputMeter.setTooltip ("Guitar input level");
    addAndMakeVisible (inputMeter);

    setWantsKeyboardFocus (true);
    setSize (1320, 860);
    showPage (s.getIntValue ("lastPage", 0));

    // --screenshot <dir> renders every page to PNG and quits (used for headless UI checks)
    auto args = juce::StringArray::fromTokens (commandLine, true);
    const int idx = args.indexOf ("--screenshot");
    if (idx >= 0 && idx + 1 < args.size())
    {
        screenshotDir = args[idx + 1].unquoted();
        a.demoInput = true;
        startTimer (1500);
    }
}

MainComponent::~MainComponent()
{
    ctx.settings.setValue ("lastPage", current);
}

juce::Component* MainComponent::page (int index)
{
    if (pages[index] == nullptr)
    {
        switch (index)
        {
            case 0: pages[index] = std::make_unique<LearnPage> (ctx); break;
            case 1: pages[index] = std::make_unique<StudioPage> (ctx); break;
            case 2: pages[index] = std::make_unique<TunerPage> (ctx); break;
            default: pages[index] = std::make_unique<SettingsPage> (ctx); break;
        }
        addChildComponent (pages[index].get());
        resized();
    }
    return pages[index].get();
}

void MainComponent::showPage (int index)
{
    index = juce::jlimit (0, 3, index);
    if (index == current) return;

    for (int i = 0; i < 4; ++i)
        if (pages[i] != nullptr) pages[i]->setVisible (false);

    current = index;
    nav[index]->setToggleState (true, juce::dontSendNotification);
    auto* p = page (index);
    p->setVisible (true);
    p->grabKeyboardFocus();
}

void MainComponent::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    g.fillAll (bg);

    auto rail = getLocalBounds().removeFromLeft (railWidth);
    g.setColour (bg.darker (0.25f));
    g.fillRect (rail);
    g.setColour (line.withAlpha (0.6f));
    g.fillRect (rail.removeFromRight (1));

    // Logo: a pick shape with the initials
    auto logo = juce::Rectangle<float> (40.0f, 44.0f).withCentre ({ railWidth * 0.5f, 40.0f });
    juce::Path pick;
    pick.startNewSubPath (logo.getCentreX(), logo.getBottom());
    pick.cubicTo (logo.getX() - 2.0f, logo.getY() + logo.getHeight() * 0.55f, logo.getX(), logo.getY(),
                  logo.getCentreX(), logo.getY());
    pick.cubicTo (logo.getRight(), logo.getY(), logo.getRight() + 2.0f, logo.getY() + logo.getHeight() * 0.55f,
                  logo.getCentreX(), logo.getBottom());
    pick.closeSubPath();
    g.setColour (brass);
    g.fillPath (pick);
    g.setColour (bg);
    g.setFont (Theme::cond (19.0f, Theme::Weight::bold));
    g.drawText ("GG", logo.withTrimmedBottom (10.0f), juce::Justification::centred, false);

    g.setColour (ink3);
    g.setFont (Theme::cond (11.0f, Theme::Weight::semibold));
    g.drawText ("IN", juce::Rectangle<int> (0, getHeight() - 30, railWidth, 16), juce::Justification::centred, false);
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    auto rail = r.removeFromLeft (railWidth);
    rail.removeFromTop (78);
    for (auto* b : nav)
        b->setBounds (rail.removeFromTop (76));

    inputMeter.setBounds (juce::Rectangle<int> (10, 110).withCentre ({ railWidth / 2, getHeight() - 100 }));

    for (auto& p : pages)
        if (p != nullptr) p->setBounds (r);
}

bool MainComponent::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown())
    {
        const auto c = k.getTextCharacter();
        if (c >= '1' && c <= '4') { showPage (c - '1'); return true; }
        if (k.getKeyCode() >= '1' && k.getKeyCode() <= '4') { showPage (k.getKeyCode() - '1'); return true; }
    }
    return false;
}

void MainComponent::timerCallback()
{
    // Screenshot mode: walk every page and every state it offers
    static int step = 0;
    struct Shot { int page; int state; };
    static std::vector<Shot> plan;
    if (plan.empty())
    {
        for (int p = 0; p < 4; ++p)
        {
            auto* comp = page (p);
            const int n = dynamic_cast<Shootable*> (comp) != nullptr ? dynamic_cast<Shootable*> (comp)->numShots() : 1;
            for (int s = 0; s < n; ++s) plan.push_back ({ p, s });
        }
        stopTimer();
        startTimer (900);
    }

    if (step > 0)
    {
        // capture the state prepared on the previous tick
        auto prev = plan[(size_t) step - 1];
        auto img = createComponentSnapshot (getLocalBounds(), true, 1.0f);
        juce::File dir (screenshotDir);
        dir.createDirectory();
        juce::FileOutputStream os (dir.getChildFile (juce::String::formatted ("%02d_page%d_state%d.png", step, prev.page, prev.state)));
        os.setPosition (0); os.truncate();
        juce::PNGImageFormat().writeImageToStream (img, os);
    }

    if (step >= (int) plan.size())
    {
        stopTimer();
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return;
    }

    auto shot = plan[(size_t) step];
    current = -1;
    showPage (shot.page);
    if (auto* s = dynamic_cast<Shootable*> (page (shot.page)))
        s->prepareShot (shot.state);
    ++step;
}

} // namespace gg
