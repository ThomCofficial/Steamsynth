#pragma once
#include <JuceHeader.h>
#include "dsp/DSP.h"
#include <array>
#include <atomic>

class SteamSynthProcessor final : public juce::AudioProcessor {
public:
 SteamSynthProcessor();
 ~SteamSynthProcessor() override = default;
 void prepareToPlay(double sampleRate,int samplesPerBlock) override;
 void releaseResources() override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 void processBlock(juce::AudioBuffer<double>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "SteamSynth";}
 bool acceptsMidi() const override {return true;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 25.;}
 int getNumPrograms() override {return 12;}
 int getCurrentProgram() override {return currentProgram;}
 void setCurrentProgram(int index) override;
 const juce::String getProgramName(int index) override;
 void changeProgramName(int index,const juce::String& name) override;
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 void pollinate();
 void undoPollination();
 bool canUndo() const noexcept {return canUndoMutation;}
 juce::AudioProcessorValueTreeState state;
 juce::MidiKeyboardState keyboard;
 std::atomic<float> meterL{0},meterR{0};
 std::atomic<uint64_t> sampleClock{0};
 juce::String presetName = "The Mangrove Choir";
 static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
private:
 steam::Instrument instrument;
 int currentProgram=0;
 juce::ValueTree undoState;
 bool canUndoMutation=false;
 juce::Random random{0x5316};
 std::array<bool,128> sustainReleased{};
 bool sustainDown=false;
 double tempo=120.;
 steam::Macro readMacros() const noexcept;
 void processMidi(const juce::MidiMessage&,const steam::Macro&) noexcept;
 template<typename Sample> void processTyped(juce::AudioBuffer<Sample>&,juce::MidiBuffer&);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteamSynthProcessor)
};