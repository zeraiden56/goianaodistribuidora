#include "computer.hpp"
const std::array<ComputerButton,20>& computerButtons() {
    static const std::array<ComputerButton,20> buttons{{
        {96,170,470,29,ComputerAction::Beer},{96,203,470,29,ComputerAction::Cigarettes},
        {96,236,470,29,ComputerAction::Spirits},{96,269,470,29,ComputerAction::Ice},
        {588,151,274,25,ComputerAction::Cameras},{588,180,274,25,ComputerAction::Level},
        {588,209,274,25,ComputerAction::Expansion},{588,238,274,25,ComputerAction::Helper},
        {711,463,151,27,ComputerAction::Close},
        {96,370,470,23,ComputerAction::Lot},{588,267,274,25,ComputerAction::Checkout},
        {588,296,274,25,ComputerAction::Bag},
        {96,302,470,29,ComputerAction::CanBeer},{96,335,470,29,ComputerAction::Soda},
        {588,325,274,25,ComputerAction::Storage},{588,354,274,25,ComputerAction::Speed},
        {96,429,220,27,ComputerAction::AutoRestock},{326,429,230,27,ComputerAction::RestockThreshold},
        {566,429,296,27,ComputerAction::RestockReserve},{96,463,460,27,ComputerAction::Delivery}
    }};
    return buttons;
}
int computerProduct(ComputerAction action) {
    if(int(action)<4)return int(action);
    if(action==ComputerAction::CanBeer)return 4;
    if(action==ComputerAction::Soda)return 5;
    return -1;
}
int computerHit(int x,int y) {
    if(x>=850&&x<879&&y>=44&&y<68)return 8;
    const auto& buttons=computerButtons();
    for(int i=0;i<int(buttons.size());++i){auto b=buttons[i];if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return i;}
    return -1;
}
std::string computerUnavailable(const Game& g,ComputerAction action) {
    int p=computerProduct(action);
    if(p>=0) {
        if(p>=g.availableProducts())return "COMPRE A SEGUNDA AMPLIACAO PARA LIBERAR ESTE PRODUTO.";
        if(g.pending!=-1)return "AGUARDE A ENTREGA ATUAL PARA ENCOMENDAR.";
        if(g.cash<g.orderCost(p))return "SALDO INSUFICIENTE PARA ESTA CAIXA.";
        if(g.occupied(p)+g.orderSize>g.products[p].capacity)return "SEM ESPACO PARA ESTE LOTE. REDUZA A QUANTIDADE.";
    } else if(action==ComputerAction::Level) {
        if(g.level>=1000)return "NIVEL MAXIMO ATINGIDO.";
        if(g.cash<g.level*200)return "SALDO INSUFICIENTE PARA MELHORAR O NIVEL.";
    } else if(action==ComputerAction::Expansion) {
        if(g.largeStore)return "LOJA NO TAMANHO MAXIMO.";
        if(g.cash<g.expansionPrice())return "SALDO INSUFICIENTE PARA AMPLIAR A LOJA.";
    } else if(action==ComputerAction::Helper) {
        if(g.staffCount()>=g.checkoutCount())return "ABRA MAIS UM CAIXA PARA CONTRATAR.";
        if(g.cash<Game::helperCost)return "SALDO INSUFICIENTE PARA CONTRATAR.";
    }
    if(action==ComputerAction::Lot&&!g.expanded)return "AMPLIE A LOJA PARA LIBERAR LOTES MAIORES.";
    if(action==ComputerAction::Checkout) {
        if(g.checkoutCount()==5)return "OS CINCO CAIXAS JA ESTAO ABERTOS.";
        if(g.secondCheckout&&!g.largeStore)return "SEGUNDA AMPLIACAO NECESSARIA PARA O CAIXA 3.";
        if(!g.expanded)return "AMPLIE A LOJA ANTES DE MELHORAR A GRADE.";
        if(g.cash<g.checkoutPrice())return "SALDO INSUFICIENTE PARA GRADE E CAIXA.";
    }
    if(action==ComputerAction::Bag) {
        if(g.bagCapacity==5)return "SACOLAS NO MAXIMO: 5 UNIDADES.";
        if(g.cash<g.bagCapacity*100)return "SALDO INSUFICIENTE PARA SACOLAS.";
    }
    if(action==ComputerAction::Storage) {
        if(g.storageLevel==3)return "CAPACIDADE DE ESTOQUE NO MAXIMO.";
        if(g.cash<250*(g.storageLevel+1))return "SALDO INSUFICIENTE PARA AUMENTAR O ESTOQUE.";
    }
    if(action==ComputerAction::Speed) {
        if(g.staffSpeedLevel==3)return "EQUIPE NA VELOCIDADE MAXIMA.";
        if(g.staffCount()==0)return "CONTRATE UM ATENDENTE PRIMEIRO.";
        if(g.cash<400*(g.staffSpeedLevel+1))return "SALDO INSUFICIENTE PARA TREINAMENTO.";
    }
    if(action==ComputerAction::Delivery) {
        if(g.deliveryLevel==3)return "ENTREGA INSTANTANEA JA LIBERADA.";
        if(g.cash<g.deliveryUpgradeCost())return "SALDO INSUFICIENTE PARA MELHORAR A ENTREGA.";
    }
    return {};
}
