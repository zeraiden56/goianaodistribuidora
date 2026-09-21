#include "world_render.hpp"
#include "world.hpp"
#include "render.hpp"
#include "shop_layout.hpp"
#include <SDL_opengl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

namespace {
using Color=std::array<float,3>;
struct Point {float x,y;};
void profile(std::initializer_list<Point> points,float width,Color color) {
    for(float side:{-1.f,1.f}) {
        glColor3f(color[0]*.83f,color[1]*.83f,color[2]*.83f);glBegin(GL_POLYGON);
        for(auto p:points)glVertex3f(p.x,p.y,side*width/2);
        glEnd();
    }
    auto begin=points.begin();
    for(auto it=begin;it!=points.end();++it) {
        auto next=it+1==points.end()?begin:it+1;
        float shade=next->x>it->x?1.08f:.92f;
        glColor3f(color[0]*shade,color[1]*shade,color[2]*shade);glBegin(GL_QUADS);
        glVertex3f(it->x,it->y,-width/2);glVertex3f(next->x,next->y,-width/2);
        glVertex3f(next->x,next->y,width/2);glVertex3f(it->x,it->y,width/2);glEnd();
    }
}
void wheel(float x,float y,float z,float radius,float width,float spin,bool wire=false) {
    glPushMatrix();glTranslatef(x,y,z);glRotatef(spin,0,0,1);
    constexpr int segments=20;
    glColor3f(.035f,.04f,.045f);glBegin(GL_QUAD_STRIP);
    for(int i=0;i<=segments;++i) {
        float a=i*6.2831853f/segments;
        glColor3f(i%2?.045f:.025f,i%2?.048f:.028f,i%2?.052f:.032f);
        for(float side:{-1.f,1.f})glVertex3f(std::cos(a)*radius,std::sin(a)*radius,side*width/2);
    }
    glEnd();
    for(float side:{-1.f,1.f}) {
        float zFace=side*(width/2+.003f);
        glColor3f(.075f,.08f,.087f);glBegin(GL_QUAD_STRIP);
        for(int i=0;i<=segments;++i){float a=i*6.2831853f/segments;for(float r:{radius,radius*.65f})glVertex3f(std::cos(a)*r,std::sin(a)*r,zFace);}glEnd();
        glColor3f(.23f,.27f,.29f);glBegin(GL_TRIANGLE_FAN);glVertex3f(0,0,zFace);
        for(int i=0;i<=segments;++i){float a=i*6.2831853f/segments;glVertex3f(std::cos(a)*radius*.64f,std::sin(a)*radius*.64f,zFace);}glEnd();
        for(int i=0;i<(wire?10:5);++i) {
            float angle=i*6.2831853f/(wire?10:5);
            rod(0,0,zFace+side*.012f,std::cos(angle)*radius*.6f,std::sin(angle)*radius*.6f,zFace+side*.012f,wire?.012f:.029f,.71f,.75f,.76f);
        }
        box({0,0,zFace+side*.018f,.085f,.085f,.028f,.74f,.76f,.76f});
    }
    glPopMatrix();
}
void windshield(float lowerX,float upperX,float lowY,float highY,float width) {
    glColor3f(.19f,.34f,.4f);glBegin(GL_QUADS);
    glVertex3f(lowerX,lowY,-width/2);glVertex3f(upperX,highY,-width*.43f);
    glVertex3f(upperX,highY,width*.43f);glVertex3f(lowerX,lowY,width/2);glEnd();
    for(float side:{-1.f,1.f})rod(lowerX,lowY,side*width/2,upperX,highY,side*width*.43f,.035f,.19f,.23f,.24f);
    rod(lowerX,lowY,0,lowerX+(upperX-lowerX)*.35f,lowY+(highY-lowY)*.35f,width*.2f,.012f,.07f,.08f,.08f);
}
void car(int style,float spin,float night) {
    static const Color colors[]={{.17f,.38f,.57f},{.64f,.2f,.14f},{.65f,.68f,.57f}};
    auto paint=colors[style];float rear=style==2?-1.72f:-2.05f,front=style==2?1.7f:2.05f;
    bool pickup=style==1;
    profile({{rear,.4f},{front,.4f},{front,.75f},{front-.22f,.91f},{.9f,1.01f},{rear+.2f,1.01f},{rear,.86f}},1.62f,paint);
    float back=pickup?-.27f:style==2?-1.12f:-.72f;
    profile({{pickup?-.38f:-1.34f,.99f},{.98f,.99f},{.54f,1.65f},{back,1.65f}},1.39f,paint);
    windshield(.965f,.535f,1.035f,1.605f,1.3f);
    windshield(pickup?-.395f:-1.355f,back-.012f,1.035f,1.605f,1.29f);
    box({(back+.54f)/2,1.675f,0,.54f-back,.045f,1.33f,paint[0]*1.12f,paint[1]*1.12f,paint[2]*1.12f});
    for(float side:{-1.f,1.f}) {
        glColor3f(.13f,.25f,.29f);glBegin(GL_QUADS);
        glVertex3f(.87f,1.06f,side*.707f);glVertex3f(.48f,1.59f,side*.707f);
        glVertex3f(pickup?-.2f:-.38f,1.59f,side*.707f);glVertex3f(pickup?-.25f:-.38f,1.06f,side*.707f);glEnd();
        if(!pickup) {
            glColor3f(.16f,.29f,.34f);glBegin(GL_QUADS);
            glVertex3f(-.47f,1.06f,side*.708f);glVertex3f(-.47f,1.59f,side*.708f);
            glVertex3f(back+.075f,1.59f,side*.708f);glVertex3f(-1.23f,1.06f,side*.708f);glEnd();
            box({-.425f,1.32f,side*.715f,.065f,.58f,.035f,.11f,.13f,.14f});
        }
        box({.1f,1.045f,side*.824f,.92f,.025f,.035f,.62f,.65f,.64f});
        box({-.19f,.94f,side*.829f,.17f,.045f,.029f,.79f,.81f,.79f});
        box({-.38f,.73f,side*.818f,.022f,.47f,.018f,.09f,.13f,.15f});
        rod(.73f,1.1f,side*.76f,.65f,1.12f,side*.99f,.025f,.12f,.15f,.17f);
        box({.65f,1.15f,side*1.03f,.23f,.14f,.15f,paint[0],paint[1],paint[2]});
        box({.525f,1.15f,side*1.03f,.008f,.1f,.12f,.5f,.62f,.67f});
        for(float axle:{rear+.68f,front-.65f}) {
            wheel(axle,.43f,side*.8f,.37f,.23f,spin);
            // Narrow painted wheel arch, leaving the dark tire visible.
            glColor3f(paint[0]*.65f,paint[1]*.65f,paint[2]*.65f);glBegin(GL_QUAD_STRIP);
            for(int i=0;i<=10;++i){float a=i*3.14159265f/10;for(float r:{.395f,.435f})glVertex3f(axle+std::cos(a)*r,.43f+std::sin(a)*r,side*.827f);}glEnd();
        }
        box({front+.012f,.79f,side*.57f,.033f,.17f,.36f,.75f+night*.25f,.8f+night*.17f,.7f+night*.09f});
        box({rear-.012f,.77f,side*.61f,.03f,.22f,.26f,.58f+night*.36f,.055f,.035f});
        box({front-.24f,.86f,side*.815f,.18f,.065f,.018f,.91f,.49f,.08f});
    }
    for(float edge:{rear-.02f,front+.02f})box({edge,.52f,0,.12f,.14f,1.56f,.16f,.18f,.2f});
    box({front+.077f,.73f,0,.018f,.21f,.62f,.035f,.045f,.05f});
    for(float y:{.67f,.73f,.79f})box({front+.09f,y,0,.02f,.016f,.59f,.47f,.52f,.53f});
    box({front+.093f,.535f,0,.026f,.115f,.4f,.83f,.85f,.78f});
    box({rear-.09f,.535f,0,.026f,.115f,.4f,.83f,.85f,.78f});
    for(float edge:{rear-.106f,front+.109f})for(int i=0;i<5;++i)box({edge,.54f,-.14f+i*.065f,.007f,.045f,.03f,.12f,.15f,.16f});
    if(pickup) {
        box({-1.25f,1.016f,0,1.46f,.022f,1.25f,.055f,.063f,.067f});
        for(float side:{-.74f,.74f})box({-1.24f,1.095f,side,1.5f,.24f,.13f,paint[0],paint[1],paint[2]});
        box({rear+.08f,1.085f,0,.16f,.25f,1.55f,paint[0],paint[1],paint[2]});
        for(float z:{-.44f,0.f,.44f})box({-1.25f,1.035f,z,1.35f,.025f,.023f,.22f,.23f,.22f});
    } else if(style==2) {
        for(float side:{-.49f,.49f})rod(-.94f,1.75f,side*.9f,.36f,1.75f,side*.9f,.03f,.18f,.2f,.22f);
    }
}
void motorcycle(int style,float spin,float seconds,float night) {
    static const Color colors[]={{.19f,.35f,.61f},{.65f,.17f,.11f},{.76f,.56f,.13f}};
    auto c=colors[style];
    wheel(-.8f,.41f,0,.34f,.16f,spin,true);wheel(.83f,.41f,0,.34f,.14f,spin,true);
    for(float side:{-.14f,.14f}) {
        rod(-.8f,.41f,side,-.15f,.81f,side,.035f,.22f,.25f,.27f);
        rod(-.15f,.81f,side,.44f,.97f,side,.035f,.24f,.28f,.3f);
        rod(.44f,.97f,side,.83f,.41f,side,.045f,.69f,.74f,.75f);
        rod(-.58f,.55f,side,-.37f,.88f,side,.045f,.63f,.65f,.64f);
        for(int i=0;i<4;++i)box({-.41f-i*.036f,.8f-i*.055f,side,.095f,.017f,.095f,.15f,.18f,.19f});
    }
    box({-.08f,.63f,0,.48f,.4f,.38f,.31f,.35f,.37f});
    for(int i=0;i<5;++i)box({.03f,.57f+i*.052f,0,.31f,.018f,.44f,.56f,.59f,.58f});
    profile({{-.24f,.85f},{.41f,.86f},{.36f,1.14f},{-.05f,1.19f},{-.28f,1.04f}},.45f,c);
    box({-.47f,1.f,0,.64f,.14f,.41f,.07f,.075f,.083f});
    box({-.83f,.94f,0,.21f,.12f,.37f,c[0],c[1],c[2]});
    rod(-.55f,.54f,.24f,.36f,.49f,.24f,.068f,.43f,.47f,.49f);
    box({-.82f,.57f,.22f,.45f,.125f,.14f,.66f,.7f,.7f});
    rod(.47f,1.12f,0,.42f,1.29f,0,.036f,.42f,.46f,.49f);
    rod(.42f,1.29f,-.35f,.42f,1.29f,.35f,.026f,.54f,.58f,.6f);
    for(float side:{-1.f,1.f}) {
        box({.4f,1.29f,side*.35f,.15f,.065f,.13f,.035f,.04f,.045f});
        rod(.4f,1.3f,side*.28f,.39f,1.53f,side*.42f,.016f,.52f,.58f,.62f);
        box({.39f,1.54f,side*.43f,.06f,.095f,.17f,.27f,.39f,.45f});
    }
    box({.65f,1.12f,0,.19f,.22f,.31f,.14f,.17f,.18f});
    box({.752f,1.12f,0,.016f,.16f,.23f,.82f+night*.18f,.84f+night*.14f,.67f+night*.15f});
    box({-.94f,.94f,0,.07f,.11f,.26f,.65f+night*.3f,.035f,.025f});
    box({-.98f,.74f,0,.035f,.15f,.25f,.88f,.88f,.79f});
    // Rider with bent elbows/knees, boots, gloves and a helmet visor.
    glPushMatrix();glTranslatef(0,std::sin(seconds*8+style)*.008f,0);
    profile({{-.54f,1.05f},{-.19f,1.04f},{.11f,1.58f},{-.3f,1.67f}},.43f,{.12f,.17f,.24f});
    for(float side:{-1.f,1.f}) {
        rod(-.34f,1.08f,side*.22f,.17f,.84f,side*.3f,.105f,.15f,.2f,.29f);
        rod(.17f,.84f,side*.3f,-.08f,.56f,side*.27f,.085f,.12f,.16f,.23f);
        box({.02f,.53f,side*.28f,.32f,.13f,.2f,.065f,.065f,.07f});
        rod(-.1f,1.55f,side*.26f,.15f,1.26f,side*.3f,.075f,.15f,.21f,.3f);
        rod(.15f,1.26f,side*.3f,.39f,1.3f,side*.35f,.065f,.19f,.25f,.34f);
        box({.41f,1.3f,side*.35f,.14f,.1f,.12f,.055f,.055f,.06f});
    }
    cylinder(.02f,1.78f,0,.235f,.34f,c[0],c[1],c[2],.2f);
    box({.245f,1.8f,0,.036f,.15f,.33f,.09f,.18f,.23f});
    box({.262f,1.83f,-.06f,.012f,.025f,.17f,.43f,.57f,.62f});
    glPopMatrix();
    if(style==2) {
        box({-.77f,1.34f,0,.58f,.54f,.57f,.69f,.2f,.12f});
        box({-.77f,1.61f,0,.6f,.055f,.59f,.83f,.29f,.15f});
        for(float z:{-.294f,.294f})box({-.77f,1.35f,z,.31f,.18f,.018f,.92f,.85f,.63f});
    }
}
void headlightBeam(float front,float offset,float night) {
    if(night<=0)return;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
    glBegin(GL_TRIANGLES);glColor4f(1.f,.87f,.57f,night*.28f);
    glVertex3f(front,.09f,offset);glColor4f(.8f,.77f,.47f,0);
    glVertex3f(front+7,.09f,offset-1.1f);glVertex3f(front+7,.09f,offset+1.1f);glEnd();
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);
}
}
void renderTraffic(double seconds) {
    float night=streetLightIntensity(seconds);
    for(const auto& v:trafficAt(seconds)) {
        glPushMatrix();glTranslatef(v.x,0,v.z);glScalef(1/shopScaleX,1,1/shopScaleZ);
        if(v.speed<0)glRotatef(180,0,1,0);
        float spin=float(std::fmod(-seconds*std::abs(v.speed)*shopScaleX/(v.motorcycle?.34:.37)*180/3.14159265,360));
        if(v.motorcycle){motorcycle(v.color,spin,float(seconds),night);headlightBeam(.8f,0,night);}
        else {car(v.color,spin,night);for(float side:{-.57f,.57f})headlightBeam(v.color==2?1.75f:2.1f,side,night);}
        glPopMatrix();
    }
}
