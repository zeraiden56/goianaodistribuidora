#pragma once
#include "settings.hpp"
#include <string>
#include <array>
#include <vector>

enum class Screen {Main,Playing,Pause,Options,ConfirmNew,CompanyName,ConfirmPrestige,SlotSelect};
struct Menu {
    Screen screen=Screen::Main,back=Screen::Main;
    int selected=0;
    std::string notice;
    std::string input="GOIANAO DISTRIBUIDORA";
    std::array<bool,3> slots{};
    bool selectingNew=false;
    void open(Screen next) {screen=next;selected=0;notice.clear();}
};
std::vector<std::string> menuRows(const Menu& menu,const Settings& settings,bool hasSave);
int menuHit(int x,int y,int count);
void renderMenu(const Menu& menu,const Settings& settings,bool hasSave);
void destroyMenuArt();
