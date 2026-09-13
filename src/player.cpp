#include "player.hpp"
#include "shop_layout.hpp"
#include <cmath>
#include <algorithm>

// Pick the nearby station the player is looking towards. Product IDs: 0..3.
int interaction(float px,float pz,float yaw,bool expanded,bool secondCheckout,bool largeStore,bool thirdCheckout,int annexCheckouts) {
    struct Station {float x,z,reach;int id;};
    const Station stations[]={{stockLocations[0].x,stockLocations[0].z,stockLocations[0].reach,0},
        {stockLocations[1].x,stockLocations[1].z,stockLocations[1].reach,1},
        {stockLocations[2].x,stockLocations[2].z,stockLocations[2].reach,2},
        {stockLocations[3].x,stockLocations[3].z,stockLocations[3].reach,3},{3.6f,1.8f,1.9f,4},{-2.1f,-2.6f,1.8f,5},
        {3.2f,6.2f,1.65f,6},{-4.7f,6.2f,1.8f,7},
        {-3.6f,8.7f,1.5f,8},{-1.5f,8.7f,1.5f,9},{.6f,8.7f,1.5f,10},{2.7f,8.7f,1.5f,11},{2.1f,-2.6f,1.8f,12},
        {5.9f,2.85f,1.55f,13},{8.f,2.85f,1.55f,14},{0,-2.8f,1.8f,15},
        {2.5f,7.35f,1.8f,16},{5.9f,8.7f,1.5f,17},{8.f,8.7f,1.5f,18},{5.9f,-2.6f,1.8f,19},{8.f,-2.6f,1.8f,20}};
    int target=-1;float best=0.72f;
    for(auto station:stations) {
        if(station.id>=6&&!expanded)continue;
        if(station.id==12&&!secondCheckout)continue;
        if((station.id==13||station.id==14||station.id>=17)&&!largeStore)continue;
        if(station.id==15&&!thirdCheckout)continue;
        if(station.id>=19&&annexCheckouts<station.id-18)continue;
        float dx=station.x-px,dz=station.z-pz,d=std::hypot(dx,dz);
        if(d<.01f||d>station.reach) continue;
        float alignment=(dx*std::sin(yaw)-dz*std::cos(yaw))/d;
        if(alignment>best) {best=alignment;target=station.id;}
    }
    return target;
}

bool walkable(float x,float z,bool expanded,bool largeStore) {
    if(!std::isfinite(x)||!std::isfinite(z)||x<=-4.65f||x>=(largeStore?8.65f:4.65f)||z<=-1.78f)return false;
    if(largeStore&&x>4.65f) {
        if(z>=9.65f)return false;
        if(z>2.5f&&z<4.25f)return false; // New refrigerators and partition.
        if(z>8.45f)return false; // New depot racks.
        return true;
    }
    if(largeStore&&x>4.35f&&z>2.5f&&z<4.35f)return false;
    if(!expanded)return z<2.5f&&!(x>2.48f&&z>1.f)&&!(x< -2.65f&&z>-.6f&&z<1.4f);
    if(z>=9.65f)return false;
    if(x>2.48f&&x<4.6f&&z>1.f&&z<2.6f)return false; // Computer desk.
    if(x< -2.65f&&z>-.6f&&z<1.4f)return false; // Freezer.
    if(x<1.8f&&z>2.5f&&z<4.25f)return false; // Front shelving / partition.
    if(z>3.65f&&z<4.35f&&(x<1.9f||x>3.85f))return false; // Back-room doorway.
    if(x>2.95f&&x<4.85f&&z>4.8f&&z<7.65f)return false; // Sofa.
    if(z>8.45f&&x<3.85f)return false; // Depot racks.
    if(x>1.85f&&x<2.95f&&z>7.05f&&z<7.85f)return false; // Sofa computer table.
    return true;
}

float movementSpeed(bool sprint,float intoxication) {
    return (sprint?4.8f:2.7f)*(1-std::clamp(intoxication,0.f,100.f)*.003f);
}
float movePlayer(Player& p,float forward,float strafe,bool sprint,float intoxication,float dt,bool expanded,bool largeStore) {
    if(p.seated)return 0;
    float norm=std::max(1.f,std::hypot(forward,strafe));
    float step=movementSpeed(sprint,intoxication)*std::clamp(dt,0.f,.05f)/norm;
    float dx=(std::sin(p.yaw)*forward+std::cos(p.yaw)*strafe)*step;
    float dz=(-std::cos(p.yaw)*forward+std::sin(p.yaw)*strafe)*step;
    float x=p.x,z=p.z;
    if(walkable(p.x+dx,p.z,expanded,largeStore))p.x+=dx;
    if(walkable(p.x,p.z+dz,expanded,largeStore))p.z+=dz;
    return std::hypot(p.x-x,p.z-z);
}
float visionBlur(float intoxication) {
    return 11.f*std::pow(std::clamp(intoxication,0.f,100.f)/100.f,1.3f);
}

bool toggleSeat(Player& p,bool expanded) {
    if(!expanded)return false;
    if(p.seated){p.seated=false;p.x=2.65f;p.z=6.2f;return true;}
    if(std::hypot(p.x-3.2f,p.z-6.2f)>1.65f)return false;
    p.seated=true;p.x=3.8f;p.z=6.2f;p.yaw=-1.5707963f;p.pitch=-5;return true;
}
