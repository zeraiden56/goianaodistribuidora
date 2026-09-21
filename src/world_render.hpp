#pragma once
#include "player.hpp"
void renderStreet(double seconds);
void renderTraffic(double seconds);
void renderSecurityDevices(int activeCamera,bool expanded,bool largeStore);
void renderManager(const Player& player);
