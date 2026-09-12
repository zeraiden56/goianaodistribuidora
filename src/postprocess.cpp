#include "postprocess.hpp"
#include "render.hpp"
#include <SDL_opengl.h>

namespace {
GLuint texture=0;
void capture(int w,int h,bool smooth) {
    if(!texture)glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,smooth?GL_LINEAR:GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,smooth?GL_LINEAR:GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB,0,0,w,h,0);
}
void image(float offsetX,float offsetY,float weight) {
    glColor3f(weight,weight,weight);glBegin(GL_QUADS);
    glTexCoord2f(offsetX,1+offsetY);glVertex2f(0,0);
    glTexCoord2f(1+offsetX,1+offsetY);glVertex2f(960,0);
    glTexCoord2f(1+offsetX,offsetY);glVertex2f(960,540);
    glTexCoord2f(offsetX,offsetY);glVertex2f(0,540);glEnd();
}
void blur(float x,float y) {
    constexpr float weights[]={1,4,7,10,13,10,7,4,1};
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
    for(int i=0;i<9;++i)image((i-4)*x/4,(i-4)*y/4,weights[i]/57.f);
    glDisable(GL_BLEND);
}
}
void finishWorldImage(int w,int h,int rw,int rh,float intoxication) {
    float radius=visionBlur(intoxication);
    if(rw==w&&rh==h&&radius<.02f){beginUI();return;}
    capture(rw,rh,radius>=.02f);glViewport(0,0,w,h);beginUI();glEnable(GL_TEXTURE_2D);
    if(radius>=.02f) {
        // Separable nine-tap blur: higher intoxication spreads samples farther.
        blur(radius/960,0);capture(w,h,true);blur(0,radius/540);
    } else image(0,0,1);
    glDisable(GL_TEXTURE_2D);glColor3f(1,1,1);
}
void destroyPostprocess(){if(texture)glDeleteTextures(1,&texture);texture=0;}
