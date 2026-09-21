#include "controller_render.hpp"
#include "render.hpp"
#include <SDL_opengl.h>
#include <cmath>

namespace {
bool enabled=false;
ControllerIcons style=ControllerIcons::Xbox;
void circle(float x,float y,float radius,bool fill) {
    glBegin(fill?GL_TRIANGLE_FAN:GL_LINE_LOOP);
    if(fill)glVertex2f(x,y);
    for(int i=0;i<=(fill?24:23);++i){const float angle=i*6.2831853f/24;glVertex2f(x+std::cos(angle)*radius,y+std::sin(angle)*radius);}
    glEnd();
}
}

void setControllerPrompts(bool active,ControllerIcons icons){enabled=active;style=icons;}
bool controllerPrompts(){return enabled;}

void padIcon(float x,float y,PadIcon button,float size) {
    glPushAttrib(GL_CURRENT_BIT|GL_LINE_BIT|GL_ENABLE_BIT);
    glDisable(GL_TEXTURE_2D);glLineWidth(1.5f);
    const bool face=button<=PadIcon::North;
    const float cx=x+size/2,cy=y+size/2;
    glColor3f(.10f,.14f,.16f);circle(cx,cy,size*.52f,true);
    glColor3f(.68f,.75f,.76f);circle(cx,cy,size*.48f,false);
    if(face&&style==ControllerIcons::PlayStation) {
        if(button==PadIcon::South)glColor3f(.50f,.73f,1.f);
        if(button==PadIcon::East)glColor3f(1.f,.48f,.51f);
        if(button==PadIcon::West)glColor3f(.91f,.55f,.85f);
        if(button==PadIcon::North)glColor3f(.48f,.94f,.70f);
        const float r=size*.25f;
        if(button==PadIcon::East)circle(cx,cy,r,false);
        else {
            glBegin(button==PadIcon::South?GL_LINES:GL_LINE_LOOP);
            if(button==PadIcon::South){glVertex2f(cx-r,cy-r);glVertex2f(cx+r,cy+r);glVertex2f(cx-r,cy+r);glVertex2f(cx+r,cy-r);}
            if(button==PadIcon::West){glVertex2f(cx-r,cy-r);glVertex2f(cx+r,cy-r);glVertex2f(cx+r,cy+r);glVertex2f(cx-r,cy+r);}
            if(button==PadIcon::North){glVertex2f(cx,cy-r);glVertex2f(cx+r,cy+r);glVertex2f(cx-r,cy+r);}
            glEnd();
        }
    } else {
        const std::string label=controllerIconLabel(button,style);
        const float font=label.size()>2?size*.12f:label.size()>1?size*.20f:size*.32f;
        float r=.92f,g=.95f,b=.94f;
        if(face&&style==ControllerIcons::Xbox) {
            if(button==PadIcon::South){r=.45f;g=.95f;b=.5f;}
            if(button==PadIcon::East){r=1;g=.45f;b=.4f;}
            if(button==PadIcon::West){r=.45f;g=.75f;b=1;}
            if(button==PadIcon::North){r=1;g=.86f;b=.35f;}
        }
        text(cx-label.size()*3*font+font*.5f,cy-3.5f*font,label,font,r,g,b);
    }
    glPopAttrib();
}

void padHint(float x,float y,PadIcon button,const std::string& label,float scale) {
    padIcon(x,y-4*scale,button,16*scale);
    text(x+22*scale,y,label,scale,.93f,.86f,.68f);
}

void renderControllerPreview(const std::string& name) {
    rect(490,166,450,210,.035f,.065f,.065f);
    text(506,181,name.substr(0,50),1.f,.91f,.77f,.5f);
    padHint(508,211,PadIcon::South,"CONFIRMAR / INTERAGIR");
    padHint(508,238,PadIcon::East,"VOLTAR / DEVOLVER");
    padHint(508,265,PadIcon::West,"CONSUMIR");
    padHint(724,265,PadIcon::North,"PEGAR CAIXA");
    padHint(508,292,PadIcon::LeftStick,"ANDAR / CORRER");
    padHint(724,292,PadIcon::RightStick,"OLHAR");
    padHint(508,319,PadIcon::Start,"PAUSAR");
    padHint(724,319,PadIcon::Back,"CONTROLES");
    text(506,351,"ICONES NAO ALTERAM A POSICAO DOS BOTOES",1.f);
}

void renderControllerKeyboard(int selected) {
    rect(490,166,450,214,.035f,.065f,.065f);
    text(506,179,"NOME DA DISTRIBUIDORA",1.1f,.91f,.77f,.5f);
    const std::string letters="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int i=0;i<40;++i) {
        const float x=504+(i%10)*42, y=204+(i/10)*32;
        rect(x,y,38,27,i==selected?.25f:.09f,i==selected?.38f:.15f,.16f);
        const std::string label=i<36?letters.substr(i,1):i==36?"ESP":i==37?"DEL":i==38?"LIM":"OK";
        text(x+8,y+9,label,1.f);
    }
    padHint(506,351,PadIcon::South,"DIGITAR");
    padHint(690,351,PadIcon::Start,"CONFIRMAR NOME");
}
