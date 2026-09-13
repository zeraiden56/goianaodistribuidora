#include "game.hpp"
#include <algorithm>
#include <cstdint>
#include "shop_layout.hpp"

bool Game::pickup(int i,bool frontStorage,bool packed) {
        if(i<0 || i>=availableProducts()) {message="PRODUTO LIBERADO NA SEGUNDA AMPLIACAO.";return false;}
        if(consumeTime>0) {message="AGUARDE TERMINAR DE CONSUMIR."; return false;}
        if(held==i) {if(frontStorage)touchStorage(i);products[i].stock+=heldCount; held=-1;heldCount=1;heldPacked=false; message="PRODUTO DEVOLVIDO AO ESTOQUE."; return true;}
        if(held!=-1) {message="MAOS OCUPADAS! ENTREGUE OU DEVOLVA O PRODUTO."; return false;}
        if(products[i].stock==0) {message="SEM ESTOQUE! ENCOMENDE NO COMPUTADOR."; return false;}
        if(packed&&(!largeStore||products[i].stock<crateUnits)){message="AMPLIE A LOJA E TENHA 12 UNIDADES PARA PEGAR ENGRADADOS.";return false;}
        heldPacked=packed;
        heldCount=packed?std::min(bagCapacity,products[i].stock/crateUnits)*crateUnits:std::min(bagCapacity,products[i].stock);products[i].stock-=heldCount;if(frontStorage)touchStorage(i); held=i; message=carriedLabel(heldCount,heldPacked)+" EM MAOS. LEVE AO CAIXA OU DEVOLVA NA ORIGEM."; return true;
    }
bool Game::sell() {return deliverPlayer();}
bool Game::deliverPlayer(int lane) {
    if(consumeTime>0){message="AGUARDE TERMINAR DE CONSUMIR.";return false;}
    bool deliveredAny=false;
    while(held>=0) {
        int unit=held;
        if(!deliver(unit,lane))break;
        deliveredAny=true;
        if(--heldCount==0){held=-1;heldCount=1;heldPacked=false;break;}
        if(!(lane>=2?extraCheckout(lane).customer:lane?second.customer:customer))break;
    }
    return deliveredAny;
}
bool Game::deliver(int& carried,int lane) {
        if(lane<0||lane>=checkoutCount())return false;
        if(&carried==&held)return deliverPlayer(lane);
        auto& customer=lane>=2?extraCheckout(lane).customer:lane?second.customer:this->customer;
        auto& wanted=lane>=2?extraCheckout(lane).wanted:lane?second.wanted:this->wanted;
        auto& quantity=lane>=2?extraCheckout(lane).quantity:lane?second.quantity:this->quantity;
        auto& delivered=lane>=2?extraCheckout(lane).delivered:lane?second.delivered:this->delivered;
        auto& arrival=lane>=2?extraCheckout(lane).arrival:lane?second.arrival:this->arrival;
        if(!customer) {message="AGUARDE UM CLIENTE PARA ENTREGAR."; return false;}
        if(carried==-1) {message="PEGUE UMA UNIDADE DO PEDIDO NA PRATELEIRA OU FREEZER."; return false;}
        if(carried!=wanted) {message="PRODUTO ERRADO! DEVOLVA NO LOCAL DE ORIGEM."; return false;}
        carried=-1; ++delivered;
        if(delivered<quantity) {message="UNIDADE ENTREGUE! BUSQUE O RESTANTE DO PEDIDO."; return true;}
        cash=int(std::min<std::int64_t>(2000000000LL,std::int64_t(cash)+std::int64_t(quantity)*salePrice(wanted)));
        sold+=quantity; customer=false; delivered=0; arrival=3; reputation=std::min(100,reputation+2);
        message="VENDA CONCLUIDA! DINHEIRO NO CAIXA."; return true;
    }
void Game::tick(float dt) {
        for(auto& door:fridgeTime)door=std::max(0.f,door-dt);
        consumeTime=std::max(0.f,consumeTime-dt);
        smokeTime=std::max(0.f,smokeTime-dt);
        intoxication=std::max(0.f,intoxication-dt*.5f);
        if(pending!=-1) {delivery=std::max(0.f,delivery-dt);if(delivery==0)receiveDelivery();}
        time+=dt;
        if(time>=180) {time-=180; ++day; message="NOVO DIA! CONTINUE EXPANDINDO A LOJA.";}
        tickCustomer(dt,0);tickHelper(*this,dt,0);
        for(int lane=1;lane<checkoutCount();++lane){tickCustomer(dt,lane);tickHelper(*this,dt,lane);}
        tickRestock(dt);
    }
void Game::tickCustomer(float dt,int lane) {
    auto& customer=lane>=2?extraCheckout(lane).customer:lane?second.customer:this->customer;
    auto& wanted=lane>=2?extraCheckout(lane).wanted:lane?second.wanted:this->wanted;
    auto& quantity=lane>=2?extraCheckout(lane).quantity:lane?second.quantity:this->quantity;
    auto& delivered=lane>=2?extraCheckout(lane).delivered:lane?second.delivered:this->delivered;
    auto& arrival=lane>=2?extraCheckout(lane).arrival:lane?second.arrival:this->arrival;
    auto& patience=lane>=2?extraCheckout(lane).patience:lane?second.patience:this->patience;
    auto& customerStyle=lane>=2?extraCheckout(lane).customerStyle:lane?second.customerStyle:this->customerStyle;
    auto& wholesale=lane>=2?extraCheckout(lane).wholesale:lane?second.wholesale:this->wholesale;
        if(customer) { patience-=dt; if(patience<=0) { patience=0; products[wanted].stock+=delivered; delivered=0; customer=false; arrival=4; reputation=std::max(0,reputation-8); message="CLIENTE DESISTIU. ITENS DO BALCAO VOLTARAM AO ESTOQUE.";} }
        else { arrival-=dt; if(arrival<=0) {
            arrival=0;customer=true;
            wholesale=largeStore&&(lane>=3||(level>=3&&random()%3==0));
            if(wholesale) {
                constexpr int drinks[]={0,2,4,5};wanted=drinks[random()%4];
                int maxCrates=std::min(5,2+(level-1)/2+(day-1)/3);
                quantity=(1+int(random()%maxCrates))*crateUnits;patience=180;
            } else {wanted=int(random()%availableProducts());quantity=1+int(random()%std::min(5,level+1));patience=65;}
            customerStyle=(customerStyle+1+int(random()%(customerLookCount-1)))%customerLookCount;
        } }

    }
int Game::occupied(int i) const {
    return products[i].stock+(held==i?heldCount:0)+(customer&&wanted==i?delivered:0)+(pending==i?pendingUnits:0)+(helper.held==i?std::max(1,helper.count):0)
        +(second.customer&&second.wanted==i?second.delivered:0)+(secondHelper.held==i?secondHelper.count:0)
        +(third.customer&&third.wanted==i?third.delivered:0)+(thirdHelper.held==i?thirdHelper.count:0)
        +(annex[0].customer&&annex[0].wanted==i?annex[0].delivered:0)+(annex[1].customer&&annex[1].wanted==i?annex[1].delivered:0)
        +(annexHelpers[0].held==i?annexHelpers[0].count:0)+(annexHelpers[1].held==i?annexHelpers[1].count:0);
}
std::string Game::stockLabel(int i) const {
    return std::to_string(products[i].stock)+"/"+std::to_string(products[i].capacity);
}
bool Game::consume() {
    if(held<0 || consumeTime>0) {message="PEGUE UMA BEBIDA OU CIGARRO PRIMEIRO.";return false;}
    if(held==3) {message="GELO NAO PODE SER CONSUMIDO. LEVE AO CLIENTE.";return false;}
    consuming=held;if(--heldCount==0){held=-1;heldCount=1;heldPacked=false;} consumeTime=2.f; ++consumed;
    if(consuming==1) {smokeTime=5.f;message="FUMANDO. UMA UNIDADE SAIU DO ESTOQUE.";}
    else {intoxication=std::min(100.f,intoxication+(consuming==5?0.f:consuming==2?35.f:15.f));message="BEBENDO. O EFEITO PASSA COM O TEMPO.";}
    return true;
}

int Game::capacityFor(int i) const {
    constexpr int base[]={48,36,24,36,48,48};
    return base[i]*((largeStore?4:expanded?2:1)+storageLevel);
}
void Game::refreshCapacity() {
    for(int i=0;i<productCount;++i)products[i].capacity=capacityFor(i);
}
bool Game::expand() {
    if(largeStore){message="LOJA NO TAMANHO MAXIMO.";return false;}
    if(cash<expansionPrice()){message="SALDO INSUFICIENTE PARA AMPLIAR.";return false;}
    cash-=expansionPrice();
    if(expanded)largeStore=true;else expanded=true;
    refreshCapacity();
    message=largeStore?"NOVA ALA ABERTA! ENCOMENDE LATAS E REFRIGERANTES.":"LOJA AMPLIADA! DEPOSITO, SOFA E TV NOS FUNDOS.";return true;
}
bool Game::upgradeStorage() {
    if(storageLevel==3){message="PRATELEIRAS DE ESTOQUE NO MAXIMO.";return false;}
    if(cash<250*(storageLevel+1)){message="SALDO INSUFICIENTE PARA ESTOQUE.";return false;}
    cash-=250*(storageLevel+1);++storageLevel;refreshCapacity();
    message="CAPACIDADE AUMENTADA! ENCOMENDE PARA REPOR OS PRODUTOS.";return true;
}
int Game::staffCount() const {
    int count=0;for(int lane=0;lane<5;++lane)count+=staff(lane).hired?1:0;return count;
}
bool Game::hire() {
    int lane=staffCount();
    if(lane>=checkoutCount()){message="ABRA MAIS UM CAIXA PARA CONTRATAR.";return false;}
    if(cash<helperCost){message="SALDO INSUFICIENTE PARA CONTRATAR.";return false;}
    cash-=helperCost;auto& worker=staff(lane);worker.hired=true;worker.x=checkoutX(lane);worker.z=-.9f;
    message="ATENDENTE "+std::to_string(lane+1)+" CONTRATADO NO SEU CAIXA.";return true;
}
bool Game::upgradeStaffSpeed() {
    if(staffSpeedLevel==3){message="EQUIPE NA VELOCIDADE MAXIMA.";return false;}
    if(staffCount()==0){message="CONTRATE UM ATENDENTE PRIMEIRO.";return false;}
    if(cash<400*(staffSpeedLevel+1)){message="SALDO INSUFICIENTE PARA TREINAMENTO.";return false;}
    cash-=400*(staffSpeedLevel+1);++staffSpeedLevel;message="EQUIPE MAIS RAPIDA! VALE PARA TODOS OS ATENDENTES.";return true;
}

void Game::touchStorage(int product) {
    if(product==0||product==2||product>=4)fridgeTime[product==0?0:product==2?1:product-2]=1.6f;
}
bool Game::upgradeLevel() {
    if(level>=1000||cash<200*level){message="SALDO INSUFICIENTE OU NIVEL MAXIMO.";return false;}
    cash-=200*level;++level;message="NIVEL MELHORADO! CLIENTES COMPRAM MAIS.";return true;
}

bool Game::allowedLot(int units) const {
    return units==12||(expanded&&(units==24||units==48))||(largeStore&&(units==96||units==192||units==288));
}
int Game::lotDiscount(int units) const {
    if(units==0)units=orderSize;
    return units==288?35:units==192?30:units==96?25:units==48?20:units==24?10:0;
}
int Game::orderCost(int i,int units) const {
    if(units==0)units=orderSize;
    return (products[i].cost*units*(100-lotDiscount(units))+99)/100;
}
int Game::marginBonus() const {return (day-1)*2+(level-1)*5;}
int Game::salePrice(int i) const {
    return products[i].price+((products[i].price-products[i].cost)*marginBonus()+50)/100;
}
bool Game::cycleOrderSize() {
    if(!expanded){message="AMPLIE A LOJA PARA COMPRAR LOTES DE 24 OU 48.";return false;}
    orderSize=orderSize==12?24:orderSize==24?48:orderSize==48&&largeStore?96:orderSize==96?192:orderSize==192?288:12;
    message="LOTE: "+std::to_string(orderSize)+" UNIDADES. DESCONTO: "+std::to_string(lotDiscount())+" POR CENTO.";return true;
}
bool Game::upgradeCheckout() {
    if(checkoutCount()==5){message="OS CINCO CAIXAS JA ESTAO ABERTOS.";return false;}
    if(secondCheckout&&!largeStore){message="COMPRE A SEGUNDA AMPLIACAO PARA O TERCEIRO CAIXA.";return false;}
    if(!expanded){message="AMPLIE A LOJA ANTES DE MELHORAR A GRADE.";return false;}
    if(cash<checkoutPrice()){message="SALDO INSUFICIENTE PARA GRADE E CAIXA.";return false;}
    cash-=checkoutPrice();if(thirdCheckout)++annexCheckouts;else if(secondCheckout)thirdCheckout=true;else secondCheckout=true;message="GRADE AMPLIADA! NOVO CAIXA ABERTO.";return true;
}
bool Game::upgradeBag() {
    if(bagCapacity>=5){message="SACOLAS JA CARREGAM 5 UNIDADES.";return false;}
    if(cash<100*bagCapacity){message="SALDO INSUFICIENTE PARA SACOLAS.";return false;}
    cash-=100*bagCapacity;++bagCapacity;
    message="SACOLA DO JOGADOR E ATENDENTES: "+std::to_string(bagCapacity)+" UNIDADES POR VIAGEM.";return true;
}
