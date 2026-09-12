#pragma once
#include "settings.hpp"
#include <string>
#include <vector>

enum class Screen {Main,Playing,Pause,Options,ConfirmNew};
struct Menu {
    Screen screen=Screen::Main,back=Screen::Main;
    int selected=0;
    std::string notice;
    void open(Screen next) {screen=next;selected=0;notice.clear();}
};
std::vector<std::string> menuRows(const Menu& menu,const Settings& settings,bool hasSave);
int menuHit(int x,int y,int count);
void renderMenu(const Menu& menu,const Settings& settings,bool hasSave);
