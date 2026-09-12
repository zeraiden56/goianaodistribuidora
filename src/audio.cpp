#include "audio.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {
constexpr double pi=3.141592653589793;
struct Synth {
    std::vector<float> samples;
    int rate;
    std::uint32_t rng=0x53ac19;
    Synth(float seconds,int sampleRate):samples(static_cast<std::size_t>(seconds*sampleRate)),rate(sampleRate){}
    float noise(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng&0xffff)/32767.5f-1;}
    void tone(float start,float duration,float hz,float gain,float endHz=0) {
        for(int i=0;i<int(duration*rate);++i) {
            auto index=static_cast<std::size_t>(start*rate)+i;if(index>=samples.size())break;
            float t=float(i)/rate,u=t/duration;
            float envelope=std::min(1.f,t/.005f)*std::min(1.f,(duration-t)/.012f)*std::exp(-2.f*u);
            double phase=2*pi*(hz*t+(endHz>0?(endHz-hz)*t*t/(2*duration):0));
            samples[index]+=gain*envelope*float(std::sin(phase));
        }
    }
    void noiseBurst(float start,float duration,float gain,float smoothing) {
        float filtered=0;
        for(int i=0;i<int(duration*rate);++i) {
            auto index=static_cast<std::size_t>(start*rate)+i;if(index>=samples.size())break;
            float t=float(i)/rate,u=t/duration;
            filtered+=smoothing*(noise()-filtered);
            float envelope=std::min(1.f,t/.004f)*std::min(1.f,(duration-t)/.02f)*std::exp(-3*u);
            samples[index]+=gain*filtered*envelope;
        }
    }
};
}
std::vector<float> synthesizeSound(Sound sound,int sampleRate) {
    if(sampleRate<8000||sampleRate>192000||sound>=Sound::Count)return {};
    Synth s(sound==Sound::Smoke?2.f:sound==Sound::Drink?1.9f:1.2f,sampleRate);
    switch(sound) {
    case Sound::Step:
        s.tone(0,.15f,90,.2f,55);s.noiseBurst(0,.09f,.1f,.08f);
        s.noiseBurst(.055f,.14f,.055f,.045f);break;
    case Sound::Drink:
        s.noiseBurst(0,.04f,.28f,.8f);s.tone(.01f,.06f,1600,.12f,700);
        s.noiseBurst(.04f,.22f,.14f,.18f);
        for(float at:{.28f,.65f,1.02f,1.39f}) {
            s.tone(at,.17f,310,.22f,105);s.tone(at+.025f,.12f,540,.08f,180);
            s.noiseBurst(at,.18f,.16f,.06f);
        }
        break;
    case Sound::Smoke:
        // Flint wheel, ignition click, brief flame, then inhale and exhale.
        s.noiseBurst(0,.045f,.55f,.85f);s.tone(0,.035f,2200,.12f,900);
        s.noiseBurst(.1f,.06f,.4f,.7f);s.noiseBurst(.16f,.32f,.19f,.26f);
        s.noiseBurst(.5f,.55f,.24f,.035f);s.noiseBurst(1.2f,.75f,.32f,.055f);break;
    case Sound::Cash:
        s.noiseBurst(0,.1f,.28f,.22f);s.tone(.025f,.65f,1850,.3f);
        s.tone(.025f,.52f,2731,.12f);s.noiseBurst(.18f,.28f,.18f,.13f);
        for(float at:{.27f,.36f,.48f})s.tone(at,.09f,3200,.09f);
        s.noiseBurst(.65f,.08f,.28f,.25f);break;
    case Sound::Card:
        s.noiseBurst(0,.13f,.12f,.18f);s.tone(.18f,.1f,1050,.21f);
        s.tone(.48f,.13f,1450,.2f);s.tone(.65f,.24f,1900,.2f);break;
    case Sound::Computer:
        s.noiseBurst(0,.04f,.2f,.8f);s.noiseBurst(.04f,.7f,.1f,.025f);
        s.tone(.12f,.16f,440,.17f);s.tone(.3f,.17f,660,.17f);s.tone(.5f,.35f,880,.18f);break;
    case Sound::Pickup:
        s.noiseBurst(0,.06f,.25f,.24f);s.tone(.015f,.13f,620,.1f,340);break;
    case Sound::Delivery:
        s.tone(0,.22f,660,.2f);s.tone(.24f,.3f,880,.2f);s.noiseBurst(.55f,.12f,.22f,.15f);break;
    default:break;
    }
    for(auto& sample:s.samples)sample=std::clamp(sample,-.8f,.8f);
    // Trim silence so inactive voices are promptly returned to the mixer.
    while(!s.samples.empty()&&std::abs(s.samples.back())<.00001f)s.samples.pop_back();
    if(!s.samples.empty())s.samples.back()=0;
    return s.samples;
}
bool Audio::initialize() {
    if(device)return true;
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return false;
    for(std::size_t i=0;i<clips.size();++i)clips[i]=synthesizeSound(static_cast<Sound>(i));
    SDL_AudioSpec wanted{};wanted.freq=48000;wanted.format=AUDIO_F32SYS;wanted.channels=1;wanted.samples=512;
    wanted.callback=&Audio::callback;wanted.userdata=this;
    device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
    return device!=0;
}
void Audio::shutdown() {if(device){SDL_CloseAudioDevice(device);device=0;}}
void Audio::callback(void* user,Uint8* stream,int bytes) {
    auto& audio=*static_cast<Audio*>(user);
    auto* out=reinterpret_cast<float*>(stream);int count=bytes/int(sizeof(float));
    std::memset(stream,0,bytes);
    for(auto& voice:audio.voices) {
        if(!voice.clip)continue;
        for(int i=0;i<count;++i) {
            auto index=static_cast<std::size_t>(voice.cursor);
            if(index>=voice.clip->size()){voice.clip=nullptr;break;}
            auto next=std::min(index+1,voice.clip->size()-1);
            float fraction=float(voice.cursor-index);
            out[i]+=((*voice.clip)[index]*(1-fraction)+(*voice.clip)[next]*fraction)*voice.gain;
            voice.cursor+=voice.rate;
        }
    }
    for(int i=0;i<count;++i)out[i]=std::clamp(out[i]*audio.volume,-.95f,.95f);
}
void Audio::play(Sound sound,float gain,float rate) {
    auto index=static_cast<std::size_t>(sound);
    if(!device||index>=clips.size()||clips[index].empty())return;
    SDL_LockAudioDevice(device);
    auto voice=std::find_if(voices.begin(),voices.end(),[](const Voice& v){return !v.clip;});
    if(voice!=voices.end())*voice={&clips[index],0,std::clamp(rate,.5f,2.f),std::clamp(gain,0.f,1.f)};
    SDL_UnlockAudioDevice(device);
}
bool Audio::payment() {
    bool card=(random()%2)==0;play(card?Sound::Card:Sound::Cash);return card;
}
void Audio::moved(float distance) {
    if(distance<=.00001f){stepDistance=0;walking=false;return;}
    if(!walking){play(Sound::Step,.4f,.92f+float(random()%17)/100);walking=true;}
    stepDistance+=distance;
    if(stepDistance>=.85f){stepDistance=std::fmod(stepDistance,.85f);play(Sound::Step,.4f,.92f+float(random()%17)/100);}
}
void Audio::setVolume(int percent) {
    if(device)SDL_LockAudioDevice(device);
    volume=float(std::clamp(percent,0,100))/100;
    if(device)SDL_UnlockAudioDevice(device);
}
void Audio::pause(bool paused) {if(device)SDL_PauseAudioDevice(device,paused?1:0);}
void Audio::clear() {
    if(device)SDL_LockAudioDevice(device);
    for(auto& voice:voices)voice={};
    stepDistance=0;walking=false;
    if(device)SDL_UnlockAudioDevice(device);
}
