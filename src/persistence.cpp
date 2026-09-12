#include "persistence.hpp"
#include <cmath>
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
    return s.resolution>=0&&s.resolution<4&&s.quality>=0&&s.quality<3&&s.fov>=60&&s.fov<=100&&s.volume>=0&&s.volume<=100;
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
    if(g.customerStyle<0||g.customerStyle>=customerLookCount||g.cash<0||g.cash>100000000||g.sold<0||g.sold>100000000||g.level<1||g.level>1000
        ||g.day<1||g.day>1000000||g.reputation<0||g.reputation>100
        ||g.wanted<0||g.wanted>3||g.quantity<1||g.quantity>5||g.pending< -1||g.pending>3
        ||g.held< -1||g.held>3||g.delivered<0||g.delivered>=g.quantity
        ||(!g.customer&&g.delivered!=0)||g.consumed<0||g.consumed>100000000
        ||g.consuming< -1||g.consuming>2) return false;
    if(!between(g.delivery,0,12)||!between(g.patience,0,65)||!between(g.arrival,0,4)
        ||!between(g.time,0,180)||!between(g.intoxication,0,100)||!between(g.consumeTime,0,2)
        ||!between(g.smokeTime,0,5)||(g.consumeTime>0&&(g.consuming<0||g.held!=-1))) return false;
    if(g.bagCapacity<1||g.bagCapacity>5||(g.orderSize!=12&&g.orderSize!=24&&g.orderSize!=48)
        ||(g.pendingUnits!=12&&g.pendingUnits!=24&&g.pendingUnits!=48)
        ||(!g.expanded&&(g.tvOn||g.secondCheckout||g.orderSize!=12||g.pendingUnits!=12))
        ||(g.secondHelper.hired&&(!g.secondCheckout||!g.helper.hired))
        ||(g.bagCapacity>1&&!g.helper.hired))return false;
    const auto& c=g.second;
    if(c.wanted<0||c.wanted>3||c.quantity<1||c.quantity>5||c.delivered<0||c.delivered>=c.quantity
        ||c.customerStyle<0||c.customerStyle>=customerLookCount||(!c.customer&&c.delivered!=0)
        ||(!g.secondCheckout&&c.customer)||!between(c.patience,0,65)||!between(c.arrival,0,4))return false;
    for(const auto* worker:{&g.helper,&g.secondHelper}) {
        const auto& h=*worker;int task=int(h.task);
        if(task<0||task>3||h.held< -1||h.held>3||h.target< -1||h.target>3
            ||!between(h.wait,0,1)||!walkable(h.x,h.z)||h.count<0||h.count>g.bagCapacity)return false;
        if((h.held<0&&h.count!=0)||(h.held>=0&&h.count<1))return false;
        if(!h.hired&&(h.held!=-1||task!=0||h.target!=-1))return false;
        if(task==0&&(h.held!=-1||h.target!=-1))return false;
        if(task==1&&(h.held!=-1||h.target<0))return false;
        if(task>=2&&(h.held<0||h.target!=h.held))return false;
    }
    constexpr int base[]={48,36,24,36};
    for(int i=0;i<4;++i) if(g.products[i].stock<0||g.products[i].capacity!=base[i]*(g.expanded?2:1)||g.occupied(i)>g.products[i].capacity)return false;
    bool validPosition=p.seated?g.expanded&&std::abs(p.x-3.8f)<.001f&&std::abs(p.z-6.2f)<.001f:walkable(p.x,p.z,g.expanded);
    return validPosition&&between(p.yaw,-1000000,1000000)&&between(p.pitch,-70,70);
}
void encodeGame(std::ostream& out,const Game& g,const Player& p) {
    out<<std::setprecision(9)<<"DISTRIBUIDORA_SAVE 4\n";
    out<<g.cash<<' '<<g.sold<<' '<<g.level<<' '<<g.day<<' '<<g.reputation<<'\n';
    for(auto product:g.products) out<<product.stock<<' ';
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
}
bool decodeGame(std::istream& in,Game& game,Player& player) {
    Game g;Player p;std::string magic;int version=0;
    if(!(in>>magic>>version)||magic!="DISTRIBUIDORA_SAVE"||(version<1||version>4)) return false;
    in>>g.cash>>g.sold>>g.level>>g.day>>g.reputation;
    for(auto& product:g.products) in>>product.stock;
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
    g.refreshCapacity();
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
    std::ostringstream out;out<<"DISTRIBUIDORA_OPTIONS 2\n"<<s.fullscreen<<' '<<s.vsync<<' '
        <<s.resolution<<' '<<s.quality<<' '<<s.fov<<' '<<s.volume<<'\n';return atomicWrite(file,out.str(),error);
}
bool loadSettings(const std::filesystem::path& file,Settings& settings) {
    std::ifstream in(file);Settings s;std::string magic;int version=0;
    if(!(in>>magic>>version>>s.fullscreen>>s.vsync>>s.resolution>>s.quality>>s.fov)
        ||magic!="DISTRIBUIDORA_OPTIONS"||(version!=1&&version!=2)) return false;
    if(version==2&&!(in>>s.volume))return false;
    if(!validSettings(s))return false;
    in>>std::ws;if(!in.eof()) return false;
    settings=s;return true;
}
