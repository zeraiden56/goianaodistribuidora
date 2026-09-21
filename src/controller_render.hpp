#pragma once
#include "controller.hpp"
void setControllerPrompts(bool active,ControllerIcons icons);
bool controllerPrompts();
void padIcon(float x,float y,PadIcon button,float size=18);
void padHint(float x,float y,PadIcon button,const std::string& label,float scale=1.f);
void renderControllerPreview(const std::string& name);
void renderControllerKeyboard(int selected);
