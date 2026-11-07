#pragma once
#include "PluginProcessor.h"
#include <array>
#include <map>
#include <memory>
#include <vector>

// All hardware is cut from the approved Victorian plate. No generic JUCE widget artwork.
class BotanicalLookAndFeel final : public juce::LookAndFeel_V4 {
public:
 BotanicalLookAndFeel();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
 void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override {}
 void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
 void setPollinationPhase(float v) noexcept {pollinationPhase=v;}
 juce::Image whiteKey(bool down) const {return down?whiteDown:whiteUp;}
 juce::Image blackKey(bool down) const {return down?blackDown:blackUp;}
 juce::Image flame() const {return flameSprite;}
private:
 juce::Image hardware;
 std::array<juce::Image,5> largeKnobs;
 std::array<juce::Image,9> smallKnobs;
 std::array<juce::Image,4> tabNormal,tabLit;
 std::array<juce::Image,3> presetHardware;
 juce::Image flower,whiteUp,whiteDown,blackUp,blackDown,led,flameSprite,undoSprite;
 float pollinationPhase=0.f;
 static void paintSprite(juce::Graphics&,const juce::Image&,juce::Rectangle<float>);
};

class SteamSynthEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit SteamSynthEditor(SteamSynthProcessor&);
 ~SteamSynthEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 // An actual 24-ivory-key / 17-ebony-key instrument: independent sprites,
 // proper chromatic black-key hit testing, MIDI illumination and note release.
 class BotanicalKeyboard final : public juce::Component {
 public:
  BotanicalKeyboard(juce::MidiKeyboardState&,BotanicalLookAndFeel&);
  ~BotanicalKeyboard() override;
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseExit(const juce::MouseEvent&) override;
  void focusLost(FocusChangeType) override;
 private:
  juce::MidiKeyboardState& state;
  BotanicalLookAndFeel& art;
  int held=-1;
  static int whiteNote(int i) noexcept;
  int noteAt(juce::Point<float>) const noexcept;
  void hold(int);
  void release();
 };
 SteamSynthProcessor& synth;
 BotanicalLookAndFeel botanical;
 juce::Image faceplate;
 std::map<std::string,std::unique_ptr<juce::Slider>> knobs;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 BotanicalKeyboard piano;
 juce::TextButton synthTab,fxTab,motionTab,pollinateSide,pollinateButton,undoButton,saveButton,programPrev,programNext,programSelect,polySelect;
 juce::ToggleButton sync;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
 std::unique_ptr<juce::FileChooser> chooser;
 int tab=0;
 float meterL=0,meterR=0,velocityL=0,velocityR=0;
 float motionPhase=0,pollinationPhase=0;
 juce::String lastEdited;
 int lastEditFrames=0;
 void addKnob(const char* id);
 void setTab(int);
 void timerCallback() override;
 void showSaveDialog();
 void showLoadDialog();
 void choosePreset();
 void choosePolyphony();
 void drawAnimatedHardware(juce::Graphics&);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteamSynthEditor)
};