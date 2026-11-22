#pragma once
#include <JuceHeader.h>

// name, min, max, default, skew (skew 0.3 для частоты и времени: крутилка мельче внизу)
struct ParamDef { const char* name; float min, max, def; float skew = 1.0f; };
struct TabDef  { const char* name; int count; ParamDef p[8]; };

inline const TabDef kTabs[10] = {
    { "OSC", 4, { {"Wave 1",0,3,1}, {"Wave 2",0,3,0}, {"Octave",-3,3,0}, {"Level",0,100,80} } },
    { "FILTER", 4, { {"Type",0,3,0}, {"Cutoff",20,20000,8000,0.3f}, {"Resonance",0,100,20}, {"Env Amt",-100,100,30} } },
    { "ENV A", 4, { {"Attack",0,5000,10,0.3f}, {"Decay",0,5000,300,0.3f}, {"Sustain",0,100,70}, {"Release",0,8000,400,0.3f} } },
    { "ENV B", 4, { {"Attack",0,5000,200,0.3f}, {"Decay",0,5000,800,0.3f}, {"Sustain",0,100,50}, {"Release",0,8000,1200,0.3f} } },
    { "LFO 1", 4, { {"Shape",0,3,0}, {"Rate",0.1f,30,2,0.5f}, {"Depth",0,100,0}, {"Phase",0,360,0} } },
    { "LFO 2", 4, { {"Shape",0,3,2}, {"Rate",0.1f,30,0.5f,0.5f}, {"Depth",0,100,0}, {"Phase",0,360,90} } },
    { "MOD", 4, { {"Vel>Amp",0,100,80}, {"Vel>Filter",-100,100,20}, {"Mod>Pitch",0,100,0}, {"Aft>Vib",0,100,0} } },
    { "FX", 8, { {"Delay Time",1,1000,375,0.5f}, {"Feedback",0,100,35}, {"Delay Mix",0,100,20}, {"Delay Tone",0,100,50},
                 {"Room Size",0,100,60}, {"Decay",0,100,40}, {"Damping",0,100,40}, {"Rev Mix",0,100,25} } },
    { "ARP", 4, { {"On",0,1,0}, {"Mode",0,5,0}, {"Rate",1,32,8}, {"Swing",0,100,0} } },
    { "MASTER", 4, { {"Volume",-60,6,-6}, {"Pitch",-12,12,0}, {"Portamento",0,2000,0}, {"Voices",1,32,8} } },
};

struct FxPreset { const char* name; float a, b, c, d; };   // delay: time, fb, mix, tone; reverb: size, decay, damping, mix

inline const FxPreset kDelayPresets[20] = {
    {"Ping Pong 1/4",375,35,35,60}, {"Ping Pong 1/8",188,45,35,60}, {"Ping Pong Dotted",562,50,30,55},
    {"Ping Pong Long",750,60,40,50}, {"Tape Echo",320,38,28,40}, {"Analog Warm",410,42,30,35},
    {"Slapback",90,10,40,70}, {"Dub Echo",500,65,45,30}, {"Dotted 1/8",375,35,25,65},
    {"Stereo Wide",280,30,35,75}, {"Shimmer Delay",600,55,40,85}, {"Filter Sweep",440,50,35,25},
    {"Bright Digital",250,30,30,95}, {"Short Thick",140,25,40,45}, {"Triplet Echo",250,35,30,60},
    {"Doubler",35,20,50,80}, {"Long Ambient",1000,70,45,40}, {"Lo-Fi Echo",330,40,35,20},
    {"Rhythm Gate",125,20,50,70}, {"Self Oscillate",500,92,30,50}
};
inline const FxPreset kReverbPresets[20] = {
    {"Small Room",20,25,60,20}, {"Medium Room",40,35,50,25}, {"Large Room",60,45,45,30},
    {"Vocal Booth",10,15,70,15}, {"Plate Bright",50,40,20,30}, {"Plate Dark",50,40,80,30},
    {"Spring",35,30,40,35}, {"Hall Classic",80,60,40,35}, {"Concert Hall",90,70,35,35},
    {"Cathedral",100,90,30,40}, {"Church Stone",95,85,60,40}, {"Shimmer",85,75,20,45},
    {"Ambient Wash",90,80,25,50}, {"Cave",100,95,70,40}, {"Garage",30,25,55,30},
    {"Studio Chamber",35,35,50,25}, {"Gated Reverb",45,15,40,40}, {"Reverse Swell",70,85,30,45},
    {"Metallic",40,50,10,35}, {"Bathroom",25,60,75,30}
};

inline juce::String paramId(int tab, int idx) { return "t" + juce::String(tab) + "p" + juce::String(idx); }
