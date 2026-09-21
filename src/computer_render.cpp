#include "computer.hpp"
#include "render.hpp"
#include "controller_render.hpp"
#include "world.hpp"
#include <cmath>
namespace {
void bevel(float x,float y,float w,float h,bool sunken=false) {
    rect(x,y,w,h,.75f,.75f,.73f);
    float light=sunken?.35f:.98f,dark=sunken?.98f:.28f;
    rect(x,y,w,2,light,light,light);rect(x,y,2,h,light,light,light);
    rect(x,y+h-2,w,2,dark,dark,dark);rect(x+w-2,y,2,h,dark,dark,dark);
}
void black(float x,float y,const std::string& label,float size=1.5f){text(x,y,label,size,.06f,.06f,.08f);}
}
void renderComputer(const Game& g,const ComputerUI& ui) {
    rect(0,0,960,540,0,.38f,.39f);
    bevel(72,36,816,465);rect(77,41,806,30,.02f,.05f,.43f);
    text(90,50,"GOIANÃO DISTRIBUIDORA - GERENCIAMENTO",1.8f,1,1,1);
    bevel(850,44,29,24);black(860,51,"X",1.5f);
    black(93,89,"DISPONIVEL",1.2f);black(93,109,"R$ "+std::to_string(g.cash)+",00",2.4f);
    black(440,87,"LUCRO BASE +"+std::to_string(g.marginBonus())+"% - DIA E NIVEL",1.25f);
    black(440,105,"SACOLA JOGADOR E EQUIPE: "+std::to_string(g.bagCapacity)+" UN. OU CX DE 12",1.2f);
    black(440,121,g.pending>=0?"ENTREGA: "+std::to_string(g.pendingUnits)+" UN. EM "+std::to_string(int(std::ceil(g.delivery)))+" S":g.deliverySeconds()==0?"ENTREGA INSTANTANEA - PAGAMENTO NA HORA":"ENTREGAS EM "+std::to_string(g.deliverySeconds())+" S - PAGAMENTO NA HORA",1.1f);
    bevel(91,139,480,224,true);black(103,151,"PRODUTO",1.3f);black(275,151,"ESTOQUE",1.2f);black(359,151,"VENDA",1.2f);black(436,151,"LOTE "+std::to_string(g.orderSize),1.2f);
    black(590,130,"SERVICOS DA LOJA",1.4f);
    auto buttons=computerButtons();
    for(int i=0;i<int(buttons.size());++i) {
        auto b=buttons[i];bool selected=i==ui.selected,enabled=computerUnavailable(g,b.action).empty();
        bevel(b.x,b.y,b.w,b.h);
        if(selected)rect(b.x+4,b.y+4,b.w-8,b.h-8,.03f,.08f,.39f);
        else if(!enabled)rect(b.x+3,b.y+3,b.w-6,b.h-6,.67f,.67f,.66f);
        float ink=selected?1.f:(enabled?.06f:.35f);
        int product=computerProduct(b.action);
        if(product>=0) {
            text(b.x+10,b.y+10,g.products[product].name,1.35f,ink,ink,ink);
            text(275,b.y+10,product>=g.availableProducts()?"TRAVADO":g.stockLabel(product),1.15f,ink,ink,ink);
            text(359,b.y+10,"R$ "+std::to_string(g.salePrice(product)),1.1f,ink,ink,ink);
            text(436,b.y+10,"R$ "+std::to_string(g.orderCost(product)),1.25f,ink,ink,ink);
        } else {
            int workers=g.staffCount();
            std::string label=i==4?"VIGILANCIA - 6 CAMERAS":i==5?"NIVEL - R$ "+std::to_string(g.level*200):
                i==6?(g.largeStore?"LOJA NO TAMANHO MAXIMO":"AMPLIAR "+std::to_string(g.expanded?2:1)+" - R$ "+std::to_string(g.expansionPrice())):
                i==7?(workers==5?"5 ATENDENTES CONTRATADOS":"ATENDENTE "+std::to_string(workers+1)+" - R$ 350"):
                i==8?"FECHAR":i==9?"LOTE "+std::to_string(g.orderSize)+" UN. - DESCONTO "+std::to_string(g.lotDiscount())+"% - CLIQUE PARA TROCAR":
                i==10?(g.checkoutCount()==5?"5 CAIXAS ABERTOS":"CAIXA "+std::to_string(g.checkoutCount()+1)+" + GRADE - R$ "+std::to_string(g.checkoutPrice())):
                i==11?(g.bagCapacity==5?"SACOLAS MAXIMAS: 5 UN.":"SACOLA "+std::to_string(g.bagCapacity+1)+" UN. - R$ "+std::to_string(g.bagCapacity*100)):
                i==14?(g.storageLevel==3?"ESTOQUE NO MAXIMO":"ESTOQUE + - R$ "+std::to_string(250*(g.storageLevel+1))):
                i==15?(g.staffSpeedLevel==3?"CORRIDA DA EQUIPE MAXIMA":"CORRIDA "+std::to_string(g.staffSpeedLevel+1)+" - R$ "+std::to_string(400*(g.staffSpeedLevel+1))):
                i==16?(g.autoRestockEnabled?"REPOSICAO AUTO: LIGADA":"REPOSICAO AUTO: DESLIGADA"):
                i==17?"REPOR COM ESTOQUE ATE "+std::to_string(g.restockThreshold)+"%":
                i==18?"RESERVAR NO CAIXA: R$ "+std::to_string(g.restockReserve):
                g.deliveryLevel==3?"ENTREGA INSTANTANEA ATIVA":"ENTREGA "+std::to_string(g.deliveryLevel==0?6:g.deliveryLevel==1?3:0)+" S - R$ "+std::to_string(g.deliveryUpgradeCost());
            text(b.x+10,b.y+9,label,i==9?1.1f:1.25f,ink,ink,ink);
        }
    }
    bevel(91,393,775,27,true);
    std::string notice=computerUnavailable(g,buttons[ui.selected].action);
    if(notice.empty())notice=ui.selected>=16&&ui.selected<=18?g.restockStatus:g.message;
    black(100,402,notice.substr(0,84),1.3f);
    if(controllerPrompts()) {
        padHint(575,465,PadIcon::Dpad,"SELECIONAR");padHint(750,465,PadIcon::South,"CONFIRMAR");
        padHint(575,487,PadIcon::East,"FECHAR");
    } else {black(575,468,"TAB / ENTER",1.f);black(575,482,"E FECHAR",1.f);}
    bevel(0,509,960,31);bevel(8,513,180,23);black(18,521,"GOIANÃO 98",1.3f);
    black(803,521,worldClock(g.time),1.4f);
}
