#include "persistence.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
bool between(float v,float low,float high) {return std::isfinite(v)&&v>=low&&v<=high;}
bool validSettings(const Settings& s) {
    return s.resolution>=0&&s.resolution<resolutionCount&&s.quality>=0&&s.quality<3&&s.fov>=60&&s.fov<=100&&s.volume>=0&&s.volume<=100&&validFrameLimit(s.frameLimit)&&s.musicVolume>=0&&s.musicVolume<=100
        &&s.controllerIcons>=0&&s.controllerIcons<=3&&s.controllerSensitivity>=50&&s.controllerSensitivity<=200&&s.controllerDeadzone>=5&&s.controllerDeadzone<=30;
}
// Replace only after the complete temporary file has been flushed and closed.
bool atomicWrite(const std::filesystem::path& file,const std::string& data,std::string& error) {
    std::error_code ec;
    if(!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path(),ec);
    if(ec) {error="NAO FOI POSSIVEL CRIAR A PASTA DE DADOS.";return false;}
    auto temp=file;temp+=".tmp";
    std::ofstream out(temp,std::ios::binary|std::ios::trunc);
    out<<data;out.flush();bool ok=bool(out);out.close();ok=ok&&!out.fail();
    if(!ok) {error="FALHA AO GRAVAR. O SAVE ANTERIOR FOI PRESERVADO.";return false;}
#ifdef _WIN32
    if(!MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        ec=std::error_code(static_cast<int>(GetLastError()),std::system_category());
#else
    std::filesystem::rename(temp,file,ec);
#endif
    if(ec) {error="FALHA AO SUBSTITUIR O ARQUIVO DE DADOS.";return false;}
    return true;
}
}
bool validGame(const Game& g,const Player& p) {
    if(g.companyName.empty()||g.companyName.size()>24||g.prestige<0||g.prestige>100000||g.perk<0||g.perk>g.prestige
        ||g.deliveryLevel<0||g.deliveryLevel>3||(g.restockThreshold!=10&&g.restockThreshold!=25&&g.restockThreshold!=50)
        ||(g.restockReserve!=0&&g.restockReserve!=250&&g.restockReserve!=500&&g.restockReserve!=1000&&g.restockReserve!=2500)
        ||g.restockCursor<0||g.restockCursor>=Game::productCount||!between(g.restockTimer,0,1))return false;
    if(g.delivery>g.deliverySeconds()||(g.deliveryLevel==3&&g.pending!=-1))return false;
    if(g.customerStyle<0||g.customerStyle>=customerLookCount||g.cash<0||g.cash>2000000000||g.sold<0||g.sold>100000000||g.level<1||g.level>1000
        ||g.day<1||g.day>1000000||g.reputation<0||g.reputation>100
        ||g.wanted<0||g.wanted>=g.availableProducts()||g.quantity<1||g.quantity>(g.wholesale?60:5)||g.pending< -1||g.pending>=g.availableProducts()
        ||g.held< -1||g.held>=g.availableProducts()||g.delivered<0||g.delivered>g.quantity
        ||(!g.customer&&g.delivered!=0)||g.consumed<0||g.consumed>100000000
        ||g.consuming< -1||g.consuming>=g.availableProducts()||g.consuming==3) return false;
    if(!between(g.delivery,0,12)||!between(g.patience,0,g.wholesale?180:65)||!between(g.arrival,0,4)
        ||!between(g.time,0,180)||!between(g.intoxication,0,100)||!between(g.consumeTime,0,2)
        ||!between(g.smokeTime,0,5)||(g.consumeTime>0&&(g.consuming<0))) return false;
    if(g.bagCapacity<1||g.bagCapacity>5||!g.allowedLot(g.orderSize)
        ||!g.allowedLot(g.pendingUnits)
        ||(!g.expanded&&(g.tvOn||g.secondCheckout||g.orderSize!=12||g.pendingUnits!=12))
        ||(g.secondHelper.hired&&(!g.secondCheckout||!g.helper.hired))
        ||(g.largeStore&&!g.expanded)||(g.thirdCheckout&&(!g.largeStore||!g.secondCheckout))
        ||(g.thirdHelper.hired&&(!g.thirdCheckout||!g.secondHelper.hired))
        ||g.storageLevel<0||g.storageLevel>3||g.heldCount<1||g.heldCount>g.bagCapacity*(g.heldPacked?Game::crateUnits:1)|| (g.held<0&&g.heldCount!=1))return false;
    if(g.annexCheckouts<0||g.annexCheckouts>2||(g.annexCheckouts>0&&!g.thirdCheckout)
        ||g.staffSpeedLevel<0||g.staffSpeedLevel>3||(g.staffSpeedLevel>0&&!g.helper.hired)
        ||(g.heldPacked&&(!g.largeStore||g.held<0))
        ||(g.wholesale&&(!g.largeStore||g.quantity%Game::crateUnits!=0)))return false;
    for(int lane=1;lane<5;++lane) {
    const auto& c=lane==1?g.second:g.extraCheckout(lane);
    if(c.wanted<0||c.wanted>=g.availableProducts()||c.quantity<1||c.quantity>(c.wholesale?60:5)||c.delivered<0||c.delivered>c.quantity
        ||(c.wholesale&&(!g.largeStore||c.quantity%Game::crateUnits!=0))
        ||c.customerStyle<0||c.customerStyle>=customerLookCount||(!c.customer&&c.delivered!=0)
        ||(lane>=g.checkoutCount()&&c.customer)||!between(c.patience,0,c.wholesale?180:65)||!between(c.arrival,0,4))return false;
    }
    for(int lane=0;lane<5;++lane) {
        int first=lane==0?g.wanted:lane==1?g.second.wanted:g.extraCheckout(lane).wanted;
        bool bulk=lane==0?g.wholesale:lane==1?g.second.wholesale:g.extraCheckout(lane).wholesale;
        const auto& order=g.mixedOrders[lane];const auto& motion=g.customerMotion[lane];
        if(!between(motion.approach,0,1)||!between(motion.departure,0,1)||motion.departingStyle<0||motion.departingStyle>=customerLookCount
            ||(!g.customerActive(lane)&&motion.approach>0))return false;
        int lines=0;
        for(int i=0;i<Game::productCount;++i) {
            int q=order.requested[i],d=order.delivered[i];
            if(q<0||q>5||d<0||d>q||((bulk||i==first||i>=g.availableProducts()||!g.customerActive(lane))&&(q||d)))return false;
            if(q)++lines;
        }
        if(lines>2||(g.customerActive(lane)&&g.orderDelivered(lane)>=g.orderTotal(lane)))return false;
        const auto& h=g.staff(lane);int task=int(h.task);
        if(h.hired&&(lane>=g.checkoutCount()||(lane>0&&!g.staff(lane-1).hired)))return false;
        if(h.packed&&(!g.largeStore||h.held<0))return false;
        if(task<0||task>3||h.held< -1||h.held>=g.availableProducts()||h.target< -1||h.target>=g.availableProducts()
            ||!between(h.wait,0,1)||!walkable(h.x,h.z,g.expanded,g.largeStore)||h.count<0||h.count>g.bagCapacity*(h.packed?Game::crateUnits:1))return false;
        if((h.held<0&&h.count!=0)||(h.held>=0&&h.count<1))return false;
        if(!h.hired&&(h.held!=-1||task!=0||h.target!=-1))return false;
        if(task==0&&(h.held!=-1||h.target!=-1))return false;
        if(task==1&&(h.held!=-1||h.target<0))return false;
        if(task>=2&&(h.held<0||h.target!=h.held))return false;
    }
    for(int i=0;i<Game::productCount;++i)
        if(g.products[i].stock<0||g.products[i].capacity!=g.capacityFor(i)||g.occupied(i)>g.products[i].capacity
            ||(i>=g.availableProducts()&&g.products[i].stock!=0))return false;
    bool validPosition=p.seated?g.expanded&&std::abs(p.x-3.8f)<.001f&&std::abs(p.z-6.2f)<.001f:walkable(p.x,p.z,g.expanded,g.largeStore);
    return validPosition&&between(p.yaw,-1000000,1000000)&&between(p.pitch,-70,70);
}
void encodeGame(std::ostream& out,const Game& g,const Player& p) {
    out<<std::setprecision(9)<<"DISTRIBUIDORA_SAVE 9\n";
    out<<g.cash<<' '<<g.sold<<' '<<g.level<<' '<<g.day<<' '<<g.reputation<<'\n';
    for(int i=0;i<4;++i) out<<g.products[i].stock<<' ';
    out<<'\n'<<g.wanted<<' '<<g.quantity<<' '<<g.pending<<' '<<g.held<<' '<<g.delivered<<' '
        <<g.customer<<' '<<g.reinforced<<'\n';
    out<<g.delivery<<' '<<g.patience<<' '<<g.arrival<<' '<<g.time<<'\n';
    out<<g.consumed<<' '<<g.consuming<<' '<<g.intoxication<<' '<<g.consumeTime<<' '<<g.smokeTime<<'\n';
    out<<p.x<<' '<<p.z<<' '<<p.yaw<<' '<<p.pitch<<'\n'<<g.random<<'\n';
    out<<g.expanded<<' '<<g.tvOn<<' '<<p.seated<<'\n';
    const auto& h=g.helper;
    out<<h.hired<<' '<<h.x<<' '<<h.z<<' '<<h.wait<<' '<<h.held<<' '<<h.target<<' '<<int(h.task)<<'\n';
    out<<g.customerStyle<<'\n';
    out<<g.secondCheckout<<' '<<g.bagCapacity<<' '<<g.orderSize<<' '<<g.pendingUnits<<' '<<g.helper.count<<'\n';
    const auto& c=g.second;
    out<<c.customer<<' '<<c.wanted<<' '<<c.quantity<<' '<<c.delivered<<' '<<c.customerStyle<<' '<<c.patience<<' '<<c.arrival<<'\n';
    const auto& h2=g.secondHelper;
    out<<h2.hired<<' '<<h2.x<<' '<<h2.z<<' '<<h2.wait<<' '<<h2.held<<' '<<h2.target<<' '<<int(h2.task)<<' '<<h2.count<<'\n';
    out<<g.largeStore<<' '<<g.thirdCheckout<<' '<<g.storageLevel<<' '<<g.heldCount<<' '<<g.products[4].stock<<' '<<g.products[5].stock<<'\n';
    const auto& c3=g.third;const auto& h3=g.thirdHelper;
    out<<c3.customer<<' '<<c3.wanted<<' '<<c3.quantity<<' '<<c3.delivered<<' '<<c3.customerStyle<<' '<<c3.patience<<' '<<c3.arrival<<'\n';
    out<<h3.hired<<' '<<h3.x<<' '<<h3.z<<' '<<h3.wait<<' '<<h3.held<<' '<<h3.target<<' '<<int(h3.task)<<' '<<h3.count<<'\n';
    out<<g.annexCheckouts<<' '<<g.staffSpeedLevel<<' '<<g.heldPacked<<' '<<g.wholesale<<' '<<g.second.wholesale<<' '<<g.third.wholesale<<'\n';
    for(int lane=0;lane<5;++lane)out<<g.staff(lane).packed<<' ';
    out<<'\n';
    for(int lane=3;lane<5;++lane) {
        const auto& c=g.extraCheckout(lane);const auto& h=g.staff(lane);
        out<<c.customer<<' '<<c.wanted<<' '<<c.quantity<<' '<<c.delivered<<' '<<c.customerStyle<<' '<<c.patience<<' '<<c.arrival<<' '<<c.wholesale<<'\n';
        out<<h.hired<<' '<<h.x<<' '<<h.z<<' '<<h.wait<<' '<<h.held<<' '<<h.target<<' '<<int(h.task)<<' '<<h.count<<'\n';
    }
    out<<g.deliveryLevel<<' '<<g.autoRestockEnabled<<' '<<g.restockThreshold<<' '<<g.restockReserve<<' '<<g.restockCursor<<' '<<g.restockTimer<<'\n';
    out<<g.prestige<<' '<<g.perk<<' '<<std::quoted(g.companyName)<<'\n';
    for(int lane=0;lane<5;++lane) {
        for(int n:g.mixedOrders[lane].requested)out<<n<<' ';
        for(int n:g.mixedOrders[lane].delivered)out<<n<<' ';
        const auto& m=g.customerMotion[lane];out<<m.approach<<' '<<m.departure<<' '<<m.departingStyle<<' '<<m.purchased<<'\n';
    }
}
bool decodeGame(std::istream& in,Game& game,Player& player) {
    Game g;Player p;std::string magic;int version=0;
    if(!(in>>magic>>version)||magic!="DISTRIBUIDORA_SAVE"||(version<1||version>9)) return false;
    in>>g.cash>>g.sold>>g.level>>g.day>>g.reputation;
    for(int i=0;i<4;++i) in>>g.products[i].stock;
    in>>g.wanted>>g.quantity>>g.pending>>g.held>>g.delivered>>g.customer>>g.reinforced;
    in>>g.delivery>>g.patience>>g.arrival>>g.time;
    in>>g.consumed>>g.consuming>>g.intoxication>>g.consumeTime>>g.smokeTime;
    in>>p.x>>p.z>>p.yaw>>p.pitch>>g.random;
    if(version>=2) {
        auto& h=g.helper;int task=0;
        in>>g.expanded>>g.tvOn>>p.seated>>h.hired>>h.x>>h.z>>h.wait>>h.held>>h.target>>task;
        h.task=static_cast<HelperTask>(task);
    }
    if(version>=3)in>>g.customerStyle;
    if(version>=4) {
        auto& c=g.second;auto& h=g.secondHelper;int task=0;
        in>>g.secondCheckout>>g.bagCapacity>>g.orderSize>>g.pendingUnits>>g.helper.count;
        in>>c.customer>>c.wanted>>c.quantity>>c.delivered>>c.customerStyle>>c.patience>>c.arrival;
        in>>h.hired>>h.x>>h.z>>h.wait>>h.held>>h.target>>task>>h.count;
        h.task=static_cast<HelperTask>(task);
    } else g.helper.count=g.helper.held>=0?1:0;
    if(version>=5) {
        auto& c=g.third;auto& h=g.thirdHelper;int task=0;
        in>>g.largeStore>>g.thirdCheckout>>g.storageLevel>>g.heldCount>>g.products[4].stock>>g.products[5].stock;
        in>>c.customer>>c.wanted>>c.quantity>>c.delivered>>c.customerStyle>>c.patience>>c.arrival;
        in>>h.hired>>h.x>>h.z>>h.wait>>h.held>>h.target>>task>>h.count;
        h.task=static_cast<HelperTask>(task);
    }
    if(version>=6) {
        in>>g.annexCheckouts>>g.staffSpeedLevel>>g.heldPacked>>g.wholesale>>g.second.wholesale>>g.third.wholesale;
        for(int lane=0;lane<5;++lane)in>>g.staff(lane).packed;
        for(int lane=3;lane<5;++lane) {
            auto& c=g.extraCheckout(lane);auto& h=g.staff(lane);int task=0;
            in>>c.customer>>c.wanted>>c.quantity>>c.delivered>>c.customerStyle>>c.patience>>c.arrival>>c.wholesale;
            in>>h.hired>>h.x>>h.z>>h.wait>>h.held>>h.target>>task>>h.count;
            h.task=static_cast<HelperTask>(task);
        }
    }
    if(version>=7)in>>g.deliveryLevel>>g.autoRestockEnabled>>g.restockThreshold>>g.restockReserve>>g.restockCursor>>g.restockTimer;
    if(version>=8)in>>g.prestige>>g.perk>>std::quoted(g.companyName);
    if(version>=9) {
        for(int lane=0;lane<5;++lane) {
            for(int& n:g.mixedOrders[lane].requested)in>>n;
            for(int& n:g.mixedOrders[lane].delivered)in>>n;
            auto& m=g.customerMotion[lane];in>>m.approach>>m.departure>>m.departingStyle>>m.purchased;
        }
    } else g.bagCapacity=std::max(3,g.bagCapacity);
    g.restockStatus=g.autoRestockEnabled?"REPOSICAO AUTOMATICA ATIVA. VERIFICANDO ESTOQUE.":"REPOSICAO AUTOMATICA DESLIGADA.";
    g.refreshCapacity();
    // The sofa terminal occupies floor that existed in older saves.
    if(version<5&&g.expanded&&!p.seated&&p.x>1.85f&&p.x<2.95f&&p.z>7.05f&&p.z<7.85f) {
        p.x=2.65f;p.z=6.2f;
    }
    if(!in||!validGame(g,p)) return false;
    in>>std::ws;if(!in.eof()) return false;
    g.message="PROGRESSO CARREGADO.";game=g;player=p;return true;
}
bool saveGame(const std::filesystem::path& file,const Game& g,const Player& p,std::string& error) {
    if(!validGame(g,p)) {error="ESTADO INVALIDO. O SAVE ANTERIOR FOI PRESERVADO.";return false;}
    std::ostringstream out;encodeGame(out,g,p);return atomicWrite(file,out.str(),error);
}
bool loadGame(const std::filesystem::path& file,Game& g,Player& p,std::string& error) {
    std::ifstream in(file);
    if(!in) {error="NAO FOI POSSIVEL ABRIR O SAVE.";return false;}
    if(!decodeGame(in,g,p)) {error="SAVE INVALIDO OU INCOMPATIVEL. NADA FOI ALTERADO.";return false;}
    return true;
}
bool saveSettings(const std::filesystem::path& file,const Settings& s,std::string& error) {
    if(!validSettings(s)) {error="CONFIGURACAO INVALIDA.";return false;}
    std::ostringstream out;out<<"DISTRIBUIDORA_OPTIONS 5\n"<<s.fullscreen<<' '<<s.vsync<<' '
        <<s.resolution<<' '<<s.quality<<' '<<s.fov<<' '<<s.volume<<' '<<s.frameLimit<<' '<<s.musicVolume<<' '
        <<s.controllerIcons<<' '<<s.controllerSensitivity<<' '<<s.controllerDeadzone<<' '<<s.controllerInvertY<<'\n';return atomicWrite(file,out.str(),error);
}
bool loadSettings(const std::filesystem::path& file,Settings& settings) {
    std::ifstream in(file);Settings s;std::string magic;int version=0;
    if(!(in>>magic>>version>>s.fullscreen>>s.vsync>>s.resolution>>s.quality>>s.fov)
        ||magic!="DISTRIBUIDORA_OPTIONS"||(version<1||version>5)) return false;
    if(version>=2&&!(in>>s.volume))return false;
    if(version>=3&&!(in>>s.frameLimit))return false;
    if(version>=4&&!(in>>s.musicVolume))return false;
    if(version>=5&&!(in>>s.controllerIcons>>s.controllerSensitivity>>s.controllerDeadzone>>s.controllerInvertY))return false;
    if(!validSettings(s))return false;
    in>>std::ws;if(!in.eof()) return false;
    settings=s;return true;
}
