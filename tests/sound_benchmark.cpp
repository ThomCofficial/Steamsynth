#include "dsp/DSP.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
static void le16(std::ofstream&f,uint16_t x){f.put(char(x&255));f.put(char(x>>8));}
static void le32(std::ofstream&f,uint32_t x){le16(f,uint16_t(x&65535));le16(f,uint16_t(x>>16));}
int main(){constexpr int sr=48000,dur=12,n=sr*dur;
 steam::Instrument i;i.prepare(sr);steam::Macro m;m.seed=.48f;m.grow=.95f;m.mutate=.66f;m.dirt=.56f;m.bloom=.87f;m.attack=.14f;m.release=.85f;m.tapeTime=.56f;m.tapeFeedback=.62f;m.tapeMix=.56f;m.poly=16;
 i.setMacro(m);auto t=std::chrono::steady_clock::now();
 std::ofstream f("SteamSynth_diagnostic.wav",std::ios::binary);f.write("RIFF",4);le32(f,36+n*4);f.write("WAVEfmt ",8);le32(f,16);le16(f,1);le16(f,2);le32(f,sr);le32(f,sr*4);le16(f,4);le16(f,16);f.write("data",4);le32(f,n*4);
 float peak=0;double sum=0;for(int x=0;x<n;++x){if(x==0)i.noteOn(48,.8f);if(x==sr*2)i.noteOn(55,.8f);if(x==sr*4)i.noteOn(60,.8f);if(x==sr*5)i.noteOn(64,.8f);
 if(x==sr*8){i.noteOff(48);i.noteOff(55);i.noteOff(60);i.noteOff(64);}float l=0,r=0;i.process(l,r,120);peak=std::max(peak,std::max(std::abs(l),std::abs(r)));sum+=double(l*l+r*r);le16(f,uint16_t(int16_t(std::lrint(l*32767.f))));le16(f,uint16_t(int16_t(std::lrint(r*32767.f))));}
 f.close();auto secs=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
 std::cout<<"rendered "<<dur<<"s of 16-voice-ready synth in "<<secs<<" s ; peak "<<peak<<" RMS "<<std::sqrt(sum/(n*2))<<" speed "<<dur/secs<<"x realtime\n";
}