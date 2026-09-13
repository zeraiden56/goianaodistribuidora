#pragma once
#include <string>
struct StockLocation {float x,z,workerX,workerZ,reach;};
// One source of truth for interaction, worker routes and furniture placement.
inline constexpr StockLocation stockLocations[]={
    {-3.45f,2.85f,-3.45f,2.2f,1.55f},
    {.65f,-2.25f,.65f,-1.45f,1.35f},
    {-.65f,2.85f,-.65f,2.2f,1.55f},
    {-3.7f,.4f,-2.2f,.4f,1.8f},
    {5.9f,2.85f,5.9f,2.2f,1.55f},{8.f,2.85f,8.f,2.2f,1.55f}
};

inline int stationProduct(int station) {
    if(station>=0&&station<4)return station;
    if(station>=8&&station<=11)return station-8;
    if(station==13||station==14)return station-9;
    if(station==17||station==18)return station-13;
    return -1;
}

inline float checkoutX(int lane) {
    constexpr float positions[]={-2.1f,2.1f,0.f,5.9f,8.f};
    return positions[lane];
}
inline int stationCheckout(int station) {
    return station==5?0:station==12?1:station==15?2:station==19?3:station==20?4:-1;
}
inline std::string carriedLabel(int units,bool packed) {
    if(!packed)return std::to_string(units)+" UN.";
    return std::to_string(units/12)+" CX"+(units%12?" + "+std::to_string(units%12)+" UN.":"");
}
