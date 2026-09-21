#include "render.hpp"
#include "shop_layout.hpp"
#include "world.hpp"
#include <algorithm>
#include <cmath>
namespace {
void panel(float x,float y,float w,float h){rect(x,y,w,h,.035f,.075f,.079f);}
}
void renderHUD(const Game& g,const Player& p,bool help) {
    panel(18,16,180,35);rect(18,16,3,35,.91f,.68f,.32f);
    text(30,27,"R$ "+std::to_string(g.cash),1.9f,.97f,.81f,.46f);
    panel(721,16,221,35);
    text(733,29,"DIA "+std::to_string(g.day)+"  "+worldClock(g.time)+(daylightAt(g.time)>.35f?"  DIA":"  NOITE"),1.3f);
    renderOrderBubbles(g,p);
    int target=interaction(p.x,p.z,p.yaw,g.expanded,g.secondCheckout,g.largeStore,g.thirdCheckout,g.annexCheckouts);
    rect(478,268,4,4,target>=0?.57f:.9f,target>=0?.88f:.9f,.71f);
    std::string hint;
    if(target==16&&(p.seated||g.expanded))hint="E / CLIQUE - COMPUTADOR";
    else if(p.seated)hint="E LEVANTAR  /  C COMPUTADOR  /  T TV";
    else if(target==4)hint="E / CLIQUE - COMPUTADOR";
    else if(stationCheckout(target)>=0)hint="E / CLIQUE - ENTREGAR NO CAIXA "+std::to_string(stationCheckout(target)+1);
    else if(target==6)hint="E / CLIQUE - SENTAR";
    else if(target==7)hint="E / CLIQUE - LIGAR OU DESLIGAR TV";
    else {
        int product=stationProduct(target);
        if(product>=0) {
            if(g.held>=0&&g.held!=product)hint="ENTREGUE A SACOLA OU DEVOLVA NA ORIGEM";
            else if(g.held==product&&g.heldCount>=g.bagCapacity*(g.heldPacked?12:1))hint="SACOLA CHEIA  /  Q DEVOLVER";
            else hint="E / CLIQUE - +1 "+std::string(g.products[product].name)+(g.held==product?"  /  Q DEVOLVER":"");
            std::string stock="ESTOQUE "+g.stockLabel(product)+(g.largeStore?"  /  F - +1 CAIXA":"");
            text(480-stock.size()*3.3f,468,stock,1.1f,.85f,.89f,.82f);
        }
    }
    if(!hint.empty()) {
        float width=hint.size()*7.2f+24;panel(480-width/2,484,width,28);
        text(492-width/2,493,hint,1.2f,.96f,.84f,.57f);
    }
    if(g.held>=0) {
        panel(720,414,222,42);text(731,423,g.products[g.held].name,1.2f,.96f,.8f,.43f);
        text(731,440,carriedLabel(g.heldCount,g.heldPacked)+" / "+std::to_string(g.bagCapacity)+(g.heldPacked?" CX":" UN.")+(g.held!=3?"  R USAR":""),1.05f);
    }
    if(g.pending>=0){panel(18,63,180,24);text(29,71,"ENTREGA EM "+std::to_string(int(std::ceil(g.delivery)))+" S",1.2f);}
    if(g.messageTime>0) {
        std::string message=g.message.substr(0,110);
        panel(218,60,message.size()*6.f+24,27);text(230,69,message,1.f,.96f,.8f,.48f);
    }
    text(20,523,"TAB CONTROLES",1.05f,.7f,.77f,.72f);
    if(help) {
        panel(18,321,340,187);text(32,334,"COMO ATENDER",1.6f,.95f,.77f,.4f);
        const char* lines[]={"WASD ANDAR / SHIFT CORRER","E OU CLIQUE: PEGAR +1 / ENTREGAR","Q NA ORIGEM: DEVOLVER SACOLA","F OU DIREITO: PEGAR +1 CAIXA","R CONSUMIR / ESC PAUSAR","COMPLETE TODOS OS ITENS DO CLIENTE","ESTOQUE E MELHORIAS NO COMPUTADOR"};
        for(int i=0;i<7;++i)text(32,361+i*19,lines[i],1.2f);
    }
}
