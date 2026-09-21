#pragma once
struct Settings {
    bool fullscreen=false,vsync=true;
    int resolution=1,quality=0,fov=75,volume=70;
    int frameLimit=0; // Zero disables the software FPS limit; VSync is independent.
    int musicVolume=35;
    int controllerIcons=0,controllerSensitivity=100,controllerDeadzone=15;
    bool controllerInvertY=false;
};
inline constexpr int widths[]={960,1280,1600,1920,2560,3840};
inline constexpr int heights[]={540,720,900,1080,1440,2160};
inline constexpr int resolutionCount=sizeof(widths)/sizeof(widths[0]);
inline constexpr int frameLimits[]={0,30,60,75,90,120,144,165};
inline constexpr int frameLimitCount=sizeof(frameLimits)/sizeof(frameLimits[0]);
inline bool validFrameLimit(int value) {
    for(int limit:frameLimits)if(value==limit)return true;
    return false;
}
