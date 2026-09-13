#pragma once
#include "game.hpp"
#include <array>
#include <string>
enum class ComputerAction {Beer,Cigarettes,Spirits,Ice,Cameras,Level,Expansion,Helper,Close,Lot,Checkout,Bag,CanBeer,Soda,Storage,Speed};
struct ComputerButton {int x,y,w,h;ComputerAction action;};
struct ComputerUI {int selected=0;};
const std::array<ComputerButton,16>& computerButtons();
int computerProduct(ComputerAction action);
int computerHit(int x,int y);
std::string computerUnavailable(const Game& game,ComputerAction action);
void renderComputer(const Game& game,const ComputerUI& ui);
