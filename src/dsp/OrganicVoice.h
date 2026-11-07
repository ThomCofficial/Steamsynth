#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace steamsynth::dsp {
inline float clamp01(float x) noexcept { return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f); }
inline float softClip(float x) noexcept { return x / (1.0f + std::abs(x)); }
class OrganicVoice {
public:
  void prepare(double sampleRate) noexcept {
    sampleRate_ = std::clamp(sampleRate, 8000.0, 384000.0);
    reset();
  }
  void reset() noexcept {
    phase_ = 0.0; age_ = 0.0; env_ = 0.0f; active_ = false; releasing_ = false;
    resonators_.fill(0.0f); rng_ = 0x9e3779b9u;
  }
  void noteOn(float hz, float velocity) noexcept {
    frequency_ = std::clamp(std::isfinite(hz) ? hz : 220.0f, 10.0f, static_cast<float>(sampleRate_ * 0.22));
    velocity_ = clamp01(velocity); age_ = 0.0; env_ = 0.0f;
    phase_ = 0.0; resonators_.fill(0.0f); active_ = true; releasing_ = false;
  }
  void noteOff() noexcept { releasing_ = true; }
  bool active() const noexcept { return active_; }
  void setMacros(float seed, float grow, float mutate, float attack, float release) noexcept {
    seed_ = clamp01(seed); grow_ = clamp01(grow); mutate_ = clamp01(mutate);
    attack_ = clamp01(attack); release_ = clamp01(release);
  }
  float process() noexcept {
    if (!active_) return 0.0f;
    const double dt = 1.0 / sampleRate_;
    age_ += dt;
    const float attackSec = 0.002f + attack_ * attack_ * 4.0f;
    const float releaseSec = 0.008f + release_ * release_ * 8.0f;
    const float coeff = std::exp(-1.0f / (static_cast<float>(sampleRate_) * (releasing_ ? releaseSec : attackSec)));
    env_ = (releasing_ ? 0.0f : 1.0f) + coeff * (env_ - (releasing_ ? 0.0f : 1.0f));
    if (releasing_ && env_ < 0.00002f) { active_ = false; return 0.0f; }
    const float motion = std::sin(static_cast<float>(age_ * (0.13 + grow_ * 0.31) * 2.0 * std::numbers::pi));
    const float shape = clamp01(seed_ * 0.75f + grow_ * (0.25f + 0.25f * motion));
    const double phaseStep = std::min(0.49, static_cast<double>(frequency_) / sampleRate_);
    phase_ += phaseStep;
    if (phase_ >= 1.0) phase_ -= 1.0;
    const float p = static_cast<float>(phase_ * 2.0 * std::numbers::pi);
    const float base = (1.0f - shape) * std::sin(p) + shape * (0.75f * std::sin(p) + 0.25f * std::sin(2.0f * p));
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    const float noise = (static_cast<float>(rng_ & 65535u) / 32767.5f - 1.0f) * mutate_ * 0.03f;
    // Damped one-pole resonant coloration: intentionally bounded for safety.
    const float excitation = base + noise;
    float texture = 0.0f;
    for (std::size_t i = 0; i < resonators_.size(); ++i) {
      const float a = 0.05f + 0.1f * static_cast<float>(i) + grow_ * 0.08f;
      resonators_[i] += a * (excitation - resonators_[i]);
      texture += resonators_[i] * (0.15f / static_cast<float>(i + 1));
    }
    return softClip((base * 0.65f + texture) * env_ * velocity_);
  }
private:
  double sampleRate_ = 48000.0, phase_ = 0.0, age_ = 0.0;
  float frequency_ = 220.0f, velocity_ = 0.8f, env_ = 0.0f;
  float seed_ = 0.5f, grow_ = 0.5f, mutate_ = 0.1f, attack_ = 0.1f, release_ = 0.3f;
  bool active_ = false, releasing_ = false;
  uint32_t rng_ = 0x9e3779b9u;
  std::array<float, 3> resonators_{};
};
}
