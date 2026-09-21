#include "music_render.hpp"
#include "music.hpp"
#include "render.hpp"
#include "controller_render.hpp"
#include <algorithm>

namespace {
// Cut only at UTF-8 boundaries. Long track names scroll in their own panel.
std::string titleWindow(const std::string& title,float seconds,std::size_t columns) {
    std::vector<std::string> letters;
    for(std::size_t i=0;i<title.size();) {
        std::size_t end=i+1;
        while(end<title.size()&&(static_cast<unsigned char>(title[end])&0xc0)==0x80)++end;
        letters.push_back(title.substr(i,end-i));i=end;
    }
    if(letters.size()<=columns)return title;
    for(int i=0;i<5;++i)letters.push_back(" ");
    const std::size_t start=static_cast<std::size_t>(std::max(0.f,seconds-3.f)*8)%letters.size();
    std::string result;
    for(std::size_t i=0;i<columns;++i)result+=letters[(start+i)%letters.size()];
    return result;
}
}

bool musicNextHit(int x,int y) {return x>=782&&x<924&&y>=464&&y<488;}

void renderMusicPlayer(const Music& music,bool compact) {
    if(compact) {
        rect(220,512,722,24,.035f,.065f,.065f);
        rect(220,512,3,24,.91f,.66f,.28f);
        text(232,520,titleWindow(music.title(),music.titleSeconds(),88),1.f,.93f,.84f,.65f);
        if(controllerPrompts())padHint(802,520,PadIcon::RightShoulder,"PROXIMA MUSICA",.9f);
        else text(802,520,"F8 PROXIMA MUSICA",1.f,.8f,.87f,.83f);
    } else {
        rect(490,398,450,102,.035f,.065f,.065f);
        rect(490,398,3,102,.91f,.66f,.28f);
        text(506,409,"TOCANDO AGORA",1.f,.91f,.66f,.28f);
        text(506,433,titleWindow(music.title(),music.titleSeconds(),57),1.2f);
        text(506,472,music.active()?"FAIXA "+std::to_string(music.index()+1)+" / "+std::to_string(music.count()):"RADIO",1.f);
        rect(782,464,142,24,.17f,.29f,.25f);
        if(controllerPrompts())padHint(790,472,PadIcon::RightShoulder,"PROXIMA MUSICA",.9f);
        else text(795,472,"F8 PROXIMA MUSICA",1.f,.97f,.81f,.46f);
    }
}
