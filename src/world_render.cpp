#include "world_render.hpp"
#include "world.hpp"
#include "render.hpp"
#include <SDL_opengl.h>
#include <cmath>

namespace {
void lightPool(float x,float z,float radius) {
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glBegin(GL_TRIANGLE_FAN);glColor4f(.65f,.46f,.2f,.22f);glVertex3f(x,.072f,z);
    glColor4f(.65f,.46f,.2f,0);
    for(int i=0;i<=16;++i){float a=i*6.2831853f/16;glVertex3f(x+std::cos(a)*radius,.072f,z+std::sin(a)*radius);}
    glEnd();glDisable(GL_BLEND);
}
void wheel(float x,float y,float z,float radius,float width) {
    for(float side:{-1.f,1.f}) {
        glColor3f(.035f,.04f,.05f);glBegin(GL_TRIANGLE_FAN);glVertex3f(x,y,z+side*width/2);
        for(int i=0;i<=10;++i){float a=i*6.2831853f/10;glVertex3f(x+std::cos(a)*radius,y+std::sin(a)*radius,z+side*width/2);}
        glEnd();box({x,y,z+side*width/2,.12f,.12f,.02f,.24f,.27f,.3f});
    }
    glColor3f(.025f,.03f,.04f);glBegin(GL_QUAD_STRIP);
    for(int i=0;i<=10;++i){float a=i*6.2831853f/10;for(float side:{-1.f,1.f})glVertex3f(x+std::cos(a)*radius,y+std::sin(a)*radius,z+side*width/2);}
    glEnd();
}
void vehicle(const TrafficVehicle& v) {
    glPushMatrix();glTranslatef(v.x,0,v.z);if(v.speed<0)glRotatef(180,0,1,0);
    if(v.motorcycle) {
        wheel(-.7f,.35f,0,.34f,.2f);wheel(.7f,.35f,0,.34f,.2f);
        box({0,.63f,0,1.3f,.22f,.36f,.35f,.09f,.08f});box({-.2f,.86f,0,.7f,.14f,.4f,.035f,.04f,.05f});
        box({.53f,.88f,0,.12f,.8f,.13f,.2f,.23f,.27f});box({.5f,1.2f,0,.12f,.09f,.65f,.25f,.28f,.3f});
        box({-.2f,1.23f,0,.38f,.6f,.45f,.12f,.19f,.27f});box({-.05f,1.73f,0,.38f,.4f,.38f,.65f,.65f,.6f});
        box({.15f,1.74f,0,.03f,.12f,.35f,.035f,.055f,.07f});
        for(float side:{-.28f,.28f})box({-.24f,.68f,side,.22f,.66f,.17f,.1f,.12f,.17f});
        box({.78f,1.02f,0,.12f,.19f,.24f,.95f,.91f,.62f});box({-.91f,.76f,0,.08f,.13f,.22f,.85f,.05f,.025f});
    } else {
        float colors[3][3]={{.14f,.23f,.35f},{.38f,.1f,.08f},{.4f,.42f,.39f}};auto c=colors[v.color];
        box({0,.65f,0,3.5f,.58f,1.5f,c[0],c[1],c[2]});box({-.15f,1.15f,0,1.9f,.6f,1.33f,c[0],c[1],c[2]});
        for(float z:{-.676f,.676f})box({-.15f,1.18f,z,1.65f,.4f,.025f,.055f,.13f,.17f});
        box({.82f,1.18f,0,.025f,.4f,1.19f,.07f,.15f,.18f});
        for(float x:{-1.07f,1.05f})for(float z:{-.76f,.76f})wheel(x,.36f,z,.35f,.19f);
        for(float z:{-.52f,.52f}) {
            box({1.77f,.69f,z,.035f,.19f,.33f,.98f,.95f,.69f});
            box({-1.77f,.7f,z,.035f,.2f,.31f,.85f,.035f,.02f});
        }
    }
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glBegin(GL_TRIANGLES);
    glColor4f(.72f,.72f,.48f,.22f);glVertex3f(v.motorcycle?.9f:1.8f,.079f,0);
    glColor4f(.72f,.72f,.48f,0);glVertex3f(7,.079f,-1.7f);glVertex3f(7,.079f,1.7f);
    glEnd();glDisable(GL_BLEND);glPopMatrix();
}
}
void renderStreet(double seconds) {
    box({0,-.1f,-14,90,.2f,50,.06f,.08f,.105f});
    box({0,.03f,-9,90,.05f,6,.035f,.045f,.065f});
    box({0,.07f,-5.4f,80,.08f,1.1f,.14f,.15f,.18f});
    box({0,.07f,-12.2f,80,.08f,.6f,.13f,.14f,.17f});
    for(int i=-8;i<=8;++i) {
        box({i*4.f,.066f,-9,1.8f,.015f,.12f,.44f,.39f,.19f});
        float x=i*5.f;
        box({x,2.05f,-15,4.7f,4.1f,5,.12f+i*.003f,.14f,.19f});
        box({x,1.15f,-12.46f,1.3f,2.3f,.05f,.045f,.07f,.09f});
        for(float dx:{-1.5f,1.5f})box({x+dx,2.65f,-12.46f,.65f,.9f,.06f,.55f,.39f,.19f});
    }
    for(float x:{-18.f,-6.f,6.f,18.f}) {
        box({x,2.6f,-5.6f,.15f,5.2f,.15f,.1f,.12f,.16f});
        box({x,5.15f,-6.1f,.15f,.12f,1.1f,.18f,.2f,.24f});
        box({x,5.06f,-6.65f,.6f,.13f,.38f,.98f,.76f,.39f});lightPool(x,-7,4.5f);
    }
    box({6,2.2f,-12.25f,.14f,4.4f,.14f,.12f,.14f,.16f});
    box({-12,14,-35,1.1f,1.1f,.1f,.78f,.83f,.84f});
    for(int i=0;i<28;++i)box({float((i*17)%80-40),float(8+(i*7)%15),-39,.05f,.05f,.05f,.45f,.52f,.62f});
    // Exterior fascia faces the street; the inside remains lit.
    sign(3.8f,3.55f,-4.13f,"GOIANÃO DISTRIBUIDORA",7.6f,true);
    for(auto v:trafficAt(seconds))vehicle(v);
}
void renderSecurityDevices(int activeCamera) {
    const auto& cameras=securityCameras();
    for(int i=0;i<int(cameras.size());++i) {
        if(i==activeCamera)continue;
        auto c=cameras[i];glPushMatrix();glTranslatef(c.x,c.y,c.z);glRotatef(-c.yaw*180/3.14159265f,0,1,0);
        box({0,0,0,.24f,.2f,.36f,.65f,.69f,.66f});box({0,0,-.2f,.15f,.13f,.03f,.035f,.06f,.07f});
        box({.075f,.045f,-.22f,.025f,.025f,.025f,.75f,.03f,.015f});glPopMatrix();
    }
}
void renderManager(const Player& p) {
    glPushMatrix();glTranslatef(p.x,0,p.z);glRotatef(-p.yaw*180/3.14159265f,0,1,0);
    box({0,1.15f,0,.6f,.75f,.33f,.15f,.38f,.29f});box({0,1.73f,0,.36f,.4f,.34f,.63f,.42f,.28f});
    for(float x:{-.18f,.18f})box({x,.43f,0,.2f,.85f,.25f,.1f,.13f,.19f});
    for(float x:{-.39f,.39f})box({x,1.13f,0,.15f,.7f,.18f,.63f,.42f,.28f});
    glPopMatrix();
}
