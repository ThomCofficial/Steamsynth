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
  processor.processBlock(buffer,midi);midi.clear();
  for(int i=0;i<buffer.getNumSamples();++i){
   const float l=buffer.getSample(0,i),r=buffer.getSample(1,i);
   if(!std::isfinite(l)||!std::isfinite(r)||std::abs(l)>1.0001f||std::abs(r)>1.0001f)return 1;
   energy+=double(l*l+r*r);
  }
 }
 if(energy<.01)return 2;
 juce::MemoryBlock saved;processor.getStateInformation(saved);
 if(saved.getSize()==0)return 3;
 processor.pollinate();processor.undoPollination();
 processor.setStateInformation(saved.getData(),int(saved.getSize()));
 processor.setCurrentProgram(3);
 auto editor=std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
 if(!editor||editor->getWidth()!=1600||editor->getHeight()!=900)return 4;
 // Paint the ACTUAL complete editor (children included) to a diagnostic PNG.
 // This exposes white keyboard overlays, missing assets and broken component geometry.
 juce::Image image(juce::Image::ARGB,1600,900,true);
 juce::Graphics graphics(image);
 editor->paintEntireComponent(graphics,true);
 auto key=image.getPixelAt(600,830);
 if(key.getRed()>252&&key.getGreen()>252&&key.getBlue()>252)return 5;
 auto file=juce::File::getCurrentWorkingDirectory().getChildFile("SteamSynth-UI-smoke.png");
 auto stream=file.createOutputStream();
 if(!stream||!juce::PNGImageFormat().writeImageToStream(image,*stream))return 6;
 stream->flush();
 editor.reset();processor.releaseResources();
 std::cout<<"PASS: MIDI, bounded audio, state, pollination, presets, layered editor, rendered UI screenshot\n";
 return 0;
}