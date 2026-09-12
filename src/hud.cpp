#include "render.hpp"
#include "customers.hpp"
#include <algorithm>
#include <cmath>
namespace {
void panel(float x,float y,float w,float h){rect(x,y,w,h,.035f,.075f,.079f);rect(x,y,3,h,.28f,.48f,.43f);}
void bar(float x,float y,float w,float ratio,float r,float g,float b) {
    rect(x,y,w,5,.13f,.19f,.19f);rect(x,y,w*std::clamp(ratio,0.f,1.f),5,r,g,b);
}
void centered(float y,const std::string& label,float size){text((960-label.size()*6*size)/2,y,label,size);}
}
void renderHUD(const Game& g,const Player& p,bool) {
    rect(0,0,960,49,.025f,.055f,.058f);
    text(18,12,"GOIANÃO DISTRIBUIDORA",2.f,.96f,.76f,.38f);text(19,34,"LUCRO BASE +"+std::to_string(g.marginBonus())+"%",1.f,.62f,.77f,.73f);
    text(340,10,"CAIXA",1.1f,.65f,.8f,.75f);text(340,26,"R$ "+std::to_string(g.cash)+",00",2);
    text(591,12,"DIA "+std::to_string(g.day)+" / NIVEL "+std::to_string(g.level),1.55f);
    text(591,34,"VENDIDOS: "+std::to_string(g.sold),1.f);
    text(795,12,"REPUTACAO "+std::to_string(g.reputation),1.1f);bar(795,32,145,g.reputation/100.f,.43f,.75f,.52f);
    panel(18,66,202,151);text(30,78,"ESTOQUE",1.4f,.7f,.87f,.82f);
    for(int i=0;i<4;++i) {
        float y=102+i*27;
        text(30,y,g.products[i].name,1.2f);text(147,y,g.stockLabel(i),1.1f);
        bool low=g.products[i].stock<4;
        bar(30,y+13,177,float(g.products[i].stock)/g.products[i].capacity,low?.94f:.35f,low?.48f:.66f,.36f);
    }
    if(g.customer) {
        panel(636,66,306,145);text(649,79,customerLooks()[g.customerStyle].name,1.35f,.74f,.87f,.8f);
        text(649,100,std::to_string(g.quantity)+" X "+g.products[g.wanted].name,2);
        text(649,125,"ENTREGUES "+std::to_string(g.delivered)+" DE "+std::to_string(g.quantity),1.2f);
        for(int i=0;i<g.quantity;++i)rect(649+i*23,144,17,10,i<g.delivered?.4f:.16f,i<g.delivered?.78f:.24f,.35f);
        bool urgent=g.patience<15;bar(649,165,279,g.patience/65.f,urgent?.95f:.4f,urgent?.35f:.75f,.36f);
        text(649,183,(urgent?"CLIENTE VAI EMBORA: ":"TEMPO RESTANTE: ")+std::to_string(int(g.patience))+" S",1.2f);
    } else {panel(698,66,244,47);text(710,80,"AGUARDANDO CLIENTE",1.5f);}
    if(g.secondCheckout) {
        panel(636,220,306,118);text(649,230,"CAIXA 2",1.2f,.74f,.87f,.8f);
        if(g.second.customer) {
            const auto& c=g.second;
            text(649,249,customerLooks()[c.customerStyle].name,1.1f);
            text(649,266,std::to_string(c.quantity)+" X "+g.products[c.wanted].name,1.8f);
            text(649,288,"ENTREGUES "+std::to_string(c.delivered)+"/"+std::to_string(c.quantity)+" - "+std::to_string(int(c.patience))+" S",1.2f);
            bar(649,317,279,c.patience/65.f,.6f,.7f,.3f);
        } else text(649,268,"AGUARDANDO CLIENTE",1.3f);
    }
    if(g.secondHelper.hired){panel(18,400,220,43);text(30,410,"ATENDENTE 2",1.1f);text(30,427,"SACOLA "+std::to_string(g.secondHelper.count)+"/"+std::to_string(g.bagCapacity),1.1f);}
    if(g.pending>=0){panel(18,228,202,43);text(30,241,"ENTREGA EM "+std::to_string(int(std::ceil(g.delivery)))+" S",1.3f);}
    if(g.intoxication>0){panel(18,282,202,43);text(30,293,"EMBRIAGUEZ "+std::to_string(int(g.intoxication)),1.2f);bar(30,310,177,g.intoxication/100,.76f,.5f,.3f);}
    if(g.helper.hired){panel(18,352,244,44);text(30,362,"ATENDENTE 1 - "+std::to_string(g.helper.count)+"/"+std::to_string(g.bagCapacity),1.1f,.67f,.84f,.77f);text(30,380,helperStatus(g).substr(0,30),1.05f);}
    int target=interaction(p.x,p.z,p.yaw,g.expanded,g.secondCheckout);
    rect(478,268,4,4,target>=0?.57f:.9f,target>=0?.88f:.9f,.71f);
    std::string hint;
    if(p.seated)hint="E LEVANTAR / T LIGAR OU DESLIGAR TV";
    else if(target==4)hint="E USAR COMPUTADOR";
    else if(target==5||target==12)hint=target==12?"E ENTREGAR NO CAIXA 2":"E ENTREGAR NO CAIXA 1";
    else if(target==6)hint="E SENTAR NO SOFA";
    else if(target==7)hint="E LIGAR OU DESLIGAR TV";
    else if(target>=0) {
        int product=target>=8?target-8:target;
        if(g.held==product)hint="E DEVOLVER "+std::string(g.products[product].name);
        else if(g.held>=0)hint="MAOS OCUPADAS - ENTREGUE OU DEVOLVA";
        else if(g.products[product].stock==0)hint="SEM ESTOQUE - ENCOMENDE NO COMPUTADOR";
        else hint="E PEGAR "+std::string(g.products[product].name);
    }
    if(!hint.empty()){panel(260,410,440,34);centered(421,hint,1.45f);}
    panel(714,355,228,42);text(726,366,g.held<0?"MAOS LIVRES":std::string("NA MAO: ")+g.products[g.held].name,1.2f);
    text(726,384,g.consumeTime>0?(g.consuming==1?"FUMANDO...":"BEBENDO..."):g.held==1?"R FUMAR":g.held==0||g.held==2?"R BEBER":"UMA UNIDADE POR VIAGEM",1.f);
    panel(18,463,924,33);text(30,475,g.message.substr(0,108),1.25f,.97f,.79f,.46f);
    rect(0,511,960,29,.025f,.055f,.058f);text(20,522,"WASD ANDAR   SHIFT CORRER   E INTERAGIR   R CONSUMIR   ESC PAUSAR   F11 TELA CHEIA",1.25f);
}
