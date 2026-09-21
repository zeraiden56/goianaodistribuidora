#include "menu.hpp"
#include "render.hpp"
#include <GL/gl.h>
#include <png.h>
#include <array>
#include <filesystem>
#include <SDL.h>

namespace {
GLuint logoTexture=0;
bool artLoaded=false;
GLuint loadTexture(const char* name) {
    char* base=SDL_GetBasePath();std::string directory=base?base:"";SDL_free(base);
    std::array<std::string,4> paths={directory+"images/"+name,std::string("src/images/")+name,std::string("../src/images/")+name,std::string("images/")+name};
    png_image image{};image.version=PNG_IMAGE_VERSION;
    for(const auto& path:paths) {
        if(!png_image_begin_read_from_file(&image,path.c_str()))continue;
        image.format=PNG_FORMAT_RGBA;
        std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(image));
        if(!png_image_finish_read(&image,nullptr,pixels.data(),0,nullptr)) {png_image_free(&image);continue;}
        GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image.width,image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        png_image_free(&image);return texture;
    }
    return 0;
}
void loadArt() {
    if(artLoaded)return;
    artLoaded=true;
    logoTexture=loadTexture("logo-menu.png");
}
void image(GLuint texture,float x,float y,float w,float h,float alpha) {
    if(!texture)return;
    glEnable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glBindTexture(GL_TEXTURE_2D,texture);glColor4f(1,1,1,alpha);
    glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(1,0);glVertex2f(x+w,y);glTexCoord2f(1,1);glVertex2f(x+w,y+h);glTexCoord2f(0,1);glVertex2f(x,y+h);glEnd();
    glColor4f(1,1,1,1);glDisable(GL_BLEND);glDisable(GL_TEXTURE_2D);
}
}

std::vector<std::string> menuRows(const Menu& m,const Settings& s,bool hasSave) {
    switch(m.screen) {
    case Screen::Main:return {hasSave?"CONTINUAR":"CONTINUAR - NENHUM SLOT","NOVO JOGO","OPCOES","SAIR PARA O DESKTOP"};
    case Screen::Pause:return {"RETOMAR JOGO","SALVAR NO SLOT ATIVO","OPCOES","SALVAR E VOLTAR AO MENU","SALVAR E SAIR","RESETAR E GANHAR PERK AUTOMATICO"};
    case Screen::ConfirmNew:return {"VOLTAR","ESCOLHER SLOT NOVO"};
    case Screen::CompanyName:return {"CONFIRMAR NOME","VOLTAR"};
    case Screen::ConfirmPrestige:return {"VOLTAR","RESETAR E GANHAR PERK"};
    case Screen::SlotSelect:return {std::string("SLOT 1 ")+(m.slots[0]?"- OCUPADO":"- VAZIO"),std::string("SLOT 2 ")+(m.slots[1]?"- OCUPADO":"- VAZIO"),std::string("SLOT 3 ")+(m.slots[2]?"- OCUPADO":"- VAZIO"),"VOLTAR"};
    case Screen::Options:return {std::string("TELA CHEIA: ")+(s.fullscreen?"SIM":"NAO"),
        "JANELA: "+std::to_string(widths[s.resolution])+" X "+std::to_string(heights[s.resolution]),
        std::string("VISUAL: ")+(s.quality==0?"NATIVO":s.quality==1?"RETRO 1/2":"RETRO 1/3"),
        std::string("VSYNC: ")+(s.vsync?"SIM":"NAO"),"CAMPO DE VISAO: "+std::to_string(s.fov),"VOLUME DOS EFEITOS: "+std::to_string(s.volume),"VOLTAR"};
    default:return {};
    }
}
int menuHit(int x,int y,int count) {
    if(x<48||x>438||y<173)return -1;
    int row=(y-173)/37;
    return row<count&&(y-173)%37<32?row:-1;
}
void renderMenu(const Menu& m,const Settings& s,bool hasSave) {
    loadArt();
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);glColor4f(.015f,.033f,.03f,.65f);glVertex2f(0,0);glVertex2f(0,540);
    glColor4f(.015f,.033f,.03f,0);glVertex2f(700,540);glVertex2f(700,0);glEnd();glDisable(GL_BLEND);
    rect(24,30,438,478,.035f,.065f,.065f);rect(24,30,4,478,.91f,.66f,.28f);
    image(logoTexture,84,35,318,106,1);
    if(!logoTexture)text(53,86,"GOIANAO DISTRIBUIDORA",2.7f,.95f,.75f,.36f);
    std::string title=m.screen==Screen::Main?"MENU PRINCIPAL":m.screen==Screen::Pause?"JOGO PAUSADO":m.screen==Screen::Options?"OPCOES DE VIDEO E AUDIO":m.screen==Screen::CompanyName?"NOME DA SUA DISTRIBUIDORA":m.screen==Screen::ConfirmPrestige?"RESET AUTOMATICO COM PERK":m.screen==Screen::SlotSelect?(m.selectingNew?"ESCOLHA O SLOT NOVO":"CARREGAR UM SLOT"):"ESCOLHER NOVO JOGO";
    text(48,150,title,1.1f);
    if(m.screen==Screen::CompanyName) {rect(48,263,390,34,.08f,.14f,.13f);text(60,275,m.input+"_",1.3f,1.f,.82f,.45f);}
    auto rows=menuRows(m,s,hasSave);
    for(int i=0;i<int(rows.size());++i) {
        bool selected=i==m.selected;
        rect(48,173+i*37,390,32,selected?.21f:.075f,selected?.32f:.13f,selected?.29f:.14f);
        text(61,185+i*37,rows[i],1.3f,selected?1.f:.82f,selected?.82f:.88f,selected?.45f:.83f);
    }
    if(m.screen==Screen::SlotSelect)text(48,441,"TRES PROGRESSOS INDEPENDENTES",1.1f);
    else if(m.screen==Screen::CompanyName)text(48,441,"ENTER CONFIRMA O NOME",1.1f);
    else if(m.screen==Screen::ConfirmPrestige)text(48,441,"O PERK E CONCEDIDO AUTOMATICAMENTE NO RESET",1.2f);
    else if(m.screen==Screen::Options)text(48,441,"TELA CHEIA USA A RESOLUCAO DO MONITOR",1.1f);
    else if(m.screen==Screen::Pause)text(48,441,"TEMPO PAUSADO - SAIDA SALVA AUTOMATICAMENTE",1.1f);
    else if(m.screen==Screen::Main)text(48,441,"SUA DISTRIBUIDORA. SEU RITMO.",1.1f);
    text(48,480,"MOUSE OU SETAS / ENTER / ESC VOLTAR",1.15f);
    if(!m.notice.empty()) {rect(0,512,960,28,.04f,.08f,.08f);text(14,520,m.notice,1.4f,.96f,.72f,.34f);}
}

void destroyMenuArt() {
    if(logoTexture)glDeleteTextures(1,&logoTexture);
    logoTexture=0;artLoaded=false;
}
