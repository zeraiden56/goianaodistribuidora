#include "world.hpp"
#include <cmath>

std::array<TrafficVehicle,6> trafficAt(double seconds) {
    std::array<TrafficVehicle,6> vehicles{};
    for(int i=0;i<6;++i) {
        bool opposite=i>=3;
        float speed=opposite?-7.5f:6.f;
        double phase=std::fmod(seconds*std::abs(speed)+(i%3)*18.0,54.0);
        if(phase<0)phase+=54;
        vehicles[i]={float(opposite?27-phase:phase-27),opposite?-10.3f:-7.7f,speed,i%2==1,i%3};
    }
    return vehicles;
}
const std::array<SecurityCamera,3>& securityCameras() {
    static const std::array<SecurityCamera,3> cameras{{
        {4.45f,3.25f,3.5f,-.86f,23.f,"CAM 01 - INTERIOR DA LOJA"},
        {-4.5f,3.4f,-4.35f,.85f,22.f,"CAM 02 - FACHADA E RUA"},
        {6.f,4.3f,-12.1f,-2.1f,31.f,"CAM 03 - RUA E ENTRADA"}
    }};
    return cameras;
}
