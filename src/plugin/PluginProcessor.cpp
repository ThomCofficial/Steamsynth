#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace {
struct Patch {const char* name;float seed,grow,mutate,dirt,bloom,attack,release,time,feedback,mix;};
static constexpr Patch factory[] = {
 {"The Mangrove Choir",.42f,.78f,.42f,.31f,.75f,.54f,.81f,.44f,.54f,.42f},
 {"Ghost Orchid",.18f,.85f,.62f,.15f,.88f,.68f,.90f,.34f,.42f,.38f},
 {"Rust in the Rain",.81f,.49f,.79f,.86f,.61f,.12f,.60f,.63f,.62f,.50f},
 {"Glass Spores",.69f,.92f,.84f,.17f,.78f,.03f,.48f,.52f,.43f,.26f},
 {"The Listening Root",.12f,.96f,.64f,.24f,.84f,.89f,.92f,.30f,.60f,.56f},
 {"Cassette Fern",.38f,.42f,.46f,.91f,.28f,.09f,.41f,.46f,.51f,.28f},
 {"Alien Herbarium",.90f,.99f,.96f,.34f,.81f,.36f,.88f,.72f,.63f,.61f},
 {"Victorian Sleep",.21f,.69f,.16f,.37f,.90f,.82f,.93f,.38f,.47f,.34f},
 {"Tidal Phosphor",.60f,.82f,.61f,.40f,.75f,.31f,.74f,.69f,.55f,.61f},
 {"Spore Clockwork",.72f,.33f,.93f,.74f,.47f,.01f,.27f,.41f,.71f,.36f},
 {"Last Light on Tape",.27f,.62f,.32f,.88f,.81f,.62f,.88f,.58f,.66f,.62f},
 {"An Unfamiliar Garden",.56f,.94f,.90f,.29f,.96f,.79f,.96f,.82f,.74f,.62f}
};
}

juce::AudioProcessorValueTreeState::ParameterLayout SteamSynthProcessor::makeLayout(){
 using namespace juce;
 AudioProcessorValueTreeState::ParameterLayout p;
 const auto f=[&p](const char* id,const char* name,float defaultValue){p.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>{0.f,1.f,.0001f},defaultValue));};
 f("seed","Seed",.42f);f("grow","Grow",.78f);f("mutate","Mutate",.42f);f("dirt","Dirt",.31f);f("bloom","Bloom",.75f);
 f("attack","Attack",.54f);f("release","Release",.81f);
 f("time","Tape Time",.44f);f("feedback","Tape Feedback",.54f);f("echoMix","Tape Mix",.42f);
 f("tone","Tape Tone",.35f);f("spore","Spore Excitation",.38f);f("resonance","Root Resonance",.55f);
 f("drift","Cassette Drift",.22f);f("reverbDamp","Bloom Damping",.56f);f("width","Stereo Width",.70f);
 f("motionRate","Motion Rate",.30f);f("motionDepth","Motion Depth",.55f);f("motionAmount","Motion Amount",.50f);
 f("glide","Glide",0.f);f("master","Master Volume",.70f);
 p.add(std::make_unique<AudioParameterInt>(ParameterID{"poly",1},"Polyphony",1,16,12));
 p.add(std::make_unique<AudioParameterBool>(ParameterID{"sync",1},"Tape Tempo Sync",true));
 return p;
}
SteamSynthProcessor::SteamSynthProcessor():AudioProcessor(BusesProperties().withOutput("Stereo",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"SteamSynthParameters",makeLayout()){}
void SteamSynthProcessor::prepareToPlay(double sampleRate,int){instrument.prepare(sampleRate);sustainReleased.fill(false);sustainDown=false;sampleClock=0;}
void SteamSynthProcessor::releaseResources(){instrument.reset();}
bool SteamSynthProcessor::isBusesLayoutSupported(const BusesLayout &l)const{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() || l.getMainOutputChannelSet()==juce::AudioChannelSet::mono();}
steam::Macro SteamSynthProcessor::readMacros() const noexcept{
 steam::Macro m;
 const auto g=[this](const char* id){return state.getRawParameterValue(id)->load(std::memory_order_relaxed);};
 m.seed=g("seed");m.grow=g("grow");m.mutate=g("mutate");m.dirt=g("dirt");m.bloom=g("bloom");
 m.attack=g("attack");m.release=g("release");m.tapeTime=g("time");m.tapeFeedback=g("feedback");m.tapeMix=g("echoMix");
 m.tapeTone=g("tone");m.spore=g("spore");m.resonance=g("resonance");m.drift=g("drift");
 m.reverbDamp=g("reverbDamp");m.stereo=g("width");m.motionRate=g("motionRate");m.motionDepth=g("motionDepth");m.motionAmount=g("motionAmount");
 m.glide=g("glide");m.master=g("master");m.poly=int(g("poly"));m.sync=g("sync")>.5f;
 return m;
}
void SteamSynthProcessor::processMidi(const juce::MidiMessage &msg,const steam::Macro &m) noexcept{
 if(msg.isNoteOn()){
  instrument.noteOn(msg.getNoteNumber(),msg.getFloatVelocity());
  sustainReleased[unsigned(msg.getNoteNumber())]=false;
 }else if(msg.isNoteOff()){
  if(sustainDown)sustainReleased[unsigned(msg.getNoteNumber())]=true;
  else instrument.noteOff(msg.getNoteNumber());
 }else if(msg.isPitchWheel()){
  const float norm=(float(msg.getPitchWheelValue())-8192.f)/8192.f;
  instrument.pitchBend(norm*2.f);
 }else if(msg.isChannelPressure()){
  instrument.aftertouch(float(msg.getChannelPressureValue())/127.f);
 }else if(msg.isAftertouch()){
  instrument.aftertouch(float(msg.getAfterTouchValue())/127.f);
 }else if(msg.isController()){
  if(msg.getControllerNumber()==1){instrument.aftertouch(float(msg.getControllerValue())/127.f);}
  if(msg.getControllerNumber()==64){
   const bool newSustain=msg.getControllerValue()>=64;
   if(sustainDown&&!newSustain){for(int n=0;n<128;++n)if(sustainReleased[unsigned(n)])instrument.noteOff(n);sustainReleased.fill(false);}
   sustainDown=newSustain;
  }
  if(msg.getControllerNumber()==120||msg.getControllerNumber()==123){instrument.allOff();sustainReleased.fill(false);}
 }else if(msg.isAllNotesOff()||msg.isAllSoundOff()){instrument.allOff();sustainReleased.fill(false);}
 (void)m;
}
template<typename Sample>
void SteamSynthProcessor::processTyped(juce::AudioBuffer<Sample>&buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 const int count=buffer.getNumSamples();
 if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())if(auto bpm=pos->getBpm())tempo=std::clamp(*bpm,25.,300.);
 auto m=readMacros();instrument.setMacro(m);
 keyboard.processNextMidiBuffer(midi,0,count,true);
 buffer.clear();
 auto *l=buffer.getWritePointer(0),*r=buffer.getNumChannels()>1?buffer.getWritePointer(1):nullptr;
 int cursor=0;
 const auto render=[&](int end) {
  while(cursor<end){float left=0.f,right=0.f;instrument.process(left,right,tempo);l[cursor]=left;if(r)r[cursor]=right;++cursor;}
 };
 for(const auto metadata:midi){
  const int at=juce::jlimit(0,count,metadata.samplePosition);
  render(at);processMidi(metadata.getMessage(),m);
 }
 render(count);
 meterL.store(instrument.meterL(),std::memory_order_relaxed);
 meterR.store(instrument.meterR(),std::memory_order_relaxed);
 sampleClock.fetch_add(uint64_t(count),std::memory_order_relaxed);
}
void SteamSynthProcessor::processBlock(juce::AudioBuffer<float>&b,juce::MidiBuffer&m){processTyped(b,m);}
void SteamSynthProcessor::processBlock(juce::AudioBuffer<double>&b,juce::MidiBuffer&m){processTyped(b,m);}
juce::AudioProcessorEditor* SteamSynthProcessor::createEditor(){return new SteamSynthEditor(*this);}
void SteamSynthProcessor::setCurrentProgram(int index){
 if(index<0||index>=12)return;
 currentProgram=index;
 const auto &p=factory[index];
 const std::pair<const char*,float> updates[]={{"seed",p.seed},{"grow",p.grow},{"mutate",p.mutate},{"dirt",p.dirt},{"bloom",p.bloom},
 {"attack",p.attack},{"release",p.release},{"time",p.time},{"feedback",p.feedback},{"echoMix",p.mix}};
 for(auto [id,value]:updates)if(auto*param=state.getParameter(id))param->setValueNotifyingHost(param->convertTo0to1(value));
 presetName=p.name;
 canUndoMutation=false;
}
const juce::String SteamSynthProcessor::getProgramName(int index){return index>=0&&index<12?factory[index].name:juce::String{};}
void SteamSynthProcessor::changeProgramName(int,const juce::String&){}
void SteamSynthProcessor::pollinate(){
 undoState=state.copyState();canUndoMutation=true;
 // Bounded, musical mutations preserve the identity of the patch; no unpredictable full-range randomization.
 for(const char* name:{"seed","grow","mutate","spore","resonance","motionDepth","motionRate","drift","tapeTime","tapeMix","bloom"}){
  auto* p=state.getParameter(name);if(!p)continue;
  const float old=p->convertFrom0to1(p->getValue());
  const float variation=.08f+.18f*random.nextFloat();
  const float direction=random.nextBool()?1.f:-1.f;
  const float fresh=juce::jlimit(0.f,1.f,old+direction*variation);
  p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(fresh));p->endChangeGesture();
 }
 presetName="Pollinated Garden";
}
void SteamSynthProcessor::undoPollination(){
 if(canUndoMutation && undoState.isValid()){state.replaceState(undoState);canUndoMutation=false;}
}
void SteamSynthProcessor::getStateInformation(juce::MemoryBlock &dest){
 auto root=state.copyState();root.setProperty("presetName",presetName,nullptr);
 if(auto xml=root.createXml())copyXmlToBinary(*xml,dest);
}
void SteamSynthProcessor::setStateInformation(const void* data,int size){
 if(auto xml=getXmlFromBinary(data,size)){
  auto tree=juce::ValueTree::fromXml(*xml);
  if(tree.isValid()&&tree.hasType(state.state.getType())){
   presetName=tree.getProperty("presetName","Custom Garden").toString();
   state.replaceState(tree);
   canUndoMutation=false;
  }
 }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SteamSynthProcessor();}