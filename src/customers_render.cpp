#include "render.hpp"
#include "customers.hpp"
#include <SDL_opengl.h>
#include <cmath>

void renderCustomer(const Game& g) {
    for(int lane=0;lane<(g.secondCheckout?2:1);++lane) {
    if(!(lane?g.second.customer:g.customer))continue;
    const auto& c=customerLooks()[lane?g.second.customerStyle:g.customerStyle];
    glPushMatrix();glTranslatef(lane?2.1f:-2.1f,0,-3.8f);glScalef(c.width,c.height,1);
    auto skin=c.skin,shirt=c.shirt,pants=c.trousers,hair=c.hair;
    box({0,1.2f,0,.62f,.76f,.35f,shirt[0],shirt[1],shirt[2]});
    for(float side:{-1.f,1.f}) {
        box({side*.19f,.44f,0,.24f,.84f,.29f,pants[0],pants[1],pants[2]});
        box({side*.19f,.08f,.075f,.25f,.12f,.41f,.065f,.065f,.07f});
        float swing=std::sin(g.time*1.8f+side)*.025f;
        box({side*.43f,1.22f+swing,0,.18f,.55f,.22f,shirt[0],shirt[1],shirt[2]});
        box({side*.43f,.9f+swing,0,.16f,.26f,.2f,skin[0],skin[1],skin[2]});
    }
    box({0,1.66f,0,.19f,.2f,.21f,skin[0],skin[1],skin[2]});
    box({0,1.88f,0,.4f,.43f,.38f,skin[0],skin[1],skin[2]});
    box({0,2.095f,-.03f,.43f,.12f,.41f,hair[0],hair[1],hair[2]});
    box({0,1.84f,.21f,.075f,.09f,.075f,skin[0]*.9f,skin[1]*.9f,skin[2]*.9f});
    for(float x:{-.1f,.1f})box({x,1.94f,.197f,.046f,.04f,.02f,.045f,.035f,.03f});
    box({0,1.74f,.199f,.13f,.025f,.02f,.28f,.15f,.12f});
    if(c.style==0||c.style==5||c.style==7) {
        box({0,1.79f,-.19f,.44f,.67f,.15f,hair[0],hair[1],hair[2]});
        for(float x:{-.23f,.23f})box({x,1.87f,-.06f,.1f,.44f,.26f,hair[0],hair[1],hair[2]});
    }
    if(c.style==1||c.style==3) {
        box({0,2.17f,0,.45f,.15f,.44f,.14f,.2f,.29f});box({0,2.115f,.23f,.46f,.045f,.2f,.14f,.2f,.29f});
    }
    if(c.style==2||c.style==6) {
        for(float x:{-.105f,.105f})box({x,1.94f,.22f,.18f,.1f,.025f,.1f,.13f,.16f});
        box({0,1.945f,.23f,.06f,.024f,.025f,.2f,.2f,.2f});
    }
    if(c.style==4||c.style==6)box({0,1.725f,.15f,.35f,.14f,.15f,hair[0],hair[1],hair[2]});
    if(c.style==5)box({0,2.25f,-.14f,.27f,.23f,.28f,hair[0],hair[1],hair[2]});
    if(c.style==7)box({.25f,1.75f,-.17f,.12f,.5f,.17f,hair[0],hair[1],hair[2]});
    glPopMatrix();
    }
}
