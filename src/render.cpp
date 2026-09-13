#include "render.hpp"
#include "shop_layout.hpp"
#include "interior.hpp"
#include "fixtures.hpp"
#include "world.hpp"
#include "world_render.hpp"
#include "postprocess.hpp"
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>

// Original 5x7 bitmap alphabet keeps the prototype independent of font assets.
const char* glyph(char c) {
    static const char* letters[]={"01110100011000111111100011000110001","11110100011000111110100011000111110","01111100001000010000100001000001111","11110100011000110001100011000111110","11111100001000011110100001000011111","11111100001000011110100001000010000","01111100001000010111100011000101111","10001100011000111111100011000110001","11111001000010000100001000010011111","00111000100001000010100101001001100","10001100101010011000101001001010001","10000100001000010000100001000011111","10001110111010110101100011000110001","10001110011010110011100011000110001","01110100011000110001100011000101110","11110100011000111110100001000010000","01110100011000110001101011001001101","11110100011000111110101001001010001","01111100001000001110000010000111110","11111001000010000100001000010000100","10001100011000110001100011000101110","10001100011000110001100010101000100","10001100011000110101101011101110001","10001100010101000100010101000110001","10001100010101000100001000010000100","11111000010001000100010001000011111"};
    static const char* digits[]={"01110100011001110101110011000101110","00100011000010000100001000010001110","01110100010000100010001000100011111","11110000010000101110000010000111110","00010001100101010010111110001000010","11111100001000011110000010000111110","01110100001000011110100011000101110","11111000010001000100010000100001000","01110100011000101110100011000101110","01110100011000101111000010000101110"};
    if(c>='A'&&c<='Z') return letters[c-'A'];
    if(c>='0'&&c<='9') return digits[c-'0'];
    if(c=='%') return "11001110100001000100010001001110011";
    if(c=='/') return "00001000100001000100010000100010000";
    if(c=='$') return "00100011111010001110001011111000100";
    if(c==',') return "00000000000000000000000000010001000";
    if(c==':') return "00000001000010000000001000010000000";
    if(c=='-') return "00000000000000011111000000000000000";
    if(c=='.') return "00000000000000000000000000010000100";
    if(c=='!') return "00100001000010000100001000000000100";
    if(c=='+') return "00000001000010011111001000010000000";
    return "00000000000000000000000000000000000";
}
void rect(float x,float y,float w,float h,float r,float g,float b) {
    glColor3f(r,g,b); glBegin(GL_QUADS); glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
void text(float x,float y,const std::string& s,float scale,float r,float g,float b) {
    for(std::size_t at=0;at<s.size();++at) {
        char c=s[at];bool tilde=false;
        if(static_cast<unsigned char>(c)==0xC3&&at+1<s.size()
            &&(static_cast<unsigned char>(s[at+1])==0x83||static_cast<unsigned char>(s[at+1])==0xA3)) {c='A';++at;tilde=true;}
        if(tilde){rect(x+scale,y-2*scale,2*scale,scale,r,g,b);rect(x+3*scale,y-scale,scale,scale,r,g,b);}
        auto bits=glyph(c);for(int j=0;j<7;++j) for(int i=0;i<5;++i) if(bits[j*5+i]=='1') rect(x+i*scale,y+j*scale,scale,scale,r,g,b);x+=6*scale;}
}
void box(Box a) {
    float x=a.x-a.w/2, X=a.x+a.w/2, y=a.y-a.h/2,Y=a.y+a.h/2,z=a.z-a.d/2,Z=a.z+a.d/2;
    glBegin(GL_QUADS);
    glColor3f(a.r,a.g,a.b);glVertex3f(x,y,Z);glVertex3f(X,y,Z);glVertex3f(X,Y,Z);glVertex3f(x,Y,Z);
    glColor3f(a.r*.7f,a.g*.7f,a.b*.7f);glVertex3f(X,y,z);glVertex3f(x,y,z);glVertex3f(x,Y,z);glVertex3f(X,Y,z);
    glColor3f(a.r*.8f,a.g*.8f,a.b*.8f);glVertex3f(x,y,z);glVertex3f(x,y,Z);glVertex3f(x,Y,Z);glVertex3f(x,Y,z);
    glVertex3f(X,y,Z);glVertex3f(X,y,z);glVertex3f(X,Y,z);glVertex3f(X,Y,Z);
    glColor3f(a.r*1.1f,a.g*1.1f,a.b*1.1f);glVertex3f(x,Y,Z);glVertex3f(X,Y,Z);glVertex3f(X,Y,z);glVertex3f(x,Y,z);
    glEnd();
}
// Text is attached to the geometry, with the same depth test as the scene.
void sign(float x,float y,float z,const std::string& title,float width,bool back) {
    glPushMatrix();glTranslatef(x,y,z);if(back)glRotatef(180,0,1,0);
    float unit=width/(title.size()*6+4);
    glScalef(unit,-unit,unit);
    rect(-2,-2,title.size()*6+4,11,.055f,.12f,.12f);
    glTranslatef(0,0,.04f);text(0,0,title,1,.97f,.8f,.4f);glPopMatrix();
}
void item(int product,float x,float y,float z,float size) {
    glPushMatrix();glTranslatef(x,y,z);glScalef(size,size,size);
    if(product==4) {
        box({0,0,0,.22f,.36f,.22f,.78f,.59f,.14f});
        for(float y:{-.19f,.19f})box({0,y,0,.23f,.025f,.23f,.78f,.81f,.81f});
    } else if(product==5) {
        box({0,0,0,.27f,.44f,.27f,.16f,.45f,.2f});
        box({0,.29f,0,.12f,.15f,.12f,.18f,.5f,.22f});
        box({0,.38f,0,.13f,.035f,.13f,.8f,.18f,.13f});
    } else if(product==3) {
        box({0,0,0,.36f,.42f,.25f,.56f,.82f,.9f});
        box({0,.23f,0,.18f,.06f,.12f,.85f,.93f,.95f});
    } else if(product==1) box({0,0,0,.23f,.31f,.12f,.85f,.82f,.7f});
    else {
        box({0,0,0,.19f,.34f,.19f,product==0?.18f:.55f,.38f,.12f});
        box({0,.23f,0,.08f,.16f,.08f,.22f,.32f,.12f});
        box({0,.32f,0,.09f,.03f,.09f,.8f,.64f,.27f});
    }
    box({0,0,.133f,.18f,.12f,.018f,.92f,.76f,.42f});
    glPopMatrix();
}
void crate(int product,float x,float y,float z,float size) {
    glPushMatrix();glTranslatef(x,y,z);glScalef(size,size,size);
    box({0,-.12f,0,.7f,.09f,.48f,.18f,.38f,.26f});
    for(float side:{-.32f,.32f})box({side,.05f,0,.065f,.32f,.48f,.2f,.43f,.29f});
    for(float side:{-.21f,.21f})box({0,.05f,side,.7f,.32f,.065f,.2f,.43f,.29f});
    for(int i=0;i<6;++i)item(product,-.2f+(i%3)*.2f,.16f,-.1f+(i/3)*.2f,.55f);
    glPopMatrix();
}
void scene(const Game& g,int activeCamera) {
    renderStreet((g.day-1)*180.0+g.time);
    renderSecurityDevices(activeCamera);
    box({0,-.12f,0,30,.2f,40,.055f,.075f,.09f});
    for(int x=-5;x<5;++x) for(int z=-4;z<4;++z) box({x+.5f,0,z+.5f,.98f,.04f,.98f,.43f,.44f,.37f});
    renderBackRoom(g);
    renderAnnex(g);
    renderHelper(g);
    box({-5,1.8f,0,.2f,3.6f,8,.58f,.64f,.48f});if(!g.largeStore)box({5,1.8f,0,.2f,3.6f,8,.58f,.64f,.48f});
    box({0,3.7f,0,10,.18f,8,.29f,.32f,.28f});
    renderCigaretteCounter(g);box({0,1.15f,-2.6f,10,.15f,1.1f,.65f,.52f,.32f});
    // Each purchased register gets a separate opening through the grade.
    auto hatch=[&](float x) {
        for(int lane=0;lane<g.checkoutCount();++lane) {
            float cx=checkoutX(lane);
            if(std::abs(x-cx)<.88f)return true;
        }
        return false;
    };
    for(float x=-4.8f;x<(g.largeStore?9:5);x+=.6f)
        box({x,hatch(x)?2.89f:2.4f,-2.9f,.045f,hatch(x)?1.42f:2.4f,.045f,.14f,.16f,.17f});
    for(float y:{1.4f,2.5f,3.4f}) {
        if(y>2.18f)box({g.largeStore?2.f:0.f,y,-2.9f,g.largeStore?14.f:10.f,.045f,.045f,.14f,.16f,.17f});
        else for(float x=-4.975f;x<(g.largeStore?9:5);x+=.05f)
            if(!hatch(x))box({x,y,-2.9f,.05f,.045f,.045f,.14f,.16f,.17f});
    }
    for(int lane=0;lane<g.checkoutCount();++lane) {
        float cx=checkoutX(lane),rx=lane>=3?cx+.7f:lane==2?.98f:lane?3.25f:-3.25f;
        for(float x:{cx-.88f,cx+.88f})box({x,1.7f,-2.9f,.08f,1.03f,.09f,.63f,.55f,.32f});
        for(float y:{1.23f,2.18f})box({cx,y,-2.9f,1.84f,.08f,.09f,.63f,.55f,.32f});
        box({rx,1.4f,-2.4f,.55f,.35f,.48f,.13f,.15f,.16f});
        box({rx,1.65f,-2.5f,.46f,.22f,.12f,.38f,.68f,.38f});
        sign(cx-.65f,2.48f,-2.83f,"CAIXA "+std::to_string(lane+1),1.3f);
        int delivered=lane>=2?g.extraCheckout(lane).delivered:lane?g.second.delivered:g.delivered;
        int wanted=lane>=2?g.extraCheckout(lane).wanted:lane?g.second.wanted:g.wanted;
        bool wholesale=lane>=2?g.extraCheckout(lane).wholesale:lane?g.second.wholesale:g.wholesale;
        if(wholesale)for(int i=0;i<std::min(5,(delivered+11)/12);++i)crate(wanted,cx-.55f+(i%3)*.38f,1.4f+(i/3)*.3f,-2.55f,.5f);
        else for(int i=0;i<delivered;++i)item(wanted,cx-.55f+i*.25f,1.46f,-2.55f,.7f);
    }
    box({3.6f,.55f,1.8f,1.7f,1.1f,1,.43f,.29f,.17f});box({3.6f,1.5f,1.9f,.85f,.65f,.2f,.12f,.16f,.17f});box({3.6f,1.51f,1.78f,.7f,.5f,.03f,.2f,.68f,.55f});
    renderRefrigerators(g);
    box({-3.7f,.6f,.4f,1.5f,1.2f,1.4f,.72f,.82f,.8f});box({-3.7f,1.25f,.4f,1.55f,.1f,1.45f,.3f,.64f,.66f});
    sign(-4.38f,1.06f,1.115f,"FREEZER - GELO",1.4f);
    sign(-4.1f,.77f,1.116f,g.stockLabel(3),.85f);
    // Additional label on the aisle-facing side of the freezer.
    glPushMatrix();glTranslatef(-2.93f,1.05f,1.02f);glRotatef(90,0,1,0);
    sign(0,0,0,"FREEZER - GELO",1.3f);
    sign(.25f,-.26f,.001f,g.stockLabel(3),.8f);glPopMatrix();
    sign(4.05f,2.03f,1.76f,"COMPUTADOR",.95f,true);
    renderCustomer(g);
    renderFridgeGlass(g);
}

void beginUI() {
    glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();
    glOrtho(0,960,540,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
}
void renderWorld(const Game& g,const Player& p,const Settings& settings,int w,int h,int camera) {
    int divisor=settings.quality==0?1:(settings.quality==1?2:3);
    int rw=std::max(1,w/divisor),rh=std::max(1,h/divisor);
    glViewport(0,0,rw,rh);
    glClearColor(.012f,.022f,.055f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();double top=.1*std::tan(settings.fov*3.14159265/360);
    glFrustum(-top*w/h,top*w/h,-top,top,.1,70);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    if(camera>=0&&camera<int(securityCameras().size())) {
        auto view=securityCameras()[camera];
        glRotatef(view.pitch,1,0,0);glRotatef(view.yaw*180/3.14159265f,0,1,0);
        glTranslatef(-view.x,-view.y,-view.z);scene(g,camera);renderManager(p);
    } else {
        float sway=std::sin(g.time*1.8f)*g.intoxication*.018f;
        glRotatef(sway,0,0,1);glRotatef(p.pitch,1,0,0);glRotatef(p.yaw*180/3.14159265f,0,1,0);
        glTranslatef(-p.x,p.seated?-1.12f:-1.7f,-p.z);scene(g);
    }
    if(camera<0&&(g.held!=-1 || g.consumeTime>0)) {
        glClear(GL_DEPTH_BUFFER_BIT);glLoadIdentity();
        float lift=g.consumeTime>0?std::sin((2-g.consumeTime)*3.14159265f/2)*.25f:0;
        int product=g.consumeTime>0?g.consuming:g.held;
        if(g.consumeTime==0&&g.heldPacked) {
            for(int i=0;i<std::min(3,(g.heldCount+11)/12);++i)crate(g.held,.3f,-.5f+i*.14f,-1.05f,.7f);
        } else if(g.consumeTime==0&&g.bagCapacity>1) {
            box({.45f,-.48f,-.85f,.44f,.4f,.32f,.69f,.61f,.4f});
            for(int i=1;i<g.heldCount;++i)item(g.held,.32f+(i%3)*.12f,-.25f,-.82f-(i/3)*.13f,.4f);
        }
        if(g.consumeTime>0&&product==1) {
            box({.3f,-.3f+lift,-.7f,.025f,.025f,.25f,.88f,.86f,.76f});
            box({.3f,-.3f+lift,-.84f,.027f,.027f,.025f,.9f,.3f,.1f});
        } else if(!g.heldPacked||g.consumeTime>0) {
            glPushMatrix();glTranslatef(.38f,-.33f+lift,-.85f);
            if(g.consumeTime>0)glRotatef(lift*180,0,0,1);
            item(product,0,0,0,.8f);glPopMatrix();
        }
        box({.38f,-.54f+lift,-.75f,.16f,.24f,.22f,.65f,.43f,.28f});
    }
    if(camera<0&&g.smokeTime>0) {
        glClear(GL_DEPTH_BUFFER_BIT);glLoadIdentity();
        for(int i=0;i<6;++i) {
            float phase=std::fmod(5-g.smokeTime+i*.3f,1.8f),size=.025f+phase*.035f;
            box({.22f+std::sin(phase*4+i)*.06f,-.18f+phase*.26f,-.8f-phase*.2f,
                size,size,size,.65f,.66f,.63f});
        }
    }
    finishWorldImage(w,h,rw,rh,g.intoxication);
    glViewport(0,0,w,h);beginUI();
}
void destroyRenderer() {destroyPostprocess();}

void renderSurveillanceHUD(const Game& g,int camera) {
    rect(0,0,960,57,.025f,.055f,.05f);
    text(24,15,securityCameras()[camera].name,2.3f,.74f,.92f,.75f);
    if(int(g.time*2)%2==0)rect(788,18,9,9,.85f,.12f,.08f);
    text(810,18,"AO VIVO",1.7f);
    rect(0,453,960,87,.025f,.055f,.05f);
    text(22,465,"1 INTERIOR  2 FACHADA  3 RUA  -  SETAS TROCAM CAMERA",1.7f);
    text(22,490,"C OU E VOLTAR AO PC  -  ESC PAUSAR  -  O TEMPO CONTINUA",1.5f);
    text(22,518,"DIA "+std::to_string(g.day)+"  22:"+(int(g.time/3)<10?"0":"")+std::to_string(int(g.time/3)),1.5f);
    if(g.customer)text(350,518,"CLIENTE ESPERANDO: "+std::to_string(int(g.patience))+" S",1.5f,.95f,.72f,.34f);
}
