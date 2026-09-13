#include "game.hpp"
#include <algorithm>

bool Game::order(int product,int units) {
    if(units==0)units=orderSize;
    if(product<0||product>=availableProducts()){message="PRODUTO AINDA NAO LIBERADO.";return false;}
    if(!allowedLot(units)){message="LOTE AINDA NAO LIBERADO.";return false;}
    if(pending!=-1){message="AGUARDE A ENTREGA ATUAL.";return false;}
    const int price=orderCost(product,units);
    if(cash<price){message="SALDO INSUFICIENTE.";return false;}
    if(occupied(product)+units>products[product].capacity){message="SEM ESPACO PARA ESTE LOTE. VENDA OU REDUZA A QUANTIDADE.";return false;}
    cash-=price;pending=product;pendingUnits=units;delivery=float(deliverySeconds());
    if(delivery==0)receiveDelivery();
    else message="ENCOMENDA PAGA. ENTREGA EM "+std::to_string(deliverySeconds())+" SEGUNDOS.";
    return true;
}
void Game::receiveDelivery() {
    if(pending<0)return;
    products[pending].stock+=pendingUnits;
    message="ENTREGA RECEBIDA: "+std::to_string(pendingUnits)+" UN. DE "+products[pending].name+".";
    pending=-1;pendingUnits=12;delivery=0;++deliveriesReceived;
}
bool Game::upgradeDelivery() {
    if(deliveryLevel==3){message="ENTREGA INSTANTANEA JA LIBERADA.";return false;}
    if(cash<deliveryUpgradeCost()){message="SALDO INSUFICIENTE PARA MELHORAR A ENTREGA.";return false;}
    cash-=deliveryUpgradeCost();++deliveryLevel;
    message="PRAZO DE ENTREGA: "+std::to_string(deliverySeconds())+" SEGUNDOS.";
    if(pending>=0) {
        delivery=std::min(delivery,float(deliverySeconds()));
        if(delivery==0)receiveDelivery();
    }
    return true;
}
bool Game::toggleAutoRestock() {
    autoRestockEnabled=!autoRestockEnabled;restockTimer=0;
    restockStatus=autoRestockEnabled?"VERIFICANDO ESTOQUE. COMPRAS USAM O DINHEIRO DO CAIXA.":"REPOSICAO AUTOMATICA DESLIGADA.";
    message=restockStatus;return true;
}
void Game::cycleRestockThreshold() {
    restockThreshold=restockThreshold==10?25:restockThreshold==25?50:10;
    restockTimer=0;
    message="REPOR QUANDO O ESTOQUE CHEGAR A "+std::to_string(restockThreshold)+"% DA CAPACIDADE.";
}
void Game::cycleRestockReserve() {
    restockReserve=restockReserve==0?250:restockReserve==250?500:restockReserve==500?1000:restockReserve==1000?2500:0;
    restockTimer=0;
    message="COMPRAS AUTOMATICAS PRESERVAM R$ "+std::to_string(restockReserve)+" NO CAIXA.";
}
void Game::tickRestock(float dt) {
    if(!autoRestockEnabled)return;
    restockTimer=std::max(0.f,restockTimer-std::max(0.f,dt));
    if(restockTimer>0)return;
    restockTimer=1.f; // One automatic purchase per check, including instant delivery.
    if(pending>=0){restockStatus="AGUARDANDO A ENTREGA ATUAL. NAO COMPRA EM DUPLICIDADE.";return;}
    int chosen=-1,chosenUnits=0;
    bool lowStock=false;
    for(int offset=0;offset<availableProducts();++offset) {
        int product=(restockCursor+offset)%availableProducts();
        if(products[product].stock*100>products[product].capacity*restockThreshold)continue;
        lowStock=true;
        for(int units:{288,192,96,48,24,12}) {
            if(units>orderSize||!allowedLot(units)||occupied(product)+units>products[product].capacity
                ||orderCost(product,units)>cash-restockReserve)continue;
            // Lowest relative stock first; rotate ties so no product monopolizes purchases.
            if(chosen<0||products[product].stock*products[chosen].capacity<products[chosen].stock*products[product].capacity) {
                chosen=product;chosenUnits=units;
            }
            break;
        }
    }
    if(chosen<0) {
        restockStatus=lowStock?"AGUARDANDO SALDO OU ESPACO PARA UM LOTE. RESERVA: R$ "+std::to_string(restockReserve):"ESTOQUES ACIMA DO LIMIAR. NENHUMA COMPRA NECESSARIA.";
        return;
    }
    if(order(chosen,chosenUnits)) {
        restockCursor=(chosen+1)%availableProducts();
        restockStatus="AUTO: "+std::to_string(chosenUnits)+" UN. DE "+products[chosen].name+" POR R$ "+std::to_string(orderCost(chosen,chosenUnits))+".";
        message=restockStatus;
    }
}
