#include "core/ChordLibrary.h"
#include "core/StudyProgress.h"
#include "core/QuizEngine.h"
#include "core/StrumSynth.h"
#include "core/PitchDetector.h"
#include "core/ChordRecognizer.h"
#include "dsp/SignalChain.h"
#include "dsp/Presets.h"
#include "audio/Session.h"

using namespace gg;

//==============================================================================
struct ChordLibraryTests : juce::UnitTest
{
    ChordLibraryTests() : juce::UnitTest ("Chord library", "GuitarGuider") {}

    void runTest() override
    {
        auto& lib = ChordLibrary::get();

        beginTest ("ids are unique and every chord is playable");
        juce::StringArray ids;
        for (auto& c : lib.all())
        {
            expect (! ids.contains (c.id), "duplicate id " + c.id);
            ids.add (c.id);
            expectGreaterOrEqual (c.soundingStrings(), 2, c.id);
            for (size_t s = 0; s < 6; ++s)
            {
                if (c.frets[s] > 0) expect (c.fingers[s] > 0, c.id + " fretted string without finger");
                if (c.frets[s] <= 0) expectEquals (c.fingers[s], 0, c.id + " finger on open/muted string");
            }
            // Shape must span at most 4 frets
            int lo = 99, hi = 0;
            for (auto f : c.frets) if (f > 0) { lo = juce::jmin (lo, f); hi = juce::jmax (hi, f); }
            if (hi > 0) expectLessOrEqual (hi - lo, 3, c.id + " stretch");
        }

        beginTest ("root note is part of the chord");
        for (auto& c : lib.all())
            expect ((c.pitchClassMask() & (1 << c.rootPitchClass())) != 0, c.id + " missing root");

        beginTest ("chord qualities are correct");
        auto pcs = [&] (const char* id) { return lib.find (id)->pitchClassMask(); };
        auto mask = [] (std::initializer_list<int> l) { int m = 0; for (auto p : l) m |= 1 << p; return m; };
        expectEquals (pcs ("C"),  mask ({ 0, 4, 7 }));
        expectEquals (pcs ("G"),  mask ({ 7, 11, 2 }));
        expectEquals (pcs ("Am"), mask ({ 9, 0, 4 }));
        expectEquals (pcs ("D7"), mask ({ 2, 6, 9, 0 }));
        expectEquals (pcs ("F"),  mask ({ 5, 9, 0 }));
        expectEquals (pcs ("Bm"), mask ({ 11, 2, 6 }));
        expectEquals (pcs ("Cmaj7"), mask ({ 0, 4, 7, 11 }));
        expectEquals (pcs ("E5"), mask ({ 4, 11 }));

        beginTest ("barre detection and base fret");
        expectEquals ((int) lib.find ("F")->barres().size(), 1);
        expectEquals (lib.find ("F")->barres()[0].fromString, 0);
        expectEquals (lib.find ("F")->barres()[0].toString, 5);
        expectEquals (lib.find ("C")->baseFret(), 1);
        expectEquals (lib.find ("Cm")->baseFret(), 3);
        expect (lib.find ("C")->barres().empty());

        beginTest ("decks reference real chords");
        for (auto& d : lib.builtInDecks())
        {
            expectGreaterOrEqual (d.chordIds.size(), 6, d.id);
            for (auto& id : d.chordIds) expect (lib.find (id) != nullptr, id);
        }
    }
};

//==============================================================================
struct QuizTests : juce::UnitTest
{
    QuizTests() : juce::UnitTest ("Quiz engine", "GuitarGuider") {}

    void runTest() override
    {
        auto deck = ChordLibrary::get().builtInDecks()[0].chordIds;

        beginTest ("flashcards sort into known and still learning");
        FlashcardSession fc (deck, true, 42);
        expectEquals (fc.size(), deck.size());
        fc.mark (true);
        fc.mark (false);
        fc.mark (false);
        expectEquals (fc.knownCards().size(), 1);
        expectEquals (fc.learningCards().size(), 2);
        expect (fc.back());
        expectEquals (fc.learningCards().size(), 1);
        while (! fc.finished()) fc.mark (true);
        expectEquals (fc.knownCards().size() + fc.learningCards().size(), deck.size());

        beginTest ("learn questions have 4 distinct options including the answer");
        LearnSession ls (deck, 7);
        for (int i = 0; i < 50 && ! ls.finished(); ++i)
        {
            auto& q = ls.question();
            expectEquals (q.options.size(), 4);
            expectEquals (q.options[q.correctIndex], q.target);
            juce::StringArray syms;
            for (auto& o : q.options) syms.addIfNotAlreadyThere (ChordLibrary::get().find (o)->symbol);
            expectEquals (syms.size(), 4);
            ls.answer (q.correctIndex);
            ls.next();
        }

        beginTest ("learn session finishes once every chord has a streak of 2");
        LearnSession all (deck, 99);
        int guard = 0;
        while (! all.finished() && guard++ < 500)
        {
            all.answer (all.question().correctIndex);
            all.next();
        }
        expect (all.finished());
        expectEquals (all.masteredCount(), deck.size());
        expectEquals (all.questionsAsked(), deck.size() * LearnSession::requiredStreak);

        beginTest ("a wrong answer resets the streak");
        LearnSession w (deck, 3);
        auto target = w.question().target;
        w.answer ((w.question().correctIndex + 1) % 4);
        expectEquals (w.streakFor (target), 0);

        beginTest ("match game pairs names with diagrams");
        MatchGame mg (deck, 6, 5);
        expectEquals ((int) mg.tiles().size(), 12);
        int a = -1, b = -1;
        for (int i = 0; i < 12; ++i)
            for (int j = 0; j < 12; ++j)
                if (i != j && mg.tiles()[(size_t) i].chordId == mg.tiles()[(size_t) j].chordId) { a = i; b = j; }
        expect (mg.tryMatch (a, b));
        expect (! mg.tryMatch (a, b));
        // Mismatch counts a mistake
        int c = -1, d = -1;
        for (int i = 0; i < 12 && c < 0; ++i)
            for (int j = 0; j < 12; ++j)
                if (! mg.tiles()[(size_t) i].matched && ! mg.tiles()[(size_t) j].matched
                    && mg.tiles()[(size_t) i].chordId != mg.tiles()[(size_t) j].chordId) { c = i; d = j; break; }
        expect (! mg.tryMatch (c, d));
        expectEquals (mg.mistakes(), 1);
    }
};

//==============================================================================
struct ProgressTests : juce::UnitTest
{
    ProgressTests() : juce::UnitTest ("Study progress", "GuitarGuider") {}

    void runTest() override
    {
        auto tmp = juce::File::createTempFile ("json");

        beginTest ("levels follow streaks");
        {
            StudyProgress p (tmp);
            expectEquals (p.stats ("C").level(), 0);
            p.recordAnswer ("C", false);
            expectEquals (p.stats ("C").level(), 1);
            p.recordAnswer ("C", true); p.recordAnswer ("C", true);
            expectEquals (p.stats ("C").level(), 2);
            p.recordAnswer ("C", true); p.recordAnswer ("C", true);
            expectEquals (p.stats ("C").level(), 3);

            Deck d { "mine", "My deck", "", { "C", "G", "Am" }, false };
            p.saveCustomDeck (d);
            expect (p.submitMatchTime ("mine", 20.0));
            expect (! p.submitMatchTime ("mine", 25.0));
            expect (p.submitMatchTime ("mine", 15.5));
        }

        beginTest ("round-trips through JSON");
        {
            StudyProgress p (tmp);
            expectEquals (p.stats ("C").correct, 4);
            expectEquals (p.stats ("C").wrong, 1);
            expectEquals ((int) p.customDecks().size(), 1);
            expectEquals (p.customDecks()[0].chordIds.size(), 3);
            expectWithinAbsoluteError (p.bestMatchTime ("mine"), 15.5, 0.001);
            p.deleteCustomDeck ("mine");
            expect (p.customDecks().empty());
        }
        tmp.deleteFile();
    }
};

//==============================================================================
struct PitchTests : juce::UnitTest
{
    PitchTests() : juce::UnitTest ("Pitch detection", "GuitarGuider") {}

    void runTest() override
    {
        const double sr = 48000.0;
        PitchDetector det;

        beginTest ("detects sine waves across the guitar range");
        for (float f : { 82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f, 659.26f })
        {
            std::vector<float> buf (4096);
            for (size_t i = 0; i < buf.size(); ++i)
                buf[i] = 0.5f * std::sin (juce::MathConstants<float>::twoPi * f * (float) i / (float) sr);
            auto r = det.detect (buf.data(), (int) buf.size(), sr);
            expectWithinAbsoluteError (r.frequency, f, f * 0.005f);
        }

        beginTest ("detects plucked strings (harmonic-rich)");
        for (int midi : { 40, 45, 50, 55, 59, 64 })
        {
            juce::AudioBuffer<float> b (1, 24000);
            b.clear();
            juce::Random rng (midi);
            StrumSynth::pluck (b, 0, 0, midi, sr, 0.5f, 2.0, rng);
            auto r = det.detect (b.getReadPointer (0) + 4000, 4096, sr);
            auto note = PitchDetector::toNote (r.frequency);
            expectEquals (note.midi, midi);
            expectLessThan (std::abs (note.cents), 15.0f);
        }

        beginTest ("silence returns no pitch");
        std::vector<float> silent (4096, 0.0f);
        expectEquals (det.detect (silent.data(), 4096, sr).frequency, 0.0f);

        beginTest ("note naming");
        auto n = PitchDetector::toNote (440.0f);
        expectEquals (n.name, juce::String ("A"));
        expectEquals (n.octave, 4);
        expectEquals (PitchDetector::toNote (82.41f).name, juce::String ("E"));
    }
};

//==============================================================================
struct ChordRecognitionTests : juce::UnitTest
{
    ChordRecognitionTests() : juce::UnitTest ("Chord recognition", "GuitarGuider") {}

    void runTest() override
    {
        const double sr = 48000.0;
        ChordRecognizer rec;
        auto& lib = ChordLibrary::get();

        beginTest ("recognises synthesized strums of common chords");
        int hits = 0, total = 0;
        for (auto id : { "C", "G", "D", "Am", "Em", "E", "A", "Dm", "F", "Bm" })
        {
            auto* chord = lib.find (id);
            auto audio = StrumSynth::renderChord (*chord, sr);
            const int start = (int) (sr * 0.3);
            expect (rec.analyse (audio.getReadPointer (0) + start, ChordRecognizer::fftSize, sr));
            const bool ok = rec.matches (*chord);
            if (ok) ++hits;
            else logMessage (juce::String ("  missed ") + id + " best=" + rec.best().chordId + " score=" + juce::String (rec.scoreFor (*chord)));
            ++total;
        }
        expectGreaterOrEqual (hits, total - 1, "recognised " + juce::String (hits) + "/" + juce::String (total));

        beginTest ("does not accept a clearly different chord");
        auto audio = StrumSynth::renderChord (*lib.find ("C"), sr);
        rec.analyse (audio.getReadPointer (0) + (int) (sr * 0.3), ChordRecognizer::fftSize, sr);
        expect (! rec.matches (*lib.find ("E")));
        expect (! rec.matches (*lib.find ("Bm")));
    }
};

//==============================================================================
struct DspTests : juce::UnitTest
{
    DspTests() : juce::UnitTest ("DSP chain", "GuitarGuider") {}

    static float runChain (SignalChain& chain, const juce::AudioBuffer<float>& input, int block)
    {
        juce::AudioBuffer<float> b (input);
        float peak = 0.0f;
        for (int pos = 0; pos < b.getNumSamples(); pos += block)
        {
            const int n = juce::jmin (block, b.getNumSamples() - pos);
            auto* d = b.getWritePointer (0) + pos;
            chain.process (d, n);
            for (int i = 0; i < n; ++i)
            {
                if (! std::isfinite (d[i])) return -1.0f;
                peak = juce::jmax (peak, std::abs (d[i]));
            }
        }
        return peak;
    }

    void runTest() override
    {
        const double sr = 48000.0;
        auto riff = StrumSynth::renderDemoRiff (sr);

        beginTest ("every pedal produces finite, bounded output");
        for (auto& info : PedalRegistry::all())
        {
            SignalChain chain;
            chain.prepare (sr, 256);
            auto fx = chain.addPedal (info.typeId);
            expect (fx != nullptr, info.typeId);
            // Max out every knob as a stress test
            for (auto& p : fx->params()) p->setNormalised (1.0f);
            auto peak = runChain (chain, riff, 256);
            expect (peak >= 0.0f, info.typeId + " produced NaN/inf");
            expectLessOrEqual (peak, 1.5f, info.typeId);
        }

        beginTest ("every amp model x cab is finite and audible");
        for (int m = 0; m < (int) AmpSim::models().size(); ++m)
            for (int c = 0; c < (int) AmpSim::cabs().size(); ++c)
            {
                SignalChain chain;
                chain.prepare (sr, 480);
                chain.amp().params()[AmpSim::pModel]->set ((float) m);
                chain.amp().params()[AmpSim::pCab]->set ((float) c);
                auto peak = runChain (chain, riff, 480);
                auto label = AmpSim::models()[(size_t) m].id + "/" + AmpSim::cabs()[(size_t) c].id;
                expect (peak > 0.02f, label + " too quiet: " + juce::String (peak));
                expectLessOrEqual (peak, 1.5f, label);
            }

        beginTest ("odd block sizes larger than prepared are handled");
        {
            SignalChain chain;
            chain.prepare (sr, 64);
            chain.addPedal ("delay");
            chain.addPedal ("reverb");
            expect (runChain (chain, riff, 1000) > 0.0f);
        }

        beginTest ("pedal editing and preset round trip");
        {
            SignalChain chain;
            chain.prepare (sr, 256);
            chain.addPedal ("overdrive");
            chain.addPedal ("delay");
            chain.addPedal ("chorus", 1);
            expectEquals (chain.pedals()[1]->typeId(), juce::String ("chorus"));
            chain.movePedal (0, 2);
            expectEquals (chain.pedals()[2]->typeId(), juce::String ("overdrive"));
            chain.pedals()[0]->param ("mix")->set (0.77f);
            chain.pedals()[1]->enabled = false;
            chain.amp().params()[AmpSim::pGain]->set (8.5f);

            auto json = juce::JSON::toString (chain.toVar());
            SignalChain other;
            other.fromVar (juce::JSON::parse (json));
            expectEquals (other.numPedals(), 3);
            expectWithinAbsoluteError (other.pedals()[0]->param ("mix")->get(), 0.77f, 1.0e-5f);
            expect (! other.pedals()[1]->enabled.load());
            expectWithinAbsoluteError (other.amp().value (AmpSim::pGain), 8.5f, 1.0e-5f);

            chain.removePedal (0);
            expectEquals (chain.numPedals(), 2);
        }

        beginTest ("factory presets load and run");
        for (auto& p : PresetManager::factoryPresets())
        {
            SignalChain chain;
            chain.prepare (sr, 256);
            chain.fromVar (p.state);
            auto peak = runChain (chain, riff, 256);
            expect (peak > 0.02f && peak <= 1.5f, p.name + " peak " + juce::String (peak));
        }
    }
};

//==============================================================================
struct SessionTests : juce::UnitTest
{
    SessionTests() : juce::UnitTest ("Session", "GuitarGuider") {}

    void runTest() override
    {
        beginTest ("tracks mix with volume, mute and solo");
        Session s;
        auto t0 = s.addTrack();
        auto t1 = s.addTrack();
        std::vector<float> ones (100, 0.5f);
        s.writeAudio (t0, 0, ones.data(), 100);
        s.writeAudio (t1, 50, ones.data(), 100);
        expectEquals ((int) s.lengthSamples(), 150);

        s.track (t0)->volume = 1.0f; s.track (t1)->volume = 1.0f;
        std::vector<float> l (10, 0.0f), r (10, 0.0f);
        s.mixInto (l.data(), r.data(), 10, 60);
        expectWithinAbsoluteError (l[0], 1.0f, 1.0e-4f);   // two tracks, centre pan: 0.5 * 2 * cos(pi/4)*sqrt2 = 1

        s.track (t1)->mute = true;
        std::fill (l.begin(), l.end(), 0.0f);
        s.mixInto (l.data(), r.data(), 10, 60);
        expectWithinAbsoluteError (l[0], 0.5f, 1.0e-4f);

        s.track (t1)->mute = false;
        s.track (t1)->solo = true;
        std::fill (l.begin(), l.end(), 0.0f);
        s.mixInto (l.data(), r.data(), 10, 10);   // only t1 audible; it starts at 50
        expectWithinAbsoluteError (l[0], 0.0f, 1.0e-6f);

        beginTest ("overdub replaces a region and keeps the rest");
        std::vector<float> twos (20, 0.2f);
        s.writeAudio (t0, 10, twos.data(), 20);
        expectWithinAbsoluteError (s.track (t0)->audio[5], 0.5f, 1.0e-6f);
        expectWithinAbsoluteError (s.track (t0)->audio[15], 0.2f, 1.0e-6f);
        expectWithinAbsoluteError (s.track (t0)->audio[40], 0.5f, 1.0e-6f);

        beginTest ("sample rate change resamples audio");
        s.setSampleRate (96000.0);
        expectWithinAbsoluteError ((double) s.track (t0)->audio.size(), 200.0, 2.0);

        beginTest ("WAV export");
        auto f = juce::File::createTempFile ("wav");
        expect (s.exportWav (f));
        expectGreaterThan (f.getSize(), (juce::int64) 44);
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        expect (s.importAudioFile (t1, f, fm));
        expectWithinAbsoluteError ((double) s.track (t1)->audio.size(), (double) s.lengthSamples(), 2.0);
        f.deleteFile();

        beginTest ("track limit");
        Session full;
        for (int i = 0; i < Session::maxTracks; ++i) expect (full.addTrack() >= 0);
        expectEquals (full.addTrack(), -1);
    }
};

static ChordLibraryTests chordLibraryTests;
static QuizTests quizTests;
static ProgressTests progressTests;
static PitchTests pitchTests;
static ChordRecognitionTests chordRecognitionTests;
static DspTests dspTests;
static SessionTests sessionTests;

int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("GuitarGuider");

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED: " + std::to_string (failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
