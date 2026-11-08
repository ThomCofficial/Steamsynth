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
 juce::Image image(juce::Image::ARGB,1600,900,true);
 juce::Graphics graphics(image);
 editor->paintEntireComponent(graphics,true);
 auto key=image.getPixelAt(600,830);
 if(key.getRed()>252&&key.getGreen()>252&&key.getBlue()>252)return 5;
 auto file=juce::File::getCurrentWorkingDirectory().getChildFile("SteamSynth-UI-smoke.png");
 auto stream=file.createOutputStream();
 if(!stream||!juce::PNGImageFormat().writeImageToStream(image,*stream))return 6;
 stream->flush();
 // Drive the actual visible sliders, just as a mouse gesture would.
 // This tests both rendering and the SliderAttachment -> parameter connection
 // without relying on an event loop that JUCE disables in plugin builds.
 const auto findSlider=[&](const char* name)->juce::Slider* {
  for(int i=0;i<editor->getNumChildComponents();++i){
   auto* child=editor->getChildComponent(i);
   if(child->getName()==name)return dynamic_cast<juce::Slider*>(child);
  }
  return nullptr;
 };
 // Verify each major knob is a LIVE rendered control, not a painted backdrop.
 struct Control {const char* id;int x,y;};
 const Control controls[]={
  {"seed",345,306},{"grow",600,306},{"mutate",850,306},
  {"dirt",1100,306},{"bloom",1360,306},
  {"attack",279,666},{"echoMix",779,666}
 };
 for(const auto& c:controls){
  auto* parameter=processor.state.getParameter(c.id);
  auto* slider=findSlider(c.id);
  if(!parameter||!slider){std::cerr<<"FAIL: knob missing: "<<c.id<<"\n";return 7;}
  slider->setValue(parameter->convertFrom0to1(.13f),juce::sendNotificationSync);
  const double lowValue=slider->getValue();
  const float lowParam=parameter->getValue();
  juce::Image low(juce::Image::ARGB,1600,900,true);
  juce::Graphics lowGraphics(low);editor->paintEntireComponent(lowGraphics,true);
  slider->setValue(parameter->convertFrom0to1(.87f),juce::sendNotificationSync);
  const double highValue=slider->getValue();
  const float highParam=parameter->getValue();
  if(std::abs(lowParam-.13f)>.02f||std::abs(highParam-.87f)>.02f){
   std::cerr<<"FAIL: slider gesture not reaching DSP parameter: "<<c.id
            <<" low="<<lowParam<<" high="<<highParam<<"\n";return 8;
  }
  if(std::abs(highValue-lowValue)<.5){
   std::cerr<<"FAIL: parameter-to-slider attachment not updating: "<<c.id
            <<" low="<<lowValue<<" high="<<highValue<<"\n";
   return 8;
  }
  juce::Image high(juce::Image::ARGB,1600,900,true);
  juce::Graphics highGraphics(high);editor->paintEntireComponent(highGraphics,true);
  int changed=0;
  const int r=(c.x>300&&c.y==306)?70:23;
  for(int y=c.y-r;y<c.y+r;y+=4)for(int x=c.x-r;x<c.x+r;x+=4){
   const auto a=low.getPixelAt(x,y),b=high.getPixelAt(x,y);
   if(std::abs(int(a.getRed())-int(b.getRed()))+
      std::abs(int(a.getGreen())-int(b.getGreen()))+
      std::abs(int(a.getBlue())-int(b.getBlue()))>35)++changed;
  }
  if(changed<20){
   int entire=0;
   for(int y=0;y<900;y+=8)for(int x=0;x<1600;x+=8){
    const auto a=low.getPixelAt(x,y),b=high.getPixelAt(x,y);
    if(std::abs(int(a.getRed())-int(b.getRed()))+
       std::abs(int(a.getGreen())-int(b.getGreen()))+
       std::abs(int(a.getBlue())-int(b.getBlue()))>35)++entire;
   }
   juce::Image directLow(juce::Image::ARGB,184,184,true),directHigh(juce::Image::ARGB,184,184,true);
   {juce::Graphics g(directLow);slider->getLookAndFeel().drawRotarySlider(g,0,0,184,184,.13f,-2.34f,2.34f,*slider);}
   {juce::Graphics g(directHigh);slider->getLookAndFeel().drawRotarySlider(g,0,0,184,184,.87f,-2.34f,2.34f,*slider);}
   int direct=0,opaque=0;
   for(int y=0;y<184;y+=3)for(int x=0;x<184;x+=3){
    const auto a=directLow.getPixelAt(x,y),b=directHigh.getPixelAt(x,y);
    if(a.getAlpha()>0)++opaque;
    if(std::abs(int(a.getRed())-int(b.getRed()))+
       std::abs(int(a.getGreen())-int(b.getGreen()))+
       std::abs(int(a.getBlue())-int(b.getBlue()))>35)++direct;
   }
   auto saveDiagnostic=[](const juce::Image& im,const juce::String& name){
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile(name);
    auto stream=file.createOutputStream();
    if(stream){juce::PNGImageFormat().writeImageToStream(im,*stream);stream->flush();}
   };
   saveDiagnostic(low,"SteamSynth-UI-knob-low.png");
   saveDiagnostic(high,"SteamSynth-UI-knob-high.png");
   saveDiagnostic(directLow,"SteamSynth-UI-direct-low.png");
   saveDiagnostic(directHigh,"SteamSynth-UI-direct-high.png");
   std::cerr<<"FAIL: knob not visually responding: "<<c.id
    <<" pixels="<<changed<<" entire="<<entire<<" direct="<<direct
    <<" opaque="<<opaque<<" low="<<lowValue<<" high="<<highValue
    <<" bounds="<<slider->getBounds().toString()
    <<" visible="<<slider->isVisible()
    <<" showing="<<slider->isShowing()
    <<" sameLAF="<<(&slider->getLookAndFeel()==&editor->getLookAndFeel())
    <<" children="<<editor->getNumChildComponents()<<"\n";
   return 8;
  }
  // The scale printed ABOVE the knob must not rotate or move.
  if(c.y==306&&low.getPixelAt(c.x,200)!=high.getPixelAt(c.x,200))return 9;
 }
 editor.reset();processor.releaseResources();
 std::cout<<"PASS: MIDI, bounded audio, state, pollination, presets, 7 interactive brass knobs, rendered UI screenshot\n";
 return 0;
}