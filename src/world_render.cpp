#include "world_render.hpp"
#include "world.hpp"
#include "shop_layout.hpp"
#include "render.hpp"
#include <SDL_opengl.h>
#include <cmath>
#include <algorithm>

namespace {
void lightPool(float x,float z,float radius,float night) {
    glBegin(GL_TRIANGLE_FAN);glColor4f(.95f,.69f,.32f,night*.42f);glVertex3f(x,.088f,z);
    glColor4f(.95f,.69f,.32f,0);
    for(int i=0;i<=32;++i){float a=i*6.2831853f/32;glVertex3f(x+std::cos(a)*radius,.088f,z+std::sin(a)*radius*.72f);}
    glEnd();
}
void glow(float x,float y,float z,float night) {
    glPushMatrix();glTranslatef(x,y,z);float matrix[16];glGetFloatv(GL_MODELVIEW_MATRIX,matrix);
    for(int col=0;col<3;++col)for(int row=0;row<3;++row)matrix[col*4+row]=col==row?1.f:0.f;
    glLoadMatrixf(matrix);glBegin(GL_TRIANGLE_FAN);glColor4f(1,.83f,.46f,night*.75f);glVertex3f(0,0,0);
    glColor4f(1,.7f,.27f,0);
    for(int i=0;i<=24;++i){float a=i*6.2831853f/24;glVertex3f(std::cos(a)*.64f,std::sin(a)*.64f,0);}glEnd();glPopMatrix();
}
void streetLamps(float night,bool effects) {
    for(float curb:{-5.7f,-12.15f})for(float x:{-26.f,-16.f,-6.f,6.f,16.f,26.f}) {
        float direction=curb>-9?-1.f:1.f,lampZ=curb+direction*1.05f;
        if(!effects) {
            texturedBox({x,.15f,curb,.38f,.3f,.38f,.38f,.4f,.38f},Surface::Plaster);
            cylinder(x,2.9f,curb,.115f,5.5f,.21f,.26f,.28f,.065f);
            cylinder(x,.5f,curb,.145f,.55f,.15f,.19f,.21f,.115f);
            box({x,1.15f,curb+.105f,.115f,.23f,.055f,.33f,.37f,.36f});
            rod(x,5.48f,curb,x,5.68f,curb+direction*.38f,.06f,.34f,.4f,.42f);
            rod(x,5.68f,curb+direction*.38f,x,5.57f,lampZ,.055f,.34f,.4f,.42f);
            box({x,5.5f,lampZ,.42f,.16f,.66f,.24f,.29f,.31f});
            box({x,5.407f,lampZ,.34f,.035f,.52f,.5f+night*.5f,.55f+night*.34f,.54f+night*.12f});
        } else if(night>0) {
            lightPool(x,lampZ,3.8f,night);glow(x,5.38f,lampZ,night);
            glBegin(GL_TRIANGLE_FAN);glColor4f(.93f,.71f,.39f,night*.065f);glVertex3f(x,5.35f,lampZ);
            glColor4f(.93f,.71f,.39f,0);
            for(int i=0;i<=20;++i){float a=i*6.2831853f/20;glVertex3f(x+std::cos(a)*3.3f,.1f,lampZ+std::sin(a)*2.25f);}glEnd();
        }
    }
}
}
void renderStreet(double seconds) {
    float day=daylightAt(seconds),night=1-day;
    box({0,-.1f,-14,90,.2f,50,.06f,.08f,.105f});
    texturedBox({0,.03f,-9,90,.05f,6,.12f+day*.09f,.13f+day*.09f,.16f+day*.09f},Surface::Asphalt);
    texturedBox({0,.04f,-4.7f,80,.08f,2.6f,.37f,.38f,.37f},Surface::Tile);
    box({0,.07f,-12.2f,80,.08f,.6f,.13f,.14f,.17f});
    for(int i=-8;i<=8;++i) {
        box({i*4.f,.066f,-9,1.8f,.015f,.12f,.44f,.39f,.19f});
        float x=i*5.f;
        float height=3.7f+((i+8)*7%4)*1.8f;
        float brightness=.52f+day*.48f;
        texturedBox({x,height/2,-15,4.7f,height,5,(.46f+(i+8)%3*.12f)*brightness,.49f*brightness,.43f*brightness},i%2?Surface::Brick:Surface::Plaster);
        box({x,1.15f,-12.46f,1.3f,2.3f,.05f,.11f,.17f,.18f});
        for(float y=2.65f;y<height-.35f;y+=1.8f)for(float dx:{-1.5f,1.5f}) {
            box({x+dx,y,-12.42f,.86f,1.06f,.13f,.17f,.19f,.19f});
            box({x+dx,y,-12.34f,.67f,.88f,.035f,.18f+night*.59f,.31f+night*.26f,.38f-night*.11f});
            box({x+dx,y,-12.31f,.04f,.9f,.03f,.23f,.25f,.25f});
        }
        box({x,height+.08f,-15,4.9f,.18f,5.2f,.25f,.29f,.28f});
        if(i%2==0) {
            box({x,2.12f,-12.1f,3.4f,.17f,.9f,.27f,.46f,.4f});
            const char* shops[]={"PADARIA","OFICINA","MERCADO","LANCHONETE"};
            sign(x-1.55f,2.65f,-12.27f,shops[(i+8)/2%4],3.1f);
        }
    }
    streetLamps(streetLightIntensity(seconds),false);
    box({6,2.2f,-12.25f,.14f,4.4f,.14f,.12f,.14f,.16f});
    if(day<.5f) {
        cylinder(-12,14,-35,.65f,.1f,.78f,.83f,.84f);
        for(int i=0;i<28;++i)box({float((i*17)%80-40),float(8+(i*7)%15),-39,.055f,.055f,.055f,.7f,.74f,.82f});
    } else {
        box({-16,17,-37,1.4f,1.4f,.2f,1.f,.87f,.56f});
        for(int i=0;i<7;++i)box({float(i*9-28)+float(std::sin(seconds*.005)*3),12.f+(i%3)*2,-38,5.f+(i%2)*2,.55f,.4f,.88f,.92f,.93f});
    }
    // Neighbours on both sides of the store, with entrances and rooftop tanks.
    for(float x:{-11.f,-18.f,15.f,22.f}) {
        float height=x<0?4.8f:6.5f;
        texturedBox({x,height/2,3,5.2f,height,13,.46f,.5f,.43f},Surface::Brick);
        texturedBox({x,height+.1f,3,5.5f,.2f,13.4f,.25f,.29f,.27f},Surface::Tile);
        box({x,1.4f,-3.53f,2.2f,2.8f,.06f,.19f,.26f,.28f});
        for(float dx:{-1.65f,1.65f})box({x+dx,3.5f,-3.54f,.9f,1.2f,.05f,.18f+night*.5f,.35f+night*.2f,.39f});
        cylinder(x+.8f,height+.5f,5,.7f,.9f,.15f,.32f,.43f);
    }
    for(float x:{-9.f,11.f,20.f}) {
        cylinder(x,1.35f,-4.6f,.1f,2.7f,.31f,.24f,.14f);
        for(int i=0;i<3;++i)cylinder(x,2.9f+i*.38f,-4.6f,.8f-i*.12f,.75f,.2f,.4f,.23f,.58f-i*.13f);
        texturedBox({x,.19f,-4.6f,1.2f,.35f,.8f,.45f,.43f,.33f},Surface::Brick);
    }
    renderTraffic(seconds);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
    streetLamps(streetLightIntensity(seconds),true);
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);
}
void renderSecurityDevices(int activeCamera,bool expanded,bool largeStore) {
    const auto& cameras=securityCameras();
    for(int i=0;i<int(cameras.size());++i) {
        if(i==activeCamera||!cameraAvailable(i,expanded,largeStore))continue;
        auto c=cameras[i];glPushMatrix();glTranslatef(c.x,c.y,c.z);glRotatef(-c.yaw*180/3.14159265f,0,1,0);
        box({0,.14f,.13f,.1f,.29f,.1f,.33f,.38f,.37f});
        glRotatef(-c.pitch,1,0,0);
        box({0,0,0,.29f,.22f,.43f,.73f,.77f,.71f});
        box({0,.12f,-.03f,.34f,.035f,.53f,.54f,.61f,.58f});
        rod(0,0,-.22f,0,0,-.26f,.08f,.045f,.065f,.073f);
        rod(0,0,-.267f,0,0,-.27f,.045f,.14f,.3f,.34f);
        box({.105f,.055f,-.225f,.027f,.027f,.015f,.95f,.06f,.025f});glPopMatrix();
    }
}
void renderManager(const Player& p) {
    glPushMatrix();glTranslatef(p.x,0,p.z);glScalef(1/shopScaleX,1,1/shopScaleZ);glRotatef(180-p.yaw*180/3.14159265f,0,1,0);
    character(4,p.walkCycle,0,0,0,true);glPopMatrix();
}
