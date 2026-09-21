#include "game.hpp"
#include "persistence.hpp"
#include "world.hpp"
#include "computer.hpp"
#include "shop_layout.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Game restore(const Game& g) {
    check(validGame(g,Player{}),"state is saveable");std::ostringstream data;encodeGame(data,g,Player{});
    Game loaded;Player player;std::istringstream in(data.str());check(decodeGame(in,loaded,player),"v9 round trip");return loaded;
}
void collection() {
    Game g;check(g.bagCapacity==3,"three slots available immediately");
    for(int count=1;count<=3;++count)check(g.pickup(2)&&g.heldCount==count&&g.products[2].stock==6-count,"each click adds exactly one");
    check(!g.pickup(2)&&g.heldCount==3&&g.occupied(2)==6,"full bag does not return or duplicate stock");
    check(!g.pickup(1)&&g.products[1].stock==10,"changing product cannot discard cargo");
    g=restore(g);check(g.consume()&&g.heldCount==2,"consumption removes only one");
    check(!g.returnHeld(),"cannot return an item during consumption");g.tick(2);
    check(g.returnHeld()&&g.products[2].stock==5&&g.held==-1,"explicit return conserves remaining stock");
    check(!g.returnHeld(),"return cannot duplicate stock");
    g.cash=10000;g.expand();g.expand();g.products[0].stock=48;
    check(g.pickup(0,true,true)&&g.heldCount==12,"one right click adds one real crate");
    check(g.pickup(0,true,true)&&g.heldCount==24,"successive crate clicks accumulate");
    check(!g.pickup(0)&&g.heldCount==24,"packed and loose modes cannot reinterpret capacity");
}
void mixedSaleAndRefund() {
    Game g;g.customer=true;g.wanted=2;g.quantity=2;g.mixedOrders[0].requested[1]=2;
    int balance=g.cash;
    check(g.pickup(2)&&g.pickup(2)&&g.sell()&&g.customer&&g.cash==balance,"first line never pays a partial order");
    check(g.delivered==2&&g.occupied(2)==6,"completed first line still reserves capacity");
    g=restore(g);check(g.pickup(1)&&g.sell()&&g.customer,"second line can also be partially delivered");
    g=restore(g);check(g.mixedOrders[0].delivered[1]==1,"partial mixed line survives save");
    check(g.pickup(1)&&g.pickup(1)&&g.sell()&&!g.customer&&g.heldCount==1,"complete exactly what is needed and retain excess");
    check(g.cash==balance+2*30+2*12&&g.sold==4,"pay all lines exactly once");
    check(!g.sell()&&g.cash==balance+84,"departing customer cannot be charged twice");
    check(g.customerMotion[0].departure==1&&g.mixedOrders[0].requested[1]==0,"completed customer leaves and clears order");
    Game refund;refund.customer=true;refund.wanted=2;refund.quantity=2;refund.mixedOrders[0].requested[1]=2;
    refund.pickup(2);refund.sell();refund.pickup(1);refund.sell();refund=restore(refund);
    refund.patience=.01f;refund.tick(.05f);
    check(refund.products[2].stock==6&&refund.products[1].stock==10&&refund.sold==0&&refund.cash==250,"timeout returns every product line without payment");
    check(validGame(refund,Player{}),"refunded state remains valid");
    Game bad=refund;bad.mixedOrders[0].requested[1]=1;check(!validGame(bad,Player{}),"inactive checkout cannot retain mixed cargo");
    bad=g;bad.heldCount=4;check(!validGame(bad,Player{}),"overfilled starting bag rejected");
}
void customersAndStaff() {
    Game arriving;arriving.arrival=0;arriving.tickCustomer(.01f,0);
    check(arriving.customer&&arriving.customerMotion[0].approach==1,"customers spawn on a walking route");
    float patience=arriving.patience;int product=arriving.wanted;
    arriving.pickup(product);check(!arriving.sell(),"cannot hand products to a customer still approaching");
    arriving.tickCustomer(2,0);check(arriving.patience==patience,"approach does not use patience");
    arriving=restore(arriving);check(arriving.customerMotion[0].approach>0,"approach is persisted");
    arriving.tickCustomer(3.01f,0);check(arriving.customerMotion[0].approach==0&&arriving.sell(),"delivery works at the counter");
    bool combination=false;
    for(int n=0;n<50;++n) {
        Game spawn;spawn.random.seed(n);spawn.arrival=0;spawn.tickCustomer(.01f,0);
        for(int p=0;p<Game::productCount;++p)if(spawn.mixedOrders[0].requested[p]>0)combination=true;
        check(validGame(spawn,Player{}),"generated combinations are valid");
    }
    check(combination,"mixed orders are available at level one");
    Game worker;worker.cash=10000;worker.hire();worker.customer=true;worker.wanted=2;worker.quantity=2;
    worker.mixedOrders[0].requested[1]=2;worker.products[2].stock=0;
    int balance=worker.cash;
    for(int n=0;n<400&&worker.mixedOrders[0].delivered[1]<2;++n)worker.tick(.05f);
    check(worker.mixedOrders[0].delivered[1]==2&&worker.cash==balance,"attendant fetches an available line when another is out of stock");
    worker=restore(worker);worker.products[2].stock=2;
    for(int n=0;n<800&&worker.customer;++n){worker.tick(.05f);check(validGame(worker,Player{}),"mixed worker route stays valid");}
    check(!worker.customer&&worker.sold==4&&worker.cash==balance+84,"attendant resumes and completes all lines");
    Game lanes;lanes.cash=100000;lanes.expand();lanes.expand();for(int i=0;i<4;++i)lanes.upgradeCheckout();
    for(int lane=0;lane<5;++lane) {
        if(lane==0){lanes.customer=true;lanes.wanted=2;lanes.quantity=1;}
        else {auto& c=lane==1?lanes.second:lanes.extraCheckout(lane);c.customer=true;c.wanted=2;c.quantity=1;}
        lanes.mixedOrders[lane].requested[1]=1;
    }
    balance=lanes.cash;
    for(int lane=4;lane>=0;--lane) {
        check(lanes.pickup(1)&&lanes.deliverPlayer(lane)&&lanes.customerActive(lane),"mixed orders accept either line first in every lane");
        lanes=restore(lanes);check(lanes.pickup(2)&&lanes.deliverPlayer(lane)&&!lanes.customerActive(lane),"independent checkout completion");
    }
    check(lanes.sold==10&&lanes.cash==balance+210&&lanes.products[2].stock==1&&lanes.products[1].stock==5,"five lanes conserve inventory and income");
}
void clockAndMigration() {
    check(daylightAt(30)>.9f&&daylightAt(105)<.1f,"day and night have distinct illumination");
    check(worldClock(0)=="08:00"&&worldClock(90)=="20:00"&&worldClock(180)=="08:00","24-hour world clock wraps with the day");
    Game g;g.time=179.5f;g.tick(1);check(g.day==2&&std::abs(g.time-.5f)<.001f,"day progression retained");
    Game old;old.bagCapacity=1;old.customer=true;old.wanted=0;old.quantity=2;old.pickup(0);old.sell();old.pickup(0);
    std::ostringstream current;encodeGame(current,old,Player{});std::istringstream lines(current.str());
    std::string line;std::ostringstream v8;v8<<"DISTRIBUIDORA_SAVE 8\n";
    for(int i=0;i<25&&std::getline(lines,line);++i)if(i>0)v8<<line<<'\n';
    Game loaded;Player player;std::istringstream input(v8.str());
    check(decodeGame(input,loaded,player)&&loaded.bagCapacity==3&&loaded.delivered==1&&loaded.heldCount==1&&loaded.products[0].stock==16,"legacy saves gain capacity without changing real inventory or orders");
}
void terminalsAndCameras() {
    check(cameraAvailable(0,false,false)&&cameraAvailable(5,false,false),"interior and avenue cameras work from the start");
    check(!cameraAvailable(3,false,false)&&!cameraAvailable(4,true,false),"closed rooms have unavailable cameras");
    check(cameraAvailable(3,true,false)&&cameraAvailable(4,true,true),"expansions enable their own feeds");
    check(!cameraAvailable(-1,true,true)&&!cameraAvailable(6,true,true),"invalid camera IDs are rejected");
    check(nextSecurityCamera(2,1,false,false)==5&&nextSecurityCamera(5,1,false,false)==0,"next camera skips closed rooms and wraps");
    check(nextSecurityCamera(0,-1,false,false)==5&&nextSecurityCamera(5,-1,true,false)==3,"previous camera skips closed rooms");
    for(int i=0;i<securityCameraCount;++i)check(surveillanceHit(25+i*155,480)==i,"mouse selects the displayed camera tile");
    check(surveillanceHit(17,480)==-1&&surveillanceHit(25,457)==-1&&surveillanceHit(170,480)==-1,"outside tiles cannot select a camera");
    check(streetLightIntensity(30)==0&&streetLightIntensity(105)==1,"streetlights are off at noon and on at night");
    check(streetLightIntensity(75)>0&&streetLightIntensity(75)<1,"streetlights fade in at dusk");
    Player p;p.x=3.6f;p.z=.65f;p.yaw=-3.1f;p.pitch=13;
    auto before=p;
    auto start=computerView(p,0,0,80,16.f/9),middle=computerView(p,0,.5f,80,16.f/9),end=computerView(p,0,1,80,16.f/9);
    check(start.x==p.x&&start.z==p.z&&start.yaw==p.yaw&&start.pitch==p.pitch&&start.fov==80,"zoom begins at the actual player view");
    check(end.x==terminals[0].x&&end.y==terminals[0].y&&end.pitch==0&&end.fov==42,"zoom aligns with the monitor screen");
    check(end.z<terminals[0].z&&std::abs(std::remainder(end.yaw-3.14159265f,6.2831853f))<.0001f,"monitor faces the service grade");
    check(middle.z>start.z&&middle.z<end.z&&middle.fov<start.fov&&middle.fov>end.fov,"zoom interpolates camera and field of view");
    check(std::abs(end.yaw-start.yaw)<.1f,"zoom takes the short rotation across angle wrap");
    check(p.x==before.x&&p.z==before.z&&p.yaw==before.yaw&&!p.seated,"visual zoom never moves the saved player");
    p.seated=true;p.x=3.8f;p.z=6.2f;
    auto sofa=computerView(p,1,1,80,16.f/9),portrait=computerView(p,1,1,80,.7f);
    check(sofa.x==terminals[1].x&&sofa.y==terminals[1].y&&portrait.z<sofa.z&&p.seated,"sofa terminal and narrow viewports fit the screen without standing up");
}

}
int main() {
    try {collection();mixedSaleAndRefund();customersAndStaff();clockAndMigration();terminalsAndCameras();}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
    std::cout<<"Incremental collection, mixed orders, walking customers, staff, clocks and save migration passed\n";
}
