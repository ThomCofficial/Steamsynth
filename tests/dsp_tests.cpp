#include "dsp/OrganicVoice.h"
#include <cassert>
#include <cmath>
#include <iostream>
using steamsynth::dsp::OrganicVoice;
int main() {
  OrganicVoice v;
  for (double sr : {44100., 48000., 96000., 192000.}) {
    v.prepare(sr);
    for (int n = 0; n < 20; ++n) {
      v.setMacros(n / 19.f, 1.f - n / 19.f, n / 19.f, 0.f, 0.f);
      v.noteOn(30.f + n * 700.f, 1.f);
      for (int i = 0; i < 20000; ++i) {
        if (i == 5000) v.noteOff();
        float s = v.process();
        assert(std::isfinite(s));
        assert(std::abs(s) <= 1.0f);
      }
    }
  }
  std::cout << "OrganicVoice finite and bounded across sample rates and parameter extremes\n";
}
