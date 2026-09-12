#pragma once
#include <array>

struct TrafficVehicle {float x,z,speed;bool motorcycle;int color;};
std::array<TrafficVehicle,6> trafficAt(double seconds);
struct SecurityCamera {float x,y,z,yaw,pitch;const char* name;};
const std::array<SecurityCamera,3>& securityCameras();
