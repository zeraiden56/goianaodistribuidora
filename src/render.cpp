#include "render.hpp"
#include "computer.hpp"
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
        cylinder(0,0,0,.115f,.35f,.85f,.57f,.12f);
        for(float y:{-.18f,.18f})cylinder(0,y,0,.118f,.018f,.78f,.82f,.81f);
        box({0,.194f,0,.065f,.01f,.04f,.32f,.37f,.37f});
        box({0,0,.111f,.16f,.19f,.014f,.14f,.38f,.29f});
    } else if(product==3) {
        box({0,0,0,.36f,.4f,.25f,.64f,.85f,.9f});
        for(float dx:{-.1f,.08f})for(float yy:{-.11f,.05f})box({dx,yy,.134f,.12f,.12f,.025f,.84f,.94f,.96f});
        box({0,.225f,0,.25f,.045f,.12f,.82f,.93f,.95f});
        box({0,.03f,.159f,.33f,.08f,.012f,.14f,.43f,.67f});
    } else if(product==1) {
        box({0,0,0,.24f,.32f,.12f,.89f,.87f,.78f});
        box({0,.112f,.004f,.245f,.09f,.124f,.58f,.12f,.1f});
        box({0,-.055f,.066f,.2f,.06f,.01f,.12f,.14f,.14f});
        for(int i=0;i<4;++i)box({-.06f+i*.04f,.185f,0,.027f,.055f,.027f,.84f,.75f,.52f});
    } else {
        float r=product==0?.22f:product==2?.56f:.13f,g=product==0?.34f:product==2?.29f:.43f,b=product==2?.09f:.17f;
        float radius=product==2?.135f:product==5?.14f:.1f;
        cylinder(0,-.02f,0,radius,.35f,r,g,b);
        cylinder(0,.19f,0,radius,.09f,r*1.1f,g*1.1f,b,.052f);
        cylinder(0,.31f,0,.052f,.16f,r,g,b);
        cylinder(0,.4f,0,.059f,.037f,product==5?.83f:.73f,product==5?.16f:.58f,.2f);
        box({0,-.015f,radius,.16f,.17f,.015f,product==5?.8f:.91f,product==5?.16f:.76f,product==5?.1f:.42f});
        box({-.038f,.08f,radius*.91f,.018f,.16f,.014f,.56f,.66f,.5f});
        box({0,-.025f,radius+.012f,.12f,.035f,.01f,.17f,.22f,.16f});
    }
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
    renderSecurityDevices(activeCamera,g.expanded,g.largeStore);

    for(int x=-5;x<5;++x) for(int z=-4;z<4;++z) texturedBox({x+.5f,0,z+.5f,1,.04f,1,.62f,.61f,.53f},Surface::Tile);
    renderBackRoom(g);
    renderAnnex(g);
    renderHelper(g);
    texturedBox({-5,1.8f,0,.2f,3.6f,8,.72f,.73f,.61f},Surface::Plaster);
    if(!g.largeStore)texturedBox({5,1.8f,0,.2f,3.6f,8,.72f,.73f,.61f},Surface::Plaster);
    for(float x:{-4.87f,4.87f})if(x<0||!g.largeStore) {
        texturedBox({x,.52f,0,.035f,1.f,8,.2f,.39f,.35f},Surface::Tile);
        box({x,1.06f,0,.045f,.06f,8,.72f,.55f,.27f});
    }
    for(float z:{-.5f,2.f})box({0,3.56f,z,2.7f,.07f,.22f,.94f,.96f,.8f});
    texturedBox({0,3.84f,-3.8f,10.2f,.7f,.24f,.12f,.29f,.24f},Surface::Wood);
    sign(4.3f,4.02f,-3.96f,g.companyName,8.6f,true);
    box({0,3.7f,0,10,.18f,8,.29f,.32f,.28f});
    renderCigaretteCounter(g);texturedBox({0,1.15f,-2.6f,10,.15f,1.1f,.65f,.52f,.32f},Surface::Wood);
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
        bool wholesale=lane>=2?g.extraCheckout(lane).wholesale:lane?g.second.wholesale:g.wholesale;
        int slot=0;
        for(int product=0;product<Game::productCount;++product) {
            int delivered=g.customerActive(lane)?g.fulfilled(lane,product):0;
            if(wholesale)for(int i=0;i<std::min(5,(delivered+11)/12);++i)crate(product,cx-.55f+(slot++%3)*.38f,1.4f+(i/3)*.3f,-2.55f,.5f);
            else for(int i=0;i<delivered;++i) {item(product,cx-.6f+(slot%5)*.25f,1.46f,-2.5f+(slot/5)*.22f,.7f);++slot;}
        }
    }
    texturedBox({3.6f,1.065f,1.8f,1.7f,.095f,1,.55f,.4f,.24f},Surface::Wood);
    for(float x:{2.84f,4.36f})for(float z:{1.38f,2.22f})box({x,.52f,z,.08f,1.04f,.07f,.2f,.26f,.25f});
    box({4.08f,.88f,1.9f,.5f,.25f,.7f,.4f,.3f,.19f});box({4.08f,.89f,1.535f,.14f,.026f,.02f,.55f,.61f,.58f});
    renderTerminal(g,0);
    renderRefrigerators(g);
    box({-3.7f,.6f,.4f,1.5f,1.2f,1.4f,.72f,.82f,.8f});box({-3.7f,1.25f,.4f,1.55f,.1f,1.45f,.3f,.64f,.66f});
    sign(-4.38f,1.06f,1.115f,"FREEZER - GELO",1.4f);
    sign(-4.1f,.77f,1.116f,g.stockLabel(3),.85f);
    // Additional label on the aisle-facing side of the freezer.
    glPushMatrix();glTranslatef(-2.93f,1.05f,1.02f);glRotatef(90,0,1,0);
    sign(0,0,0,"FREEZER - GELO",1.3f);
    sign(.25f,-.26f,.001f,g.stockLabel(3),.8f);glPopMatrix();
    sign(4.05f,2.22f,1.735f,"COMPUTADOR",.95f,true);
    renderCustomer(g);
    renderFridgeGlass(g);
}

void beginUI() {
    glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();
    glOrtho(0,960,540,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
}
void renderWorld(const Game& g,const Player& p,const Settings& settings,int w,int h,int camera,float computerFocus,int terminal) {
    int divisor=settings.quality==0?1:(settings.quality==1?2:3);
    int rw=std::max(1,w/divisor),rh=std::max(1,h/divisor);
    glViewport(0,0,rw,rh);
    float daylight=daylightAt(g.time);
    glClearColor(.018f+daylight*.4f,.028f+daylight*.61f,.065f+daylight*.69f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
    auto focused=computerView(p,terminal,computerFocus,settings.fov,float(w)/h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();double top=.1*std::tan((camera>=0?settings.fov:focused.fov)*3.14159265/360);
    glFrustum(-top*w/h,top*w/h,-top,top,.1,160);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    if(cameraAvailable(camera,g.expanded,g.largeStore)) {
        auto view=securityCameras()[camera];
        glRotatef(view.pitch,1,0,0);glRotatef(view.yaw*180/3.14159265f,0,1,0);
        glTranslatef(-view.x*shopScaleX,-view.y,-view.z*shopScaleZ);glScalef(shopScaleX,1,shopScaleZ);scene(g,camera);renderManager(p);
    } else {
        float sway=std::sin(g.time*1.8f)*g.intoxication*.018f*(1-computerFocus);
        float bob=p.seated?0.f:std::sin(p.walkCycle)*.018f*(1-computerFocus);
        glRotatef(sway+std::sin(p.walkCycle)*.35f*(1-computerFocus),0,0,1);glRotatef(focused.pitch,1,0,0);glRotatef(focused.yaw*180/3.14159265f,0,1,0);
        glTranslatef(-focused.x*shopScaleX,-focused.y+bob,-focused.z*shopScaleZ);glScalef(shopScaleX,1,shopScaleZ);scene(g);captureOrderPositions(g);
    }
    if(camera<0&&computerFocus==0&&(g.held!=-1 || g.consumeTime>0)) {
        glClear(GL_DEPTH_BUFFER_BIT);glLoadIdentity();
        float lift=g.consumeTime>0?std::sin((2-g.consumeTime)*3.14159265f/2)*.25f:0;
        int product=g.consumeTime>0?g.consuming:g.held;
        if(g.consumeTime==0&&g.heldPacked) {
            for(int i=0;i<std::min(3,(g.heldCount+11)/12);++i)crate(g.held,.3f,-.5f+i*.14f,-1.05f,.7f);
        } else if(g.consumeTime==0&&g.heldCount>1) {
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
    if(camera<0&&computerFocus==0&&g.smokeTime>0) {
        glClear(GL_DEPTH_BUFFER_BIT);glLoadIdentity();
        for(int i=0;i<6;++i) {
            float phase=std::fmod(5-g.smokeTime+i*.3f,1.8f),size=.025f+phase*.035f;
            box({.22f+std::sin(phase*4+i)*.06f,-.18f+phase*.26f,-.8f-phase*.2f,
                size,size,size,.65f,.66f,.63f});
        }
    }
    finishWorldImage(w,h,rw,rh,g.intoxication*(1-computerFocus));
    glViewport(0,0,w,h);beginUI();
}
void destroyRenderer() {destroyPostprocess();destroyMaterials();}

void renderSurveillanceHUD(const Game& g,int camera) {
    rect(0,0,960,57,.025f,.055f,.05f);
    text(24,15,securityCameras()[camera].name,2.3f,.74f,.92f,.75f);
    if(int(g.time*2)%2==0)rect(788,18,9,9,.85f,.12f,.08f);
    text(810,18,"AO VIVO",1.7f);
    rect(0,450,960,90,.025f,.055f,.05f);
    for(int i=0;i<securityCameraCount;++i) {
        bool available=cameraAvailable(i,g.expanded,g.largeStore),selected=camera==i;float x=18+i*155.f;
        rect(x,458,148,46,selected?.18f:.07f,selected?.34f:.13f,selected?.29f:.13f);
        if(selected)rect(x,458,148,3,.93f,.7f,.32f);
        text(x+9,470,std::to_string(i+1)+" "+securityCameras()[i].label,1.3f,available?.88f:.42f,available?.93f:.49f,available?.86f:.46f);
        text(x+9,489,available?(selected?"SELECIONADA":"AO VIVO"):"AMPLIE A LOJA",.95f,available?.56f:.49f,.68f,.57f);
    }
    text(20,521,"1-6 / CLIQUE / SETAS TROCAR  |  C OU E VOLTAR  |  ESC PAUSAR",1.15f);
    text(775,521,worldClock(g.time)+"  DIA "+std::to_string(g.day),1.15f);

}

void renderMenuWorld(const Game&,const Settings& settings,int w,int h,double seconds) {
    static Game preview=[] {Game g;g.cash=100000;g.expand();g.expand();for(int i=0;i<2;++i){g.upgradeCheckout();g.hire();}g.hire();return g;}();
    static double last=seconds;
    preview.tick(float(std::clamp(seconds-last,0.,.05)));last=seconds;
    preview.time=float(std::fmod(20+seconds*.7,180));
    for(int i=0;i<Game::productCount;++i)preview.products[i].stock=24;
    int shot=int(seconds/14)%4;float t=float(std::fmod(seconds,14)/14);t=t*t*(3-2*t);
    float x=0,y=0,z=0,tx=0,ty=1.4f,tz=0;
    if(shot==0){x=-8+t*15;y=3.3f+std::sin(t*3.14f)*.8f;z=-12;tx=1;ty=1.8f;tz=-1;}
    if(shot==1){x=-1.6f+t*3.2f;y=1.9f;z=1.45f;tx=-.7f;tz=-4.2f;}
    if(shot==2){x=.7f+t*1.4f;y=2.2f;z=-.8f;tx=-1.4f;ty=1.3f;tz=3.2f;}
    if(shot==3){x=14-t*7;y=8+t*2;z=-17+t*2;tx=-2;ty=1;tz=-5;}
    float dx=(tx-x)*shopScaleX,dz=(tz-z)*shopScaleZ,dy=ty-y;
    glViewport(0,0,w,h);float day=daylightAt(preview.time);
    glClearColor(.018f+day*.4f,.028f+day*.61f,.065f+day*.69f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();double top=.1*std::tan(64*3.14159265/360);glFrustum(-top*w/h,top*w/h,-top,top,.1,160);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();glRotatef(-std::atan2(dy,std::hypot(dx,dz))*180/3.14159265f,1,0,0);
    glRotatef(std::atan2(dx,-dz)*180/3.14159265f,0,1,0);glTranslatef(-x*shopScaleX,-y,-z*shopScaleZ);glScalef(shopScaleX,1,shopScaleZ);scene(preview);
    beginUI();
    float edge=float(std::fmod(seconds,14));float fade=std::clamp((.65f-std::min(edge,14-edge))/.65f,0.f,1.f);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glColor4f(.015f,.03f,.03f,fade);
    glBegin(GL_QUADS);glVertex2f(0,0);glVertex2f(960,0);glVertex2f(960,540);glVertex2f(0,540);glEnd();glDisable(GL_BLEND);
    (void)settings;
}
