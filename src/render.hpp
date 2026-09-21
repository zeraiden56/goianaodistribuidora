#pragma once
#include "game.hpp"
#include "player.hpp"
#include "settings.hpp"
struct Box {float x,y,z,w,h,d,r,g,b;};
void rect(float x,float y,float w,float h,float r,float g,float b);
void text(float x,float y,const std::string& s,float scale=2,float r=.9f,float g=.93f,float b=.85f);
void box(Box a);
void sign(float x,float y,float z,const std::string& title,float width,bool back=false);
void crate(int product,float x,float y,float z,float size=1);
void item(int product,float x,float y,float z,float size=1);
enum class Surface {Tile,Plaster,Brick,Wood,Asphalt};
void texturedBox(Box box,Surface surface);
void destroyMaterials();
void rod(float ax,float ay,float az,float bx,float by,float bz,float radius,float r,float g,float b);
void cylinder(float x,float y,float z,float radius,float height,float r,float g,float b,float topRadius=-1);
void character(int style,float phase,float walk,float carry,float reach,bool staff=false);
void renderOrderBubbles(const Game& game,const Player& player);
void captureOrderPositions(const Game& game);
void renderMenuWorld(const Game& game,const Settings& settings,int w,int h,double seconds);
void scene(const Game& g,int activeCamera=-1);
void renderWorld(const Game& g,const Player& player,const Settings& settings,int w,int h,int camera=-1,float computerFocus=0,int terminal=0);
void beginUI();
void renderHUD(const Game& g,const Player& player,bool computer);
void destroyRenderer();
void renderSurveillanceHUD(const Game& game,int camera);
void renderCustomer(const Game& game);
