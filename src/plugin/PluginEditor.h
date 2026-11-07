#pragma once
#include "PluginProcessor.h"
#include <map>
#include <memory>
#include <vector>

class BotanicalLookAndFeel final : public juce::LookAndFeel_V4 {
public:
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class SteamSynthEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit SteamSynthEditor(SteamSynthProcessor&);
 ~SteamSynthEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 SteamSynthProcessor &synth;
 BotanicalLookAndFeel botanical;
 juce::Image art;
 std::map<std::string,std::unique_ptr<juce::Slider>> knobs;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 juce::MidiKeyboardComponent piano;
 juce::TextButton synthTab{"SYNTH"},fxTab{"FX"},motionTab{"MOTION"},advTab{"ADV"};
 juce::TextButton pollinate{"POLLINATE"},undo{"UNDO"},save{"SAVE"},load{"LOAD"};
 juce::ToggleButton sync;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
 juce::ComboBox programs, polyBox;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> polyAttachment;
 std::unique_ptr<juce::FileChooser> chooser;
 int tab=0;
 float lastL=0,lastR=0;
 void initArt();
 void addKnob(const char*,float,float,float);
 void setTab(int which);
 void timerCallback() override;
 void paintMovingParts(juce::Graphics&);
 void showSaveDialog();
 void showLoadDialog();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteamSynthEditor)
};