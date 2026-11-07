#include "PluginEditor.h"
#include <BinaryData.h>
#include <array>
#include <functional>
#include <cmath>
#include <numbers>

namespace {
using namespace juce;
constexpr float SX=1600.f/1672.f, SY=900.f/941.f;
struct Sprite { const char* id; int ax,ay,side; int cx,cy; float normal; };
// Coordinates are registered to the *approved* art, not approximate illustrated stand-ins.
constexpr Sprite artKnobs[]={
 {"seed",0,0,161,362,321,.70f}, {"grow",178,0,161,629,321,.70f},
 {"mutate",356,0,161,887,313,.70f}, {"dirt",534,0,161,1152,321,.70f},
 {"bloom",712,0,161,1425,321,.70f},
 {"attack",0,180,55,291,694,.62f}, {"release",89,180,55,400,694,.62f},
 {"time",178,180,55,598,691,.62f}, {"feedback",267,180,55,707,691,.62f},
 {"echoMix",356,180,55,805,691,.62f},
 {"glide",445,180,47,203,863,.62f},
 {"motionRate",534,180,45,1351,856,.62f},
 {"motionDepth",623,180,45,1435,855,.62f},
 {"motionAmount",712,180,45,1519,855,.62f},
 {"tone",534,180,45,1351,856,.62f}, {"drift",623,180,45,1435,855,.62f},
 {"reverbDamp",712,180,45,1519,855,.62f},
 {"spore",534,180,45,1351,856,.62f}, {"resonance",623,180,45,1435,855,.62f},
 {"width",712,180,45,1519,855,.62f},
};
const Sprite* findSprite(const String& id) {
 for(const auto& s:artKnobs)if(id==s.id)return &s;
 return nullptr;
}
const Colour amber=Colour::fromRGB(242,157,55),ink=Colour::fromRGB(62,40,20),parchment=Colour::fromRGB(231,198,152);
static void letterpress(Graphics& g,const String& s,Rectangle<float> r,float size=18.f){
 g.setColour(Colour::fromRGB(245,216,169));g.setFont(Font(FontOptions("Georgia",size,Font::italic)));g.drawFittedText(s,r.toNearestInt(),Justification::centred,1);
}
}
BotanicalLookAndFeel::BotanicalLookAndFeel(){
 atlas=juce::ImageCache::getFromMemory(BinaryData::knob_atlas_png,BinaryData::knob_atlas_pngSize);
}
void BotanicalLookAndFeel::drawRotarySlider(Graphics&g,int x,int y,int w,int h,float proportion,float,float,Slider& slider){
 if(auto* s=findSprite(slider.getName())) {
  if(!atlas.isValid())return;
  Graphics::ScopedSaveState scoped(g);
  const float delta=(proportion-s->normal)*4.40f;
  const float cx=x+w*.5f,cy=y+h*.5f;
  g.addTransform(AffineTransform::rotation(delta,cx,cy));
  g.drawImage(atlas,Rectangle<float>(float(x),float(y),float(w),float(h)),Rectangle<float>(float(s->ax),float(s->ay),float(s->side),float(s->side)),false);
 }
}
SteamSynthEditor::SteamSynthEditor(SteamSynthProcessor& p):AudioProcessorEditor(p),synth(p),piano(p.keyboard){
 setLookAndFeel(&botanical);
 faceplate=ImageCache::getFromMemory(BinaryData::faceplate_jpg,BinaryData::faceplate_jpgSize);
 for(const auto& spec:artKnobs)addKnob(spec.id);
 auto makeButton=[this](TextButton& b,const String& tooltip,std::function<void()> callback){
  b.setButtonText({});b.setTooltip(tooltip);b.onClick=std::move(callback);addAndMakeVisible(b);
 };
 makeButton(synthTab,"Synth",[this]{setTab(0);});
 makeButton(fxTab,"Effects",[this]{setTab(1);});
 makeButton(motionTab,"Motion",[this]{setTab(2);});
 makeButton(pollinateSide,"Pollinate: musical patch variation",[this]{synth.pollinate();repaint();});
 makeButton(pollinateButton,"Pollinate: musical patch variation",[this]{synth.pollinate();repaint();});
 makeButton(undoButton,"Undo pollination",[this]{synth.undoPollination();repaint();});
 makeButton(saveButton,"Save preset to a file",[this]{showSaveDialog();});
 makeButton(programPrev,"Previous preset",[this]{synth.setCurrentProgram((synth.getCurrentProgram()+synth.getNumPrograms()-1)%synth.getNumPrograms());repaint();});
 makeButton(programNext,"Next preset",[this]{synth.setCurrentProgram((synth.getCurrentProgram()+1)%synth.getNumPrograms());repaint();});
 makeButton(programSelect,"Choose a SteamSynth preset",[this]{choosePreset();});
 makeButton(polySelect,"Choose polyphony",[this]{choosePolyphony();});
 sync.setButtonText({});sync.setTooltip("Tape echo sync to host tempo");addAndMakeVisible(sync);
 syncAttachment=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(synth.state,"sync",sync);
 addAndMakeVisible(piano);piano.setAvailableRange(48,95);piano.setKeyWidth(33.f);
 setSize(1600,900);setResizable(true,true);setResizeLimits(960,540,1920,1080);
 if(getConstrainer()!=nullptr)getConstrainer()->setFixedAspectRatio(1600.0/900.0);
 setTab(0);startTimerHz(30);
}
SteamSynthEditor::~SteamSynthEditor(){stopTimer();setLookAndFeel(nullptr);}
void SteamSynthEditor::addKnob(const char* id){
 auto slider=std::make_unique<Slider>();slider->setName(id);
 slider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
 slider->setTextBoxStyle(Slider::NoTextBox,false,0,0);
 slider->setRotaryParameters(-2.4f,2.4f,true);
 slider->setPopupDisplayEnabled(true,true,this);
 slider->setScrollWheelEnabled(true);
 slider->setDoubleClickReturnValue(true,synth.state.getParameter(id)->getDefaultValue());
 slider->setTooltip(synth.state.getParameter(id)->getName(120));
 addAndMakeVisible(*slider);
 attachments.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(synth.state,id,*slider));
 knobs.emplace(id,std::move(slider));
}
void SteamSynthEditor::setTab(int which){
 tab=which;
 for(auto id:{"motionRate","motionDepth","motionAmount","tone","drift","reverbDamp","spore","resonance","width"})knobs.at(id)->setVisible(false);
 const char* const byMode[3][3]={{"motionRate","motionDepth","motionAmount"},{"tone","drift","reverbDamp"},{"spore","resonance","width"}};
 const int mode=which==1?1:which==3?2:0;
 for(auto id:byMode[mode])knobs.at(id)->setVisible(true);
 repaint();
}
void SteamSynthEditor::timerCallback(){
 lastL=lastL*.79f+synth.meterL.load(std::memory_order_relaxed)*.21f;
 lastR=lastR*.79f+synth.meterR.load(std::memory_order_relaxed)*.21f;
 motionPhase+=.075f;
 repaint();
}
void SteamSynthEditor::resized(){
 const float sx=float(getWidth())/1600.f,sy=float(getHeight())/900.f;
 const auto layout=[&](Component&c,float x,float y,float w,float h){c.setBounds(roundToInt(x*sx),roundToInt(y*sy),roundToInt(w*sx),roundToInt(h*sy));};
 for(const auto& s:artKnobs){
  const float diameter=float(s.side)*SX;
  layout(*knobs.at(s.id),SX*float(s.cx)-diameter*.5f,SY*float(s.cy)-diameter*.5f,diameter,diameter);
 }
 layout(synthTab,30,255,145,65);layout(fxTab,30,328,145,65);layout(motionTab,30,402,145,65);
 layout(pollinateSide,30,476,145,68);
 layout(programPrev,523,127,51,59);layout(programSelect,578,128,425,58);layout(programNext,1001,127,53,59);
 layout(saveButton,1063,130,62,56);layout(polySelect,121,779,94,37);
 layout(pollinateButton,988,620,89,90);layout(undoButton,1095,646,61,62);
 layout(sync,812,646,54,55);
 layout(piano,243,738,1007,143);
}
void SteamSynthEditor::drawMovingParts(Graphics&g){
 const float peak=(lastL+lastR)*.5f;
 const bool tapeSync=synth.state.getRawParameterValue("sync")->load(std::memory_order_relaxed)>.5f;
 // Lamp in the original plate remains alive; an unlit sync LED is deliberately dark.
 if(tapeSync){g.setColour(amber.withAlpha(.2f+.1f*peak));g.fillEllipse(810.f,644.f,23.f,23.f);}
 else{g.setColour(ink.withAlpha(.88f));g.fillEllipse(817.f,651.f,10.f,10.f);}
 // Subtle living light, never an arbitrary cartoon rendering over the engraving.
 const float breathe=.5f+.5f*std::sin(motionPhase*.31f);
 ColourGradient warm(amber.withAlpha(.18f+.14f*peak+.03f*breathe),85,120,amber.withAlpha(0.f),142,165,true);
 g.setGradientFill(warm);g.fillEllipse(27.f,48.f,122.f,196.f);
 // Genuine two-channel meter readings; separate moving needles track live stereo audio.
 for(int i=0;i<2;++i){
  const float value=jlimit(0.f,1.f,i?lastR:lastL);
  const float pivotX=(i?1498.f:1383.f),pivotY=704.f;
  const float a=(-.55f+value*.98f)+(i?.18f:-.18f);
  const float dx=std::sin(a)*72.f,dy=std::cos(a)*72.f;
  g.setColour(Colour::fromRGB(90,42,22).withAlpha(.96f));
  g.drawLine(pivotX,pivotY,pivotX+dx,pivotY-dy,1.9f);
 }
 // Motion graph performs even with no notes; its alpha is driven by the same main timer.
 Path osc;for(int i=0;i<=66;++i){
  const float x=1394.f+float(i)*1.42f;
  const float y=781.f+5.f*std::sin(float(i)*.22f+motionPhase);
  if(i==0)osc.startNewSubPath(x,y);else osc.lineTo(x,y);
 }
 g.setColour(Colour::fromRGB(250,194,80).withAlpha(.8f));g.strokePath(osc,PathStrokeType(1.4f));
 if(tab!=0){
  // Do not draw boxes on the main faceplate: only a subtle selection halo on the engraved tab.
  const float y=tab==1?334.f:408.f;
  g.setColour(amber.withAlpha(.21f));g.fillRoundedRectangle(35.f,y,139.f,58.f,14.f);
 }
 // Navigation is exact on the approved art; the program title is overprinted only after it changes.
 if(synth.presetName!="The Mangrove Choir"){
  g.setColour(Colour::fromRGB(53,35,20));g.fillRoundedRectangle(584.f,132.f,416.f,40.f,6.f);
  letterpress(g,synth.presetName,{591.f,134.f,401.f,35.f},19.f);
 }
 // Polyphony number is dynamically printed into the existing vintage indicator window.
 if(auto* p=synth.state.getRawParameterValue("poly")){
  const int voices=int(p->load());
  g.setColour(Colour::fromRGB(65,44,27));g.fillRoundedRectangle(165.f,775.f,37.f,24.f,2.f);
  g.setColour(Colour::fromRGB(246,210,149));
  g.setFont(Font(FontOptions("Georgia",17.f,Font::plain)));
  g.drawText(String(voices),{166,776,36,21},Justification::centred);
 }
 if(tab==1||tab==3){
  const String help=tab==1?"TONE         DRIFT        DAMP":"SPORE        RESONANCE        WIDTH";
  g.setColour(parchment.withAlpha(.94f));g.fillRoundedRectangle(1284.f,873.f,249.f,23.f,3.f);
  g.setColour(ink);g.setFont(Font(FontOptions("Georgia",13.f,Font::plain)));
  g.drawFittedText(help,{1287,875,240,19},Justification::centred,1);
 }
}
void SteamSynthEditor::paint(Graphics& g){
 if(faceplate.isValid())g.drawImage(faceplate,getLocalBounds().toFloat());
 else g.fillAll(parchment);
 Graphics::ScopedSaveState state(g);
 g.addTransform(AffineTransform::scale(float(getWidth())/1600.f,float(getHeight())/900.f));
 drawMovingParts(g);
}
void SteamSynthEditor::choosePreset(){
 PopupMenu menu;for(int i=0;i<synth.getNumPrograms();++i)menu.addItem(i+1,synth.getProgramName(i),true,i==synth.getCurrentProgram());
 menu.addSeparator();menu.addItem(3001,"Load user preset...");menu.addItem(3002,"Advanced parameters...");
 SafePointer<SteamSynthEditor> weak(this);
 menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&programSelect),[weak](int selected){if(!weak)return;if(selected==3001){weak->showLoadDialog();return;}if(selected==3002){weak->setTab(3);return;}if(selected>0&&selected<=weak->synth.getNumPrograms()){weak->synth.setCurrentProgram(selected-1);weak->repaint();}});
}
void SteamSynthEditor::choosePolyphony(){
 PopupMenu menu;const auto* p=synth.state.getParameter("poly");const int current=p?int(p->convertFrom0to1(p->getValue())):12;
 for(int i=1;i<=16;++i)menu.addItem(i,String(i)+" voices",true,i==current);
 SafePointer<SteamSynthEditor> weak(this);
 menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&polySelect),[weak](int selected){
  if(!weak||selected<1)return;
  if(auto* p=weak->synth.state.getParameter("poly")){
   p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(selected)));p->endChangeGesture();
  }
 });
}
void SteamSynthEditor::showSaveDialog(){
 auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("SteamSynth").getChildFile("Presets");dir.createDirectory();
 chooser=std::make_unique<FileChooser>("Save your garden",dir.getChildFile(synth.presetName+".stspreset"),"*.stspreset");
 SafePointer<SteamSynthEditor> weak(this);
 chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[weak](const FileChooser& fc){
  if(!weak)return;auto file=fc.getResult();if(file==File{})return;
  MemoryBlock data;weak->synth.presetName=file.getFileNameWithoutExtension();weak->synth.getStateInformation(data);
  file.replaceWithData(data.getData(),data.getSize());
 });
}
void SteamSynthEditor::showLoadDialog(){
 auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("SteamSynth").getChildFile("Presets");dir.createDirectory();
 chooser=std::make_unique<FileChooser>("Load your garden",dir,"*.stspreset");
 SafePointer<SteamSynthEditor> weak(this);
 chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[weak](const FileChooser& fc){
  if(!weak)return;auto file=fc.getResult();if(!file.existsAsFile())return;
  MemoryBlock data;if(file.loadFileAsData(data))weak->synth.setStateInformation(data.getData(),int(data.getSize()));
 });
}