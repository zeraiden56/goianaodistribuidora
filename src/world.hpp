#pragma once
#include <array>
#include <string>

struct TrafficVehicle {float x,z,speed;bool motorcycle;int color;};
std::array<TrafficVehicle,6> trafficAt(double seconds);
struct SecurityCamera {float x,y,z,yaw,pitch;const char* name;const char* label;int expansion=0;};
inline constexpr int securityCameraCount=6;
const std::array<SecurityCamera,securityCameraCount>& securityCameras();
bool cameraAvailable(int camera,bool expanded,bool largeStore);
int nextSecurityCamera(int camera,int direction,bool expanded,bool largeStore);
int surveillanceHit(int x,int y);
float streetLightIntensity(double seconds);

float daylightAt(double seconds);
std::string worldClock(double seconds);
