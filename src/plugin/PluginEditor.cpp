#include "PluginEditor.h"
#include <BinaryData.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

namespace {
using namespace juce;
constexpr float W=1600.f,H=900.f;
const Colour amber{0xfff3a848}, ink{0xff4b2d18};
struct KnobSpec {const char* id;float cx,cy,diameter;};
constexpr KnobSpec knobSpecs[]={
 {"seed",345,306,184},{"grow",600,306,184},{"mutate",850,306,184},
 {"dirt",1100,306,184},{"bloom",1360,306,184},
 {"attack",279,666,60},{"release",383,666,60},
 {"time",580,666,60},{"feedback",683,666,60},{"echoMix",779,666,60},
 {"glide",196,838,60},
 {"motionRate",1308,832,60},{"motionDepth",1393,832,60},{"motionAmount",1476,832,60},
 {"tone",1308,832,60},{"drift",1393,832,60},{"reverbDamp",1476,832,60},
 {"spore",1308,832,60},{"resonance",1393,832,60},{"width",1476,832,60}
};
static int indexOf(const String& id){
 for(int i=0;i<int(std::size(knobSpecs));++i)if(id==knobSpecs[i].id)return i;
 return -1;
}
static void italic(Graphics& g,const String& s,Rectangle<float> r,float size){
 g.setColour(Colour::fromRGB(243,213,164));
 g.setFont(Font(FontOptions("Georgia",size,Font::italic)));
 g.drawFittedText(s,r.toNearestInt(),Justification::centred,1);
}
}

BotanicalLookAndFeel::BotanicalLookAndFeel(){
 hardware=ImageCache::getFromMemory(BinaryData::hardware_png,BinaryData::hardware_pngSize);
 if(!hardware.isValid())return;
 auto crop=[this](int x,int y,int w,int h){return hardware.getClippedImage({x,y,w,h});};
 for(int i=0;i<5;++i)largeKnobs[i]=crop(i*190,0,184,184);
 for(int i=0;i<9;++i)smallKnobs[i]=crop(i*65,389,60,60);
 for(int i=0;i<4;++i){tabNormal[i]=crop(i*154,492,150,68);tabLit[i]=crop(i*154,565,150,68);}
 for(int i=0;i<3;++i)presetHardware[i]=crop(i*70,645,65,65);
 flower=crop(700,389,92,92);
 whiteUp=crop(0,738,40,132);whiteDown=crop(48,738,40,132);
 blackUp=crop(102,738,20,72);blackDown=crop(130,738,20,72);
 led=crop(170,738,32,32);flameSprite=crop(230,736,70,100);
 undoSprite=crop(400,645,65,65);
}
void BotanicalLookAndFeel::paintSprite(Graphics&g,const Image&im,Rectangle<float>r){
 if(im.isValid())g.drawImage(im,r,RectanglePlacement::stretchToFit,false);
}
void BotanicalLookAndFeel::drawRotarySlider(Graphics&g,int x,int y,int w,int h,float value,float,float,Slider& slider){
 const int i=indexOf(slider.getName());if(i<0)return;
 const float cx=x+w*.5f,cy=y+h*.5f;
 const Rectangle<float> dest{float(x),float(y),float(w),float(h)};
 const bool large=i<5;
 const int smallIndex=large?0:(i>=11?6+(i-11)%3:i-5);
 const Image& sprite=large?largeKnobs[unsigned(i)]:smallKnobs[unsigned(smallIndex)];
 if(!sprite.isValid())return;
 // Like Steamstrip: ONE photographed, transparent brass knob rotates as a rigid body.
 // No second ring, no stationary pointer on the medallion, no baked knob on the plate.
 // Calibrate each asset's photographed pointer angle so MIDI/host automation and
 // mouse dragging agree with the printed -134 to +134 degree dial scale.
 constexpr float photographedLarge=.61f;
 constexpr float photographedSmall[9]={.38f,-.50f,.42f,.35f,.35f,-.12f,0.f,0.f,0.f};
 const float photographed=large?photographedLarge:photographedSmall[smallIndex];
 const float angle=-2.34f+juce::jlimit(0.f,1.f,value)*4.68f-photographed;
 Graphics::ScopedSaveState guard(g);
 g.addTransform(AffineTransform::rotation(angle,cx,cy));
 paintSprite(g,sprite,dest);
}
void BotanicalLookAndFeel::drawButtonBackground(Graphics&g,Button& b,const Colour&,bool hover,bool down){
 const auto id=b.getName();
 const Rectangle<float> r=b.getLocalBounds().toFloat();
 if(id.startsWith("tab_")){
  const int n=id=="tab_synth"?0:id=="tab_fx"?1:id=="tab_motion"?2:3;
  paintSprite(g,tabNormal[unsigned(n)],r);
  // The amber hardware is independent; individual tabs fade between actual
  // unlit/illuminated photographs instead of overlaying a second GUI.
  {
   Graphics::ScopedSaveState litLayer(g);
   g.setOpacity(std::clamp(tabLights[unsigned(n)]+(hover?.15f:0.f),0.f,1.f));
   paintSprite(g,tabLit[unsigned(n)],r);
  }
  if(down){g.setColour(Colour::fromRGB(93,44,15).withAlpha(.16f));g.fillRoundedRectangle(r.reduced(7.f),12.f);}
  return;
 }
 if(id=="preset_prev"||id=="preset_next"||id=="preset_save"){
  const int n=id=="preset_prev"?0:id=="preset_next"?1:2;
  paintSprite(g,presetHardware[unsigned(n)],r);
  if(hover){g.setColour(amber.withAlpha(down?.23f:.13f));g.fillEllipse(r.reduced(13.f));}
  return;
 }
 if(id=="pollinate_main"){
  // Original golden brass medallion with actual cut-out transparency.
  // Never dim it with the translucent old faceplate asset.
  paintSprite(g,flower,down?r.translated(0.f,1.5f):r);
  const float glow=std::clamp(pollinationPhase,0.f,1.f);
  if(glow>.001f||hover){
   g.setColour(amber.withAlpha(.55f*glow+(hover?.10f:0.f)));
   g.drawEllipse(r.reduced(2.f),2.2f);
   g.setColour(Colour::fromRGB(255,221,139).withAlpha(.21f*glow));
   g.fillEllipse(r.reduced(14.f));
  }
  return;
 }
 if(id=="undo"){
  paintSprite(g,undoSprite,r);
  if(hover){g.setColour(amber.withAlpha(.18f));g.fillEllipse(r.reduced(12.f));}
 }
 // Other hit regions deliberately do not paint rectangular JUCE buttons.
}
void BotanicalLookAndFeel::drawToggleButton(Graphics&g,ToggleButton& b,bool hover,bool down){
 const auto r=b.getLocalBounds().toFloat();
 const auto ledRect=r.withSizeKeepingCentre(32.f,32.f);
 g.setColour(Colour::fromRGB(66,41,20));g.fillEllipse(ledRect.reduced(7.f));
 g.setColour(Colour::fromRGB(178,121,53));g.drawEllipse(ledRect.reduced(7.f),1.2f);
 if(b.getToggleState()){
  paintSprite(g,led,ledRect);
  if(hover||down){g.setColour(amber.withAlpha(.20f));g.fillEllipse(ledRect.expanded(3.f));}
 }else if(hover){g.setColour(amber.withAlpha(.20f));g.fillEllipse(ledRect.reduced(10.f));}
}

SteamSynthEditor::BotanicalKeyboard::BotanicalKeyboard(MidiKeyboardState& st,BotanicalLookAndFeel& a):state(st),art(a){
 setInterceptsMouseClicks(true,false);setWantsKeyboardFocus(true);
}
SteamSynthEditor::BotanicalKeyboard::~BotanicalKeyboard(){release();}
int SteamSynthEditor::BotanicalKeyboard::whiteNote(int i) noexcept{
 constexpr int offsets[]={0,2,4,5,7,9,11};
 return 48+(i/7)*12+offsets[i%7];
}
int SteamSynthEditor::BotanicalKeyboard::noteAt(Point<float> pos) const noexcept{
 if(pos.x<0.f||pos.x>=float(getWidth())||pos.y<0.f||pos.y>=float(getHeight()))return -1;
 const float whiteWidth=float(getWidth())/24.f;
 const float blackHeight=float(getHeight())*72.f/132.f;
 if(pos.y<blackHeight){
  for(int i=0;i<23;++i){
   const int lo=whiteNote(i),hi=whiteNote(i+1);
   if(hi-lo!=2)continue;
   const float bx=float(i+1)*whiteWidth-whiteWidth*.25f;
   if(pos.x>=bx&&pos.x<bx+whiteWidth*.5f)return lo+1;
  }
 }
 const int idx=juce::jlimit(0,23,int(pos.x/whiteWidth));
 return whiteNote(idx);
}
void SteamSynthEditor::BotanicalKeyboard::hold(int note){
 if(note==held)return;
 release();
 if(note>=0){held=note;state.noteOn(1,note,.85f);}
 repaint();
}
void SteamSynthEditor::BotanicalKeyboard::release(){
 if(held>=0){state.noteOff(1,held,0.f);held=-1;repaint();}
}
void SteamSynthEditor::BotanicalKeyboard::mouseDown(const MouseEvent& e){grabKeyboardFocus();hold(noteAt(e.position));}
void SteamSynthEditor::BotanicalKeyboard::mouseDrag(const MouseEvent& e){hold(noteAt(e.position));}
void SteamSynthEditor::BotanicalKeyboard::mouseUp(const MouseEvent&){release();}
void SteamSynthEditor::BotanicalKeyboard::mouseExit(const MouseEvent&){release();}
void SteamSynthEditor::BotanicalKeyboard::focusLost(FocusChangeType){release();}
void SteamSynthEditor::BotanicalKeyboard::paint(Graphics&g){
 const float kw=float(getWidth())/24.f;
 const float sy=float(getHeight())/132.f;
 // Each white key is an independently rendered and highlighted component.
 for(int i=0;i<24;++i){
  const bool down=state.isNoteOnForChannels(0xffff,whiteNote(i));
  const auto sprite=art.whiteKey(down);
  if(sprite.isValid())g.drawImage(sprite,{float(i)*kw,down?2.f*sy:0.f,kw,float(getHeight())-2.f*sy},RectanglePlacement::stretchToFit,false);
 }
 // Ebony keys are rendered after ivory keys and win mouse hit testing.
 for(int i=0;i<23;++i){
  const int lo=whiteNote(i),hi=whiteNote(i+1);if(hi-lo!=2)continue;
  const int note=lo+1;
  const bool down=state.isNoteOnForChannels(0xffff,note);
  const auto sprite=art.blackKey(down);
  const float bx=float(i+1)*kw-kw*.25f;
  if(sprite.isValid())g.drawImage(sprite,{bx,down?2.f*sy:0.f,kw*.5f,72.f*sy},RectanglePlacement::stretchToFit,false);
 }
}

SteamSynthEditor::SteamSynthEditor(SteamSynthProcessor& p):AudioProcessorEditor(p),synth(p),piano(p.keyboard,botanical){
 setLookAndFeel(&botanical);
 faceplate=ImageCache::getFromMemory(BinaryData::plate_jpg,BinaryData::plate_jpgSize);
 for(const auto& spec:knobSpecs)addKnob(spec.id);
 auto button=[this](TextButton&b,const char* name,const String&tip,std::function<void()> fn){
  b.setName(name);b.setButtonText({});b.setTooltip(tip);b.onClick=std::move(fn);addAndMakeVisible(b);
 };
 button(synthTab,"tab_synth","Synth",[this]{setTab(0);});
 button(fxTab,"tab_fx","Tape and Bloom effects",[this]{setTab(1);});
 button(motionTab,"tab_motion","Living modulation",[this]{setTab(2);});
 button(pollinateSide,"tab_pollinate","Pollination and variation controls",[this]{setTab(3);});
 button(pollinateButton,"pollinate_main","Pollinate: create a musical variation",[this]{synth.pollinate();pollinationPhase=1.f;repaint();});
 button(undoButton,"undo","Undo pollination",[this]{synth.undoPollination();repaint();});
 button(saveButton,"preset_save","Save this garden",[this]{showSaveDialog();});
 button(programPrev,"preset_prev","Previous preset",[this]{synth.setCurrentProgram((synth.getCurrentProgram()+synth.getNumPrograms()-1)%synth.getNumPrograms());repaint();});
 button(programNext,"preset_next","Next preset",[this]{synth.setCurrentProgram((synth.getCurrentProgram()+1)%synth.getNumPrograms());repaint();});
 button(programSelect,"preset_display","Choose or load a preset",[this]{choosePreset();});
 button(polySelect,"poly_display","Choose number of voices",[this]{choosePolyphony();});
 sync.setName("tape_sync");sync.setButtonText({});sync.setTooltip("Sync tape echo to the host tempo");addAndMakeVisible(sync);
 syncAttachment=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(synth.state,"sync",sync);
 addAndMakeVisible(piano);
 setSize(1600,900);setResizable(true,true);setResizeLimits(960,540,1920,1080);
 if(getConstrainer())getConstrainer()->setFixedAspectRatio(16.0/9.0);
 setTab(0);startTimerHz(30);
}
SteamSynthEditor::~SteamSynthEditor(){stopTimer();setLookAndFeel(nullptr);}
void SteamSynthEditor::addKnob(const char* id){
 auto slider=std::make_unique<Slider>();slider->setName(id);
 slider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
 slider->setTextBoxStyle(Slider::NoTextBox,false,0,0);
 slider->setRotaryParameters(-2.34f,2.34f,true);
 slider->setPopupDisplayEnabled(false,false,nullptr);
 slider->setScrollWheelEnabled(true);
 if(auto* param=synth.state.getParameter(id)){
  slider->setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
  slider->setTooltip(param->getName(100));
 }
 slider->onDragStart=[this,id]{lastEdited=id;lastEditFrames=45;};
 // Host preset recall changes the dial silently; only intentional dragging opens the engraved value readout.
 addAndMakeVisible(*slider);
 attachments.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(synth.state,id,*slider));
 knobs.emplace(id,std::move(slider));
}
void SteamSynthEditor::setTab(int which){
 tab=juce::jlimit(0,3,which);
 botanical.setActiveTab(tab);
 for(auto id:{"motionRate","motionDepth","motionAmount","tone","drift","reverbDamp","spore","resonance","width"})knobs.at(id)->setVisible(false);
 const char* const pages[3][3]={{"motionRate","motionDepth","motionAmount"},{"tone","width","reverbDamp"},{"spore","resonance","drift"}};
 const int page=tab==1?1:tab==3?2:0;
 for(const auto id:pages[page])knobs.at(id)->setVisible(true);
 synthTab.setToggleState(tab==0,dontSendNotification);
 fxTab.setToggleState(tab==1,dontSendNotification);
 motionTab.setToggleState(tab==2,dontSendNotification);
 pollinateSide.setToggleState(tab==3,dontSendNotification);
 repaint();
}
void SteamSynthEditor::timerCallback(){
 const auto smooth=[](float previous,float target){return previous+(target-previous)*(target>previous?.32f:.09f);};
 meterL=smooth(meterL,juce::jlimit(0.f,1.f,synth.meterL.load(std::memory_order_relaxed)));
 meterR=smooth(meterR,juce::jlimit(0.f,1.f,synth.meterR.load(std::memory_order_relaxed)));
 const float rate=synth.state.getRawParameterValue("motionRate")->load(std::memory_order_relaxed);
 motionPhase+=.03f+.12f*rate;
 pollinationPhase*=.87f;
 botanical.setPollinationPhase(pollinationPhase);
 botanical.animateTabLights();
 synthTab.repaint();fxTab.repaint();motionTab.repaint();pollinateSide.repaint();
 pollinateButton.repaint();
 piano.repaint();
 if(lastEditFrames>0)--lastEditFrames;
 repaint();
}
void SteamSynthEditor::resized(){
 const float sx=float(getWidth())/W,sy=float(getHeight())/H;
 const auto place=[&](Component&c,float x,float y,float w,float h){c.setBounds(roundToInt(x*sx),roundToInt(y*sy),roundToInt(w*sx),roundToInt(h*sy));};
 for(const auto& spec:knobSpecs)place(*knobs.at(spec.id),spec.cx-spec.diameter*.5f,spec.cy-spec.diameter*.5f,spec.diameter,spec.diameter);
 place(synthTab,31,256,150,68);place(fxTab,31,327,150,68);place(motionTab,31,397,150,68);place(pollinateSide,31,467,150,68);
 place(programPrev,526,122,65,65);place(programSelect,594,130,406,50);place(programNext,970,122,65,65);place(saveButton,1012,122,65,65);
 place(polySelect,129,777,85,37);
 place(pollinateButton,988,623,92,92);place(undoButton,1093,640,65,65);
 place(sync,813,652,40,40);
 place(piano,269,751,960,132);
}
void SteamSynthEditor::drawAnimatedHardware(Graphics&g){
 const float breathe=.5f+.5f*std::sin(motionPhase*.64f);
 // The glass, frame and piping are stationary; flame, filament and spill light are independent.
 ColourGradient lamp(amber.withAlpha(.15f+.09f*breathe),101.f,149.f,amber.withAlpha(0.f),163.f,189.f,true);
 g.setGradientFill(lamp);g.fillEllipse(41.f,70.f,116.f,157.f);
 if(auto flame=botanical.flame();flame.isValid()){
  Graphics::ScopedSaveState save(g);
  g.setOpacity(.70f+.28f*breathe);
  g.drawImage(flame,{75.f,98.f,57.f,98.f},RectanglePlacement::stretchToFit,false);
 }
 // Real stereo needles with attack/release ballistics, not the reference's baked-in needle art.
 const auto needle=[&](float pivotX,float level,float shift){
  const float db=level<=.0001f?-60.f:20.f*std::log10(level);
  const float t=juce::jlimit(0.f,1.f,(db+45.f)/48.f);
  const float angle=-.60f+t*1.20f+shift;
  const float tipX=pivotX+std::sin(angle)*59.f;
  const float tipY=695.f-std::cos(angle)*59.f;
  g.setColour(Colour::fromRGB(73,37,17).withAlpha(.5f));g.drawLine(pivotX+1.f,696.f,tipX+1.f,tipY,2.5f);
  g.setColour(Colour::fromRGB(54,27,14));g.drawLine(pivotX,695.f,tipX,tipY,1.4f);
  g.setColour(Colour::fromRGB(170,94,43));g.fillEllipse(pivotX-2.5f,692.5f,5.f,5.f);
 };
 needle(1343.f,meterL,-.08f);needle(1410.f,meterR,.08f);
 // Real hand-engraved oscilloscope inside the botanical bay.
 // Its trace changes with the selected page: motion, tape/widening or growth.
 const float depth=synth.state.getRawParameterValue("motionDepth")->load(std::memory_order_relaxed);
 const float amount=synth.state.getRawParameterValue("motionAmount")->load(std::memory_order_relaxed);
 const float width=synth.state.getRawParameterValue("width")->load(std::memory_order_relaxed);
 const float mut=synth.state.getRawParameterValue("mutate")->load(std::memory_order_relaxed);
 {
  const Rectangle<float> scope(1341.f,774.f,99.f,30.f);
  g.setColour(Colour::fromRGB(65,38,20));g.fillRoundedRectangle(scope.expanded(3.f),6.f);
  g.setColour(Colour::fromRGB(164,103,39));g.drawRoundedRectangle(scope.expanded(3.f),6.f,2.f);
  g.setColour(Colour::fromRGB(27,40,31));g.fillRoundedRectangle(scope,4.f);
  Graphics::ScopedSaveState guard(g);g.reduceClipRegion(scope.toNearestInt().reduced(3));
  Path trace;
  for(int q=0;q<=92;++q){
   const float x=1344.f+q*1.0f;
   const float t=q*.16f+motionPhase;
   const float oscillation=tab==1?
     std::sin(t*.7f)*std::cos(t*.22f+.7f*width):
     tab==3?std::sin(t*.47f)*std::sin(t*.15f+1.f+mut):
     std::sin(t)*(.45f+.5f*std::sin(t*.26f));
   const float y=789.f-(4.5f+8.f*depth)*(tab==1?(.4f+.6f*width):(.3f+.7f*amount))*oscillation;
   if(q==0)trace.startNewSubPath(x,y);else trace.lineTo(x,y);
  }
  g.setColour(amber.withAlpha(.26f));g.strokePath(trace,PathStrokeType(3.2f));
  g.setColour(Colour::fromRGB(250,217,121));g.strokePath(trace,PathStrokeType(1.2f));
 }
 // Mutate's orange ring responds to its value, without spinning the scale.
 {
  const float light=.09f+.30f*mut*(.74f+.26f*breathe);
  Path ember;
  ember.addCentredArc(850.f,306.f,109.f,109.f,0.f,-2.18f,-1.24f,true);
  g.setColour(Colour::fromRGB(255,147,48).withAlpha(light*.35f));
  g.strokePath(ember,PathStrokeType(6.f));
  g.setColour(Colour::fromRGB(235,115,32).withAlpha(light));
  g.strokePath(ember,PathStrokeType(1.6f));
 }
 // Engraved preset and polyphony windows are independent of the buttons.
 // The antique preset display is already on the plate: no opaque rectangle.
 italic(g,synth.presetName,{595.f,140.f,390.f,29.f},21.f);
 if(auto* p=synth.state.getRawParameterValue("poly")){
  g.setColour(Colour::fromRGB(52,38,22));g.fillRoundedRectangle(148.f,783.f,41.f,21.f,2.f);
  g.setColour(Colour::fromRGB(245,211,150));g.setFont(Font(FontOptions("Georgia",17.f,Font::plain)));
  g.drawFittedText(String(juce::roundToInt(p->load())),{148,782,41,22},Justification::centred,1);
 }
 // Echo time always matches the DSP quantization. Sync OFF displays milliseconds.
 const bool synced=synth.state.getRawParameterValue("sync")->load(std::memory_order_relaxed)>.5f;
 const float time=juce::jlimit(0.f,1.f,synth.state.getRawParameterValue("time")->load(std::memory_order_relaxed));
 const String divisions[]={"1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4","1/2","1/1"};
 const int index=juce::jlimit(0,8,juce::roundToInt(time*8.f));
 const String label=synced?divisions[index]:String(juce::roundToInt(45.f+2650.f*time*time))+" ms";
 g.setColour(Colour::fromRGB(77,49,25));g.setFont(Font(FontOptions("Georgia",13.f,Font::plain)));
 g.drawFittedText(label,{552,713,56,20},Justification::centred,1);
 // Page-dependent engravings, not a generic rectangular overlay over the artwork.
 const String heading=tab==1?"T A P E   &   S P A C E":tab==3?"P O L L I N A T E":"M O T I O N";
 g.setColour(Colour::fromRGB(70,47,28));g.setFont(Font(FontOptions("Georgia",19.f,Font::plain)));
 g.drawFittedText(heading,{1280,741,259,26},Justification::centred,1);
 const String labels[3][3]={{"RATE","DEPTH","AMOUNT"},{"TONE","WIDTH","DAMP"},{"SPORE","ROOT","DRIFT"}};
 const int page=tab==1?1:tab==3?2:0;
 g.setFont(Font(FontOptions("Georgia",13.f,Font::plain)));
 for(int i=0;i<3;++i)g.drawFittedText(labels[page][i],{1272+i*85,861,72,20},Justification::centred,1);
 if(lastEditFrames>0){
  if(auto* p=synth.state.getParameter(lastEdited)){
   const String display=p->getName(80)+"  "+String(p->convertFrom0to1(p->getValue()),2);
   g.setColour(Colour::fromRGB(68,42,22));g.setFont(Font(FontOptions("Georgia",15.f,Font::italic)));
   g.drawFittedText(display,{642,174,317,24},Justification::centred,1);
  }
 }
}
void SteamSynthEditor::paint(Graphics&g){
 if(faceplate.isValid())g.drawImage(faceplate,getLocalBounds().toFloat(),RectanglePlacement::stretchToFit,false);
 else g.fillAll(Colour::fromRGB(232,199,156));
 Graphics::ScopedSaveState guard(g);
 g.addTransform(AffineTransform::scale(float(getWidth())/W,float(getHeight())/H));
 drawAnimatedHardware(g);
}
void SteamSynthEditor::choosePreset(){
 PopupMenu menu;for(int i=0;i<synth.getNumPrograms();++i)menu.addItem(i+1,synth.getProgramName(i),true,i==synth.getCurrentProgram());
 menu.addSeparator();menu.addItem(3001,"Load a garden...");menu.addItem(3002,"Save this garden...");
 SafePointer<SteamSynthEditor> weak(this);
 menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&programSelect),[weak](int choice){
  if(!weak)return;
  if(choice==3001){weak->showLoadDialog();return;}
  if(choice==3002){weak->showSaveDialog();return;}
  if(choice>0&&choice<=weak->synth.getNumPrograms()){weak->synth.setCurrentProgram(choice-1);weak->repaint();}
 });
}
void SteamSynthEditor::choosePolyphony(){
 PopupMenu menu;const auto* p=synth.state.getParameter("poly");
 const int current=p?juce::roundToInt(p->convertFrom0to1(p->getValue())):12;
 for(int i=1;i<=16;++i)menu.addItem(i,String(i)+" voices",true,i==current);
 SafePointer<SteamSynthEditor> weak(this);
 menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&polySelect),[weak](int choice){
  if(!weak||choice<1)return;
  if(auto* p=weak->synth.state.getParameter("poly")){
   p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(choice)));p->endChangeGesture();
  }
 });
}
void SteamSynthEditor::showSaveDialog(){
 auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("SteamSynth").getChildFile("Presets");dir.createDirectory();
 chooser=std::make_unique<FileChooser>("Save your garden",dir.getChildFile(synth.presetName+".stspreset"),"*.stspreset");
 SafePointer<SteamSynthEditor> weak(this);
 chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[weak](const FileChooser&fc){
  if(!weak)return;auto file=fc.getResult();if(file==File{})return;
  MemoryBlock data;weak->synth.presetName=file.getFileNameWithoutExtension();weak->synth.getStateInformation(data);
  file.replaceWithData(data.getData(),data.getSize());weak->repaint();
 });
}
void SteamSynthEditor::showLoadDialog(){
 auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("SteamSynth").getChildFile("Presets");dir.createDirectory();
 chooser=std::make_unique<FileChooser>("Load your garden",dir,"*.stspreset");
 SafePointer<SteamSynthEditor> weak(this);
 chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[weak](const FileChooser&fc){
  if(!weak)return;auto file=fc.getResult();if(!file.existsAsFile())return;
  MemoryBlock data;if(file.loadFileAsData(data))weak->synth.setStateInformation(data.getData(),int(data.getSize()));
  weak->repaint();
 });
}