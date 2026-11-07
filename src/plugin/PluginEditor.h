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
 void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override {}
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
 class InvisibleKeyboard final : public juce::Component {
 public:
  explicit InvisibleKeyboard(juce::MidiKeyboardState& st):state(st){setInterceptsMouseClicks(true,false);}
  void paint(juce::Graphics&) override {} // No opaque JUCE keyboard over the original illustrated keys.
  void mouseDown(const juce::MouseEvent& e) override {updateNote(e.position);}
  void mouseDrag(const juce::MouseEvent& e) override {updateNote(e.position);}
  void mouseUp(const juce::MouseEvent&) override {release();}
  void mouseExit(const juce::MouseEvent&) override {release();}
 private:
  juce::MidiKeyboardState& state;
  int playing=-1;
  void release(){if(playing>=0){state.noteOff(1,playing,0.f);playing=-1;}}
  void updateNote(juce::Point<float> p){
   const float x=juce::jlimit(0.f,1.f,p.x/juce::jmax(1.f,float(getWidth())));
   const int next=juce::jlimit(48,95,48+int(x*48.f));
   if(next==playing)return;
   release();playing=next;state.noteOn(1,playing,0.85f);
  }
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