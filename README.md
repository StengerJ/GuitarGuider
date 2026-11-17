# GuitarGuider

Open-source guitar training software for Windows (also builds on Linux), written in C++ with [JUCE](https://juce.com).

GuitarGuider has two halves:

- **Learn**: Quizlet-style study of guitar chords.
- **Studio**: plug your guitar in and play through a virtual amp, pedalboard and multitrack recorder.

## What's inside

### Learn
- **Decks**: open chords, sevenths, major sevenths, sus/add/colour chords, barre chords, power chords and the full library (47 voicings). You can also **make your own deck** from any chords.
- **Flashcards**: flip a card (click or Space) to see the name or the shape. Sort each card into *Still learning* (←) or *Know it* (→), then keep reviewing the ones that haven't stuck. Cards can show the diagram first, and the deck can be shuffled. **Hear it** (H) plays a synthesized strum of the chord.
- **Learn**: multiple-choice quiz that mixes "which chord is this?" with "find the shape". Each chord has to be answered correctly twice in a row, and a wrong answer resets it. Number keys 1-4 pick an answer.
- **Match**: pair 6 chord names with their diagrams against the clock. A wrong pair adds a second, and your best time per deck is saved.
- **Play along**: strum the chord on your real guitar. GuitarGuider listens through your interface, shows which notes it hears, and confirms when you've played it.
- Progress (mastery per chord, custom decks, best times) is saved automatically.

### Studio
- **Amp**: five voicings: Glass Clean, Tweed Breakup, Brit Crunch, Plexi Lead and Modern Hi-Gain. Each has Gain, Bass, Middle, Treble, Presence and Master, with 4x-oversampled tube-style clipping, a tone stack and power-amp sag.
- **Cabinet & room**: 1x12, 2x12, 4x12 or DI, plus mic position, room ambience and a noise gate.
- **Pedalboard**: add pedals, drag them to reorder, and click the footswitch to toggle (bypass is click-free). There are 15 pedals: compressor, clean boost, auto-wah, octaver, overdrive, distortion, fuzz, EQ, chorus, flanger, phaser, tremolo, delay, reverb and noise gate.
- **Presets**: 9 factory rigs. **Save** stores your own.
- **Recorder**: up to 8 tracks with a transport (play/stop/record, R / Space / Enter), metronome, count-in, tempo, loop, mute/solo/volume, waveform view and click-to-seek. Recordings are latency-compensated.
- **Backing tracks**: **Import audio** loads WAV, AIFF, FLAC or MP3 onto a track, or you can drag files onto a lane. **Export mix** writes a WAV file.
- **Demo guitar**: no guitar handy? Switch the input to *Demo* to loop a built-in riff through the rig.

### Tuner
Chromatic tuner with a needle and an in-tune zone, string targets, an adjustable A4 reference, and optional muting of the amp while you tune.

### Settings
Audio driver, input/output device, the input your guitar is plugged into, sample rate, buffer size (with a round-trip latency readout), input gain and monitoring.

## Plugging in your guitar
Use an audio interface (any USB guitar interface works) and pick it under **Settings → Input device**, then choose the input your guitar is in. For the lowest latency, choose the *Windows Audio (Low Latency Mode)* or *Exclusive Mode* driver and a 128-256 sample buffer. Use headphones or monitors, not laptop speakers next to a mic, to avoid feedback.

## Building on Windows

Requirements:
- Visual Studio 2022 or newer (or the free Build Tools) with the **Desktop development with C++** workload. This includes the Windows SDK.
- CMake 3.22+ and, optionally, Ninja. The VS "C++ CMake tools" component provides both.
- Git. JUCE 8 is downloaded automatically.

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -RunTests
```

This builds a Release version, runs the unit tests, installs to `%LOCALAPPDATA%\Programs\GuitarGuider` and adds a **GuitarGuider** shortcut to your desktop. Use `-NoInstall` to only build, or `-JuceDir C:\path\to\JUCE` to use an existing JUCE checkout.

Manual build:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\GuitarGuiderTests.exe
build\GuitarGuider.exe
```

> **Smart App Control:** Windows 11's Smart App Control blocks *every* unsigned program you compile yourself, including this one ("An Application Control policy has blocked this file"). To run a self-built copy, either turn Smart App Control off (Windows Security → App & browser control → Smart App Control) or sign the executable with a trusted code-signing certificate. The build itself never needs to run a self-built tool, so it compiles fine either way.

## Building on Linux

```bash
sudo apt install build-essential cmake ninja-build pkg-config libfreetype-dev libfontconfig1-dev \
     libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxcomposite-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
./build/GuitarGuiderTests && ./build/GuitarGuider
```

`tools/linux.Dockerfile` contains the same environment. `GuitarGuider --screenshot <dir>` renders every screen to PNG and exits, which is useful for checking the UI headlessly (e.g. under `xvfb-run`).

## Project layout

```
src/core      chord library, quiz logic, study progress, pitch detection, chord recognition, strum synth
src/dsp       pedals, amp/cab model, signal chain, presets
src/audio     audio engine (device I/O, transport, recording, metronome) and multitrack session
src/ui        theme / look-and-feel, widgets, chord diagram renderer
src/learn     deck browser, deck editor, flashcards, learn, match, play along
src/studio    transport, tracks, pedalboard, amp panel
src/tuner     tuner page
src/settings  audio & app settings
tests         unit tests (chords, quiz, progress, pitch, chord recognition, DSP, session)
```

## Licences
GuitarGuider's own code is MIT licensed (see `LICENSE`). It links **JUCE 8**, which is dual-licensed under AGPLv3 and a commercial licence. Binaries you distribute must comply with one of those. The bundled Barlow fonts are under the SIL Open Font License (`assets/fonts/OFL.txt`).
