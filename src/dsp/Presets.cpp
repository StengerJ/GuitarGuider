#include "Presets.h"

namespace gg
{

PresetManager::PresetManager (juce::File userDirectory) : dir (std::move (userDirectory)) {}

juce::File PresetManager::defaultDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("GuitarGuider").getChildFile ("Presets");
}

namespace
{
    // Compact builder: amp settings + list of (type, {param: value}) pedals.
    struct PedalDef { const char* type; std::initializer_list<std::pair<const char*, float>> params; };

    juce::var build (int model, float gain, float bass, float mid, float treble, float presence, float master,
                     int cab, float mic, float room, std::initializer_list<PedalDef> pedals, float gateDb = -62.0f)
    {
        SignalChain chain;
        auto& amp = chain.amp();
        amp.params()[AmpSim::pModel]->set ((float) model);
        amp.params()[AmpSim::pGain]->set (gain);
        amp.params()[AmpSim::pBass]->set (bass);
        amp.params()[AmpSim::pMid]->set (mid);
        amp.params()[AmpSim::pTreble]->set (treble);
        amp.params()[AmpSim::pPresence]->set (presence);
        amp.params()[AmpSim::pMaster]->set (master);
        amp.params()[AmpSim::pCab]->set ((float) cab);
        amp.params()[AmpSim::pMic]->set (mic);
        amp.params()[AmpSim::pRoom]->set (room);
        chain.gate().param ("threshold")->set (gateDb);

        for (auto& p : pedals)
            if (auto fx = chain.addPedal (p.type))
                for (auto& [id, v] : p.params)
                    if (auto* prm = fx->param (id)) prm->set (v);

        return chain.toVar();
    }
}

std::vector<Preset> PresetManager::factoryPresets()
{
    static const std::vector<Preset> list = []
    {
        std::vector<Preset> l;
        l.push_back ({ "Clean Sparkle",
                       build (0, 4.0f, 5.0f, 4.5f, 6.5f, 6.0f, 6.0f, 1, 0.2f, 0.2f,
                              { { "comp", { { "sustain", 4.0f } } },
                                { "chorus", { { "rate", 0.7f }, { "depth", 0.35f }, { "mix", 0.4f } } },
                                { "reverb", { { "size", 0.6f }, { "mix", 0.3f } } } }) });
        l.push_back ({ "Blues Breaker",
                       build (1, 5.5f, 5.5f, 6.0f, 5.0f, 5.0f, 6.0f, 0, 0.35f, 0.2f,
                              { { "overdrive", { { "drive", 3.5f }, { "tone", 5.5f }, { "level", 8.0f } } },
                                { "delay", { { "time", 320.0f }, { "feedback", 0.25f }, { "mix", 0.18f } } } }) });
        l.push_back ({ "Classic Crunch",
                       build (2, 6.0f, 5.0f, 6.5f, 6.0f, 5.5f, 6.5f, 2, 0.3f, 0.15f, {}) });
        l.push_back ({ "Lead Solo",
                       build (3, 7.0f, 5.0f, 7.0f, 6.0f, 6.0f, 6.0f, 2, 0.25f, 0.2f,
                              { { "boost", { { "gain", 6.0f }, { "bright", 2.0f } } },
                                { "delay", { { "time", 450.0f }, { "feedback", 0.35f }, { "mix", 0.28f } } },
                                { "reverb", { { "size", 0.5f }, { "mix", 0.22f } } } }) });
        l.push_back ({ "Metal Rhythm",
                       build (4, 7.5f, 6.0f, 4.0f, 6.5f, 6.0f, 6.0f, 2, 0.2f, 0.05f,
                              { { "overdrive", { { "drive", 0.5f }, { "tone", 6.5f }, { "level", 9.0f } } } }, -52.0f) });
        l.push_back ({ "Fuzz Garage",
                       build (1, 4.0f, 6.0f, 5.0f, 5.0f, 5.0f, 6.0f, 1, 0.4f, 0.15f,
                              { { "fuzz", { { "drive", 7.0f }, { "tone", 4.0f }, { "level", 7.0f } } } }) });
        l.push_back ({ "Ambient Swells",
                       build (0, 3.0f, 5.0f, 5.0f, 6.0f, 5.0f, 6.0f, 1, 0.3f, 0.3f,
                              { { "comp", { { "sustain", 6.0f } } },
                                { "flanger", { { "rate", 0.1f }, { "depth", 0.5f }, { "feedback", 0.3f }, { "mix", 0.3f } } },
                                { "delay", { { "time", 620.0f }, { "feedback", 0.6f }, { "mix", 0.4f }, { "tone", 4.0f } } },
                                { "reverb", { { "size", 0.95f }, { "tone", 4.0f }, { "mix", 0.55f } } } }) });
        l.push_back ({ "Funk Quack",
                       build (0, 3.5f, 4.5f, 6.0f, 6.5f, 6.0f, 6.0f, 1, 0.2f, 0.1f,
                              { { "comp", { { "sustain", 6.5f }, { "attack", 4.0f } } },
                                { "wah", { { "sens", 7.0f }, { "freq", 3.0f }, { "q", 7.0f } } } }) });
        l.push_back ({ "Surf Trem",
                       build (0, 4.5f, 5.0f, 4.0f, 7.0f, 6.0f, 6.0f, 1, 0.15f, 0.25f,
                              { { "tremolo", { { "rate", 6.0f }, { "depth", 0.55f } } },
                                { "reverb", { { "size", 0.75f }, { "tone", 8.0f }, { "mix", 0.45f } } } }) });
        for (auto& p : l) p.factory = true;
        return l;
    }();
    return list;
}

std::vector<Preset> PresetManager::all() const
{
    auto list = factoryPresets();

    juce::Array<juce::File> files;
    if (dir.isDirectory())
        files = dir.findChildFiles (juce::File::findFiles, false, "*.json");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0; });

    for (auto& f : files)
    {
        auto v = juce::JSON::parse (f.loadFileAsString());
        if (! v.isObject()) continue;
        list.push_back ({ f.getFileNameWithoutExtension(), v, false, f });
    }
    return list;
}

bool PresetManager::saveUserPreset (const juce::String& name, const SignalChain& chain)
{
    auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    dir.createDirectory();
    return dir.getChildFile (clean + ".json").replaceWithText (juce::JSON::toString (chain.toVar()));
}

bool PresetManager::deleteUserPreset (const juce::String& name)
{
    auto f = dir.getChildFile (juce::File::createLegalFileName (name) + ".json");
    return f.existsAsFile() && f.deleteFile();
}

} // namespace gg
