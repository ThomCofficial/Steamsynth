#include "plugin/PluginProcessor.h"
#include <JuceHeader.h>
#include <cmath>
#include <iostream>

int main(){
  juce::ScopedJuceInitialiser_GUI initialise;
  SteamSynthProcessor processor;
  processor.setRateAndBufferSizeDetails(48000.0,512);
  processor.prepareToPlay(48000.0,512);
  juce::AudioBuffer<float> buffer(2,512);
  juce::MidiBuffer midi;
  midi.addEvent(juce::MidiMessage::noteOn(1,48,(juce::uint8)100),0);
  double energy=0.0;
  for(int block=0;block<40;++block){
    buffer.clear();
    if(block==20)midi.addEvent(juce::MidiMessage::noteOff(1,48),0);
    processor.processBlock(buffer,midi);
    midi.clear();
    for(int i=0;i<buffer.getNumSamples();++i){
      const float l=buffer.getSample(0,i),r=buffer.getSample(1,i);
      if(!std::isfinite(l)||!std::isfinite(r)||std::abs(l)>1.0001f||std::abs(r)>1.0001f)return 1;
      energy+=double(l*l+r*r);
    }
  }
  if(energy<0.01)return 2;
  juce::MemoryBlock saved;
  processor.getStateInformation(saved);
  if(saved.getSize()==0)return 3;
  processor.pollinate();
  processor.undoPollination();
  processor.setStateInformation(saved.getData(),int(saved.getSize()));
  processor.setCurrentProgram(3);
  auto editor=std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
  if(!editor||editor->getWidth()<900||editor->getHeight()<500)return 4;
  juce::Image image(juce::Image::ARGB,800,450,true);
  juce::Graphics graphics(image);
  editor->paintEntireComponent(graphics,true);
  editor.reset();
  processor.releaseResources();
  std::cout<<"PASS: MIDI rendering, audio bounds, state, pollination, presets, GUI creation + painting\n";
  return 0;
}