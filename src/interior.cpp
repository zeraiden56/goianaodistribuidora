#include "interior.hpp"
#include "render.hpp"
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>

namespace {
void television(const Game& g) {
    box({-4.8f,1.85f,6.2f,.18f,1.8f,3.06f,.04f,.05f,.06f});
    glPushMatrix();glTranslatef(-4.695f,2.66f,7.62f);glRotatef(90,0,1,0);glScalef(.0045f,-.0045f,.0045f);
    glDepthFunc(GL_LEQUAL);
    rect(0,0,630,360,.025f,.045f,.045f);glTranslatef(0,0,.2f);
    if(g.tvOn) {
        rect(0,0,630,360,.08f,.32f,.19f);
        for(int i=0;i<7;++i)rect(i*90,32,45,280,.095f,.36f,.21f);
        rect(30,30,570,3,.75f,.81f,.63f);rect(30,309,570,3,.75f,.81f,.63f);
        rect(30,30,3,282,.75f,.81f,.63f);rect(597,30,3,282,.75f,.81f,.63f);rect(314,30,3,282,.75f,.81f,.63f);
        float ballX=315+std::sin(g.time*.7f)*230,ballY=165+std::sin(g.time*1.2f)*85;
        for(int i=0;i<6;++i){float x=75+i*82+std::sin(g.time+i)*16,y=105+(i%2)*115;rect(x,y,14,24,i%2?.8f:.15f,.25f,i%2?.15f:.8f);}
        rect(ballX,ballY,9,9,.94f,.92f,.78f);rect(8,5,614,21,.035f,.08f,.055f);
        text(18,11,"TV DISTRIBUIDORA - FUTEBOL DA FIRMA",1.7f);
        text(145,333,"AZUIS 1 X 1 VERMELHOS",1.7f);
    } else text(130,160,"TV DESLIGADA",3,.35f,.42f,.4f);
    glDepthFunc(GL_LESS);glPopMatrix();
    box({-4.69f,1.f,4.85f,.025f,.025f,.025f,g.tvOn?.1f:.65f,g.tvOn?.8f:.05f,.04f});
}
}
void renderBackRoom(const Game& g) {
    if(!g.expanded) {
        box({0,1.8f,4,10,3.6f,.2f,.69f,.65f,.44f});
        sign(4.1f,2.8f,3.88f,"AMPLIACAO",1.9f,true);
        sign(4.1f,2.45f,3.88f,"B NO COMPUTADOR",1.9f,true);return;
    }
    // Partition with a wide passage beside the original shelves.
    box({-1.7f,1.8f,4,6.6f,3.6f,.2f,.69f,.65f,.44f});
    box({4.6f,1.8f,4,.8f,3.6f,.2f,.69f,.65f,.44f});
    box({2.9f,3.15f,4,2.6f,.9f,.2f,.69f,.65f,.44f});
    sign(3.95f,3.4f,3.87f,"DEPOSITO E DESCANSO",2.2f,true);
    for(int x=-5;x<5;++x)for(int z=4;z<10;++z)box({x+.5f,0,z+.5f,.98f,.04f,.98f,.42f,.4f,.34f});
    for(float x:{-5.f,5.f})box({x,1.8f,7,.2f,3.6f,6,.46f,.53f,.46f});
    box({0,1.8f,10,10,3.6f,.2f,.49f,.56f,.46f});box({0,3.7f,7,10,.18f,6,.3f,.32f,.27f});
    box({0,3.57f,6.3f,2,.06f,.3f,.93f,.89f,.69f});
    for(int p=0;p<4;++p) {
        float x=-3.6f+p*2.1f;
        sign(x+.8f,2.8f,8.61f,g.products[p].name,1.6f,true);
        sign(x+.5f,2.44f,8.6f,g.stockLabel(p),1.f,true);
        for(float y:{.2f,1.2f})box({x,y,9.15f,1.85f,.09f,.85f,.2f,.26f,.24f});
        for(float dx:{-.9f,.9f})box({x+dx,1.15f,9.15f,.08f,2.3f,.85f,.2f,.26f,.24f});
        for(int i=0;i<std::min(8,(g.products[p].stock+5)/6);++i) {
            float xx=x-.63f+(i%4)*.42f,yy=.53f+(i/4);
            box({xx,yy,9.15f,.37f,.55f,.6f,p==3?.38f:.48f,p==3?.58f:.35f,p==3?.63f:.2f});
            box({xx,yy,8.84f,.23f,.13f,.02f,.85f,.75f,.5f});
        }
    }
    // Sofa faces the television on the opposite wall.
    box({3.88f,.46f,6.2f,1.35f,.5f,2.3f,.19f,.31f,.33f});
    box({4.44f,.95f,6.2f,.27f,.85f,2.45f,.16f,.27f,.3f});
    for(float z:{5.02f,7.38f})box({3.87f,.75f,z,1.45f,.4f,.22f,.17f,.28f,.31f});
    for(float z:{5.65f,6.75f})box({3.74f,.77f,z,1.05f,.18f,1.05f,.25f,.39f,.4f});
    sign(3.21f,1.5f,7.12f,"SOFA - E SENTAR",1.85f,true);
    television(g);
}
void renderHelper(const Game& g) {
    for(int lane=0;lane<2;++lane) {
    const auto& h=lane?g.secondHelper:g.helper;
    if(!h.hired)continue;
    glPushMatrix();glTranslatef(h.x,0,h.z);
    float swing=h.route.empty()?0:std::sin(g.time*9)*.1f;
    box({0,1.15f,0,.58f,.75f,.33f,lane?.5f:.2f,lane?.5f:.35f,lane?.2f:.65f});
    box({0,1.7f,0,.36f,.4f,.35f,.59f,.39f,.26f});box({0,1.94f,0,.42f,.1f,.4f,.16f,.25f,.48f});
    box({0,1.25f,-.18f,.36f,.38f,.025f,.75f,.68f,.35f});
    for(float side:{-1.f,1.f}) {
        box({side*.18f,.44f,side*swing,.21f,.85f,.25f,.13f,.17f,.24f});
        box({side*.38f,1.12f,0,.16f,.67f,.18f,.59f,.39f,.26f});
    }
    if(h.held>=0) {
        if(g.bagCapacity>1)box({.36f,.98f,-.32f,.5f,.5f,.38f,.7f,.62f,.38f});
        for(int i=0;i<h.count;++i)item(h.held,.2f+(i%3)*.13f,1.2f+(i/3)*.14f,-.27f,.45f);
    }
    glPopMatrix();
    sign(h.x-.48f,2.25f,h.z+.05f,"ATENDENTE "+std::to_string(lane+1)+" - "+std::to_string(h.count)+"/"+std::to_string(g.bagCapacity),1.5f);
    sign(h.x+.48f,2.25f,h.z-.05f,"ATENDENTE "+std::to_string(lane+1)+" - "+std::to_string(h.count)+"/"+std::to_string(g.bagCapacity),1.5f,true);
    }
}
