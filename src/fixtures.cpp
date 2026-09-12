#include "fixtures.hpp"
#include "render.hpp"
#include "shop_layout.hpp"
#include <SDL_opengl.h>
#include <algorithm>

namespace {
float doorAngle(float remaining) {
    if(remaining>1.3f)return (1.6f-remaining)/.3f*68;
    if(remaining>.4f)return 68;
    return remaining/.4f*68;
}
void doorTransform(const Game& g,int product) {
    glTranslatef(stockLocations[product].x-.96f,0,2.78f);
    glRotatef(doorAngle(g.fridgeTime[product==0?0:1]),0,1,0);
}
}
void renderRefrigerators(const Game& g) {
    for(int p:{0,2}) {
        float x=stockLocations[p].x;
        box({x,1.32f,3.8f,2.08f,2.64f,.12f,.19f,.25f,.28f});
        for(float side:{-.99f,.99f})box({x+side,1.32f,3.3f,.1f,2.64f,1,.67f,.73f,.73f});
        for(float y:{.09f,2.62f})box({x,y,3.3f,2.08f,.15f,1,.61f,.69f,.69f});
        for(float y:{.25f,.91f,1.57f,2.23f})box({x,y,3.3f,1.85f,.045f,.87f,.67f,.78f,.76f});
        box({x,2.47f,3.13f,1.72f,.04f,.17f,.87f,.99f,.89f});
        sign(x+.89f,2.98f,2.75f,p==0?"CERVEJAS GELADAS":"DESTILADOS",1.8f,true);
        sign(x+.42f,2.73f,2.74f,g.stockLabel(p),.85f,true);
        for(int i=0;i<std::min(24,g.products[p].stock);++i)item(p,x-.76f+(i%6)*.3f,.47f+(i/6)*.66f,3.17f,.74f);
        glPushMatrix();doorTransform(g,p);
        for(float xx:{0.f,1.92f})box({xx,1.29f,0,.055f,2.5f,.07f,.31f,.39f,.42f});
        for(float yy:{.08f,2.51f})box({.96f,yy,0,1.92f,.055f,.07f,.31f,.39f,.42f});
        box({1.76f,1.28f,-.08f,.055f,.63f,.075f,.8f,.84f,.81f});glPopMatrix();
    }
}
void renderFridgeGlass(const Game& g) {
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    for(int p:{0,2}) {
        glPushMatrix();doorTransform(g,p);glColor4f(.43f,.73f,.8f,.15f);glBegin(GL_QUADS);
        glVertex3f(.035f,.11f,0);glVertex3f(1.885f,.11f,0);glVertex3f(1.885f,2.48f,0);glVertex3f(.035f,2.48f,0);glEnd();
        glColor4f(.85f,.96f,.98f,.19f);glBegin(GL_QUADS);
        glVertex3f(.16f,.35f,-.006f);glVertex3f(.25f,.35f,-.006f);glVertex3f(.76f,2.3f,-.006f);glVertex3f(.67f,2.3f,-.006f);glEnd();
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);
}
void renderCigaretteCounter(const Game& g) {
    // An open cubby on the manager's side, underneath the continuous counter top.
    box({-2.7f,.55f,-2.6f,4.4f,1.1f,.9f,.17f,.32f,.29f});
    box({3.3f,.55f,-2.6f,3.2f,1.1f,.9f,.17f,.32f,.29f});
    box({.65f,.53f,-2.98f,2.1f,1.05f,.08f,.1f,.2f,.18f});
    for(float y:{.08f,.55f})box({.65f,y,-2.6f,2.1f,.07f,.88f,.4f,.36f,.27f});
    sign(-.22f,1.03f,-2.11f,"CIGARROS",1.72f);
    sign(.26f,.8f,-2.1f,g.stockLabel(1),.79f);
    for(int i=0;i<std::min(16,g.products[1].stock);++i)item(1,-.19f+(i%8)*.24f,.29f+(i/8)*.43f,-2.34f,.72f);
}
