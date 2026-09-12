#pragma once
#include <SDL.h>
#include <array>
#include <cstddef>
#include <random>
#include <vector>

enum class Sound {Step,Drink,Smoke,Cash,Card,Computer,Pickup,Delivery,Count};
// Generated PCM assets are immutable while the SDL callback is running.
std::vector<float> synthesizeSound(Sound sound,int sampleRate=48000);
class Audio {
    struct Voice {const std::vector<float>* clip=nullptr;double cursor=0;float rate=1,gain=1;};
    SDL_AudioDeviceID device=0;
    std::array<std::vector<float>,static_cast<std::size_t>(Sound::Count)> clips;
    std::array<Voice,24> voices{};
    std::mt19937 random{8192};
    float volume=.7f,stepDistance=0;
    bool walking=false;
    static void callback(void* user,Uint8* stream,int bytes);
public:
    Audio()=default;
    Audio(const Audio&)=delete;
    Audio& operator=(const Audio&)=delete;
    ~Audio(){shutdown();}
    bool initialize();
    void shutdown();
    bool available() const {return device!=0;}
    void play(Sound sound,float gain=1,float rate=1);
    bool payment(); // Cosmetic choice, independent of the saved customer RNG.
    void moved(float distance);
    void setVolume(int percent);
    void pause(bool paused);
    void clear();
};
