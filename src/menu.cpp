#include "menu.hpp"
#include "render.hpp"

std::vector<std::string> menuRows(const Menu& m,const Settings& s,bool hasSave) {
    switch(m.screen) {
    case Screen::Main:return {hasSave?"CONTINUAR":"CONTINUAR - SEM SAVE","NOVO JOGO","OPCOES","SAIR PARA O DESKTOP"};
    case Screen::Pause:return {"RETOMAR JOGO","SALVAR PROGRESSO","OPCOES","SALVAR E VOLTAR AO MENU","SALVAR E SAIR"};
    case Screen::ConfirmNew:return {"CANCELAR","INICIAR E SUBSTITUIR SAVE"};
    case Screen::Options:return {std::string("TELA CHEIA: ")+(s.fullscreen?"SIM":"NAO"),
        "JANELA: "+std::to_string(widths[s.resolution])+" X "+std::to_string(heights[s.resolution]),
        std::string("VISUAL: ")+(s.quality==0?"NATIVO":s.quality==1?"RETRO 1/2":"RETRO 1/3"),
        std::string("VSYNC: ")+(s.vsync?"SIM":"NAO"),"CAMPO DE VISAO: "+std::to_string(s.fov),"VOLUME DOS EFEITOS: "+std::to_string(s.volume),"VOLTAR"};
    default:return {};
    }
}
int menuHit(int x,int y,int count) {
    if(x<245||x>715||y<173)return -1;
    int row=(y-173)/37;
    return row<count&&(y-173)%37<32?row:-1;
}
void renderMenu(const Menu& m,const Settings& s,bool hasSave) {
    rect(180,28,600,478,.035f,.065f,.075f);
    rect(180,28,8,478,.91f,.66f,.28f);
    text(237,51,"GOIANÃO",4,.95f,.75f,.36f);text(239,88,"DISTRIBUIDORA",2);
    std::string title=m.screen==Screen::Main?"GERENCIE SUA DISTRIBUIDORA":m.screen==Screen::Pause?"PAUSADO":m.screen==Screen::Options?"OPCOES DE VIDEO E AUDIO":"SUBSTITUIR O PROGRESSO SALVO?";
    text(238,131,title,1.8f);
    auto rows=menuRows(m,s,hasSave);
    for(int i=0;i<int(rows.size());++i) {
        bool selected=i==m.selected;
        rect(245,173+i*37,470,32,selected?.21f:.075f,selected?.32f:.13f,selected?.29f:.14f);
        text(262,185+i*37,rows[i],1.7f,selected?1.f:.82f,selected?.82f:.88f,selected?.45f:.83f);
    }
    if(m.screen==Screen::Options)text(232,441,"TELA CHEIA USA A RESOLUCAO DO MONITOR",1.4f);
    else if(m.screen==Screen::Pause)text(232,414,"TEMPO PAUSADO - SAIDA SALVA AUTOMATICAMENTE",1.3f);
    text(221,468,"MOUSE OU SETAS + ENTER  -  ESC VOLTAR",1.5f);
    if(!m.notice.empty()) {rect(0,512,960,28,.04f,.08f,.08f);text(14,520,m.notice,1.4f,.96f,.72f,.34f);}
}
