#pragma once
struct StockLocation {float x,z,workerX,workerZ,reach;};
// One source of truth for interaction, worker routes and furniture placement.
inline constexpr StockLocation stockLocations[]={
    {-3.45f,2.85f,-3.45f,2.2f,1.55f},
    {.65f,-2.25f,.65f,-1.45f,1.35f},
    {-.65f,2.85f,-.65f,2.2f,1.55f},
    {-3.7f,.4f,-2.2f,.4f,1.8f}
};
