#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <vector>

namespace steam {
constexpr float pi = 3.14159265358979323846f;
inline float clip01(float f) noexcept { return std::clamp(std::isfinite(f)?f:0.f,0.f,1.f); }
inline float finite(float f) noexcept { return std::isfinite(f)?f:0.f; }
inline float safe(float f) noexcept { return std::clamp(finite(f), -32.f, 32.f); }
inline float fastSat(float x) noexcept { x=safe(x); return x/(1.f+std::abs(x)); }
inline float lerp(float a,float b,float x) noexcept {return a+(b-a)*x;}
inline float hzToCoeff(float hz,double sr) noexcept {return 1.f-std::exp(-2.f*pi*std::max(hz,0.001f)/float(sr));}
struct RNG { uint32_t s=0x9e3779b9u; float bipolar() noexcept { s^=s<<13; s^=s>>17; s^=s<<5; return float(s&0xffffu)/32767.5f-1.f; } };
struct Macro {
 float seed=.42f,grow=.64f,mutate=.38f,dirt=.32f,bloom=.55f;
 float attack=.26f,release=.62f,spore=.38f,resonance=.55f,drift=.22f;
 float motionRate=.3f,motionDepth=.55f,motionAmount=.5f,glide=0.f;
 float tapeTime=.48f,tapeFeedback=.48f,tapeMix=.35f,tapeTone=.35f;
 float reverbDamp=.56f,stereo=.7f,master=.7f;
 bool sync=true; int poly=12;
};
inline float polyBlep(float phase,float step) noexcept {
 if(phase<step){float t=phase/step;return t+t-t*t-1.f;}
 if(phase>1.f-step){float t=(phase-1.f)/step;return t*t+t+t+1.f;}
 return 0.f;
}
struct Pole {
 float re=0.f, im=0.f, cr=0.f, ci=0.f, drive=0.f;
 void clear() noexcept {re=im=0.f;}
 float tick(float excitation) noexcept {
  const float nr=cr*re-ci*im+drive*excitation;
  const float ni=ci*re+cr*im;
  re=safe(nr);im=safe(ni);
  return re;
 }
};
class Voice {
public:
 void prepare(double sampleRate) noexcept {sr=std::clamp(sampleRate,8000.,384000.);reset();}
 void reset() noexcept {gate=false;rel=false;phase=0;phase2=0;env=age=0;smoothedSeed=.5f;smoothedMutation=.5f;tickCounter=0;lastOut=0;stealTail=0;for(auto &p:poles)p.clear();rng.s=0x9e3779b9u;}
 void start(int n,float v,uint64_t when) noexcept {const float previous=gate?lastOut:0.f;reset();stealTail=previous;note=n;velocity=clip01(v);startOrder=when;gate=true;freq=440.f*std::exp2((float(n)-69.f)/12.f);currentFreq=freq;rng.s=0x1234567u+uint32_t(n)*127u+uint32_t(when)*17u;}
 void off() noexcept {rel=true;}
 bool active() const noexcept {return gate;}
 bool isReleasing() const noexcept {return rel;}
 int midiNote() const noexcept {return note;}
 uint64_t order() const noexcept {return startOrder;}
 void bend(float semitones) noexcept { bendSemitones=std::clamp(finite(semitones),-24.f,24.f); }
 void expression(float e) noexcept {expr=clip01(e);}
 float render(const Macro &m) noexcept {
  if(!gate)return 0.f;
  const float a=.002f+5.f*m.attack*m.attack, r=.01f+11.f*m.release*m.release;
  const float target=rel?0.f:1.f;
  const float coefficient=std::exp(-1.f/(float(sr)*(rel?r:a)));
  env=target+(env-target)*coefficient;
  if(rel && env<.00003f){gate=false;return 0.f;}
  age+=1.f/float(sr);
  const float motion=std::sin(2.f*pi*age*(.035f+2.8f*m.motionRate*m.motionRate));
  const float growth=clip01(age/(.18f+10.f*(1.f-m.grow)));
  smoothedSeed+=.002f*(clip01(m.seed)-smoothedSeed);
  smoothedMutation+=.001f*(clip01(m.mutate)-smoothedMutation);
  const float evolving=clip01(smoothedSeed + m.grow*(growth-.5f)*.58f +motion*m.motionDepth*m.motionAmount*.22f);
  float wantedFreq=freq*std::exp2(bendSemitones/12.f);
  float slew= m.glide<.005f ? 1.f : hzToCoeff(1.f/(.005f+m.glide*m.glide*1.8f),sr);
  currentFreq+=slew*(wantedFreq-currentFreq);
  const float step=std::clamp(currentFreq/float(sr),.000005f,.45f);
  phase+=step; if(phase>=1.f)phase-=1.f;
  phase2+=std::min(.45f,step*1.0043f);if(phase2>=1.f)phase2-=1.f;
  const float sine=std::sin(2.f*pi*phase);
  const float saw=2.f*phase-1.f-polyBlep(phase,step);
  const float pulse=(phase<.44f?1.f:-1.f)+polyBlep(phase,step)-polyBlep(std::fmod(phase+.56f,1.f),step);
  const float alt=std::sin(2.f*pi*phase2);
  // smooth evolution of nontrivial band-limited harmonic shapes
  const float timbre=evolving<.5f?lerp(sine,saw,evolving*2.f):lerp(saw,pulse,(evolving-.5f)*2.f);
  const float seed= .67f*timbre+.20f*alt+.13f*sine;
  const float flutter=rng.bipolar()*(.001f+.09f*m.spore*smoothedMutation);
  float modal=0;
  ++tickCounter;
  for(int i=0;i<4;++i){
   auto &p=poles[unsigned(i)];
   if(tickCounter%32==0 || p.cr==0.f){
    const float harmonic=(i==0?1.23f:i==1?2.11f:i==2?3.72f:5.61f);
    const float detune=1.f+smoothedMutation*(.12f+.055f*float(i))*std::sin(.37f*age + i*1.7f +motion*.8f);
    const float f=std::min(float(sr)*.42f,currentFreq*harmonic*detune);
    const float decay=.025f+ (.12f+1.5f*m.resonance)/(1.f+float(i));
    const float radius=std::exp(-1.f/(float(sr)*decay));
    p.cr=radius*std::cos(2.f*pi*f/float(sr));p.ci=radius*std::sin(2.f*pi*f/float(sr));
    p.drive=(.012f+.036f*m.resonance)/(1.f+float(i)*.24f);
   }
   modal+=p.tick(seed*.12f+flutter) * (.3f/(1.f+float(i)*.26f));
  }
  const float x=(seed*.7f+ modal*(.5f+smoothedMutation*.7f)+ flutter*.2f)*env*velocity*(.8f+.2f*expr);
  float out=safe(fastSat(x*1.38f)*1.4f);
  if(std::abs(stealTail)>.00001f){out+=stealTail;stealTail*=std::exp(-1.f/(float(sr)*.002f));}
  lastOut=out;return safe(out);
 }
private:
 double sr=48000;float freq=440,currentFreq=440,phase=0,phase2=0,env=0,age=0,velocity=0,expr=0,bendSemitones=0,smoothedSeed=.5f,smoothedMutation=.5f,lastOut=0,stealTail=0;
 int note=-1,tickCounter=0;uint64_t startOrder=0;bool gate=false,rel=false;RNG rng;std::array<Pole,4> poles{};
};
class Voices {
public:
 void prepare(double s) noexcept {for(auto &v:voices)v.prepare(s);last=0;}
 void allOff() noexcept {for(auto &v:voices)v.reset();}
 void capVoices(int n) noexcept {n=std::clamp(n,1,16);for(int i=n;i<16;++i)voices[unsigned(i)].off();}
 void noteOn(int n,float vel,int maxVoices) noexcept {
  maxVoices=std::clamp(maxVoices,1,16);
  Voice* chosen=nullptr;
  for(int i=0;i<maxVoices;++i)if(!voices[unsigned(i)].active()){chosen=&voices[unsigned(i)];break;}
  if(!chosen)for(int i=0;i<maxVoices;++i)if(voices[unsigned(i)].isReleasing()&&(!chosen||voices[unsigned(i)].order()<chosen->order()))chosen=&voices[unsigned(i)];
  if(!chosen)for(int i=0;i<maxVoices;++i)if(!chosen||voices[unsigned(i)].order()<chosen->order())chosen=&voices[unsigned(i)];
  chosen->start(n,vel,++last);chosen->bend(bendValue);chosen->expression(expr);
 }
 void noteOff(int n) noexcept {for(auto &v:voices)if(v.active()&&v.midiNote()==n)v.off();}
 void pitchBend(float semi) noexcept {bendValue=semi;for(auto &v:voices)v.bend(semi);}
 void aftertouch(float pressure) noexcept {expr=clip01(pressure);for(auto &v:voices)v.expression(expr);}
 float process(const Macro&m) noexcept {float s=0;for(auto &voice:voices)s+=voice.render(m);return safe(s*.19f);}
 int activeCount() const noexcept {int n=0;for(auto &v:voices)if(v.active())++n;return n;}
private:
 std::array<Voice,16> voices{};uint64_t last=0;float bendValue=0,expr=1;
};
class Cassette {
public:
 void prepare(double s){sr=std::clamp(s,8000.,384000.);size=size_t(sr*.15)+8;bufL.assign(size,0);bufR.assign(size,0);write=0;lpL=lpR=dcxL=dcxR=dcyL=dcyR=0;phase=0;}
 void reset() noexcept {std::fill(bufL.begin(),bufL.end(),0.0f);std::fill(bufR.begin(),bufR.end(),0.0f);write=0;lpL=lpR=dcxL=dcxR=dcyL=dcyR=0;}
 void process(float &l,float &r,float amount,float drift) noexcept {
  amount=clip01(amount);drift=clip01(drift);
  if(amount<.0001f)return;
  l=processChannel(l,lpL,dcxL,dcyL);r=processChannel(r,lpR,dcxR,dcyR);
  bufL[write]=l;bufR[write]=r;
  phase+=1.f/float(sr);if(phase>10000.f)phase=0;
  const float wow=std::sin(2*pi*.52f*phase)*(.5f+2.f*drift)*amount;
  const float flutter=std::sin(2*pi*7.1f*phase)*(.14f+.55f*drift)*amount;
  const float delay=float(sr*.015)+(wow+flutter);
  float a=read(bufL,delay),b=read(bufR,delay);
  ++write;if(write>=size)write=0;
  l=lerp(l,a,amount*.45f);r=lerp(r,b,amount*.45f);
 }
private:
 float processChannel(float x,float &lp,float &dcx,float &dcy) noexcept {
  const float d=clip01(dirt);
  // dirt is set on calling side (for channel consistency)
  const float coefficient=hzToCoeff(650.f+14500.f*(1.f-d)*(1.f-d),sr);
  lp+=coefficient*(safe(x)-lp);
  const float drive=1.f+6.f*d*d;
  const float sh=fastSat(lp*drive)*(1.f+drive)/drive;
  const float dc=sh-dcx+.995f*dcy;dcx=sh;dcy=dc;
  return safe(lerp(x,dc,d));
 }
 float read(const std::vector<float>&buf,float delay) const noexcept {
  const float idx=float(write)+float(size)-std::clamp(delay,1.f,float(size-3));
  float wrapped=idx;if(wrapped>=float(size))wrapped-=float(size);if(wrapped<0)wrapped+=float(size);
  const size_t i=size_t(wrapped);size_t j=i+1;if(j>=size)j=0;
  return lerp(buf[i],buf[j],wrapped-float(i));
 }
public:
 void setDirt(float x) noexcept {dirt=clip01(x);}
private:
 double sr=48000;size_t size=0,write=0;std::vector<float>bufL,bufR;
 float dirt=0,lpL=0,lpR=0,dcxL=0,dcxR=0,dcyL=0,dcyR=0,phase=0;
};
class Echo {
public:
 void prepare(double s){sr=std::clamp(s,8000.,384000.);len=size_t(sr*4.)+4;l.assign(len,0);r.assign(len,0);pos=0;filteredL=filteredR=0;phase=0;delayS=sr*.35;}
 void reset() noexcept {std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);pos=0;filteredL=filteredR=0;}
 void process(float &left,float &right,float time,float feedback,float wet,float tone,bool sync,double bpm) noexcept {
  wet=clip01(wet);feedback=clip01(feedback);tone=clip01(tone);
  if(wet<.0001f&&feedback<.0001f)return;
  const double beats=std::clamp(std::isfinite(bpm)?bpm:120.,25.,300.);
  const double target=(sync?(60.0/beats)*(0.125*std::exp2(clip01(time)*4.0)):(.045+2.65*clip01(time)*clip01(time)))*sr;
  const double wanted=std::clamp(target,4.,double(len)-5.);
  delayS+=(wanted-delayS)*.00006;
  phase+=1.f/float(sr);if(phase>10000.f)phase=0;
  const float warble=std::sin(2*pi*.6f*phase)*1.8f+std::sin(2*pi*5.8f*phase)*.34f;
  const float d=float(delayS)+warble;
  const float dl=read(l,d),dr=read(r,d);
  const float coeff=hzToCoeff(900.f+12500.f*(1.f-tone)*(1.f-tone),sr);
  filteredL+=coeff*(dl-filteredL);filteredR+=coeff*(dr-filteredR);
  const float fb=std::min(.88f,feedback*.88f);
  l[pos]=fastSat(safe(left)*.8f+filteredR*fb)*1.35f;
  r[pos]=fastSat(safe(right)*.8f+filteredL*fb)*1.35f;
  pos=(pos+1)%len;
  left=safe(left*(1.f-wet)+dl*wet*.85f);right=safe(right*(1.f-wet)+dr*wet*.85f);
 }
private:
 float read(const std::vector<float>&buf,float delay) const noexcept {
  const float idx=float(pos)+float(len)-std::clamp(delay,1.f,float(len-3));
  float wrapped=idx;if(wrapped>=float(len))wrapped-=float(len);if(wrapped<0)wrapped+=float(len);
  size_t i=size_t(wrapped),j=(i+1)%len;return lerp(buf[i],buf[j],wrapped-float(i));
 }
 double sr=48000,delayS=0;size_t len=0,pos=0;std::vector<float> l,r;
 float filteredL=0,filteredR=0,phase=0;
};
class Bloom {
public:
 void prepare(double s){sr=std::clamp(s,8000.,384000.);int lengths[8]={1493,1607,1867,1999,2251,2399,2671,2797};
  for(int i=0;i<8;++i){auto &line=lines[unsigned(i)];line.buffer.assign(size_t(double(lengths[i])*sr/48000.)+1,0);line.index=0;line.lp=0;}}
 void reset() noexcept {for(auto &line:lines){std::fill(line.buffer.begin(),line.buffer.end(),0);line.index=0;line.lp=0;}}
 void process(float &l,float &r,float wet,float damping,float width) noexcept {
  wet=clip01(wet);if(wet<.00001f)return;damping=clip01(damping);width=clip01(width);
  std::array<float,8> values{};float sum=0;
  for(int i=0;i<8;++i){auto &x=lines[unsigned(i)];values[unsigned(i)]=x.buffer[x.index];sum+=values[unsigned(i)];}
  const float coeff=hzToCoeff(1200.f+10000.f*(1.f-damping)*(1.f-damping),sr);
  const float gain=std::clamp(.54f+wet*.38f,0.f,.94f);
  const float input=safe((l+r)*.5f)*.075f;
  for(int i=0;i<8;++i){auto &x=lines[unsigned(i)];const float householder=sum*.25f-values[unsigned(i)];
   x.lp+=coeff*(householder-x.lp);x.buffer[x.index]=safe(input*(i%2==0?1.f:-1.f)+gain*x.lp);
   if(++x.index==x.buffer.size())x.index=0;
  }
  float outL=0,outR=0;
  for(int i=0;i<8;++i){outL+=values[unsigned(i)]*(i%2==0?1.f:-1.f);outR+=values[unsigned(i)]*((i/2)%2==0?1.f:-1.f);}
  outL*=.25f;outR*=.25f;
  const float mono=(outL+outR)*.5f;
  l=safe(l*(1.f-wet*.46f)+lerp(mono,outL,width)*wet*.7f);
  r=safe(r*(1.f-wet*.46f)+lerp(mono,outR,width)*wet*.7f);
 }
private:
 struct Line {std::vector<float>buffer;size_t index=0;float lp=0;};
 double sr=48000;std::array<Line,8>lines{};
};
class Instrument {
public:
 void prepare(double rate){sr=std::clamp(rate,8000.,384000.);voices.prepare(sr);cassette.prepare(sr);echo.prepare(sr);bloom.prepare(sr);reset();}
 void reset() noexcept {voices.allOff();cassette.reset();echo.reset();bloom.reset();peakL=peakR=0;}
 void noteOn(int n,float vel) noexcept {voices.noteOn(std::clamp(n,0,127),vel,macro.poly);}
 void noteOff(int n) noexcept {voices.noteOff(std::clamp(n,0,127));}
 void allOff() noexcept {voices.allOff();}
 void pitchBend(float semi) noexcept {voices.pitchBend(semi);}
 void aftertouch(float e) noexcept {voices.aftertouch(e);}
 void setMacro(const Macro&m) noexcept {if(m.poly<target.poly)voices.capVoices(m.poly);target=m;target.poly=std::clamp(m.poly,1,16);macro.poly=target.poly;macro.sync=target.sync;}
 const Macro& getMacro() const noexcept {return macro;}
 void process(float &l,float&r,double bpm) noexcept {
  const float k=.0025f;
  #define SMOOTH(n) macro.n+=k*(clip01(target.n)-macro.n)
  SMOOTH(seed);SMOOTH(grow);SMOOTH(mutate);SMOOTH(dirt);SMOOTH(bloom);
  SMOOTH(attack);SMOOTH(release);SMOOTH(spore);SMOOTH(resonance);SMOOTH(drift);
  SMOOTH(motionRate);SMOOTH(motionDepth);SMOOTH(motionAmount);SMOOTH(glide);
  SMOOTH(tapeTime);SMOOTH(tapeFeedback);SMOOTH(tapeMix);SMOOTH(tapeTone);
  SMOOTH(reverbDamp);SMOOTH(stereo);SMOOTH(master);
  #undef SMOOTH
  cassette.setDirt(macro.dirt);
  float x=voices.process(macro);
  l=safe(l+x);r=safe(r+x);
  cassette.process(l,r,macro.dirt,macro.drift);
  echo.process(l,r,macro.tapeTime,macro.tapeFeedback,macro.tapeMix,macro.tapeTone,macro.sync,bpm);
  bloom.process(l,r,macro.bloom,macro.reverbDamp,macro.stereo);
  float gain=.25f+macro.master*1.2f;
  l=std::clamp(safe(l*gain),-1.f,1.f);r=std::clamp(safe(r*gain),-1.f,1.f);
  peakL=std::max(std::abs(l),peakL*.9994f);peakR=std::max(std::abs(r),peakR*.9994f);
 }
 float meterL() const noexcept{return peakL;}float meterR() const noexcept{return peakR;}
 int voicesActive() const noexcept {return voices.activeCount();}
private:
 double sr=48000;Voices voices;Cassette cassette;Echo echo;Bloom bloom;Macro macro,target;float peakL=0,peakR=0;
};
}