#include "world.hpp"
#include "shop_layout.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

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
const std::array<SecurityCamera,securityCameraCount>& securityCameras() {
    auto view=[](float x,float y,float z,float tx,float ty,float tz,const char* name,const char* label,int expansion=0) {
        float dx=(tx-x)*shopScaleX,dz=(tz-z)*shopScaleZ;
        return SecurityCamera{x,y,z,std::atan2(dx,-dz),-std::atan2(ty-y,std::hypot(dx,dz))*180/3.14159265f,name,label,expansion};
    };
    static const std::array<SecurityCamera,securityCameraCount> cameras{{
        view(4.45f,3.25f,2.65f,-1.f,1.f,-1.f,"CAM 01 - INTERIOR DA LOJA","LOJA"),
        view(-4.5f,3.4f,-4.35f,1.f,.8f,-8.f,"CAM 02 - CALCADA E FACHADA","FACHADA"),
        view(6.25f,4.3f,-11.98f,0,1.2f,-2.9f,"CAM 03 - ENTRADA E CAIXAS","ENTRADA"),
        view(4.4f,3.15f,9.4f,-1.f,1.f,6.2f,"CAM 04 - DEPOSITO E DESCANSO","DEPOSITO",1),
        view(8.55f,3.2f,2.4f,5.8f,1.f,-2.2f,"CAM 05 - ALA DO ATACADO","ATACADO",2),
        view(-15.65f,5.05f,-5.95f,-3.f,.8f,-8.8f,"CAM 06 - AVENIDA E TRAFEGO","AVENIDA")
    }};
    return cameras;
}
bool cameraAvailable(int camera,bool expanded,bool largeStore) {
    if(camera<0||camera>=securityCameraCount)return false;
    int required=securityCameras()[camera].expansion;
    return required==0||(required==1?expanded:largeStore);
}
int nextSecurityCamera(int camera,int direction,bool expanded,bool largeStore) {
    int step=direction<0?-1:1;
    for(int i=0;i<securityCameraCount;++i) {
        camera=(camera+step+securityCameraCount)%securityCameraCount;
        if(cameraAvailable(camera,expanded,largeStore))return camera;
    }
    return 0;
}
int surveillanceHit(int x,int y) {
    if(y<458||y>=504||x<18)return -1;
    int cell=(x-18)/155;
    return cell<securityCameraCount&&(x-18)%155<148?cell:-1;
}
float streetLightIntensity(double seconds) {
    float dusk=std::clamp((.55f-daylightAt(seconds))/.55f,0.f,1.f);
    return dusk*dusk*(3-2*dusk);
}

float daylightAt(double seconds) {
    double hour=std::fmod(8.+seconds/180.*24.,24.);
    return std::clamp(float(std::sin((hour-6.)*3.141592653589793/12.)*1.8+.15),0.f,1.f);
}
std::string worldClock(double seconds) {
    int minutes=int(std::fmod(480.+seconds/180.*1440.,1440.));
    char clock[32];std::snprintf(clock,sizeof(clock),"%02d:%02d",minutes/60,minutes%60);return clock;
}
