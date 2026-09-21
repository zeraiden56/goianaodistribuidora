#include "audio.hpp"
#include "music.hpp"
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void wav(const std::filesystem::path& file) {
    std::ofstream out(file,std::ios::binary);
    auto little=[&](unsigned value,int bytes){for(int i=0;i<bytes;++i)out.put(static_cast<char>(value>>(i*8)));};
    constexpr unsigned frames=7200, bytes=frames*4;
    out.write("RIFF",4);little(36+bytes,4);out.write("WAVEfmt ",8);little(16,4);little(1,2);little(2,2);
    little(48000,4);little(48000*4,4);little(4,2);little(16,2);out.write("data",4);little(bytes,4);
    for(unsigned i=0;i<frames;++i)for(int channel=0;channel<2;++channel)
        little(static_cast<unsigned>(static_cast<short>(std::sin(i*.08)*(channel?3000:6000))),2);
}
std::atomic<bool> audible{false}, stereo{false};
void observe(int,void* stream,int bytes,void*) {
    auto* samples=static_cast<float*>(stream);
    for(int i=0;i+1<bytes/int(sizeof(float));i+=2) {
        if(std::abs(samples[i])>.001f)audible=true;
        if(std::abs(samples[i]-samples[i+1])>.001f)stereo=true;
    }
}
}

int main(int argc,char** argv) {
    SDL_SetMainReady();
    const auto directory=std::filesystem::temp_directory_path()/
        ("distribuidora-music-"+std::to_string(SDL_GetPerformanceCounter()));
    try {
        std::filesystem::create_directories(directory);
        wav(directory/std::filesystem::u8path(u8"02 - Canção curta.wav"));
        wav(directory/"10 - Second.wav");
        std::ofstream(directory/"03 - Broken.mp3")<<"invalid MP3";
        std::ofstream(directory/"readme.txt")<<"not a song";
        const auto tracks=discoverMusic(directory);
        check(tracks.size()==3&&tracks[0].title==u8"Canção curta"&&tracks[2].title=="Second","numeric ordering and UTF-8 titles");
        Audio audio;check(audio.initialize(),"shared stereo device opens");
        Music music;check(music.initialize(directory,35),"first track starts");
        check(Mix_RegisterEffect(MIX_CHANNEL_POST,observe,nullptr,nullptr)!=0,"observe music stream");
        audio.pause(true);SDL_Delay(100);
        check(audible&&stereo,"music produces stereo PCM while game effects are paused");
        music.suspend(true);SDL_Delay(200);music.update();
        check(music.index()==0&&Mix_PausedMusic(),"focus pause retains current track");
        music.suspend(false);SDL_Delay(250);music.update();
        check(music.index()==2&&music.active(),"automatic advance skips broken audio");
        music.setVolume(0);check(Mix_VolumeMusic(-1)==0,"music can be muted independently");
        audio.setVolume(100);audio.pause(false);audio.play(Sound::Cash);
        check(Mix_VolumeMusic(-1)==0,"effects volume does not unmute music");
        music.setVolume(100);check(Mix_VolumeMusic(-1)==MIX_MAX_VOLUME,"music volume maximum");
        check(music.next()&&music.index()==0,"playlist wraps around");
        for(int i=0;i<20;++i)check(music.next(),"repeated manual skips");
        Mix_UnregisterEffect(MIX_CHANNEL_POST,observe);
        music.shutdown();music.shutdown();
        check(!music.initialize(directory/"missing",35)&&!music.active(),"missing folder is non-fatal");
        std::filesystem::create_directory(directory/"bad");
        std::ofstream(directory/"bad"/"bad.mp3")<<"invalid";
        check(!music.initialize(directory/"bad",35)&&!music.next(),"all-broken playlist stops retrying");
        if(argc>1&&!discoverMusic(std::filesystem::u8path(argv[1])).empty()) {
            check(music.initialize(std::filesystem::u8path(argv[1]),0),"actual MP3 playlist opens");
            const int count=music.count();
            for(int i=0;i<count;++i) {
                check(music.index()==i,"all actual music files decode in order");
                check(music.next(),"actual MP3 skip");
            }
            std::cout<<count<<" actual music files decoded\n";
        }
        music.shutdown();audio.shutdown();SDL_Quit();
        std::filesystem::remove_all(directory);
        std::cout<<"Music playlist, streaming, volume and lifecycle tests passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        std::filesystem::remove_all(directory);SDL_Quit();return 1;
    }
}
