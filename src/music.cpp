#include "music.hpp"
#include <algorithm>
#include <cctype>
#include <climits>
#include <iostream>

namespace {
int trackNumber(const std::filesystem::path& file) {
    const auto name=file.stem().u8string();
    int number=0;
    if(name.empty()||!std::isdigit(static_cast<unsigned char>(name[0])))return INT_MAX;
    for(unsigned char c:name) {
        if(!std::isdigit(c))break;
        if(number>(INT_MAX-9)/10)return INT_MAX;
        number=number*10+(c-'0');
    }
    return number;
}
std::string trackTitle(const std::filesystem::path& file) {
    auto name=file.stem().u8string();
    const auto separator=name.find(" - ");
    if(separator!=std::string::npos&&separator>0&&
       std::all_of(name.begin(),name.begin()+separator,[](unsigned char c){return std::isdigit(c);}))
        name.erase(0,separator+3);
    return name;
}
}

std::vector<MusicTrack> discoverMusic(const std::filesystem::path& directory) {
    std::vector<MusicTrack> result;
    std::error_code error;
    std::filesystem::directory_iterator it(directory,error), end;
    while(!error&&it!=end) {
        if(it->is_regular_file(error)) {
            auto extension=it->path().extension().u8string();
            std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return std::tolower(c);});
            if(extension==".mp3"||extension==".ogg"||extension==".wav"||extension==".flac")
                result.push_back({it->path(),trackTitle(it->path())});
        }
        it.increment(error);
    }
    std::sort(result.begin(),result.end(),[](const MusicTrack& a,const MusicTrack& b){
        const int first=trackNumber(a.file), second=trackNumber(b.file);
        return first==second?a.file.filename().u8string()<b.file.filename().u8string():first<second;
    });
    return result;
}

bool Music::initialize(const std::filesystem::path& directory,int volume) {
    shutdown();
    tracks=discoverMusic(directory);
    failed.assign(tracks.size(),false);
    if(tracks.empty()) { notice="NENHUMA MUSICA ENCONTRADA"; return false; }
    if(!Mix_QuerySpec(nullptr,nullptr,nullptr)) { notice="MUSICA INDISPONIVEL - SEM AUDIO"; return false; }
    initialized=true;
    if((Mix_Init(MIX_INIT_MP3)&MIX_INIT_MP3)==0)
        std::cerr<<"MP3 decoder unavailable: "<<Mix_GetError()<<'\n';
    setVolume(volume);
    return next();
}

void Music::shutdown() {
    if(playing) { Mix_HaltMusic(); Mix_FreeMusic(playing); playing=nullptr; }
    if(initialized)Mix_Quit();
    initialized=false;suspended=false;current=-1;
    tracks.clear();failed.clear();
}

bool Music::next() {
    if(!initialized||tracks.empty())return false;
    if(playing) { Mix_HaltMusic();Mix_FreeMusic(playing);playing=nullptr; }
    for(std::size_t attempts=0;attempts<tracks.size();++attempts) {
        current=(current+1)%static_cast<int>(tracks.size());
        if(failed[current])continue;
        playing=Mix_LoadMUS(tracks[current].file.u8string().c_str());
        if(playing&&Mix_FadeInMusic(playing,0,350)==0) {
            if(suspended)Mix_PauseMusic();
            trackStarted=SDL_GetTicks();
            return true;
        }
        std::cerr<<"Cannot play "<<tracks[current].file.u8string()<<": "<<Mix_GetError()<<'\n';
        if(playing)Mix_FreeMusic(playing);
        playing=nullptr;failed[current]=true;
    }
    current=-1;notice="NAO FOI POSSIVEL TOCAR AS MUSICAS";
    return false;
}

void Music::update() {
    // File IO stays on the main thread; SDL_mixer streams only the active song.
    if(playing&&!suspended&&!Mix_PlayingMusic())next();
}

void Music::setVolume(int percent) {
    if(initialized)Mix_VolumeMusic(std::clamp(percent,0,100)*MIX_MAX_VOLUME/100);
}

void Music::suspend(bool paused) {
    suspended=paused;
    if(!playing)return;
    if(paused)Mix_PauseMusic();else Mix_ResumeMusic();
}
