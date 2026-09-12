#include "computer.hpp"
const std::array<ComputerButton,12>& computerButtons() {
    static const std::array<ComputerButton,12> buttons{{
        {96,181,470,37,ComputerAction::Beer},{96,224,470,37,ComputerAction::Cigarettes},
        {96,267,470,37,ComputerAction::Spirits},{96,310,470,37,ComputerAction::Ice},
        {588,151,274,38,ComputerAction::Cameras},{588,199,274,38,ComputerAction::Level},
        {588,247,274,38,ComputerAction::Expansion},{588,295,274,38,ComputerAction::Helper},
        {711,429,151,31,ComputerAction::Close},
        {96,354,470,30,ComputerAction::Lot},{588,337,274,23,ComputerAction::Checkout},
        {588,365,274,23,ComputerAction::Bag}
    }};
    return buttons;
}
int computerHit(int x,int y) {
    if(x>=850&&x<879&&y>=44&&y<68)return 8;
    const auto& buttons=computerButtons();
    for(int i=0;i<int(buttons.size());++i){auto b=buttons[i];if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return i;}
    return -1;
}
std::string computerUnavailable(const Game& g,ComputerAction action) {
    int p=int(action);
    if(p<4) {
        if(g.pending!=-1)return "AGUARDE A ENTREGA ATUAL PARA ENCOMENDAR.";
        if(g.cash<g.orderCost(p))return "SALDO INSUFICIENTE PARA ESTA CAIXA.";
        if(g.occupied(p)+g.orderSize>g.products[p].capacity)return "SEM ESPACO PARA ESTE LOTE. REDUZA A QUANTIDADE.";
    } else if(action==ComputerAction::Level) {
        if(g.level>=1000)return "NIVEL MAXIMO ATINGIDO.";
        if(g.cash<g.level*200)return "SALDO INSUFICIENTE PARA MELHORAR O NIVEL.";
    } else if(action==ComputerAction::Expansion) {
        if(g.expanded)return "A LOJA JA FOI AMPLIADA.";
        if(g.cash<Game::expansionCost)return "SALDO INSUFICIENTE PARA AMPLIAR A LOJA.";
    } else if(action==ComputerAction::Helper) {
        if(g.secondHelper.hired)return "OS DOIS ATENDENTES JA ESTAO CONTRATADOS.";
        if(g.helper.hired&&!g.secondCheckout)return "ABRA O SEGUNDO CAIXA PARA CONTRATAR OUTRO.";
        if(g.cash<Game::helperCost)return "SALDO INSUFICIENTE PARA CONTRATAR.";
    }
    if(action==ComputerAction::Lot&&!g.expanded)return "AMPLIE A LOJA PARA LIBERAR LOTES MAIORES.";
    if(action==ComputerAction::Checkout) {
        if(g.secondCheckout)return "SEGUNDO CAIXA JA ABERTO.";
        if(!g.expanded)return "AMPLIE A LOJA ANTES DE MELHORAR A GRADE.";
        if(g.cash<Game::checkoutCost)return "SALDO INSUFICIENTE PARA GRADE E CAIXA.";
    }
    if(action==ComputerAction::Bag) {
        if(!g.helper.hired)return "CONTRATE UM ATENDENTE PRIMEIRO.";
        if(g.bagCapacity==5)return "SACOLAS NO MAXIMO: 5 UNIDADES.";
        if(g.cash<g.bagCapacity*100)return "SALDO INSUFICIENTE PARA SACOLAS.";
    }
    return {};
}
