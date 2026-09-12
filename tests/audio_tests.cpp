#include "audio.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main() {
    SDL_SetMainReady();
    try {
        for(int i=0;i<int(Sound::Count);++i) {
            auto sound=synthesizeSound(static_cast<Sound>(i));
            check(!sound.empty()&&sound.size()<=96000,"clip duration");
            double energy=0;
            for(float value:sound){check(std::isfinite(value)&&std::abs(value)<=.8f,"finite bounded samples");energy+=value*value;}
            check(energy>1,"audible signal energy");
            check(sound.front()==0&&sound.back()==0,"quiet clip boundaries");
        }
        check(synthesizeSound(Sound::Cash)!=synthesizeSound(Sound::Card),"distinct payment sounds");
        check(synthesizeSound(Sound::Count).empty(),"invalid sound rejected");
        Audio audio;check(audio.initialize(),"dummy audio device opens");audio.setVolume(70);audio.pause(false);
        // Exercise overlapping playback, callback synchronization and safe teardown.
        for(int i=0;i<80;++i)audio.play(static_cast<Sound>(i%int(Sound::Count)));
        SDL_Delay(60);audio.pause(true);audio.clear();audio.moved(.03f);audio.moved(0);
        audio.setVolume(0);audio.pause(false);audio.play(Sound::Smoke);SDL_Delay(25);
        audio.setVolume(100);bool cash=false,card=false;
        for(int i=0;i<32;++i){if(audio.payment())card=true;else cash=true;}
        check(cash&&card,"both payment variants selected");
        audio.clear();audio.shutdown();audio.shutdown();SDL_Quit();
        std::cout<<"Synthesis, playback and audio lifecycle tests passed\n";return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
