# SteamSynth — Product specification (working draft)

## North star
An expressive, CPU-conscious botanical instrument: an evolving alien plant inhabiting a beautifully degraded cassette-and-tape-echo world. It must sound compelling with effects bypassed. The interface is an open, hand-illustrated Victorian scientific plate: parchment, antique brass, restrained green, and no rectangular boxes around each control.

## Main performance controls
- **SEED**: continuous timbral morph, not a waveform menu.
- **GROW**: coordinated evolution of harmonic profile, resonator excitation, and motion over the duration of a note.
- **MUTATE**: bounded, reproducible variations in resonant structure and timbral irregularity; no unbounded random amplitude.
- **DIRT**: macro for cassette-style soft saturation, bandwidth, compression and subtle pitch drift; zero means genuinely clean.
- **BLOOM**: lush stereo diffuse reverb; zero means bypass.
- **ATTACK / RELEASE**: discreet amplitude envelope controls.
- **TAPE ECHO**: time (free/sync), feedback, wet mix, with repeat darkening and pitch movement.
- **POLLINATE / UNDO**: deterministic, seedable and undoable musically constrained patch mutations.
- **Advanced**: optional panel for precise parameter editing, without burdening the performance page.

## DSP architecture
MIDI -> voice manager -> organic excitation (band-limited source + shaped noise/impulses) -> stable, damped resonator bank -> envelope -> per-voice sum -> cassette -> tape echo -> bloom reverb -> master protection.

Polyphony target: 8–16 voices. All expensive spatial effects shared across voices. Use fixed allocations on the audio thread; smooth automatable parameters; denormal protection; bounded feedback and resonance; stable across 44.1–192 kHz. Oversample only nonlinear stages where measurements justify it.

## UI acceptance criteria
The approved visual is a reference, **not** a static background that substitutes for widgets. Every control has a genuine hit area, animated knob rotation, keyboard and meter updates, accessibility labels, proper focus states, high-DPI scaling and host parameter attachment. Legends and numeric markings belong on the faceplate, never on a knob cap. Test click/drag/double-click and host automation.

## Release gates (not yet met)
1. Compilable instrument plugin (VST3) with reproducible dependencies and Windows CI.
2. Automated tests for voice allocation, note lifecycle, envelope, oscillator/resonator, cassette, delay feedback, reverb, MIDI and parameter automation.
3. Audio-render tests: no NaNs, clipping explosions, zipper discontinuities or runaway feedback during stress.
4. Actual in-host tests in Cubase, including preset save/reload, sample-rate switching and GUI resizing.
5. Real animated assets, readable labels, responsive controls and functioning meters.
6. Preset library with sound-design review; CPU profiling on documented hardware.
7. Only claim “finished” after all gates pass.
