#include "dsp/DSP.h"
#include <cassert>
#include <cmath>
#include <chrono>
#include <iostream>
#include <limits>
#include <vector>
using namespace steam;
static int tests=0;
void require(bool v, const char*reason){if(!v){std::cerr<<"FAIL: "<<reason<<"\n";std::exit(1);}}
void testVoice(){
 for(double rate:{44100.,48000.,96000.,192000.}) {
  Voice v;v.prepare(rate);Macro m;
  for(int note:{21,42,69,96,120}){
   v.start(note,1.f,uint64_t(note));
   for(int n=0;n<5000;++n){m.seed=float(n%11)/10.f;m.grow=float(n%19)/18.f;m.mutate=float(n%23)/22.f;
    float s=v.render(m);require(std::isfinite(s)&&std::abs(s)<=32.f,"voice numerical stability");}
   v.off();for(int n=0;n<int(rate*1.5);++n){float s=v.render(m);require(std::isfinite(s),"release finite");}
  }
 }
 ++tests;
}
void testPoly(){Voices v;v.prepare(48000);Macro m;m.poly=8;for(int n=40;n<100;++n)v.noteOn(n,1.f,8);
 require(v.activeCount()==8,"voice stealing respects polyphony");
 for(int i=0;i<12000;++i)require(std::isfinite(v.process(m)),"polyphonic bounded");
 v.allOff();require(v.activeCount()==0,"all notes off");++tests;}
void testCassette(){Cassette x;x.prepare(48000);for(int n=0;n<180000;++n){x.setDirt(float(n%2000)/1999.f);float l=n%13? .8f:20.f,r=n%23?-.8f:-20.f;
 x.process(l,r,1.f,float(n%100)/100.f);require(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<=32.f,"cassette bounded");}
 x.reset();++tests;}
void testEcho(){Echo x;x.prepare(44100);float maxAmp=0;for(int n=0;n<300000;++n){float l=n==0?1.f:0.f,r=n==0?1.f:0.f;
 x.process(l,r,float(n%1000)/1000.f,1.f,1.f,float(n%50)/50.f,n%2==0,120.);
 require(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<=32.f,"echo feedback bounded");maxAmp=std::max(maxAmp,std::abs(l));}
 require(maxAmp<=32.f,"echo amplitude");x.reset();++tests;}
void testBloom(){Bloom b;b.prepare(96000);for(int n=0;n<220000;++n){float l=n==0?1.f:0.f,r=l;
 b.process(l,r,1.f,1.f,1.f);require(std::isfinite(l)&&std::abs(l)<32.f&&std::isfinite(r),"reverb bounded");}
 b.reset();++tests;}
void testIntegration(){Instrument x;x.prepare(44100);Macro m;m.poly=16;m.tapeMix=1;m.tapeFeedback=1;m.bloom=1;m.dirt=1;
 for(int n=0;n<300000;++n){if(n%143==0)x.noteOn(21+(n/143)%85,.8f);if(n%143==90)x.noteOff(21+(n/143)%85);
 m.seed=float(n%283)/282.f;m.mutate=float(n%473)/472.f;m.grow=float(n%541)/540.f;
 x.setMacro(m);float l=0,r=0;x.process(l,r,120);
 require(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<=1.f&&std::abs(r)<=1.f,"complete instrument bounded");}
 ++tests;}
void testStereoAndPitch(){
 StereoSpread spread;spread.prepare(48000);
 double sideEnergy=0.,monoError=0.;
 for(int n=0;n<9000;++n){
  const float input=std::sin(2.f*pi*220.f*float(n)/48000.f);
  float l=input,r=input;
  spread.process(l,r,.95f);
  sideEnergy+=double(l-r)*double(l-r);
  monoError=std::max(monoError,std::abs((double(l)+double(r))*.5-double(input)));
  require(std::isfinite(l)&&std::isfinite(r),"stereo decorator finite");
 }
 require(sideEnergy>5.0,"stereo sound must be observably wider than mono");
 require(monoError<.00001,"stereo decorrelation must sum back to mono");
 Voice v;v.prepare(48000);
 Macro m;m.seed=.18f;m.grow=.82f;m.mutate=.88f;m.attack=0.f;
 v.start(69,1.f,1);
 double tunedSin=0.,tunedCos=0.,offSin=0.,offCos=0.;
 for(int n=0;n<18000;++n){
  const double sample=v.render(m);
  if(n<5000)continue;
  const double a=double(n)*2.0*double(pi)/48000.;
  tunedSin+=sample*std::sin(440.*a);tunedCos+=sample*std::cos(440.*a);
  offSin+=sample*std::sin(466.16*a);offCos+=sample*std::cos(466.16*a);
 }
 const double tuned=std::hypot(tunedSin,tunedCos),detuned=std::hypot(offSin,offCos);
 require(tuned>detuned*1.7,"organic note must have a stable MIDI-tuned fundamental");
 ++tests;
}
void testSanitize(){require(clip01(std::numeric_limits<float>::quiet_NaN())==0,"nonfinite parameter safe");
 require(std::isfinite(fastSat(std::numeric_limits<float>::infinity())),"nonfinite saturation safe");++tests;}
int main(){auto begin=std::chrono::steady_clock::now();testSanitize();testVoice();testPoly();testCassette();testEcho();testBloom();testStereoAndPitch();testIntegration();
 auto d=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
 std::cout<<"PASS: "<<tests<<" suites, voice/cassette/echo/reverb/automation and 44.1-192 kHz; "<<d<<" seconds\n";}