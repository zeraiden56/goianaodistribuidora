#include "computer.hpp"
#include "render.hpp"
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
    bevel(72,36,816,442);rect(77,41,806,30,.02f,.05f,.43f);
    text(90,50,"GOIANÃO DISTRIBUIDORA - GERENCIAMENTO",1.8f,1,1,1);
    bevel(850,44,29,24);black(860,51,"X",1.5f);
    black(93,89,"DISPONIVEL",1.2f);black(93,109,"R$ "+std::to_string(g.cash)+",00",2.4f);
    black(440,87,"LUCRO BASE +"+std::to_string(g.marginBonus())+"% - DIA E NIVEL",1.25f);
    black(440,105,"VENDA: "+std::to_string(g.salePrice(0))+" / "+std::to_string(g.salePrice(1))+" / "+std::to_string(g.salePrice(2))+" / "+std::to_string(g.salePrice(3))+" REAIS",1.2f);
    black(440,121,g.pending>=0?"ENTREGA: "+std::to_string(g.pendingUnits)+" UN. EM "+std::to_string(int(std::ceil(g.delivery)))+" S":"ENTREGAS EM 12 S - PAGAMENTO NA HORA",1.1f);
    bevel(91,139,480,222,true);black(103,151,"PRODUTO",1.3f);black(275,151,"ESTOQUE",1.3f);black(414,151,"LOTE "+std::to_string(g.orderSize),1.3f);
    black(590,130,"SERVICOS DA LOJA",1.4f);
    auto buttons=computerButtons();
    for(int i=0;i<int(buttons.size());++i) {
        auto b=buttons[i];bool selected=i==ui.selected,enabled=computerUnavailable(g,b.action).empty();
        bevel(b.x,b.y,b.w,b.h);
        if(selected)rect(b.x+4,b.y+4,b.w-8,b.h-8,.03f,.08f,.39f);
        else if(!enabled)rect(b.x+3,b.y+3,b.w-6,b.h-6,.67f,.67f,.66f);
        float ink=selected?1.f:(enabled?.06f:.35f);
        if(i<4) {
            text(b.x+10,b.y+14,g.products[i].name,1.5f,ink,ink,ink);
            text(275,b.y+14,g.stockLabel(i),1.4f,ink,ink,ink);
            text(414,b.y+14,"R$ "+std::to_string(g.orderCost(i)),1.5f,ink,ink,ink);
        } else {
            std::string label=i==4?"CAMERAS AO VIVO":i==5?"NIVEL - R$ "+std::to_string(g.level*200):
                i==6?(g.expanded?"LOJA AMPLIADA":"AMPLIAR - R$ 600"):
                i==7?(g.secondHelper.hired?"2 ATENDENTES CONTRATADOS":g.helper.hired?"2O ATENDENTE - R$ 350":"ATENDENTE - R$ 350"):
                i==8?"FECHAR":i==9?"LOTE "+std::to_string(g.orderSize)+" UN. - DESCONTO "+(g.orderSize==48?"20":g.orderSize==24?"10":"0")+"% - CLIQUE PARA TROCAR":
                i==10?(g.secondCheckout?"SEGUNDO CAIXA ABERTO":"GRADE + CAIXA 2 - R$ 500"):
                g.bagCapacity==5?"SACOLA MAXIMA: 5 UNIDADES":"SACOLA "+std::to_string(g.bagCapacity+1)+" UN. - R$ "+std::to_string(g.bagCapacity*100);
            text(b.x+12,b.y+(i>=9?8:13),label,i==9?1.15f:i>=10?1.25f:1.45f,ink,ink,ink);
        }
    }
    bevel(91,393,775,27,true);
    std::string notice=computerUnavailable(g,buttons[ui.selected].action);
    if(notice.empty())notice=g.message;
    black(100,402,notice.substr(0,84),1.3f);
    black(97,434,"SETAS OU TAB SELECIONAM - ENTER CONFIRMA",1.25f);black(97,452,"E FECHAR / ESC PAUSAR - O TEMPO CONTINUA",1.1f);
    bevel(0,509,960,31);bevel(8,513,180,23);black(18,521,"GOIANÃO 98",1.3f);
    black(803,521,"22:"+(int(g.time/3)<10?std::string("0"):std::string())+std::to_string(int(g.time/3)),1.4f);
}
