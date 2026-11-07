# STEAMSYNTH 🌿
**Botanical synthesizer — a living instrument, beautifully dirty.**

Hybrid band-limited source + four organic damped resonant modes, evolving per-voice growth, musical mutation, cassette saturation, stereo tape echo, eight-line feedback-delay-network Bloom reverb, polyphony, pitch bend, sustain, modulation, MIDI keyboard, twelve presets, user preset save/load, editable parameters, host automation, stereo metering and animated tape reels.

The hand-illustrated interface uses **runtime vector drawing and genuinely interactive sliders** rather than a static PNG faceplate. Values and scales are on the parchment surface, not printed onto knob caps.

## Local DSP tests (offline)
```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Windows VST3 + standalone (requires internet for JUCE FetchContent)
```sh
cmake -S . -B build-plugin -A x64 -DSTEAMSYNTH_BUILD_PLUGIN=ON
cmake --build build-plugin --config Release --parallel 2
```

JUCE 8 is downloaded by CMake during plugin configuration. GitHub Actions validates a Windows VST3 build and produces a downloadable build artifact. This repository does not include or distribute third-party binary dependencies.

## Release status
Development candidate: **not yet certified as release-ready**. Offline DSP tests may pass while host, sound-design, automation-audibility, GUI and actual VST3 binary validation are still outstanding. See `docs/VALIDATION.md`.