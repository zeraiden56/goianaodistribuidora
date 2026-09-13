#pragma once
struct Player {float x=0,z=1,yaw=0,pitch=0;bool seated=false;};
int interaction(float px,float pz,float yaw,bool expanded=false,bool secondCheckout=false,bool largeStore=false,bool thirdCheckout=false,int annexCheckouts=0);
bool walkable(float x,float z,bool expanded=false,bool largeStore=false);
float movementSpeed(bool sprint,float intoxication);
float movePlayer(Player& player,float forward,float strafe,bool sprint,float intoxication,float dt,bool expanded=false,bool largeStore=false);
float visionBlur(float intoxication);
bool toggleSeat(Player& player,bool expanded);
