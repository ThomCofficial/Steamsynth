#include "PluginEditor.h"
#include <cmath>
#include <numbers>

namespace {
using namespace juce;
const Colour ink=Colour::fromRGB(67,48,30),gold=Colour::fromRGB(151,105,46),paper=Colour::fromRGB(240,215,169),green=Colour::fromRGB(65,88,68),water=Colour::fromRGB(94,134,117);
void label(Graphics&g,const String&s,float x,float y,float w,float h,int size=18,bool bold=false){
 g.setColour(ink);g.setFont(Font(FontOptions("Georgia",float(size),bold?Font::bold:Font::plain)));
 g.drawText(s,Rectangle<float>(x,y,w,h),Justification::centred,false);
}
void stroke(Graphics&g,Path&p,Colour c,float thickness=1.f){g.setColour(c);g.strokePath(p,PathStrokeType(thickness,PathStrokeType::curved,PathStrokeType::rounded));}
void leaf(Graphics&g,float x,float y,float sz,float theta,Colour c){
 Graphics::ScopedSaveState state(g);g.addTransform(AffineTransform::rotation(theta,x,y));
 Path p;p.startNewSubPath(x,y);p.cubicTo(x-sz*.38f,y-sz*.35f,x-sz*.38f,y-sz*.83f,x,y-sz);
 p.cubicTo(x+sz*.38f,y-sz*.83f,x+sz*.38f,y-sz*.35f,x,y);p.closeSubPath();
 g.setColour(c.withAlpha(.74f));g.fillPath(p);stroke(g,p,ink.withAlpha(.44f),.8f);
 Path vein;vein.startNewSubPath(x,y);vein.lineTo(x,y-sz);stroke(g,vein,ink.withAlpha(.35f),.5f);
}
void bloom(Graphics&g,float x,float y,float radius){
 for(int i=0;i<6;++i){const float a=float(i)*2.f*float(std::numbers::pi)/6.f;
  leaf(g,x+std::sin(a)*radius*.15f,y-std::cos(a)*radius*.15f,radius,a,Colour::fromRGB(242,225,178));}
 g.setColour(gold);g.fillEllipse(x-4,y-4,8,8);
}
void twigs(Graphics&g,float x,float y,float size,bool right){
 Path branch;branch.startNewSubPath(x,y);branch.cubicTo(x+(right?-1:1)*size*.25f,y-size*.2f,x+(right?-1:1)*size*.4f,y-size*.68f,x+(right?-1:1)*size*.8f,y-size);
 stroke(g,branch,ink.withAlpha(.55f),1.5f);
 for(int j=0;j<7;++j){float t=float(j)/7.f;float sx=x+(right?-1:1)*size*(.16f+.54f*t),sy=y-size*(.12f+.82f*t);
  leaf(g,sx,sy,size*.19f,(right?-.6f:.6f)+float(j%3)*.55f,green);}
 bloom(g,x+(right?-1:1)*size*.55f,y-size*.78f,size*.18f);
}
void mangrove(Graphics&g,float x,float y,float scale){
 Path stem;stem.startNewSubPath(x,y);stem.cubicTo(x-8*scale,y-32*scale,x-4*scale,y-66*scale,x+5*scale,y-104*scale);
 stroke(g,stem,ink.withAlpha(.68f),3.f*scale);
 for(int i=-4;i<=4;++i){
  Path root;root.startNewSubPath(x+float(i)*6*scale,y-77*scale);
  root.cubicTo(x+float(i)*22*scale,y-55*scale,x+float(i)*44*scale,y-15*scale,x+float(i)*58*scale,y+13*scale);
  stroke(g,root,ink.withAlpha(.56f),std::max(.6f,2.f*scale));
 }
 for(int i=-3;i<=3;++i)leaf(g,x+float(i)*14*scale,y-95*scale,20*scale,float(i)*.28f,green.withAlpha(.42f));
}
}

void BotanicalLookAndFeel::drawRotarySlider(Graphics&g,int x,int y,int w,int h,float proportion,float start,float end,Slider&slider){
 const float cx=x+w*.5f,cy=y+h*.5f,r=std::min(w,h)*.39f;
 const float angle=start+proportion*(end-start);
 // Faceplate numerals and scale remain outside, separate from the knob cap.
 for(int i=0;i<13;++i){float a=start+(end-start)*float(i)/12.f;
  const float length=(i%3==0?9.f:5.f);const float x1=cx+std::sin(a)*(r+11),y1=cy-std::cos(a)*(r+11);
  const float x2=cx+std::sin(a)*(r+11+length),y2=cy-std::cos(a)*(r+11+length);
  g.setColour(ink.withAlpha(.75f));g.drawLine(x1,y1,x2,y2,i%3==0?1.6f:.8f);
 }
 Path arc;arc.addCentredArc(cx,cy,r+9,r+9,0.f,start,angle,true);
 g.setColour(Colours::darkorange.withAlpha(.32f));g.strokePath(arc,PathStrokeType(6.f));
 ColourGradient base(Colour::fromRGB(241,195,101),cx-r,cy-r,Colour::fromRGB(86,57,32),cx+r,cy+r,false);
 g.setColour(Colours::black.withAlpha(.3f));g.fillEllipse(cx-r-3,cy-r+4,(r+3)*2,(r+3)*2);
 g.setGradientFill(base);g.fillEllipse(cx-r,cy-r,r*2,r*2);
 g.setColour(ink);g.drawEllipse(cx-r,cy-r,r*2,r*2,3.f);
 g.setColour(Colour::fromRGB(241,199,124).withAlpha(.7f));g.drawEllipse(cx-r+5,cy-r+5,(r-5)*2,(r-5)*2,1.f);
 // Knob botanical medallion, dynamically rotated indicator.
 const auto name=slider.getName().toLowerCase();
 if(name=="seed"||name=="grow"||name=="bloom"){
  leaf(g,cx,cy+r*.32f,r*.70f,name=="seed"?0.f:name=="grow"?.4f:-.3f,ink.withAlpha(.6f));
 }else if(name=="dirt"){
  g.setColour(ink.withAlpha(.45f));g.drawEllipse(cx-r*.47f,cy-r*.16f,r*.42f,r*.42f,2.f);g.drawEllipse(cx+r*.05f,cy-r*.16f,r*.42f,r*.42f,2.f);
 }else if(name=="mutate"){
  for(int i=0;i<7;++i){float a=float(i)*2.f*float(std::numbers::pi)/7.f;
   g.setColour(ink.withAlpha(.65f));g.fillEllipse(cx+std::cos(a)*r*.4f-3.f,cy+std::sin(a)*r*.4f-3.f,6.f,6.f);}
 }
 const float sx=cx+std::sin(angle)*r*.75f,sy=cy-std::cos(angle)*r*.75f;
 g.setColour(Colour::fromRGB(41,27,18));g.drawLine(cx,cy,sx,sy,std::max(2.f,r*.055f));
 g.setColour(Colour::fromRGB(251,225,166));g.fillEllipse(cx-2,cy-2,4,4);
}

SteamSynthEditor::SteamSynthEditor(SteamSynthProcessor& p):AudioProcessorEditor(p),synth(p),piano(p.keyboard,MidiKeyboardComponent::horizontalKeyboard){
 setLookAndFeel(&botanical);initArt();
 for(const auto &x:std::initializer_list<std::tuple<const char*,float,float,float>>{
  {"seed",335,307,170},{"grow",605,307,170},{"mutate",865,307,170},{"dirt",1126,307,170},{"bloom",1390,307,170},
  {"attack",291,673,90},{"release",399,673,90},
  {"time",595,673,90},{"feedback",715,673,90},{"echoMix",833,673,90},
  {"glide",210,854,55},
  {"motionRate",1340,854,60},{"motionDepth",1440,854,60},{"motionAmount",1540,854,60},
  {"tone",1340,854,60},{"drift",1440,854,60},{"reverbDamp",1540,854,60},
  {"spore",1340,854,60},{"resonance",1440,854,60},{"width",1540,854,60},{"master",1540,764,50}}
 ) addKnob(std::get<0>(x),std::get<1>(x),std::get<2>(x),std::get<3>(x));
 const auto button=[this](TextButton&b){addAndMakeVisible(b);b.setColour(TextButton::buttonColourId,Colour::fromRGB(77,52,30));b.setColour(TextButton::textColourOffId,Colour::fromRGB(247,217,155));b.setColour(TextButton::buttonOnColourId,Colour::fromRGB(164,105,35));};
 for(auto*b:{&synthTab,&fxTab,&motionTab,&advTab,&pollinate,&undo,&save,&load})button(*b);
 synthTab.onClick=[this]{setTab(0);};fxTab.onClick=[this]{setTab(1);};motionTab.onClick=[this]{setTab(2);};advTab.onClick=[this]{setTab(3);};
 pollinate.onClick=[this]{synth.pollinate();repaint();};undo.onClick=[this]{synth.undoPollination();repaint();};
 save.onClick=[this]{showSaveDialog();};load.onClick=[this]{showLoadDialog();};
 addAndMakeVisible(sync);sync.setButtonText("SYNC");sync.setColour(ToggleButton::textColourId,ink);
 syncAttachment=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(synth.state,"sync",sync);
 addAndMakeVisible(programs);programs.setColour(ComboBox::backgroundColourId,Colour::fromRGB(55,46,36));programs.setColour(ComboBox::textColourId,Colour::fromRGB(236,209,167));
 for(int i=0;i<synth.getNumPrograms();++i)programs.addItem(synth.getProgramName(i),i+1);
 programs.setSelectedId(synth.getCurrentProgram()+1,dontSendNotification);
 programs.onChange=[this]{synth.setCurrentProgram(programs.getSelectedId()-1);};
 addAndMakeVisible(polyBox);for(int i=1;i<=16;++i)polyBox.addItem(juce::String(i),i);
 polyAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(synth.state,"poly",polyBox);
 polyBox.setColour(ComboBox::backgroundColourId,Colour::fromRGB(88,60,36));
 polyBox.setColour(ComboBox::textColourId,Colour::fromRGB(243,218,173));
 addAndMakeVisible(piano);piano.setAvailableRange(24,96);piano.setKeyWidth(17);
 piano.setColour(MidiKeyboardComponent::whiteNoteColourId,Colour::fromRGB(244,222,186));
 piano.setColour(MidiKeyboardComponent::blackNoteColourId,Colour::fromRGB(38,30,27));
 piano.setColour(MidiKeyboardComponent::keyDownOverlayColourId,Colour::fromRGB(181,105,37));
 setSize(1600,900);setResizable(true,true);setResizeLimits(960,540,1920,1080);
 setTab(0);startTimerHz(25);
}
SteamSynthEditor::~SteamSynthEditor(){stopTimer();setLookAndFeel(nullptr);}
void SteamSynthEditor::addKnob(const char*id,float,float,float){
 auto s=std::make_unique<Slider>();s->setName(id);
 s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);s->setTextBoxStyle(Slider::NoTextBox,false,0,0);
 s->setRotaryParameters(-2.5f,2.5f,true);s->setRange(0.,1.,.0001);s->setPopupDisplayEnabled(true,true,this);
 s->setScrollWheelEnabled(true);s->setDoubleClickReturnValue(true,synth.state.getParameter(id)->getDefaultValue());
 addAndMakeVisible(*s);attachments.emplace_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(synth.state,id,*s));
 knobs.emplace(id,std::move(s));
}
void SteamSynthEditor::setTab(int which){
 tab=which;
 for(const auto *id:{"motionRate","motionDepth","motionAmount","tone","drift","reverbDamp","spore","resonance","width"})knobs.at(id)->setVisible(false);
 const char* names[3][3]={{"motionRate","motionDepth","motionAmount"},{"tone","drift","reverbDamp"},{"spore","resonance","width"}};
 const int choice=which==1?1:which==3?2:0;
 knobs.at("master")->setVisible(which==3);
 for(auto n:names[choice])knobs.at(n)->setVisible(true);
 synthTab.setToggleState(which==0,dontSendNotification);fxTab.setToggleState(which==1,dontSendNotification);
 motionTab.setToggleState(which==2,dontSendNotification);advTab.setToggleState(which==3,dontSendNotification);
 repaint();
}
void SteamSynthEditor::timerCallback(){if(programs.getText()!=synth.presetName)programs.setText(synth.presetName,dontSendNotification);lastL=lastL*.82f+synth.meterL.load()*.18f;lastR=lastR*.82f+synth.meterR.load()*.18f;repaint();}
void SteamSynthEditor::resized(){
 const auto sx=float(getWidth())/1600.f,sy=float(getHeight())/900.f;
 const auto bounds=[&](Component&c,float x,float y,float w,float h){c.setBounds(int(x*sx),int(y*sy),int(w*sx),int(h*sy));};
 const float px[]={335,605,865,1126,1390};const char*keys[]={"seed","grow","mutate","dirt","bloom"};
 for(int i=0;i<5;++i)bounds(*knobs.at(keys[i]),px[i]-85,222,170,170);
 for(const auto &info:std::initializer_list<std::tuple<const char*,float,float,float>>{{"attack",291,673,88},{"release",399,673,88},{"time",595,673,88},{"feedback",715,673,88},{"echoMix",833,673,88}}){
  auto id=std::get<0>(info);float radius=std::get<3>(info);bounds(*knobs.at(id),std::get<1>(info)-radius*.5f,std::get<2>(info)-radius*.5f,radius,radius);
 }
 bounds(*knobs.at("glide"),181,823,58,58);
 bounds(*knobs.at("master"),1517,744,50,50);
 knobs.at("master")->setVisible(tab==3);
 for(int z=0;z<3;++z){const char*rows[3][3]={{"motionRate","motionDepth","motionAmount"},{"tone","drift","reverbDamp"},{"spore","resonance","width"}};
  for(int j=0;j<3;++j)bounds(*knobs.at(rows[z][j]),1310+100*j,817,64,64);
 }
 bounds(synthTab,35,255,122,45);bounds(fxTab,35,324,122,45);bounds(motionTab,35,393,122,45);bounds(advTab,35,462,122,45);
 bounds(pollinate,994,624,149,42);bounds(undo,1158,628,75,36);
 bounds(save,1050,126,73,36);bounds(load,1131,126,73,36);
 bounds(programs,520,126,515,40);bounds(polyBox,75,816,92,38);bounds(sync,920,685,90,35);
 bounds(piano,275,777,1015,112);
}
void SteamSynthEditor::initArt(){
 art=Image(Image::ARGB,1600,900,true);Graphics g(art);
 g.fillAll(paper);
 ColourGradient gradient(Colour::fromRGB(245,226,189),800,150,Colour::fromRGB(199,167,124),850,920,false);
 g.setGradientFill(gradient);g.fillAll();
 // Fine archival-paper texture, deterministic and cached, rather than redrawn every frame.
 uint32_t seed=0x811C9DC5u;
 for(int i=0;i<24000;++i){seed=seed*1664525u+1013904223u;const float x=float(seed%1600);seed=seed*1664525u+1013904223u;const float y=float(seed%900);
  g.setColour((i%3==0?ink:Colours::white).withAlpha(.024f));g.fillRect(x,y,float(i%3+1),.8f);}
 g.setColour(gold);g.drawRoundedRectangle(10.f,9.f,1580.f,882.f,14.f,2.f);g.drawRoundedRectangle(18.f,17.f,1564.f,866.f,9.f,1.2f);
 for(int x:{25,1575})for(int y:{24,876}){g.setColour(gold);g.fillEllipse(float(x-8),float(y-8),16,16);g.setColour(ink);g.drawEllipse(float(x-8),float(y-8),16,16,2);}
 // Glass tube + copper plumbing, at the left of the field plate.
 g.setColour(gold.darker(.15f));g.fillRoundedRectangle(52,35,97,190,34);
 g.setColour(Colour::fromRGB(246,210,150).withAlpha(.55f));g.fillRoundedRectangle(64,44,73,146,32);
 g.setColour(Colour::fromRGB(220,109,30).withAlpha(.42f));g.fillEllipse(81,90,38,88);
 g.setColour(Colour::fromRGB(255,206,84));g.fillEllipse(93,129,16,23);
 g.setColour(ink);g.drawRoundedRectangle(52,35,97,190,34,3);
 Path pipe;pipe.startNewSubPath(99,224);pipe.lineTo(99,584);pipe.cubicTo(108,641,129,687,196,735);
 stroke(g,pipe,gold,13.f);stroke(g,pipe,Colour::fromRGB(223,159,79),5.f);
 // Antique typographic header / science illustration numbering.
 g.setColour(ink);g.setFont(Font(FontOptions("Georgia",67.f,Font::plain)));g.drawText("STEAMSYNTH",Rectangle<float>(460,9,700,81),Justification::centred);
 label(g,"B O T A N I C A L    S Y N T H E S I Z E R",550,85,500,28,17);
 label(g,"Pl. 34",382,25,100,32,19);label(g,"Rhizophora mangle",1255,19,265,55,20);
 twigs(g,492,183,165,false);twigs(g,1432,192,161,true);bloom(g,333,66,68);
 // Five unboxed macro instruments. Legends and values are engraved on the faceplate.
 const char*name[]={"SEED","GROW","MUTATE","DIRT","BLOOM"};
 const char*detail[]={"SOURCE  ·  TIMBRE  ·  DNA","EVOLUTION  ·  MOVEMENT  ·  LIFE","HARMONICS  ·  RESONANCE  ·  TEXTURE","TAPE  ·  SATURATION  ·  DECAY","SPACE  ·  DIFFUSION  ·  ATMOSPHERE"};
 const float centers[]={335,605,865,1126,1390};
 for(int i=0;i<5;++i){label(g,name[i],centers[i]-145,400,290,42,34);label(g,detail[i],centers[i]-157,450,314,30,15);}
 // Mangrove plate, fine root linework and translucent washes of cool sea-green ink.
 ColourGradient sea(water.withAlpha(.22f),200,491,water.withAlpha(.37f),1100,624,false);
 g.setGradientFill(sea);g.fillRect(200,505,1390,113);
 for(int y=531;y<620;y+=9){g.setColour(water.withAlpha(.15f));g.drawHorizontalLine(y,240.f,1560.f);}
 for(int x:{350,630,1050,1450})mangrove(g,float(x),576,x==630?1.3f:.85f);
 for(int i=0;i<70;++i){float x=185+float((i*117)%1390);float y=575+float((i*41)%51);
  g.setColour(green.withAlpha(.12f));g.drawLine(x,y,x+23,y,1.f);}
 // Tape deck and botanical details sit on parchment, not boxed parameter modules.
 label(g,"A T T A C K",230,600,125,30,16);label(g,"R E L E A S E",340,600,125,30,16);
 label(g,"T A P E   E C H O",565,604,350,33,22);
 g.setColour(gold.withAlpha(.75f));g.drawRoundedRectangle(465,637,125,77,7.f,1.5f);
 for(int x:{502,547}){g.setColour(ink);g.fillEllipse(float(x-19),654,38,38);g.setColour(gold);g.drawEllipse(float(x-16),657,32,32,2);}
 label(g,"TIME",552,725,90,27,16);label(g,"FEEDBACK",653,725,127,27,16);label(g,"MIX",791,725,93,27,16);
 label(g,"POLLINATE",1004,586,205,35,21);
 bloom(g,1100,575,32);twigs(g,1233,732,77,true);
 // Vintage VU meter faceplate, with animated needles in paintMovingParts.
 g.setColour(Colour::fromRGB(247,225,186));g.fillRoundedRectangle(1270,620,284,110,20.f);
 g.setColour(gold);g.drawRoundedRectangle(1270,620,284,110,20.f,4.f);
 for(int i=0;i<7;++i){float x=1305+i*34.f;g.setColour(ink.withAlpha(.72f));g.drawLine(x,655,x+4,659,1.5f);}
 label(g,"L                                      R",1291,688,239,30,14);
 // Grounding keyboard area and performance labels.
 label(g,"POLY",53,784,110,24,17);label(g,"GLIDE",140,843,90,20,16);
 label(g,"MOTION",1338,767,202,28,20);
 label(g,"RATE",1306,882,73,15,13);label(g,"DEPTH",1402,882,74,15,13);label(g,"AMOUNT",1500,882,81,15,13);
 g.setColour(ink.withAlpha(.7f));g.drawLine(207,765,1304,765,2.f);
}
void SteamSynthEditor::paintMovingParts(Graphics&g){
 const float clock=float(synth.sampleClock.load(std::memory_order_relaxed)%2000000)/48000.f;
 // Spin tape reels at a grounded transport-like speed while the audio engine advances.
 for(int c:{504,549}){
  const float angle=clock*1.65f*(c==504?1.f:-1.f);
  for(int i=0;i<3;++i){const float a=angle+float(i)*2.f*float(std::numbers::pi)/3.f;
   const float dx=std::cos(a)*13.f,dy=std::sin(a)*13.f;
   g.setColour(gold.brighter(.5f));g.drawLine(float(c),674.f,float(c)+dx,674.f+dy,2.2f);}
 }
 // True stereo peak needles; the meters react to sound rendered in processBlock.
 for(int side=0;side<2;++side){
  const float x=side?1475.f:1350.f,level=juce::jlimit(0.f,1.f,side?lastR:lastL);
  const float theta=-.80f+level*1.60f;
  g.setColour(Colour::fromRGB(109,53,26));g.drawLine(x,716.f,x+std::sin(theta)*65.f,716.f-std::cos(theta)*65.f,2.4f);
  g.setColour(ink);g.fillEllipse(x-4,712,8,8);
 }
}
void SteamSynthEditor::paint(Graphics&g){
 g.drawImage(art,getLocalBounds().toFloat());
 Graphics::ScopedSaveState save(g);g.addTransform(AffineTransform::scale(float(getWidth())/1600.f,float(getHeight())/900.f));
 paintMovingParts(g);
 const String mode=(tab==1?"TONE      DRIFT      DAMP":tab==3?"SPORE      RESONANCE      WIDTH":"RATE      DEPTH      AMOUNT");
 label(g,mode,1300,746,281,18,13);
}
void SteamSynthEditor::showSaveDialog(){
 auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("SteamSynth").getChildFile("Presets");dir.createDirectory();
 chooser=std::make_unique<FileChooser>("Save your garden",dir.getChildFile(synth.presetName+".stspreset"),"*.stspreset");
 SafePointer<SteamSynthEditor> safe(this);
 chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[safe](const FileChooser& fc){
  if(!safe)return;auto file=fc.getResult();if(file==File{})return;
  MemoryBlock block;safe->synth.presetName=file.getFileNameWithoutExtension();safe->synth.getStateInformation(block);
  file.replaceWithData(block.getData(),block.getSize());
 });
}
void SteamSynthEditor::showLoadDialog(){
 auto dir=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("SteamSynth").getChildFile("Presets");dir.createDirectory();
 chooser=std::make_unique<FileChooser>("Load your garden",dir,"*.stspreset");
 SafePointer<SteamSynthEditor> safe(this);
 chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe](const FileChooser& fc){
  if(!safe)return;auto file=fc.getResult();if(!file.existsAsFile())return;
  MemoryBlock block;if(file.loadFileAsData(block))safe->synth.setStateInformation(block.getData(),int(block.getSize()));
 });
}