#include "game.hpp"
#include <algorithm>

bool Game::order(int i) {
        if(i<0 || i>3) return false;
        if((orderSize!=12&&orderSize!=24&&orderSize!=48)||(!expanded&&orderSize!=12))return false;
        if(pending!=-1) {message="AGUARDE A ENTREGA ATUAL."; return false;}
        if(cash<orderCost(i)) {message="SALDO INSUFICIENTE."; return false;}
        if(occupied(i)+orderSize>products[i].capacity) {message="SEM ESPACO PARA ESTE LOTE. VENDA OU REDUZA O LOTE."; return false;}
        cash-=orderCost(i); pending=i; pendingUnits=orderSize; delivery=12;
        message="ENCOMENDA PAGA. ENTREGA EM 12 SEGUNDOS."; return true;
    }
bool Game::pickup(int i,bool frontStorage) {
        if(i<0 || i>3) return false;
        if(consumeTime>0) {message="AGUARDE TERMINAR DE CONSUMIR."; return false;}
        if(held==i) {if(frontStorage)touchStorage(i);++products[i].stock; held=-1; message="PRODUTO DEVOLVIDO AO ESTOQUE."; return true;}
        if(held!=-1) {message="MAOS OCUPADAS! ENTREGUE OU DEVOLVA O PRODUTO."; return false;}
        if(products[i].stock==0) {message="SEM ESTOQUE! ENCOMENDE NO COMPUTADOR."; return false;}
        --products[i].stock;if(frontStorage)touchStorage(i); held=i; message="PRODUTO NA MAO. LEVE AO CAIXA OU E PARA DEVOLVER."; return true;
    }
bool Game::sell() {return deliver(held);}
bool Game::deliver(int& carried,int lane) {
        if(lane!=0&&!secondCheckout)return false;
        auto& customer=lane?second.customer:this->customer;
        auto& wanted=lane?second.wanted:this->wanted;
        auto& quantity=lane?second.quantity:this->quantity;
        auto& delivered=lane?second.delivered:this->delivered;
        auto& arrival=lane?second.arrival:this->arrival;
        if(!customer) {message="AGUARDE UM CLIENTE PARA ENTREGAR."; return false;}
        if(carried==-1) {message="PEGUE UMA UNIDADE DO PEDIDO NA PRATELEIRA OU FREEZER."; return false;}
        if(carried!=wanted) {message="PRODUTO ERRADO! DEVOLVA NO LOCAL DE ORIGEM."; return false;}
        carried=-1; ++delivered;
        if(delivered<quantity) {message="UNIDADE ENTREGUE! BUSQUE O RESTANTE DO PEDIDO."; return true;}
        cash+=quantity*salePrice(wanted);
        sold+=quantity; customer=false; delivered=0; arrival=3; reputation=std::min(100,reputation+2);
        message="VENDA CONCLUIDA! DINHEIRO NO CAIXA."; return true;
    }
void Game::tick(float dt) {
        for(auto& door:fridgeTime)door=std::max(0.f,door-dt);
        consumeTime=std::max(0.f,consumeTime-dt);
        smokeTime=std::max(0.f,smokeTime-dt);
        intoxication=std::max(0.f,intoxication-dt*.5f);
        if(pending!=-1) { delivery-=dt; if(delivery<=0) {products[pending].stock+=pendingUnits; pending=-1; delivery=0; message="ENTREGA RECEBIDA: "+std::to_string(pendingUnits)+" UNIDADES NO ESTOQUE.";pendingUnits=12;} }
        time+=dt;
        if(time>=180) {time-=180; ++day; message="NOVO DIA! CONTINUE EXPANDINDO A LOJA.";}
        tickCustomer(dt,0);tickHelper(*this,dt,0);
        if(secondCheckout){tickCustomer(dt,1);tickHelper(*this,dt,1);}
    }
void Game::tickCustomer(float dt,int lane) {
    auto& customer=lane?second.customer:this->customer;
    auto& wanted=lane?second.wanted:this->wanted;
    auto& quantity=lane?second.quantity:this->quantity;
    auto& delivered=lane?second.delivered:this->delivered;
    auto& arrival=lane?second.arrival:this->arrival;
    auto& patience=lane?second.patience:this->patience;
    auto& customerStyle=lane?second.customerStyle:this->customerStyle;
        if(customer) { patience-=dt; if(patience<=0) { patience=0; products[wanted].stock+=delivered; delivered=0; customer=false; arrival=4; reputation=std::max(0,reputation-8); message="CLIENTE DESISTIU. ITENS DO BALCAO VOLTARAM AO ESTOQUE.";} }
        else { arrival-=dt; if(arrival<=0) {arrival=0; customer=true; wanted=int(random()%4); quantity=1+int(random()%std::min(5,level+1)); patience=65;customerStyle=(customerStyle+1+int(random()%(customerLookCount-1)))%customerLookCount;} }

    }
int Game::occupied(int i) const {
    return products[i].stock+(held==i?1:0)+(customer&&wanted==i?delivered:0)+(pending==i?pendingUnits:0)+(helper.held==i?std::max(1,helper.count):0)
        +(second.customer&&second.wanted==i?second.delivered:0)+(secondHelper.held==i?std::max(1,secondHelper.count):0);
}
std::string Game::stockLabel(int i) const {
    return std::to_string(products[i].stock)+"/"+std::to_string(products[i].capacity);
}
bool Game::consume() {
    if(held<0 || consumeTime>0) {message="PEGUE UMA BEBIDA OU CIGARRO PRIMEIRO.";return false;}
    if(held==3) {message="GELO NAO PODE SER CONSUMIDO. LEVE AO CLIENTE.";return false;}
    consuming=held; held=-1; consumeTime=2.f; ++consumed;
    if(consuming==1) {smokeTime=5.f;message="FUMANDO. UMA UNIDADE SAIU DO ESTOQUE.";}
    else {intoxication=std::min(100.f,intoxication+(consuming==0?15.f:35.f));message="BEBENDO. O EFEITO PASSA COM O TEMPO.";}
    return true;
}

void Game::refreshCapacity() {
    constexpr int capacities[]={48,36,24,36};
    for(int i=0;i<4;++i)products[i].capacity=capacities[i]*(expanded?2:1);
}
bool Game::expand() {
    if(expanded){message="A LOJA JA FOI AMPLIADA.";return false;}
    if(cash<expansionCost){message="SALDO INSUFICIENTE PARA AMPLIAR.";return false;}
    cash-=expansionCost;expanded=true;refreshCapacity();
    message="LOJA AMPLIADA! DEPOSITO, SOFA E TV NOS FUNDOS.";return true;
}
bool Game::hire() {
    auto& worker=helper.hired?secondHelper:helper;
    if(worker.hired){message="OS DOIS ATENDENTES JA ESTAO CONTRATADOS.";return false;}
    if(helper.hired&&!secondCheckout){message="MELHORE A GRADE PARA ABRIR O SEGUNDO CAIXA.";return false;}
    if(cash<helperCost){message="SALDO INSUFICIENTE PARA CONTRATAR.";return false;}
    cash-=helperCost;worker.hired=true;
    if(&worker==&secondHelper)worker.x=1.9f;
    message="ATENDENTE CONTRATADO! BUSCA E VENDE NO SEU CAIXA.";return true;
}

void Game::touchStorage(int product) {
    if(product==0||product==2)fridgeTime[product==0?0:1]=1.6f;
}
bool Game::upgradeLevel() {
    if(level>=1000||cash<200*level){message="SALDO INSUFICIENTE OU NIVEL MAXIMO.";return false;}
    cash-=200*level;++level;message="NIVEL MELHORADO! CLIENTES COMPRAM MAIS.";return true;
}

int Game::orderCost(int i) const {
    const int discount=orderSize==48?80:orderSize==24?90:100;
    return (products[i].cost*orderSize*discount+99)/100;
}
int Game::marginBonus() const {return std::min(100,(day-1)*2+(level-1)*5);}
int Game::salePrice(int i) const {
    return products[i].price+((products[i].price-products[i].cost)*marginBonus()+50)/100;
}
bool Game::cycleOrderSize() {
    if(!expanded){message="AMPLIE A LOJA PARA COMPRAR LOTES DE 24 OU 48.";return false;}
    orderSize=orderSize==12?24:orderSize==24?48:12;
    message="LOTE: "+std::to_string(orderSize)+" UNIDADES. DESCONTO: "+(orderSize==48?"20":orderSize==24?"10":"0")+" POR CENTO.";return true;
}
bool Game::upgradeCheckout() {
    if(secondCheckout){message="O SEGUNDO CAIXA JA ESTA ABERTO.";return false;}
    if(!expanded){message="AMPLIE A LOJA ANTES DE MELHORAR A GRADE.";return false;}
    if(cash<checkoutCost){message="SALDO INSUFICIENTE PARA GRADE E CAIXA.";return false;}
    cash-=checkoutCost;secondCheckout=true;message="GRADE AMPLIADA! SEGUNDO CAIXA ABERTO AO LADO.";return true;
}
bool Game::upgradeBag() {
    if(!helper.hired){message="CONTRATE UM ATENDENTE PRIMEIRO.";return false;}
    if(bagCapacity>=5){message="SACOLAS JA CARREGAM 5 UNIDADES.";return false;}
    if(cash<100*bagCapacity){message="SALDO INSUFICIENTE PARA SACOLAS.";return false;}
    cash-=100*bagCapacity;++bagCapacity;
    message="SACOLAS DOS ATENDENTES: "+std::to_string(bagCapacity)+" UNIDADES POR VIAGEM.";return true;
}
