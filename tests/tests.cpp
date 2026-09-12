#include "game.hpp"
#include "player.hpp"
#include "world.hpp"
#include "computer.hpp"
#include "shop_layout.hpp"
#include <cmath>
#include "persistence.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

void economyTests() {
    Game g;
    auto check=[](bool ok){if(!ok) {std::cerr<<"Economy test failed\n";std::exit(1);}};
    check(!g.sell()); check(g.order(0));check(g.cash==202);check(!g.order(1));g.tick(12);check(g.products[0].stock==30&&g.pending==-1);
    g.customer=true;g.wanted=0;g.quantity=2;check(!g.sell());
    check(g.pickup(1));check(!g.sell());check(!g.pickup(0));check(g.pickup(1));check(g.products[1].stock==10);
    check(g.pickup(0));check(g.sell());check(g.cash==202&&g.delivered==1&&g.customer);
    check(g.pickup(0));check(g.sell());check(g.cash==218&&g.products[0].stock==28&&g.sold==2&&g.held==-1);
    g.customer=true;g.patience=65;check(g.pickup(0));check(g.sell());check(g.pickup(0));
    g.tick(70);check(!g.customer&&g.reputation==92&&g.delivered==0&&g.products[0].stock==27&&g.held==0);
    check(g.pickup(0));check(g.products[0].stock==28);
    g.products[0].stock=0;check(!g.pickup(0));check(!g.pickup(-1));
    g.cash=0;check(!g.order(3));check(!g.order(-1));
    check(interaction(-2.1f,-1.7f,0)==5);check(interaction(.65f,-1.6f,0)==1);
    check(interaction(-.65f,2.4f,3.14159265f)==2);
    check(interaction(-2.4f,.4f,-3.14159265f/2)==3);
    std::cout<<"Economy and interaction tests passed\n";
}

void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void persistenceTests(const std::filesystem::path& directory) {
    Game g;Player p;std::string error;
    g.customer=true;g.wanted=0;g.quantity=3;g.pickup(0);g.sell();g.pickup(0);
    check(g.order(2),"order while holding");g.tick(3);g.cash=923;g.level=3;g.day=8;g.reinforced=true;
    p.x=-2.1f;p.z=-1.3f;p.yaw=.4f;p.pitch=-12;
    auto file=directory/"progress.save";
    check(saveGame(file,g,p,error),"save partial order");
    Game loaded;Player position;
    check(loadGame(file,loaded,position,error),"load partial order");
    std::ostringstream before,after;encodeGame(before,g,p);encodeGame(after,loaded,position);
    check(before.str()==after.str(),"save round trip preserves all fields and RNG");
    check(loaded.sell()&&loaded.delivered==2,"resume partial order");
    check(loaded.pickup(0)&&loaded.sell()&&!loaded.customer,"complete resumed order");
    loaded.tick(9);check(loaded.pending==-1&&loaded.products[2].stock==18,"resume delivery");
    g.held=-1;g.consumeTime=0;check(g.pickup(1)&&g.consume(),"smoke item");
    check(saveGame(file,g,p,error),"save consumption");check(loadGame(file,loaded,position,error),"load consumption");
    check(loaded.consuming==1&&loaded.consumeTime==2&&loaded.smokeTime==5,"consumption persisted");
    auto preserved=loaded.cash;
    std::ofstream(file)<<"DISTRIBUIDORA_SAVE 1\n99999";
    check(!loadGame(file,loaded,position,error)&&loaded.cash==preserved,"truncated save does not mutate state");
    std::istringstream wrongVersion("DISTRIBUIDORA_SAVE 99\n");check(!decodeGame(wrongVersion,loaded,position),"reject version");
    Game invalid=g;invalid.products[0].stock=100000;
    std::ostringstream invalidData;encodeGame(invalidData,invalid,p);
    std::istringstream invalidInput(invalidData.str());check(!decodeGame(invalidInput,loaded,position),"reject over capacity");
    check(saveGame(file,g,p,error),"restore valid save");
    check(!saveGame(file,invalid,p,error),"reject invalid state before overwriting");
    check(loadGame(file,loaded,position,error),"previous save survives rejected write");
    check(!saveGame(directory/"progress.save"/"blocked.save",g,p,error),"write failure reported");
    Settings settings;settings.fullscreen=true;settings.vsync=false;settings.resolution=3;settings.quality=2;settings.fov=100;settings.volume=30;
    check(saveSettings(directory/"options.cfg",settings,error),"save settings");Settings restored;
    check(loadSettings(directory/"options.cfg",restored),"load settings");
    check(restored.fullscreen&&!restored.vsync&&restored.resolution==3&&restored.quality==2&&restored.fov==100&&restored.volume==30,"settings round trip");
    std::ofstream(directory/"options.cfg")<<"DISTRIBUIDORA_OPTIONS 1\n1 1 100 2 90\n";
    check(!loadSettings(directory/"options.cfg",restored)&&restored.resolution==3,"reject invalid settings without mutation");
    std::ofstream(directory/"options.cfg")<<"DISTRIBUIDORA_OPTIONS 1\n0 1 2 1 80\n";
    check(loadSettings(directory/"options.cfg",restored)&&restored.volume==70&&restored.resolution==2,"legacy options keep graphics and default volume");
    std::ofstream(directory/"options.cfg")<<"DISTRIBUIDORA_OPTIONS 2\n0 1 2 1 80 101\n";
    check(!loadSettings(directory/"options.cfg",restored)&&restored.volume==70,"invalid volume does not mutate settings");
    std::cout<<"Persistence tests passed\n";
}
void capacityAndConsumptionTests() {
    Game g;g.cash=10000;g.products[0].stock=48;
    check(!g.order(0),"full shelf rejects order");
    check(g.pickup(0)&&g.products[0].stock==47,"pickup from full shelf");
    check(!g.order(0),"held item reserves storage");
    check(g.pickup(0)&&g.products[0].stock==48,"return to full shelf");
    g.products[0].stock=36;check(g.order(0),"exact capacity order");g.tick(12);check(g.products[0].stock==48,"exact capacity delivery");
    g.products[0].stock=36;g.pickup(0);g.customer=true;g.wanted=0;g.quantity=2;g.sell();
    check(g.order(0),"partial delivery reserves capacity");g.tick(70);
    check(g.products[0].stock==48,"expiry and inbound delivery preserve capacity");
    Game drinks;check(!drinks.consume(),"empty hands cannot consume");
    drinks.pickup(3);check(!drinks.consume()&&drinks.held==3,"ice cannot be consumed");drinks.pickup(3);
    drinks.pickup(0);check(drinks.consume(),"drink beer");
    check(drinks.products[0].stock==17&&drinks.held==-1&&drinks.consumed==1&&drinks.intoxication==15,"consumption counted once");
    check(!drinks.consume()&&!drinks.pickup(2),"no duplicate consumption or pickup during animation");
    drinks.tick(2);drinks.pickup(2);check(drinks.consume()&&drinks.intoxication==49,"spirits effect");
    drinks.tick(2);drinks.pickup(1);check(drinks.consume()&&drinks.smokeTime==5,"smoke effect");
    drinks.tick(120);check(drinks.smokeTime==0&&drinks.consumeTime==0&&drinks.intoxication==0,"effects expire");
    check(validGame(drinks,Player{}),"expired timers remain saveable");
    std::cout<<"Capacity and consumption tests passed\n";
}
void movementAndWorldTests() {
    Player walk,run,diagonal;
    float a=movePlayer(walk,1,0,false,0,.05f),b=movePlayer(run,1,0,true,0,.05f);
    float c=movePlayer(diagonal,1,1,true,0,.05f);
    check(b>a&&std::abs(b/c-1)<.0001f,"sprint is faster and diagonal speed is normalized");
    Player blocked;blocked.z=-1.77f;
    check(movePlayer(blocked,1,0,true,0,.05f)==0&&walkable(blocked.x,blocked.z),"sprint cannot cross counter");
    Player drunk;
    check(movePlayer(drunk,1,0,true,100,.05f)<b,"intoxication slows sprint");
    check(visionBlur(0)==0&&visionBlur(25)<visionBlur(50)&&visionBlur(50)<visionBlur(100),"blur increases with intoxication");
    auto first=trafficAt(2),later=trafficAt(3),repeat=trafficAt(2);
    for(int i=0;i<6;++i) {
        check(first[i].x==repeat[i].x,"traffic deterministic for saved clock");
        check(later[i].x!=first[i].x,"traffic moves");
        check(std::remainder(later[i].x-first[i].x,54.f)*first[i].speed>0,"traffic follows lane direction");
    }
    check(first[0].motorcycle!=first[1].motorcycle,"cars and motorcycles are present");
    for(auto vehicle:trafficAt(180000000.0))check(vehicle.x>=-27&&vehicle.x<=27,"traffic remains within loop");
    check(securityCameras().size()==3,"interior and street cameras available");
    std::cout<<"Movement, vision and traffic tests passed\n";
}
void expansionAndHelperTests(const std::filesystem::path& directory) {
    Game g;Player p;std::string error;
    check(!g.expand()&&!g.hire(),"upgrades require funds");
    g.cash=2000;auto original=g.products;
    check(g.expand()&&g.cash==1400&&g.expanded,"buy physical expansion");
    for(int i=0;i<4;++i)check(g.products[i].stock==original[i].stock&&g.products[i].capacity==original[i].capacity*2,"expansion increases capacity without free stock");
    check(!g.expand()&&g.cash==1400,"cannot buy expansion twice");
    check(g.hire()&&g.cash==1050&&!g.hire(),"hire once");
    check(!walkable(2.7f,4.1f)&&walkable(2.7f,4.1f,true),"expansion opens doorway");
    check(!walkable(0,4,true)&&!walkable(3.8f,6.2f,true),"partition and sofa block walking");
    p.x=2.1f;p.z=2.3f;p.yaw=3.14159265f;
    for(int i=0;i<26;++i)movePlayer(p,1,0,false,0,.05f,true);
    check(p.z>5,"can walk from shop into back room");
    p.x=2.65f;p.z=6.2f;
    check(toggleSeat(p,true)&&p.seated&&movePlayer(p,1,0,true,0,.05f,true)==0,"sitting prevents walking");
    g.tvOn=true;
    check(saveGame(directory/"expanded.save",g,p,error),"save seated in expanded room");
    Game loaded;Player position;
    check(loadGame(directory/"expanded.save",loaded,position,error)&&position.seated&&loaded.expanded&&loaded.tvOn&&loaded.helper.hired,"restore upgrades, helper, seat and TV");
    check(toggleSeat(position,true)&&walkable(position.x,position.z,true),"stand in safe location");
    check(interaction(-3.6f,7.8f,3.14159265f,true)==8,"depot stock is interactable");
    for(int product=0;product<4;++product) {
        Game worker;worker.cash=1000;worker.hire();worker.customer=true;worker.wanted=product;worker.quantity=3;
        int cash=worker.cash,stock=worker.products[product].stock;
        for(int frame=0;frame<1250&&worker.customer;++frame){worker.tick(.05f);check(walkable(worker.helper.x,worker.helper.z),"helper stays in walkable space");}
        check(worker.sold==3&&worker.cash==cash+3*worker.products[product].price,"helper fetches and sells full order");
        check(worker.products[product].stock==stock-3&&worker.helper.held==-1,"helper sales conserve stock");
    }
    Game partial;partial.cash=1000;partial.hire();partial.customer=true;partial.wanted=0;partial.quantity=2;
    for(int i=0;i<500&&partial.helper.held<0;++i)partial.tick(.05f);
    check(partial.helper.held==0&&partial.occupied(0)==18,"helper-held stock remains reserved");
    check(saveGame(directory/"worker.save",partial,Player{},error),"save helper carrying stock");
    check(loadGame(directory/"worker.save",loaded,position,error)&&loaded.helper.held==0,"load helper carrying stock");
    for(int i=0;i<1000&&loaded.customer;++i)loaded.tick(.05f);
    check(loaded.sold==2,"helper resumes delivery after load");
    partial.patience=.01f;partial.tick(.05f);partial.arrival=4;
    for(int i=0;i<500&&partial.helper.held>=0;++i){partial.arrival=4;partial.tick(.05f);}
    check(partial.helper.held==-1&&partial.products[0].stock==18&&partial.sold==0,"expired customer returns helper-held stock without payment");
    Game race;race.cash=1000;race.hire();race.customer=true;race.wanted=1;race.quantity=1;
    for(int i=0;i<500&&race.helper.held<0;++i)race.tick(.05f);
    check(race.pickup(1)&&race.sell(),"player can finish while helper carries another unit");
    for(int i=0;i<500&&race.helper.held>=0;++i){race.arrival=4;race.tick(.05f);}
    check(race.sold==1&&race.products[1].stock==9,"no double sale when player and helper cooperate");
    Game legacy;legacy.products[0].stock=7;std::ostringstream encoded;encodeGame(encoded,legacy,Player{});
    std::istringstream lines(encoded.str());std::vector<std::string> records;std::string line;
    while(std::getline(lines,line))records.push_back(line);
    std::ostringstream old;old<<"DISTRIBUIDORA_SAVE 1\n";
    for(std::size_t i=1;i<8;++i)old<<records[i]<<'\n';
    std::istringstream oldInput(old.str());
    check(decodeGame(oldInput,loaded,position)&&loaded.products[0].stock==7&&!loaded.expanded&&!loaded.helper.hired,"v1 saves migrate without resetting stock");
    std::cout<<"Expansion, helper, seating and migration tests passed\n";
}
void computerAndCustomerTests() {
    Game g;const auto& buttons=computerButtons();
    for(int i=0;i<int(buttons.size());++i){auto b=buttons[i];check(computerHit(b.x+b.w/2,b.y+b.h/2)==i,"mouse hit matches rendered computer button");}
    check(computerHit(860,50)==8&&computerHit(20,20)==-1,"window close and desktop hit areas");
    check(computerUnavailable(g,ComputerAction::Beer).empty(),"affordable order enabled");
    check(g.order(0)&&!computerUnavailable(g,ComputerAction::Cigarettes).empty(),"pending delivery disables other orders");
    g.tick(12);g.products[0].stock=48;check(!computerUnavailable(g,ComputerAction::Beer).empty(),"full storage disables purchase");
    g.cash=0;check(!computerUnavailable(g,ComputerAction::Helper).empty(),"unaffordable upgrade disabled");
    g.cash=1000;g.expand();check(!computerUnavailable(g,ComputerAction::Expansion).empty(),"owned upgrade disabled");
    Game doors;check(doors.pickup(0)&&doors.fridgeTime[0]>0,"beer pickup animates glass door");
    doors.tick(2);check(doors.fridgeTime[0]==0,"door closes automatically");
    doors.pickup(0,false);check(doors.fridgeTime[0]==0,"depot interaction does not open distant refrigerator");
    for(int i=0;i<4;++i)check(walkable(stockLocations[i].workerX,stockLocations[i].workerZ),"worker can reach relocated stock");
    Game customers;std::array<bool,customerLookCount> seen{};
    for(int i=0;i<80;++i) {
        int previous=customers.customerStyle;customers.customer=false;customers.arrival=0;customers.tick(.01f);
        check(customers.customerStyle!=previous,"consecutive customers have different appearances");seen[customers.customerStyle]=true;
    }
    for(bool appeared:seen)check(appeared,"all customer appearances can spawn");
    std::ostringstream encoded;encodeGame(encoded,customers,Player{});Game loaded;Player player;std::istringstream input(encoded.str());
    check(decodeGame(input,loaded,player)&&loaded.customerStyle==customers.customerStyle,"customer appearance survives loading");
    std::istringstream records(encoded.str());std::vector<std::string> lines;std::string line;
    while(std::getline(records,line))lines.push_back(line);
    std::ostringstream v2;v2<<"DISTRIBUIDORA_SAVE 2\n";
    for(std::size_t i=1;i<10;++i)v2<<lines[i]<<'\n';
    std::istringstream old(v2.str());check(decodeGame(old,loaded,player)&&loaded.customerStyle==0,"v2 saves remain compatible");
    std::cout<<"Computer interaction, doors and customer variety tests passed\n";
}
void progressionTests() {
    Game g;g.cash=10000;
    check(!g.cycleOrderSize()&&!g.upgradeCheckout()&&!g.upgradeBag(),"expansion and staff gate upgrades");
    int stock=g.products[0].stock;
    check(g.expand()&&g.upgradeCheckout()&&g.products[0].stock==stock,"grade unlocks second checkout without restocking");
    int cash=g.cash;check(!g.upgradeCheckout()&&g.cash==cash,"grade cannot charge twice");
    check(g.hire()&&g.hire()&&!g.hire(),"two attendants can be hired only once");
    for(int i=2;i<=5;++i)check(g.upgradeBag()&&g.bagCapacity==i,"bag upgrades step by step");
    check(!g.upgradeBag(),"bag capacity capped at five");
    check(g.cycleOrderSize()&&g.orderSize==24&&g.orderCost(0)==87,"24 pack discount rounded to whole reais");
    check(g.cycleOrderSize()&&g.orderSize==48&&g.orderCost(0)==154,"48 pack discount");
    cash=g.cash;check(g.order(0)&&g.cash==cash-154&&g.occupied(0)==stock+48,"bulk stock and payment reserved");
    check(g.cycleOrderSize()&&g.orderSize==12&&g.pendingUnits==48,"changing lot does not alter paid shipment");
    std::ostringstream encoded;encodeGame(encoded,g,Player{});Game loaded;Player p;std::istringstream input(encoded.str());
    check(decodeGame(input,loaded,p)&&loaded.pendingUnits==48&&loaded.secondHelper.hired&&loaded.bagCapacity==5,"bulk shipment and upgrades survive save");
    loaded.tick(12);check(loaded.products[0].stock==stock+48&&loaded.pending==-1,"exact paid bulk quantity arrives");
    loaded.products[0].stock=90;loaded.orderSize=24;cash=loaded.cash;
    check(!loaded.order(0)&&loaded.cash==cash,"bulk capacity rejection does not charge");
    Game margin;int base=margin.salePrice(2);margin.day=11;
    check(margin.marginBonus()==20&&margin.salePrice(2)>base,"days increase profit");
    int price=margin.salePrice(2);margin.level=5;
    check(margin.marginBonus()==40&&margin.salePrice(2)>price,"levels increase profit");
    margin.customer=true;margin.wanted=2;margin.quantity=1;cash=margin.cash;
    check(margin.pickup(2)&&margin.sell()&&margin.cash==cash+margin.salePrice(2),"sale pays progressive price");
    margin.day=1000000;check(margin.marginBonus()==100,"margin has bounded growth");
    check(interaction(2.1f,-1.6f,0,true,true)==12&&interaction(2.1f,-1.6f,0,true,false)!=12,"second hatch has its own interaction");
    Game workers;workers.cash=10000;workers.expand();workers.upgradeCheckout();workers.hire();workers.hire();
    for(int i=0;i<4;++i)workers.upgradeBag();
    workers.customer=true;workers.wanted=0;workers.quantity=5;
    workers.second.customer=true;workers.second.wanted=1;workers.second.quantity=5;
    cash=workers.cash;bool firstBag=false,secondBag=false;
    for(int i=0;i<1500&&(workers.customer||workers.second.customer);++i) {
        if(!workers.customer)workers.arrival=4;
        if(!workers.second.customer)workers.second.arrival=4;
        workers.tick(.05f);
        firstBag=firstBag||workers.helper.count==5;secondBag=secondBag||workers.secondHelper.count==5;
        check(validGame(workers,Player{}),"two-lane worker state remains saveable each tick");
        if(workers.helper.count>0&&workers.secondHelper.count>0) {
            std::ostringstream out;encodeGame(out,workers,Player{});std::istringstream in(out.str());
            Game restored;check(decodeGame(in,restored,p),"both loaded bags survive serialization");workers=restored;
        }
    }
    check(firstBag&&secondBag&&workers.sold==10&&workers.cash==cash+5*8+5*12,"independent attendants sell full bags at both registers");
    check(workers.products[0].stock==13&&workers.products[1].stock==5,"two-lane stock conserved");
    Game expired;expired.cash=5000;expired.expand();expired.upgradeCheckout();expired.hire();expired.hire();
    for(int i=0;i<4;++i)expired.upgradeBag();
    expired.second.customer=true;expired.second.wanted=0;expired.second.quantity=5;
    for(int i=0;i<600&&expired.secondHelper.count==0;++i){expired.arrival=4;expired.tick(.05f);}
    check(expired.secondHelper.count==5,"second worker collects bag");
    expired.second.patience=.01f;expired.tick(.05f);
    for(int i=0;i<600&&expired.secondHelper.count>0;++i){expired.arrival=4;expired.second.arrival=4;expired.tick(.05f);}
    check(expired.products[0].stock==18&&expired.sold==0&&expired.secondHelper.count==0,"abandoned order refunds full bag");
    std::ostringstream current;encodeGame(current,Game{},Player{});std::istringstream records(current.str());
    std::string line;std::ostringstream v3;v3<<"DISTRIBUIDORA_SAVE 3\n";
    for(int i=0;i<11&&std::getline(records,line);++i)if(i>0)v3<<line<<'\n';
    std::istringstream legacy(v3.str());check(decodeGame(legacy,loaded,p)&&!loaded.secondCheckout&&loaded.bagCapacity==1,"v3 saves migrate to default upgrades");
    Game invalid=workers;invalid.secondHelper.count=6;std::ostringstream corrupt;encodeGame(corrupt,invalid,p);
    std::istringstream bad(corrupt.str());check(!decodeGame(bad,loaded,p),"corrupt bag count rejected");
    std::cout<<"Bulk orders, progressive profit, two checkouts, bags and v4 saves passed\n";
}
int main() {
    // Atomic directory creation avoids collisions on Linux and Windows.
    std::filesystem::path directory;
    std::random_device random;
    for(int attempt=0;attempt<100;++attempt) {
        auto candidate=std::filesystem::temp_directory_path()/("distribuidora-tests-"+std::to_string(random()));
        std::error_code error;
        if(std::filesystem::create_directory(candidate,error)){directory=candidate;break;}
        if(error){std::cerr<<error.message()<<'\n';return 1;}
    }
    if(directory.empty())return 1;
    int result=0;
    try {economyTests();capacityAndConsumptionTests();persistenceTests(directory);movementAndWorldTests();expansionAndHelperTests(directory);computerAndCustomerTests();progressionTests();}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    std::error_code ec;std::filesystem::remove_all(directory,ec);return result;
}
