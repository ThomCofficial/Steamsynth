#pragma once
#include "PluginProcessor.h"
#include <map>
#include <memory>
#include <vector>

// The original approved Victorian plate is the actual interface artwork.
// Controls are separate animated layers, never painted-on fake controls.
class BotanicalLookAndFeel final : public juce::LookAndFeel_V4 {
public:
 BotanicalLookAndFeel();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override {}
 void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override {}
private:
 juce::Image atlas;
};
class SteamSynthEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit SteamSynthEditor(SteamSynthProcessor&);
 ~SteamSynthEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 class InvisibleKeyboard final : public juce::MidiKeyboardComponent {
 public:
  explicit InvisibleKeyboard(juce::MidiKeyboardState& st):juce::MidiKeyboardComponent(st,juce::MidiKeyboardComponent::horizontalKeyboard){}
  void paint(juce::Graphics&) override {} // Click and MIDI handling only; approved painted keys stay visible.
 };
 SteamSynthProcessor& synth;
 BotanicalLookAndFeel botanical;
 juce::Image faceplate;
 std::map<std::string,std::unique_ptr<juce::Slider>> knobs;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 InvisibleKeyboard piano;
 juce::TextButton synthTab,fxTab,motionTab,pollinateSide,pollinateButton,undoButton,saveButton,programPrev,programNext,programSelect,polySelect;
 juce::ToggleButton sync;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
 std::unique_ptr<juce::FileChooser> chooser;
 int tab=0;
 float lastL=0,lastR=0;
 float motionPhase=0;
 int hotButton=0;
 void addKnob(const char* id);
 void setTab(int which);
 void timerCallback() override;
 void showSaveDialog();
 void showLoadDialog();
 void choosePreset();
 void choosePolyphony();
 void drawMovingParts(juce::Graphics&);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteamSynthEditor)
};