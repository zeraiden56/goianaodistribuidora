#include "game.hpp"
#include "persistence.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void fill(Game& g){for(int i=0;i<g.availableProducts();++i)g.products[i].stock=g.products[i].capacity;}
Game restore(const Game& g) {
    check(validGame(g,Player{}),"valid state before saving");
    std::ostringstream out;encodeGame(out,g,Player{});std::istringstream in(out.str());Game loaded;Player p;
    check(decodeGame(in,loaded,p),"supplier save round trip");return loaded;
}
void deliveries() {
    Game g;g.cash=10000;check(g.order(0)&&g.delivery==12,"original delivery");
    g.tick(3);check(g.upgradeDelivery()&&g.delivery==6,"shorten paid delivery to six seconds");
    check(g.upgradeDelivery()&&g.delivery==3,"shorten to three seconds");g=restore(g);
    int cash=g.cash;
    check(g.upgradeDelivery()&&g.pending==-1&&g.products[0].stock==30&&g.cash==cash-1200,"instant upgrade receives existing shipment without charging twice");
    cash=g.cash;check(!g.upgradeDelivery()&&g.cash==cash,"maximum upgrade not charged twice");
    check(g.order(0)&&g.products[0].stock==42&&g.cash==cash-48&&g.pending==-1,"instant purchase");
    cash=g.cash;check(!g.order(0)&&g.cash==cash,"instant delivery respects capacity");
    g.tick(12);check(g.products[0].stock==42&&g.deliveriesReceived==2,"no repeated delivery");
    Game poor;check(!poor.upgradeDelivery()&&poor.cash==250,"unaffordable upgrade rejected");
}
void automation() {
    Game g;g.cash=2000;fill(g);g.products[0].stock=0;g.tick(.1f);
    check(g.pending==-1&&g.cash==2000,"automatic spending defaults off");g.toggleAutoRestock();g.tick(.1f);
    check(g.pending==0&&g.pendingUnits==12&&g.cash==1952,"stock threshold triggers paid order");
    for(int i=0;i<20;++i)g.tickRestock(.1f);
    check(g.cash==1952,"no duplicate order while pending");g=restore(g);check(g.autoRestockEnabled,"mode saved");
    g.toggleAutoRestock();g.tick(12);check(g.products[0].stock==12&&g.cash==1952&&g.pending==-1,"disable preserves paid shipment and stops reordering");
    Game instant;instant.cash=2000;instant.deliveryLevel=3;fill(instant);instant.products[0].stock=0;
    instant.toggleAutoRestock();instant.tickRestock(.01f);check(instant.products[0].stock==12,"instant auto shipment");
    for(int i=0;i<10;++i)instant.tickRestock(.01f);
    check(instant.products[0].stock==12&&instant.cash==1952,"instant auto spending rate limited");
    instant.tickRestock(1);check(instant.products[0].stock==24,"replenishes again at threshold");
    int cash=instant.cash;instant.tickRestock(1);check(instant.cash==cash,"stops above threshold");
    Game reserve;reserve.cash=300;reserve.expanded=true;reserve.refreshCapacity();fill(reserve);
    reserve.products[0].stock=0;reserve.orderSize=48;reserve.restockReserve=250;reserve.toggleAutoRestock();reserve.tickRestock(.1f);
    check(reserve.pending==0&&reserve.pendingUnits==12&&reserve.cash==252&&reserve.orderSize==48,"smaller affordable lot preserves reserve and selected size");
    Game blocked;fill(blocked);blocked.products[0].stock=0;blocked.cash=290;blocked.restockReserve=250;
    blocked.toggleAutoRestock();blocked.tickRestock(.1f);check(blocked.pending==-1&&blocked.cash==290,"minimum balance preserved");
    blocked.cash=300;blocked.tickRestock(1);check(blocked.pending==0,"resumes after funds return");
    Game priority;fill(priority);priority.cash=2000;priority.products[0].stock=8;priority.products[1].stock=2;
    priority.toggleAutoRestock();priority.tickRestock(.1f);check(priority.pending==1,"lowest relative stock first");
    Game affordable;fill(affordable);affordable.cash=100;affordable.products[0].stock=4;affordable.products[2].stock=0;
    affordable.toggleAutoRestock();affordable.tickRestock(.1f);check(affordable.pending==0,"expensive product does not block affordable products");
    Game locked;fill(locked);locked.cash=2000;locked.toggleAutoRestock();locked.tickRestock(.1f);
    check(locked.pending==-1&&locked.products[4].stock==0&&locked.products[5].stock==0,"locked products excluded");
    Game fair;fair.cash=10000;fair.deliveryLevel=3;for(int i=0;i<4;++i)fair.products[i].stock=0;
    fair.toggleAutoRestock();for(int i=0;i<4;++i)fair.tickRestock(1);
    for(int i=0;i<4;++i)check(fair.products[i].stock==12,"fair purchasing among empty products");
    Game reserved;reserved.cash=10000;reserved.expanded=reserved.largeStore=reserved.secondCheckout=true;
    reserved.bagCapacity=5;reserved.refreshCapacity();fill(reserved);reserved.products[0].stock=2;
    reserved.held=0;reserved.heldCount=60;reserved.heldPacked=true;
    for(auto* h:{&reserved.helper,&reserved.secondHelper}) {h->hired=h->packed=true;h->held=h->target=0;h->count=60;h->task=HelperTask::Returning;}
    check(validGame(reserved,Player{}),"reserved stock fixture valid");reserved.toggleAutoRestock();reserved.tickRestock(.1f);
    check(reserved.pending==-1&&reserved.cash==10000,"player and worker bags reserve capacity");
    Game bulk;bulk.cash=10000;bulk.expanded=bulk.largeStore=true;bulk.storageLevel=3;bulk.refreshCapacity();fill(bulk);
    bulk.products[0].stock=0;bulk.orderSize=288;bulk.restockReserve=1000;bulk.toggleAutoRestock();bulk.tickRestock(.1f);
    check(bulk.pendingUnits==288&&bulk.cash==9251,"automatic wholesale discount");bulk=restore(bulk);bulk.toggleAutoRestock();bulk.tick(12);
    check(bulk.products[0].stock==288&&bulk.cash==9251,"loaded large shipment delivered once");
}
void migration() {
    Game g;g.cash=3000;g.deliveryLevel=2;g.restockThreshold=50;g.restockReserve=500;g.restockCursor=3;g.restockTimer=.42f;g.autoRestockEnabled=true;g.order(0);
    Game loaded=restore(g);check(loaded.deliveryLevel==2&&loaded.restockThreshold==50&&loaded.restockReserve==500&&loaded.restockCursor==3&&loaded.restockTimer==g.restockTimer,"settings persist");
    for(int threshold:{10,25,50}){loaded.cycleRestockThreshold();check(loaded.restockThreshold==threshold,"threshold cycles");}
    Game controls;for(int amount:{250,500,1000,2500,0}){controls.cycleRestockReserve();check(controls.restockReserve==amount,"reserve cycles");}
    Game legacy;legacy.order(0);std::ostringstream data;encodeGame(data,legacy,Player{});std::istringstream records(data.str());
    std::vector<std::string> lines;std::string line;while(std::getline(records,line))lines.push_back(line);
    std::ostringstream v6;v6<<"DISTRIBUIDORA_SAVE 6\n";for(std::size_t i=1;i<23;++i)v6<<lines[i]<<'\n';
    std::istringstream input(v6.str());Player p;
    check(decodeGame(input,loaded,p)&&!loaded.autoRestockEnabled&&loaded.deliveryLevel==0&&loaded.pending==0&&loaded.delivery==12,"v6 migration retains delivery with automation off");
    Game bad=g;bad.restockThreshold=100;check(!validGame(bad,Player{}),"invalid threshold rejected");
    bad=g;bad.restockReserve=-1;check(!validGame(bad,Player{}),"invalid reserve rejected");
    bad=g;bad.deliveryLevel=3;check(!validGame(bad,Player{}),"instant delivery cannot remain pending");
    bad=g;bad.restockTimer=2;check(!validGame(bad,Player{}),"invalid timer rejected");
}
}
int main() {
    try {deliveries();automation();migration();}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
    std::cout<<"Automatic purchasing, delivery upgrades, budgets and migration passed\n";
}
