#pragma once
#include <SDL.h>
#include <SDL_mixer.h>
#include <filesystem>
#include <string>
#include <vector>

struct MusicTrack {
    std::filesystem::path file;
    std::string title;
};
std::vector<MusicTrack> discoverMusic(const std::filesystem::path& directory);

// Uses the stereo mixer owned by Audio. Destroy this before shutting Audio down.
class Music {
    std::vector<MusicTrack> tracks;
    std::vector<bool> failed;
    Mix_Music* playing = nullptr;
    int current = -1;
    bool initialized = false, suspended = false;
    std::string notice = "NENHUMA MUSICA ENCONTRADA";
    Uint32 trackStarted = 0;
public:
    Music() = default;
    Music(const Music&) = delete;
    Music& operator=(const Music&) = delete;
    ~Music() { shutdown(); }
    bool initialize(const std::filesystem::path& directory, int volume);
    void shutdown();
    bool next();
    void update();
    void setVolume(int percent);
    void suspend(bool paused);
    std::string title() const { return playing ? tracks[current].title : notice; }
    int index() const { return current; }
    int count() const { return static_cast<int>(tracks.size()); }
    bool active() const { return playing != nullptr; }
    float titleSeconds() const { return (SDL_GetTicks()-trackStarted)/1000.f; }
};
