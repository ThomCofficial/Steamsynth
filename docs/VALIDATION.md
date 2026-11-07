# Release gate checklist

- [x] Offline C++ DSP tests for voice, polyphony, cassette, tape echo, reverb, extreme automation, output safety
- [ ] Windows VST3/standalone build in GitHub Actions and artifact verified
- [ ] Cubase instantiation and end-to-end MIDI/audio tested
- [ ] Detailed CPU profiles on documented hardware at 44.1/48/96/192 kHz and 8/16 voices
- [ ] Long-run resonance, delay, reverb and macro automation listening tests
- [ ] Preset save/reload and plugin parameter automation exercised in a host
- [ ] GUI scale, click/drag/wheel, meter response, accessible labels and animation review
- [ ] Listening/sound-design approval and a complete curated preset bank

The checks above must be marked complete based on measured results, **not inferred from code existing**.