#pragma once
struct Settings {
    bool fullscreen=false,vsync=true;
    int resolution=1,quality=0,fov=75,volume=70;
};
inline constexpr int widths[]={960,1280,1600,1920};
inline constexpr int heights[]={540,720,900,1080};
