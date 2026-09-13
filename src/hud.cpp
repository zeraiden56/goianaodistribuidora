#include "render.hpp"
#include "customers.hpp"
#include "shop_layout.hpp"
#include <algorithm>
#include <cmath>
namespace {
void panel(float x,float y,float w,float h){rect(x,y,w,h,.035f,.075f,.079f);rect(x,y,3,h,.28f,.48f,.43f);}
void bar(float x,float y,float w,float ratio,float r,float g,float b) {
    rect(x,y,w,5,.13f,.19f,.19f);rect(x,y,w*std::clamp(ratio,0.f,1.f),5,r,g,b);
}
}
void renderHUD(const Game& g,const Player& p,bool) {
    rect(0,0,960,49,.025f,.055f,.058f);
    text(18,12,"GOIANÃO DISTRIBUIDORA",2.f,.96f,.76f,.38f);
    text(19,34,"LUCRO BASE +"+std::to_string(g.marginBonus())+"%",1.f,.62f,.77f,.73f);
    text(340,10,"CAIXA",1.1f,.65f,.8f,.75f);text(340,26,"R$ "+std::to_string(g.cash)+",00",2);
    text(591,12,"DIA "+std::to_string(g.day)+" / NIVEL "+std::to_string(g.level),1.55f);
    text(591,34,"VENDIDOS: "+std::to_string(g.sold),1.f);
    text(795,12,"REPUTACAO "+std::to_string(g.reputation),1.1f);bar(795,32,145,g.reputation/100.f,.43f,.75f,.52f);
    panel(18,66,230,198);text(30,78,"ESTOQUE",1.4f,.7f,.87f,.82f);
    for(int i=0;i<Game::productCount;++i) {
        float y=102+i*26;
        text(30,y,g.products[i].name,1.1f);text(171,y,i<g.availableProducts()?g.stockLabel(i):"TRAVADO",1.05f);
        bool low=g.products[i].stock<4;
        bar(30,y+13,205,float(g.products[i].stock)/g.products[i].capacity,low?.94f:.35f,low?.48f:.66f,.36f);
    }
    for(int lane=0;lane<g.checkoutCount();++lane) {
        bool active=lane>=2?g.extraCheckout(lane).customer:lane?g.second.customer:g.customer;
        float y=66+lane*65.f;
        panel(646,y,296,61);text(658,y+10,"CAIXA "+std::to_string(lane+1),1.2f,.74f,.87f,.8f);
        if(!active){text(658,y+32,"AGUARDANDO CLIENTE",1.3f);continue;}
        int wanted=lane>=2?g.extraCheckout(lane).wanted:lane?g.second.wanted:g.wanted;
        int quantity=lane>=2?g.extraCheckout(lane).quantity:lane?g.second.quantity:g.quantity;
        int delivered=lane>=2?g.extraCheckout(lane).delivered:lane?g.second.delivered:g.delivered;
        int style=lane>=2?g.extraCheckout(lane).customerStyle:lane?g.second.customerStyle:g.customerStyle;
        float patience=lane>=2?g.extraCheckout(lane).patience:lane?g.second.patience:g.patience;
        text(735,y+11,customerLooks()[style].name,1.f);
        bool wholesale=lane>=2?g.extraCheckout(lane).wholesale:lane?g.second.wholesale:g.wholesale;
        text(658,y+25,(wholesale?std::to_string(quantity/12)+" CX DE 12 - ":std::to_string(quantity)+" X ")+g.products[wanted].name,1.2f);
        text(658,y+40,"ENTREGUES "+std::to_string(delivered)+"/"+std::to_string(quantity)+" - "+std::to_string(int(patience))+" S",1.2f);
        bar(658,y+54,270,patience/(wholesale?180.f:65.f),patience<15?.95f:.4f,patience<15?.35f:.75f,.36f);
    }
    if(g.pending>=0){panel(18,274,230,28);text(30,285,"ENTREGA "+std::to_string(g.pendingUnits)+" UN. EM "+std::to_string(int(std::ceil(g.delivery)))+" S",1.1f);}
    if(g.intoxication>0){panel(18,310,230,31);text(30,318,"EMBRIAGUEZ "+std::to_string(int(g.intoxication)),1.1f);bar(30,331,205,g.intoxication/100,.76f,.5f,.3f);}
    for(int lane=0;lane<g.checkoutCount();++lane) {
        const auto& h=g.staff(lane);if(!h.hired)continue;
        float y=342+lane*20.f;panel(18,y,230,20);
        text(30,y+7,"ATENDENTE "+std::to_string(lane+1)+" - "+carriedLabel(h.count,h.packed),1.f);
    }
    int target=interaction(p.x,p.z,p.yaw,g.expanded,g.secondCheckout,g.largeStore,g.thirdCheckout,g.annexCheckouts);
    rect(478,268,4,4,target>=0?.57f:.9f,target>=0?.88f:.9f,.71f);
    std::string hint;
    if(target==16&&(p.seated||g.expanded))hint="E OU CLIQUE: ABRIR COMPUTADOR";
    else if(p.seated)hint="E LEVANTAR / C COMPUTADOR / T TV";
    else if(target==4)hint="E OU CLIQUE: ABRIR COMPUTADOR";
    else if(stationCheckout(target)>=0)hint="E OU CLIQUE: ENTREGAR SACOLA NO CAIXA "+std::to_string(stationCheckout(target)+1);
    else if(target==6)hint="E OU CLIQUE: SENTAR NO SOFA";
    else if(target==7)hint="E OU CLIQUE: LIGAR OU DESLIGAR TV";
    else {
        int product=stationProduct(target);
        if(product>=0) {
            if(g.held==product)hint="E OU CLIQUE: DEVOLVER SACOLA";
            else if(g.held>=0)hint="SACOLA OCUPADA - ENTREGUE OU DEVOLVA";
            else if(g.products[product].stock==0)hint="SEM ESTOQUE - ENCOMENDE NO COMPUTADOR";
            else hint=g.largeStore?"E/CLIQUE: UNIDADES  F/DIREITO: CX DE 12":"E OU CLIQUE: PEGAR "+std::string(g.products[product].name);
        }
    }
    if(!hint.empty()){panel(252,448,456,15);text(260,452,hint,1.1f);}
    panel(710,397,232,52);
    text(722,405,g.held<0?"SACOLA VAZIA":g.products[g.held].name,1.25f);
    text(722,425,carriedLabel(g.held<0?0:g.heldCount,g.heldPacked)+(g.held==1?" - R FUMAR":g.held>=0&&g.held!=3?" - R BEBER":""),1.1f);
    panel(18,463,924,33);text(30,475,g.message.substr(0,108),1.25f,.97f,.79f,.46f);
    rect(0,511,960,29,.025f,.055f,.058f);text(20,522,"WASD ANDAR  SHIFT CORRER  E/CLIQUE INTERAGIR  F/DIREITO CAIXAS  R CONSUMIR  ESC MENU",1.2f);
}
